#include "network_protocol_internal.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace tenhou {
    using protocol_detail::Attribute;
    using protocol_detail::Digit;
    using protocol_detail::Escape;
    using protocol_detail::Fail;
    using protocol_detail::Integer;
    using protocol_detail::Letter;

    namespace {
        bool Space(const char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
        bool Name(const char c) { return Letter(c) || Digit(c) || c == '-' || c == '.' || c == ':'; }
        void ValidateUtf8(const std::string_view value) {
            for (std::size_t i = 0; i < value.size();) {
                const auto b0 = static_cast<unsigned char>(value[i]);
                if (b0 < 0x80) {
                    ++i;
                } else if ((b0 & 0xE0) == 0xC0) {
                    if (i + 1 >= value.size()) Fail("Invalid UTF-8 protocol text");
                    const auto b1 = static_cast<unsigned char>(value[i + 1]);
                    if ((b1 & 0xC0) != 0x80) Fail("Invalid UTF-8 protocol text");
                    const std::uint32_t cp = ((b0 & 0x1Fu) << 6) | (b1 & 0x3Fu);
                    if (cp < 0x80) Fail("Invalid UTF-8 protocol text");
                    i += 2;
                } else if ((b0 & 0xF0) == 0xE0) {
                    if (i + 2 >= value.size()) Fail("Invalid UTF-8 protocol text");
                    const auto b1 = static_cast<unsigned char>(value[i + 1]);
                    const auto b2 = static_cast<unsigned char>(value[i + 2]);
                    if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) Fail("Invalid UTF-8 protocol text");
                    const std::uint32_t cp = ((b0 & 0x0Fu) << 12) | ((b1 & 0x3Fu) << 6) | (b2 & 0x3Fu);
                    if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) Fail("Invalid UTF-8 protocol text");
                    i += 3;
                } else if ((b0 & 0xF8) == 0xF0) {
                    if (i + 3 >= value.size()) Fail("Invalid UTF-8 protocol text");
                    const auto b1 = static_cast<unsigned char>(value[i + 1]);
                    const auto b2 = static_cast<unsigned char>(value[i + 2]);
                    const auto b3 = static_cast<unsigned char>(value[i + 3]);
                    if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80)
                        Fail("Invalid UTF-8 protocol text");
                    const std::uint32_t cp = ((b0 & 0x07u) << 18) | ((b1 & 0x3Fu) << 12) |
                                             ((b2 & 0x3Fu) << 6) | (b3 & 0x3Fu);
                    if (cp < 0x10000 || cp > 0x10FFFF) Fail("Invalid UTF-8 protocol text");
                    i += 4;
                } else {
                    Fail("Invalid UTF-8 protocol text");
                }
            }
        }

        void AppendUtf8(std::string &out, const std::uint32_t cp) {
            if (cp < 0x80) {
                out.push_back(static_cast<char>(cp));
            } else if (cp < 0x800) {
                out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else if (cp < 0x10000) {
                out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else {
                out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
        }

        void Validate(const std::string &value) {
            ValidateUtf8(value);
            for (unsigned char c: value)
                if ((c < 32 && c != 9 && c != 10 && c != 13) || c == 127)
                    Fail("Invalid control character in protocol text");
        }

        std::string Entities(const std::string &value) {
            std::string out;
            for (std::size_t i = 0; i < value.size(); ++i) {
                if (value[i] != '&') {
                    out += value[i];
                    continue;
                }
                const auto end = value.find(';', i);
                if (end == std::string::npos) Fail("Unterminated XML entity");
                const auto entity = value.substr(i + 1, end - i - 1);
                if (entity == "amp") out += '&';
                else if (entity == "quot") out += '"';
                else if (entity == "apos") out += '\'';
                else if (entity == "lt") out += '<';
                else if (entity == "gt") out += '>';
                else if (!entity.empty() && entity[0] == '#') {
                    const bool hex = entity.size() > 1 && entity[1] == 'x';
                    unsigned code = Integer(entity.substr(hex ? 2 : 1), hex ? 16 : 10);
                    if (!code || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff) || code == 0xfffe || code ==
                        0xffff)
                        Fail("Invalid XML scalar");
                    AppendUtf8(out, code);
                } else Fail("Unsupported XML entity");
                i = end;
            }
            Validate(out);
            return out;
        }

    } // namespace

    std::string DecodeNetworkText(const std::string &value) {
        std::string out;
        for (std::size_t i = 0; i < value.size(); ++i) {
            if (value[i] != '%') out += value[i];
            else {
                if (i + 2 >= value.size()) Fail("Invalid percent encoding");
                out += static_cast<char>(Integer(value.substr(i + 1, 2), 16));
                i += 2;
            }
        }
        Validate(out);
        return out;
    }

    NetworkMessage ParseNetworkMessage(const std::string &xml, const bool official_server) {
        if (xml.empty() || xml.size() > kNetworkMessageLimit) Fail("Empty or oversized network message");
        Validate(xml);
        NetworkMessage message;
        std::size_t p = 0;
        auto spaces = [&] { while (p < xml.size() && Space(xml[p])) ++p; };
        auto expect = [&](const char c) { if (p == xml.size() || xml[p++] != c) Fail("Malformed protocol XML"); };
        auto name = [&] {
            const auto start = p;
            if (p == xml.size() || !Letter(xml[p])) Fail("Invalid protocol XML name");
            while (p < xml.size() && Name(xml[p])) ++p;
            if (p - start > 64) Fail("Protocol XML name exceeds limit");
            return xml.substr(start, p - start);
        };
        spaces();
        expect('<');
        message.tag = name();
        // Origin extracts HELO attributes up to the closing quote without resolving XML entities.
        const bool literal_values = official_server && message.tag == "HELO";
        for (;;) {
            const bool spaced = p < xml.size() && Space(xml[p]);
            spaces();
            if (p < xml.size() && xml[p] == '/') {
                ++p;
                expect('>');
                break;
            }
            // Some original server messages use <TAG ...> without a closing slash.
            if (p < xml.size() && xml[p] == '>') {
                ++p;
                break;
            }
            if (!spaced) Fail("Missing attribute whitespace");
            const auto key = name();
            spaces();
            expect('=');
            spaces();
            if (p == xml.size() || (xml[p] != '\'' && xml[p] != '"')) Fail("Unquoted protocol attribute");
            const char quote = xml[p++];
            const auto start = p;
            while (p < xml.size() && xml[p] != quote) {
                if (xml[p] == '<') Fail("Invalid protocol attribute");
                ++p;
            }
            if (p == xml.size()) Fail("Truncated protocol attribute");
            const auto raw = xml.substr(start, p - start);
            if (message.attributes.size() >= 256 || !message.attributes.emplace(
                    key, literal_values ? raw : Entities(raw)).second)
                Fail("Duplicate or excessive protocol attributes");
            ++p;
        }
        spaces();
        if (p != xml.size()) Fail("Trailing or nested protocol XML");
        return message;
    }

    std::vector<std::string> NetworkFramer::Feed(const char *bytes, const std::size_t count) {
        std::vector<std::string> messages;
        for (std::size_t i = 0; i < count; ++i) {
            if (bytes[i] == '\0') {
                if (!pending_.empty()) {
                    messages.push_back(std::move(pending_));
                    pending_.clear();
                }
            } else {
                if (pending_.size() == kNetworkMessageLimit) Fail("Network message exceeds 64 KiB limit");
                pending_ += bytes[i];
            }
        }
        return messages;
    }

    std::string AuthenticationResponse(const std::string &challenge) {
        if (challenge.empty() || challenge.size() > 128 || !std::all_of(challenge.begin(), challenge.end(), [](const char c) {
            return Letter(c) || Digit(c) || c == '-' || c == '.';
        }))
            Fail("Invalid authentication token");
        if (challenge.size() != 17 || challenge[8] != '-') return challenge; // Original opaque-token fallback.
        if (!std::all_of(challenge.begin(), challenge.begin() + 8, Digit)) Fail(
            "Invalid authentication challenge prefix");
        static constexpr std::array<std::uint32_t, 12> table{
            {
                4129170786u, 3225465664u, 643717713u, 3921069994u,
                3593408605u, 38016083u, 2763975236u, 3634488961u, 3889429448u, 568446438u, 3275163606u, 4107603335u
            }
        };
        const auto index = Integer("2" + challenge.substr(2, 6)) % (60u - static_cast<unsigned char>(challenge[7]));
        const auto value = Integer(challenge.substr(9), 16) ^ table[index];
        std::ostringstream out;
        out << challenge.substr(0, 9) << std::hex << std::setfill('0') << std::setw(8) << value;
        return out.str();
    }

    std::vector<int> DecodeLobbyNumbers(const std::string &value) {
        // Original 004c6630: base-52 letter values and decimal runs of comma separators.
        if (value.empty() || value.size() > 4096) Fail("Invalid lobby statistics");
        std::string numbers;
        unsigned number = 0, commas = 0;
        int sign = 1;
        auto append = [&](const std::string &part) {
            if (numbers.size() + part.size() > 16384) Fail("Lobby statistics exceed limit");
            numbers += part;
        };
        for (char c: value) {
            if (c == '-') sign = -1;
            else if (Letter(c) && c != '_') {
                if (commas) {
                    append(std::string(commas, ','));
                    commas = 0;
                }
                const unsigned digit = static_cast<unsigned>(c >= 'a' ? c - 'a' + 26 : c - 'A');
                if (number > (2147483647u - digit) / 52) Fail("Lobby statistic overflow");
                number = number * 52 + digit;
            } else if (Digit(c)) {
                if (number) append(std::to_string(sign * static_cast<int>(number)));
                number = 0;
                sign = 1;
                commas = commas * 10 + static_cast<unsigned>(c - '0');
                if (commas > 1024) Fail("Lobby statistic run exceeds limit");
            } else Fail("Invalid lobby statistic encoding");
        }
        append(std::to_string(sign * static_cast<int>(number)));
        std::vector<int> result;
        std::size_t start = 0;
        for (;;) {
            const auto end = numbers.find(',', start);
            const auto token = numbers.substr(start, end - start);
            int n = 0;
            if (!token.empty()) {
                const auto parsed = std::from_chars(token.data(), token.data() + token.size(), n);
                if (parsed.ec != std::errc() || parsed.ptr != token.data() + token.size()) Fail(
                    "Invalid decoded statistic");
            }
            result.push_back(n);
            if (result.size() > 1024) Fail("Too many lobby statistics");
            if (end == std::string::npos) break;
            start = end + 1;
        }
        return result;
    }

    ServerErrorNotice FormatServerError(const int code, const std::string &wire_text) {
        if (!wire_text.empty()) {
            ValidateUtf8(wire_text);
            return {"Error", wire_text, true};
        }
        // Exact mapping from tenhou.exe.c:63966-64319 (WM_USER 0x8202).
        switch (code) {
            case 0: return {"Error", "已正常结束", false};
            case 1001: return {"Error", "玩家名不正确", true};
            case 1002: return {"Error", "使用该玩家名需要通过 ID 登录", true};
            case 1003: return {"Error", "ID 不正确\r\n\r\n■ 关于玩家 ID 重新发行\r\nhttp://tenhou.net/support.html\r\n\r\n超过 180 天未进行对局的 ID 会被停用或删除。\r\n七段以上且可确认付费结算信息的账号有可能恢复，请尽早联系官方。", true};
            case 1004: return {"Error", "该玩家已在线", true};
            case 1005: return {"Error", "该玩家已注册", true};
            case 1006: return {"Error", "当前连接来源约 7 天内无法进行注册／举报／创建大会会场", true};
            case 1007: return {"Error", "注册失败", true};
            case 1008: return {"Error", "当前连接来源在一定期间内无法访问", true};
            case 1009: return {"Error", "举报已完成", true};
            case 1010: return {"Error", "该功能在个室中不可用\r\n请移动至排名战大厅", true};
            case 1011: return {"Error", "举报失败", true};
            case 1012: return {"玩家 ID 注册", "注册已完成\r\n\r\n※请注意玩家 ID 无法重新发行\r\n\r\n新的玩家 ID 已显示在输入框中。\r\n关闭此对话框后，请点击登录入场。", false};
            case 1013: return {"Error", "当前没有可观战的对局", false};
            case 1014: return {"Error", "未找到观战信息\r\n该对局可能已经结束", false};
            case 1015: return {"Error", "大会会场创建已完成", true};
            case 1016: return {"Error", "大会会场创建失败", true};
            case 1017: return {"Error", "预约中无法浏览牌谱", false};
            case 1018: return {"Error", "预约中无法观战", false};
            case 1019: return {"Error", "未找到大会会场", false};
            case 1020: return {"Error", "参加所需的有效期限不足", false};
            case 1021: return {"Error", "外部登录服务器无响应", true};
            case 1022: return {"Error", "无法移动至 ID 不兼容的会场", true};
            case 1023: return {"Error", "临时使用的外部 ID 已过期", true};
            case 1024: return {"Error", "未满足参加所需的条件", true};
            case 1025: return {"Error", "外部 ID 无法使用", true};
            case 1026: return {"Error", "该会场无法进行参赛报名", false};
            case 1027: return {"参赛报名", "参赛报名已完成", false};
            case 1028: return {"Error", "已完成参赛报名", false};
            case 2000: return {"Error", "不允许连接到该域名", true};
            case 2001: return {"Error", "已在其他客户端连接", true};
            case 2002: return {"Error", "与服务器的连接已断开\r\n是否重新连接？", false};
            case 2003: return {"Error", "牌谱读取失败\r\n进行中的牌谱将在对局结束后可供浏览", false};
            case 2004: return {"Error", "请选择性别", true};
            default:
                return {"Error", code >= 0 ? "ERR=" + std::to_string(code) : "服务器拒绝了本次连接", true};
        }
    }

    const char *LobbyPhaseName(const LobbyPhase phase) {
        switch (phase) {
            case LobbyPhase::Disconnected: return "Disconnected";
            case LobbyPhase::Resolving: return "Resolving";
            case LobbyPhase::Connecting: return "Connecting";
            case LobbyPhase::AwaitHelo: return "Awaiting HELO";
            case LobbyPhase::AwaitLobby: return "Awaiting lobby confirmation";
            case LobbyPhase::Lobby: return "Lobby";
            case LobbyPhase::AwaitRecovery: return "Awaiting recovery snapshot";
            case LobbyPhase::Game: return "Read-only game";
            case LobbyPhase::Matching: return "Matching";
            case LobbyPhase::Cancelling: return "Cancelling match";
            case LobbyPhase::ChangingLobby: return "Changing lobby";
            case LobbyPhase::MatchComplete: return "Match complete";
            case LobbyPhase::AwaitReturn: return "Returning to lobby";
            default: return "Error";
        }
    }


    std::string NetworkJson(const std::string &value) {
        std::ostringstream out;
        out << '"';
        for (unsigned char c: value) {
            if (c == '"' || c == '\\') out << '\\' << c;
            else if (c < 32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<unsigned>(c);
            else out << c;
        }
        out << '"';
        return out.str();
    }
} // namespace tenhou
