#include "network_protocol.hpp"
#include <windows.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <set>

namespace tenhou {
    namespace {
        [[noreturn]] void Fail(const char *message) { throw std::runtime_error(message); }
        bool Space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
        bool Letter(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }
        bool Digit(char c) { return c >= '0' && c <= '9'; }
        bool Name(char c) { return Letter(c) || Digit(c) || c == '-' || c == '.' || c == ':'; }

        unsigned Integer(const std::string &value, int base = 10) {
            unsigned result = 0;
            const auto p = std::from_chars(value.data(), value.data() + value.size(), result, base);
            if (value.empty() || p.ec != std::errc() || p.ptr != value.data() + value.size()) Fail(
                "Invalid protocol number");
            return result;
        }

        void Validate(const std::string &value) {
            (void) NetworkWide(value);
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
                    std::wstring wide;
                    if (code <= 0xffff) wide += static_cast<wchar_t>(code);
                    else {
                        code -= 0x10000;
                        wide += static_cast<wchar_t>(0xd800 + (code >> 10));
                        wide += static_cast<wchar_t>(0xdc00 + (code & 1023));
                    }
                    out += NetworkUtf8(wide);
                } else Fail("Unsupported XML entity");
                i = end;
            }
            Validate(out);
            return out;
        }

        std::string Attribute(const NetworkMessage &message, const std::string &key) {
            const auto found = message.attributes.find(key);
            return found == message.attributes.end() ? "" : found->second;
        }

        std::string Escape(const std::string &value) {
            std::string out;
            for (char c: value) {
                if (c == '&') out += "&amp;";
                else if (c == '"') out += "&quot;";
                else if (c == '<') out += "&lt;";
                else if (c == '>') out += "&gt;";
                else out += c;
            }
            return out;
        }
    } // namespace

    std::wstring NetworkWide(const std::string &value) {
        if (value.empty()) return {};
        const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                             static_cast<int>(value.size()), nullptr, 0);
        if (!size) Fail("Invalid UTF-8 protocol text");
        std::wstring out(static_cast<std::size_t>(size), L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), out.data(),
                            size);
        return out;
    }

    std::string NetworkUtf8(const std::wstring &value) {
        if (value.empty()) return {};
        const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                                             static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (!size) Fail("Invalid Unicode input");
        std::string out(static_cast<std::size_t>(size), '\0');
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), out.data(),
                            size, nullptr, nullptr);
        return out;
    }

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

    NetworkMessage ParseNetworkMessage(const std::string &xml, bool official_server) {
        if (xml.empty() || xml.size() > kNetworkMessageLimit) Fail("Empty or oversized network message");
        Validate(xml);
        NetworkMessage message;
        std::size_t p = 0;
        auto spaces = [&] { while (p < xml.size() && Space(xml[p])) ++p; };
        auto expect = [&](char c) { if (p == xml.size() || xml[p++] != c) Fail("Malformed protocol XML"); };
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

    std::vector<std::string> NetworkFramer::Feed(const char *bytes, std::size_t count) {
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
        if (challenge.empty() || challenge.size() > 128 || !std::all_of(challenge.begin(), challenge.end(), [](char c) {
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

    ServerErrorNotice FormatServerError(int code, const std::string &wire_text) {
        if (!wire_text.empty()) return {L"Error", NetworkWide(wire_text), true};
        // Exact mapping from tenhou.exe.c:63966-64319 (WM_USER 0x8202).
        switch (code) {
            case 0: return {L"Error", L"已正常结束", false};
            case 1001: return {L"Error", L"玩家名不正确", true};
            case 1002: return {L"Error", L"使用该玩家名需要通过 ID 登录", true};
            case 1003: return {L"Error", L"ID 不正确\r\n\r\n■ 关于玩家 ID 重新发行\r\nhttp://tenhou.net/support.html\r\n\r\n超过 180 天未进行对局的 ID 会被停用或删除。\r\n七段以上且可确认付费结算信息的账号有可能恢复，请尽早联系官方。", true};
            case 1004: return {L"Error", L"该玩家已在线", true};
            case 1005: return {L"Error", L"该玩家已注册", true};
            case 1006: return {L"Error", L"当前连接来源约 7 天内无法进行注册／举报／创建大会会场", true};
            case 1007: return {L"Error", L"注册失败", true};
            case 1008: return {L"Error", L"当前连接来源在一定期间内无法访问", true};
            case 1009: return {L"Error", L"举报已完成", true};
            case 1010: return {L"Error", L"该功能在个室中不可用\r\n请移动至排名战大厅", true};
            case 1011: return {L"Error", L"举报失败", true};
            case 1012: return {L"玩家 ID 注册", L"注册已完成\r\n\r\n※请注意玩家 ID 无法重新发行\r\n\r\n新的玩家 ID 已显示在输入框中。\r\n关闭此对话框后，请点击登录入场。", false};
            case 1013: return {L"Error", L"当前没有可观战的对局", false};
            case 1014: return {L"Error", L"未找到观战信息\r\n该对局可能已经结束", false};
            case 1015: return {L"Error", L"大会会场创建已完成", true};
            case 1016: return {L"Error", L"大会会场创建失败", true};
            case 1017: return {L"Error", L"预约中无法浏览牌谱", false};
            case 1018: return {L"Error", L"预约中无法观战", false};
            case 1019: return {L"Error", L"未找到大会会场", false};
            case 1020: return {L"Error", L"参加所需的有效期限不足", false};
            case 1021: return {L"Error", L"外部登录服务器无响应", true};
            case 1022: return {L"Error", L"无法移动至 ID 不兼容的会场", true};
            case 1023: return {L"Error", L"临时使用的外部 ID 已过期", true};
            case 1024: return {L"Error", L"未满足参加所需的条件", true};
            case 1025: return {L"Error", L"外部 ID 无法使用", true};
            case 1026: return {L"Error", L"该会场无法进行参赛报名", false};
            case 1027: return {L"参赛报名", L"参赛报名已完成", false};
            case 1028: return {L"Error", L"已完成参赛报名", false};
            case 2000: return {L"Error", L"不允许连接到该域名", true};
            case 2001: return {L"Error", L"已在其他客户端连接", true};
            case 2002: return {L"Error", L"与服务器的连接已断开\r\n是否重新连接？", false};
            case 2003: return {L"Error", L"牌谱读取失败\r\n进行中的牌谱将在对局结束后可供浏览", false};
            case 2004: return {L"Error", L"请选择性别", true};
            default:
                return {L"Error", code >= 0 ? L"ERR=" + std::to_wstring(code) : L"服务器拒绝了本次连接", true};
        }
    }

    const char *LobbyPhaseName(LobbyPhase phase) {
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

    std::vector<std::string> LobbySession::Connected(const std::string &identity, char gender, std::uint64_t revision) {
        if (match_games && (!interactive_games || observe_games)) Fail(
            "Matchmaking requires local interactive mode without read-only observation");
        // The fixed original accepts NoName or a 19-character registered ID, not arbitrary guest names.
        const bool registered = identity.size() == 19 && identity.compare(0, 2, "ID") == 0 &&
                                std::all_of(identity.begin(), identity.end(), [](char c) {
                                    return (Letter(c) && c != '_') || Digit(c) || c == '-';
                                });
        const bool local_account = require_native && identity.size() >= 3 && identity.size() <= 32 && std::all_of(
                                       identity.begin(), identity.end(), [](char c) {
                                           return Letter(c) || Digit(c) || c == '-';
                                       });
        if ((identity != "NoName" && !registered && !local_account) || (gender != 'M' && gender != 'F'))
            Fail("请输入本机账号，或使用 NoName 访客登录");
        snapshot = {};
        game_ = {};
        actions_ = {};
        actions_.official = official;
        actions_.snapshot.enabled = interactive_games;
        snapshot.actions = actions_.snapshot;
        snapshot.phase = LobbyPhase::AwaitHelo;
        rejoin_received_ = recovery_go_ = false;
        official_start_ready_ = false;
        official_profile_dirty_ = official;
        official_identity_ = identity;
        official_gender_ = gender;
        actions_.snapshot.revision = revision;
        snapshot.actions = actions_.snapshot;
        snapshot.recovering = resume_connection;
        snapshot.silent_recovery = silent_recovery;
        snapshot.official = official;
        snapshot.matchmaking = match_games;
        snapshot.lobby_revision = revision;
        room_ = origin_epoch_ = origin_request_ = 0;
        official_profile_type_ = -1;
        const auto game_id = official && !official_game_id.empty() ? " gpid=\"" + Escape(official_game_id) + "\"" : "";
        return {"<HELO name=\"" + Escape(identity) + "\"" + game_id + " sx=\"" + gender + "\" tid=\"w0\" />"};
    }

    std::vector<std::string> LobbySession::Receive(const NetworkMessage &message) {
        auto next = *this;
        auto replies = next.Apply(message);
        *this = std::move(next);
        return replies;
    }

    std::vector<std::string> LobbySession::Apply(const NetworkMessage &message) {
        const auto &tag = message.tag;
        if (snapshot.phase == LobbyPhase::Disconnected || snapshot.phase == LobbyPhase::Error || snapshot.phase ==
            LobbyPhase::Resolving || snapshot.phase == LobbyPhase::Connecting)
            Fail("Message outside connected session");
        ++snapshot.received;
        snapshot.last_tag = tag;
        if (tag == "ERR") {
            const auto text = Attribute(message, "text");
            const auto code_str = Attribute(message, "code");
            int code = -1;
            if (!code_str.empty() && code_str.size() <= 10 && std::all_of(code_str.begin(), code_str.end(), Digit))
                code = static_cast<int>(Integer(code_str));
            const auto notice = FormatServerError(code, text);
            // Non-fatal official ERR codes (tenhou.exe.c:63966-64318) display a dialog without closing the lobby.
            if (official && !notice.fatal && snapshot.phase != LobbyPhase::AwaitHelo &&
                snapshot.phase != LobbyPhase::AwaitLobby) {
                snapshot.server_error_receipt = snapshot.received;
                snapshot.server_error_code = code;
                snapshot.server_error_text = text;
                snapshot.server_error_fatal = false;
                return {};
            }
            if (!text.empty()) throw ServerRejection(code, text, text);
            if (code >= 0) throw ServerRejection(code, {}, "Server rejected session (ERR code=" + code_str + ")");
            throw ServerRejection(-1, {}, "Server rejected session (ERR)");
        }
        if (tag == "BYE" && !message.attributes.count("who")) Fail("Server ended session (BYE)");
        const auto phase = snapshot.phase;
        const auto previous_room = official && tag == "CHAT" ? snapshot.lobby : std::string{};
        if (official && ApplyOfficialLobby(message)) {
            if (!previous_room.empty() && previous_room != snapshot.lobby)
                return {RequestOfficialProfile(snapshot.origin_lobby->ranked() ?
                    static_cast<int>(preferred_monthly_profile) : -1)};
            return {};
        }
        if (snapshot.native_service) {
            if (origin_lobby && ApplyOriginLobby(message)) return {};
            if (tag == "READYACK") {
                // STARTREADY can be acknowledged after the initial VSTATE offers.
                // Only NEXTREADY consumes an action; opening acknowledgements do not.
                if (snapshot.variant_active && Attribute(message, "table") == snapshot.exit_table &&
                    Attribute(message, "kind") == "NEXTREADY" && snapshot.actions.pending == "next") {
                    snapshot.actions.pending.clear();
                    snapshot.actions.choices.clear();
                    ++snapshot.actions.revision;
                }
                return {};
            }
            if (tag == "VGO") {
                const auto players = snapshot.game.players;
                snapshot.variant_active = true;
                snapshot.variant_state = message.attributes;
                snapshot.variant = {};
                snapshot.exit_table = Attribute(message, "table");
                snapshot.exit_allowed = Attribute(message, "exit") == "1";
                snapshot.ranked = false;
                snapshot.game = {};
                for (int i = 0; i < 4; ++i) {
                    snapshot.game.players[i].gender = players[i].gender;
                    snapshot.game.players[i].rank = players[i].rank;
                }
                snapshot.actions = {};
                snapshot.actions.enabled = interactive_games;
                snapshot.phase = LobbyPhase::Game;
                return snapshot.recovering ? std::vector<std::string>{} : std::vector<std::string>{"<GOK/>"};
            }
            if (tag == "VSTATE") {
                if (!snapshot.variant_active) Fail("Variant state before VGO");
                // VSTATE is a full snapshot. Retaining absent handN fields would retain
                // entities from an older open-hand view across recovery.
                auto fields = message.attributes;
                for (const auto &entry: snapshot.variant_state)
                    if (entry.first == "rule" || entry.first == "title" || entry.first == "table" ||
                        entry.first == "n0" || entry.first == "n1" || entry.first == "n2" || entry.first == "n3")
                        fields.emplace(entry);
                auto variant = ParseVariantState(fields);
                const bool same_preparation = !snapshot.recovering && snapshot.variant.Preparing() &&
                    variant.phase == snapshot.variant.phase && variant.hand_number == snapshot.variant.hand_number &&
                    variant.local == snapshot.variant.local;
                snapshot.variant = std::move(variant);
                snapshot.variant_state = std::move(fields);
                const auto window = Integer(Attribute(message, "window"));
                if (window != snapshot.native_window || Attribute(message, "actions").empty() || snapshot.recovering)
                    snapshot.actions.pending.clear();
                auto choices = ParseVariantActions(Attribute(message, "actions"), snapshot.exit_table,
                                                   window, snapshot.variant.wall_total);
                const bool unchanged_offer = same_preparation && window == snapshot.native_window &&
                    std::equal(choices.begin(), choices.end(), snapshot.actions.choices.begin(), snapshot.actions.choices.end(),
                        [](const OnlineAction &a, const OnlineAction &b) {
                            return a.kind == b.kind && a.tiles == b.tiles && a.suit == b.suit && a.messages == b.messages;
                        });
                snapshot.native_window = window;
                if (!unchanged_offer) ++snapshot.actions.revision;
                snapshot.actions.choices = std::move(choices);
                if (snapshot.game.variant && snapshot.game.round != snapshot.variant.hand_number - 1 &&
                    !snapshot.game.settlement.receipts.empty()) {
                    snapshot.last_settlement = snapshot.game.settlement;
                    for (int i = 0; i < 4; ++i) snapshot.last_result_names[i] = snapshot.game.players[i].name;
                    snapshot.last_result_dealer = snapshot.game.dealer;
                    snapshot.last_result_honba = snapshot.last_result_sticks = -1;
                    snapshot.last_result_red = false;
                }
                auto projected = VariantTableState(snapshot.variant, snapshot.variant_state);
                for (int i = 0; i < 4; ++i) {
                    projected.players[i].gender = snapshot.game.players[i].gender;
                    projected.players[i].rank = snapshot.game.players[i].rank;
                    projected.players[i].connected = snapshot.game.players[i].connected;
                }
                snapshot.game = std::move(projected);
                const auto stage = Attribute(message, "phase");
                snapshot.game.complete = stage == "complete";
                snapshot.phase = snapshot.game.complete ? LobbyPhase::MatchComplete : LobbyPhase::Game;
                if (stage == "ended") snapshot.actions.choices.push_back({"next", "下一盘", {}, {"<NEXTREADY/>"}});
                return {};
            }
            if (tag == "CATALOGBEGIN") {
                snapshot.catalog_ready = false;
                snapshot.rules.clear();
                snapshot.queues.clear();
                return {};
            }
            if (tag == "RULE") {
                const auto id = Attribute(message, "id");
                if (id.empty() || id.size() > 48 || snapshot.rules.size() >= 64 || snapshot.rules.count(id))
                    Fail("Invalid rule catalog");
                snapshot.rules[id] = {id, Attribute(message, "name"), Attribute(message, "presentation"), {}};
                return {};
            }
            if (tag == "RULEFIELD") {
                auto rule = snapshot.rules.find(Attribute(message, "rule"));
                const auto key = Attribute(message, "key");
                if (rule == snapshot.rules.end() || rule->second.fields.size() >= 32 || key.empty() ||
                    std::any_of(rule->second.fields.begin(), rule->second.fields.end(), [&](const RuleField &f) { return f.key == key; }))
                    Fail("Invalid rule field");
                rule->second.fields.push_back({key, Attribute(message, "label"), Attribute(message, "default"),
                    Attribute(message, "min"), Attribute(message, "max"), Integer(Attribute(message, "step")), {}});
                return {};
            }
            if (tag == "RULECHOICE") {
                auto rule = snapshot.rules.find(Attribute(message, "rule"));
                if (rule == snapshot.rules.end()) Fail("Choice without a rule");
                auto &fields = rule->second.fields;
                auto field = std::find_if(fields.begin(), fields.end(), [&](const RuleField &f) { return f.key == Attribute(message, "key"); });
                if (field == fields.end() || field->choices.size() >= 64) Fail("Invalid rule choice");
                field->choices.push_back({Attribute(message, "value"), Attribute(message, "label")});
                return {};
            }
            if (tag == "QUEUEDEF") {
                if (snapshot.queues.size() >= 64) Fail("Too many server queues");
                snapshot.queues.push_back({Integer(Attribute(message, "id")), Integer(Attribute(message, "type")),
                    Attribute(message, "name"), Attribute(message, "rule")});
                return {};
            }
            if (tag == "CATALOGEND") {
                for (const auto &queue: snapshot.queues)
                    if (!snapshot.rules.count(queue.rule)) Fail("Queue without a rule");
                snapshot.catalog_ready = true;
                ++snapshot.lobby_revision;
                return {};
            }
            if (tag == "PROFILE") {
                snapshot.profile = message.attributes;
                return {};
            }
            if (tag == "REPLAYBEGIN" || tag == "REPLAYCHUNK" || tag == "REPLAYEND" || tag == "REPLAYERROR") return {};
            if (message.attributes.count("window")) snapshot.native_window = Integer(Attribute(message, "window"));
            if (tag == "SESSION") {
                const auto token = Attribute(message, "token");
                if (token.size() != 64 || !std::all_of(token.begin(), token.end(), [](char c) {
                    return Digit(c) || (c >= 'a' && c <= 'f');
                })) Fail("Invalid native session token");
                snapshot.native_token = token;
                native_token = token;
                snapshot.native_request = Integer(Attribute(message, "rid"));
                return {};
            }
            if (tag == "ACCEPT" || tag == "REJECT") {
                if (snapshot.native_pending.empty() || Integer(Attribute(message, "rid")) != snapshot.native_request)
                    Fail("Unexpected native request receipt");
                snapshot.native_error = tag == "REJECT" ? Attribute(message, "code") : "";
                if (tag == "REJECT") snapshot.native_pending.clear();
                if (tag == "ACCEPT") {
                    room_ = Integer(Attribute(message, "room"));
                    if (room_ != 0 && (room_ < 1000 || room_ > 9999)) Fail("Invalid allocated room");
                    std::ostringstream name;
                    name << 'L' << std::setw(4) << std::setfill('0') << room_;
                    snapshot.lobby = name.str();
                    snapshot.room_settings.clear();
                    if (!room_) snapshot.native_pending.clear();
                }
                ++snapshot.lobby_revision;
                return {};
            }
            if (tag == "QUEUED") {
                if (phase != LobbyPhase::Matching || Integer(Attribute(message, "room")) != room_ ||
                    Integer(Attribute(message, "type")) != snapshot.match_type) Fail("Queue receipt mismatch");
                snapshot.queue_confirmed = true;
                return {};
            }
            if (tag == "QUEUEREJECT") {
                if (phase != LobbyPhase::Matching) Fail("Queue rejection outside matching");
                snapshot.phase = LobbyPhase::Lobby;
                snapshot.queue_confirmed = false;
                snapshot.native_error = Attribute(message, "code");
                ++snapshot.lobby_revision;
                return {};
            }
            if (tag == "RESTORE") {
                const auto revision = snapshot.actions.revision + 1;
                game_ = {};
                actions_ = {};
                actions_.snapshot.enabled = interactive_games;
                actions_.snapshot.revision = revision;
                snapshot.game = {};
                snapshot.actions = actions_.snapshot;
                snapshot.recovering = true;
                snapshot.phase = LobbyPhase::Lobby;
                return {};
            }
            if (tag == "RESTORED") {
                snapshot.recovering = false;
                resume_connection = false;
                snapshot.phase = snapshot.variant_active ? (snapshot.game.complete ? LobbyPhase::MatchComplete : LobbyPhase::Game) : snapshot.game.complete
                                     ? LobbyPhase::MatchComplete
                                     : snapshot.game.started
                                           ? LobbyPhase::Game
                                           : LobbyPhase::Lobby;
                return {};
            }
            if (tag == "EXITVOTE") {
                if (Attribute(message, "table") != snapshot.exit_table) return {};
                snapshot.exit_voting = Attribute(message, "show") == "1";
                snapshot.exit_pending = false;
                if (snapshot.exit_voting) {
                    snapshot.exit_vote_id = Integer(Attribute(message, "id"));
                    snapshot.exit_vote_ms = Integer(Attribute(message, "ms"));
                    snapshot.exit_total = Integer(Attribute(message, "total"));
                    snapshot.exit_yes = Integer(Attribute(message, "yes"));
                    snapshot.exit_no = Integer(Attribute(message, "no"));
                    const auto choice = Attribute(message, "choice");
                    snapshot.exit_choice = choice == "-1" ? -1 : static_cast<int>(Integer(choice));
                    if (!snapshot.exit_vote_id || snapshot.exit_vote_ms > 30000 || snapshot.exit_total < 2 || snapshot.
                        exit_total > 4 ||
                        snapshot.exit_yes + snapshot.exit_no > snapshot.exit_total || snapshot.exit_choice > 1)
                        Fail("Invalid exit vote snapshot");
                }
                snapshot.exit_ballot_pending = false;
                return {};
            }
            if (tag == "TABLEEND") {
                if (Attribute(message, "table") != snapshot.exit_table || Attribute(message, "reason") != "exit") Fail(
                    "Unexpected table end");
                const auto revision = snapshot.actions.revision + 1;
                game_ = {};
                actions_ = {};
                actions_.snapshot.enabled = interactive_games;
                actions_.snapshot.revision = revision;
                snapshot.game = {};
                snapshot.actions = actions_.snapshot;
                snapshot.last_settlement = {};
                snapshot.variant_active = false;
                snapshot.variant_state.clear();
                snapshot.last_result_names = {};
                snapshot.exit_table.clear();
                snapshot.exit_allowed = snapshot.exit_pending =
                                        snapshot.exit_voting = snapshot.exit_ballot_pending = false;
                snapshot.table_paused = snapshot.recovering = false;
                snapshot.native_window = 0;
                snapshot.phase = LobbyPhase::Lobby;
                snapshot.target_lobby.clear();
                snapshot.match_result = "exit";
                ++snapshot.lobby_revision;
                return {};
            }
            if (tag == "PAUSED" || tag == "TABLEERROR") {
                snapshot.table_paused = tag == "TABLEERROR" || Attribute(message, "show") == "1";
                if (tag == "PAUSED" && message.attributes.count("offline")) {
                    const auto offline = Integer(Attribute(message, "offline"));
                    if (offline > 15) Fail("Invalid offline seat mask");
                    for (int i = 0; i < 4; ++i) {
                        const bool connected = !SeatPresent(offline, i);
                        snapshot.game.players[i].connected = connected;
                        game_.state.players[i].connected = connected;
                    }
                }
                return {};
            }
            if (tag == "READYREQUEST") { return {Attribute(message, "kind") == "gok" ? "<GOK />" : "<STARTREADY />"}; }
            if (tag == "DRAINING") {
                snapshot.native_error = "DRAINING";
                return {};
            }
        }
        if (tag == "ROOM" && match_games && phase == LobbyPhase::Lobby) {
            if (Attribute(message, "room") != std::to_string(room_)) Fail("Room settings do not match current lobby");
            if (!message.attributes.count("revision") || !message.attributes.count("type") ||
                !snapshot.rules.count(Attribute(message, "rule"))) Fail("Incomplete room settings");
            snapshot.room_settings = message.attributes;
            if (snapshot.native_service && Attribute(message, "rid") == std::to_string(snapshot.native_request))
                snapshot.native_pending.clear();
            ++snapshot.lobby_revision;
            return {};
        }
        const bool waiting_lobby = phase == LobbyPhase::Cancelling || phase == LobbyPhase::ChangingLobby || phase ==
                                   LobbyPhase::AwaitReturn;
        // A known lobby identifier is the controlled-service confirmation barrier.
        // LN statistics and CS without a matching lobby cannot acknowledge a request.
        if (match_games && (tag == "CS" || tag == "CHAT") && message.attributes.count("lobby")) {
            const auto lobby = Attribute(message, "lobby");
            if (!waiting_lobby && (phase == LobbyPhase::AwaitLobby || phase == LobbyPhase::Lobby) &&
                lobby.size() == 5 && lobby[0] == 'L' && std::all_of(lobby.begin() + 1, lobby.end(), Digit)) {
                const auto room = Integer(lobby.substr(1));
                if (room == 0 || room >= 1000) {
                    if (room_ != room) snapshot.room_settings.clear();
                    room_ = room;
                }
            }
            if (waiting_lobby && lobby == snapshot.target_lobby) {
                const auto revision = snapshot.actions.revision + 1;
                game_ = {};
                actions_ = {};
                actions_.snapshot.enabled = interactive_games;
                actions_.snapshot.revision = revision;
                snapshot.game = {};
                snapshot.actions = actions_.snapshot;
                snapshot.phase = LobbyPhase::Lobby;
                snapshot.target_lobby.clear();
                ++snapshot.lobby_revision;
            }
        }
        if (tag == "REJOIN") {
            const bool official_phase = official &&
                (phase == LobbyPhase::AwaitLobby || phase == LobbyPhase::Lobby || phase == LobbyPhase::Matching);
            if (!recover_games || (!resume_connection && !official) || !interactive_games || rejoin_received_ ||
                recovery_go_ || (!official_phase && phase != LobbyPhase::Lobby))
                Fail("Unexpected or disabled REJOIN");
            const auto type = Attribute(message, "t");
            if (official) {
                if (type.empty() || type.size() > 128 || !std::all_of(type.begin(), type.end(), [](unsigned char c) {
                    return c >= 33 && c <= 126 && c != '"' && c != '\'' && c != '<' && c != '>' && c != '&';
                })) Fail("Invalid REJOIN t");
            } else {
                if (type.empty() || type.size() > 64) Fail("Invalid REJOIN t");
                std::size_t start = 0;
                int fields = 0;
                for (;;) {
                    const auto end = type.find(',', start);
                    if (Integer(type.substr(start, end == std::string::npos ? end : end - start)) > 65535 || ++fields > 4)
                        Fail("Invalid REJOIN t range");
                    if (end == std::string::npos) break;
                    start = end + 1;
                }
            }
            rejoin_received_ = true;
            if (official && phase != LobbyPhase::Matching) {
                resume_connection = snapshot.recovering = true;
                snapshot.phase = LobbyPhase::AwaitRecovery;
            }
            return {"<JOIN t=\"" + type + "\" />"};
        }
        if (tag == "REINIT" && !recover_games) Fail("Online rejoin is not supported in this stage");
        if ((observe_games || interactive_games) && OnlineGame::Handles(tag)) {
            // After official mid-game <BYE />, ignore stale in-flight game frames until <HELO> reopens the lobby.
            if (official && phase == LobbyPhase::AwaitReturn) return {};
            if (tag == "GO") { snapshot.variant_active = false; snapshot.variant_state.clear(); }
            if (official && tag == "GO" && (Integer(Attribute(message, "type")) & 0x810) == 0x810) return {};
            if (match_games && !snapshot.recovering) {
                if (tag == "GO") {
                    if (phase != LobbyPhase::Matching && phase != LobbyPhase::Cancelling &&
                        !(official && phase == LobbyPhase::Lobby)) Fail(
                        "GO outside an active match request");
                    if (snapshot.origin_lobby && !official) {
                        const auto type = Integer(Attribute(message, "reservation"));
                        if (Attribute(message, "lobby") != snapshot.lobby || !snapshot.origin_lobby->Reserved(type))
                            Fail("GO did not select a confirmed reservation");
                    } else if (!official && Integer(Attribute(message, "type")) != snapshot.match_type) Fail(
                        "GO rule differs from requested match");
                    snapshot.target_lobby.clear();
                    ++snapshot.lobby_revision;
                } else if (tag == "UN" || tag == "TAIKYOKU" || tag == "SAIKAI") {
                    if (phase != LobbyPhase::Matching && phase != LobbyPhase::Cancelling && phase != LobbyPhase::Game &&
                        phase != LobbyPhase::AwaitRecovery)
                        Fail("Game identity outside matchmaking/game");
                    if (!game_.state.started && tag == "UN") {
                        game_.Receive(message);
                        snapshot.game = game_.state;
                        return {};
                    }
                } else if (phase != LobbyPhase::Game && phase != LobbyPhase::AwaitRecovery && !(
                               phase == LobbyPhase::MatchComplete && tag == "PROF"))
                    Fail("Game event outside active game");
            }
            if (snapshot.phase != LobbyPhase::Lobby && snapshot.phase != LobbyPhase::Matching && snapshot.phase !=
                LobbyPhase::Cancelling &&
                snapshot.phase != LobbyPhase::MatchComplete && snapshot.phase != LobbyPhase::AwaitRecovery && snapshot.
                phase != LobbyPhase::Game &&
                !(official && snapshot.recovering && (snapshot.phase == LobbyPhase::AwaitLobby ||
                                                      (snapshot.phase == LobbyPhase::AwaitHelo && tag == "GO"))))
                Fail("Game message before lobby confirmation");
            if (!snapshot.native_service && resume_connection && snapshot.recovering && tag != "GO" && tag != "UN" &&
                tag != "TAIKYOKU" && tag != "SAIKAI" && tag != "REINIT" && !(official && (tag == "INIT" || tag == "BYE")))
                Fail("Expected REINIT before resumed game events");
            if (tag == "REINIT" && (!game_.state.started || (resume_connection && !recovery_go_ && !official))) Fail(
                "REINIT before recovery GO");
            const auto before = game_.state;
            auto game_message = message;
            if (snapshot.native_service) game_message.attributes.erase("window");
            if (official && tag == "GO") {
                if (snapshot.origin_lobby) snapshot.origin_lobby->Cancel();
                if (snapshot.lobby.empty() && message.attributes.count("lobby")) {
                    if (const auto room = LobbyNumber(NetworkWide(Attribute(message, "lobby"))))
                        snapshot.lobby = NetworkUtf8(*room);
                }
                snapshot.match_type = Integer(Attribute(message, "type"));
                snapshot.exit_allowed = !message.attributes.count("gpid");
                rejoin_received_ = false;
                official_start_ready_ = false;
            }
            if (official && tag == "SAIKAI") {
                resume_connection = snapshot.recovering = recovery_go_ = true;
            }
            if (snapshot.native_service && tag == "GO") {
                if (snapshot.origin_lobby) {
                    const auto type = Integer(Attribute(message, "reservation"));
                    if (Attribute(message, "lobby") != snapshot.lobby ||
                        (type & ~0xA0u) != Integer(Attribute(message, "type")) ||
                        Attribute(message, "open_hands") == "1")
                        Fail("GO rule differs from the lobby reservation");
                    snapshot.origin_lobby->Cancel();
                    snapshot.origin_pending.clear();
                    snapshot.origin_error.clear();
                    snapshot.match_type = type;
                }
                snapshot.exit_table = Attribute(message, "table");
                snapshot.exit_allowed = Attribute(message, "exit") == "1";
                snapshot.ranked = Attribute(message, "ranked") == "1";
                snapshot.exit_pending = false;
                for (const char *key: {"table", "exit", "ranked"}) game_message.attributes.erase(key);
            }
            game_.Receive(game_message);
            actions_.Receive(game_message, before, game_.state);
            snapshot.game = game_.state;
            snapshot.actions = actions_.snapshot;
            if (game_.state.started)
                snapshot.phase = snapshot.recovering ? LobbyPhase::AwaitRecovery : LobbyPhase::Game;
            if (tag == "REINIT" || (official && tag == "INIT" && snapshot.recovering)) {
                snapshot.recovering = false;
                snapshot.phase = LobbyPhase::Game;
                resume_connection = false;
            }
            if (match_games &&game_.state.complete && (official || tag == "PROF" || phase == LobbyPhase::MatchComplete))
            {
                snapshot.phase = LobbyPhase::MatchComplete;
                snapshot.last_settlement = game_.state.settlement;
                snapshot.last_result_red = (game_.state.type & 2) == 0;
                snapshot.last_result_dealer = game_.state.dealer;
                snapshot.last_result_honba = game_.state.honba;
                snapshot.last_result_sticks = game_.state.sticks;
                for (int i = 0; i < 4; ++i) snapshot.last_result_names[i] = game_.state.players[i].name;
                if (phase != LobbyPhase::MatchComplete) {
                    ++snapshot.lobby_revision;
                    snapshot.match_result = "Match complete | " + game_.state.result;
                    for (int i = 0; i < 4; ++i) snapshot.match_result += " | Seat " + std::to_string(i) + ": " +
                                                std::to_string(game_.state.players[i].score);
                    if (!game_.state.final_scores.empty())
                        snapshot.match_result += " | Final scores / points: " + game_.state.final_scores;
                }
            }
            if (official) {
                if (tag == "GO") return {"<GOK />"};
                if (tag == "TAIKYOKU" && !official_start_ready_ && !snapshot.recovering) {
                    official_start_ready_ = true;
                    return {"<NEXTREADY />"};
                }
                return {};
            }
            if (snapshot.native_service && snapshot.recovering) return {};
            if (tag == "GO" && resume_connection) {
                recovery_go_ = true;
                if (interactive_games) return {"<GOK />", "<REINIT />"};
            }
            if (interactive_games &&tag == "GO") return {"<GOK />"};
            return {};
        }
        if (tag == "GO" || tag == "TAIKYOKU" || tag == "INIT" || tag == "REINIT")
            Fail("Online game/rejoin is not supported in this stage");
        if (tag == "HELO") {
            if (snapshot.phase != LobbyPhase::AwaitHelo && !(official && snapshot.phase == LobbyPhase::AwaitReturn))
                Fail("Unexpected repeated HELO");
            if (!Attribute(message, "closing").empty()) Fail("Server is closing");
            snapshot.server = message.attributes;
            snapshot.server.erase("auth");
            snapshot.identity = DecodeNetworkText(Attribute(message, "uname"));
            if (snapshot.identity.size() > 1024) Fail("Server identity exceeds limit");
            snapshot.phase = LobbyPhase::AwaitLobby;
            if (official) {
                // Receiving HELO means the server opened the lobby instead of resuming a saved gpid
                // (live gpid replies directly with GO; confirmed in tenhou.exe.c:63830-63957).
                const auto notice = Attribute(message, "nintei");
                snapshot.certification = LocalizeCertification(NetworkWide(DecodeNetworkText(notice)));
                snapshot.certification_receipt = notice.empty() ? 0 : snapshot.received;
                official_game_id.clear();
                FinishOfficialRecovery();
                official_profile_dirty_ = true;
                std::vector<std::string> replies;
                if (message.attributes.count("auth"))
                    replies.push_back("<AUTH val=\"" + AuthenticationResponse(Attribute(message, "auth")) + "\"/>");
                replies.push_back(RequestOfficialProfile(static_cast<int>(preferred_monthly_profile)));
                return replies;
            }
            if (Attribute(message, "uname") == "TENHOU-PLATFORM" && Attribute(message, "version") == "1") {
                if (origin_lobby && ("," + Attribute(message, "caps") + ",").find(",origin-lobby,") == std::string::npos)
                    Fail("自建服务器未开放重建大厅协议");
                snapshot.native_service = true;
                std::string credentials;
                if (!native_password.empty() || native_register)
                    credentials = " password=\"" + Escape(native_password) + "\" nickname=\"" + Escape(native_nickname)
                                  + "\" register=\"" + (native_register ? "1" : "0") + "\"";
                native_password.clear();
                native_nickname.clear();
                native_register = false;
                return {
                    "<CAPS version=\"1\" token=\"" + Escape(native_token) + "\" sx=\"" + native_gender +
                    "\" presentation=\"" + (presentation ? "1" : "0") + "\"" +
                    (origin_lobby ? " lobby=\"origin\"" : "") +
                    (origin_lobby && resume_connection ? " resume=\"1\"" : "") + credentials + " />"
                };
            }
            if (require_native) Fail("The configured endpoint is not a Tenhou platform service");
            if (message.attributes.count("auth")) return {
                "<AUTH val=\"" + AuthenticationResponse(Attribute(message, "auth")) + "\"/>"
            };
            return {};
        }
        if (tag == "LN" || tag == "CS" || tag == "CHAT") {
            if (snapshot.phase == LobbyPhase::AwaitHelo) Fail("Lobby message before HELO");
            if (tag == "LN") {
                snapshot.players = DecodeLobbyNumbers(Attribute(message, "n"));
                snapshot.waiting = DecodeLobbyNumbers(Attribute(message, "j"));
                if (message.attributes.count("g")) snapshot.games = DecodeLobbyNumbers(Attribute(message, "g"));
                if (snapshot.phase == LobbyPhase::AwaitLobby) snapshot.phase = LobbyPhase::Lobby;
            } else if (tag == "CS") {
                if (message.attributes.empty()) Fail("Empty CS confirmation");
                // Keep only the current CS record so extension keys cannot accumulate forever.
                for (auto it = snapshot.server.begin(); it != snapshot.server.end();) {
                    if (it->first.compare(0, 3, "CS.") == 0) it = snapshot.server.erase(it);
                    else ++it;
                }
                for (const auto &item: message.attributes) if (item.first != "auth")
                    snapshot.server["CS." + item.first] = item.second;
                if (message.attributes.count("lobby")) {
                    snapshot.lobby = Attribute(message, "lobby");
                    if (snapshot.lobby.size() > 64) Fail("Lobby identifier exceeds limit");
                }
                if (snapshot.phase == LobbyPhase::AwaitLobby) snapshot.phase = LobbyPhase::Lobby;
            } else {
                if (message.attributes.count("lobby")) {
                    snapshot.lobby = Attribute(message, "lobby");
                    if (snapshot.lobby.size() > 64) Fail("Lobby identifier exceeds limit");
                }
                const auto text = DecodeNetworkText(Attribute(message, "text"));
                const auto name = DecodeNetworkText(Attribute(message, "uname"));
                if (text.size() > 4096 || name.size() > 1024) Fail("Received chat display exceeds limit");
                if (!text.empty()) {
                    if (snapshot.messages.size() == 32) snapshot.messages.erase(snapshot.messages.begin());
                    snapshot.messages.push_back(name.empty() ? text : name + ": " + text);
                }
                // CHAT is display data; it alone does not confirm authentication.
            }
        }
        return {}; // Unknown extensions never advance the session.
    }

    namespace {
        int ProfileInteger(std::string_view text) {
            int value = 0;
            const auto end = text.data() + text.size();
            const auto parsed = std::from_chars(text.data(), end, value);
            return parsed.ec == std::errc{} && parsed.ptr == end ? value : 0;
        }

        double ProfileDecimal(std::string_view text) {
            double value = 0;
            const auto end = text.data() + text.size();
            const auto parsed = std::from_chars(text.data(), end, value);
            if (parsed.ec != std::errc{} || parsed.ptr != end || !(value >= std::numeric_limits<int>::min() &&
                value <= std::numeric_limits<int>::max())) return 0;
            return value;
        }

        std::int64_t ProfileScoreTenths(std::string_view text) {
            return static_cast<std::int64_t>(10 * ProfileDecimal(text));
        }

        std::vector<std::string_view> ProfileFields(std::string_view text, std::size_t max_fields) {
            std::vector<std::string_view> fields;
            if (text.empty()) return fields;
            while (fields.size() < max_fields) {
                const auto comma = text.find(',');
                fields.push_back(text.substr(0, comma));
                if (comma == std::string_view::npos) return fields;
                text.remove_prefix(comma + 1);
            }
            return {};
        }

        std::int64_t FillPlacements(LobbyStatistics &stats, const std::array<int, 4> &places, double score_hundredths) {
            stats.place_counts = places;
            const auto games = std::accumulate(places.begin(), places.end(), std::int64_t{0});
            if (!games) return 0;
            constexpr auto max_int = std::numeric_limits<int>::max();
            stats.games = static_cast<int>(std::min<std::int64_t>(games, max_int));
            std::int64_t weighted_places = 0;
            for (std::size_t i = 0; i < places.size(); ++i) {
                weighted_places += static_cast<int>(i + 1) * static_cast<std::int64_t>(places[i]);
                stats.place_per_mille[i] = static_cast<int>(1000LL * places[i] / games);
            }
            stats.average_place_hundredths = static_cast<int>(100 * weighted_places / games);
            const auto score = score_hundredths / static_cast<double>(games);
            stats.average_score_hundredths = static_cast<int>(std::clamp(score, -double(max_int), double(max_int)));
            return games;
        }

        void FillLifetimeStatistics(LobbyStatistics &stats, const std::vector<std::string_view> &fields,
                                    std::size_t score_index) {
            const auto field = [&](std::size_t offset) {
                const auto index = score_index + offset;
                return index < fields.size() ? fields[index] : std::string_view{""};
            };
            const auto count = [&](std::size_t offset) { return std::max(0, ProfileInteger(field(offset))); };
            // Origin converts the wire score in points to integer tenths before averaging.
            const auto score_tenths = ProfileScoreTenths(field(0));
            stats.total_score = static_cast<double>(score_tenths) * 0.1;
            const auto games = FillPlacements(stats, {count(1), count(2), count(3), count(4)},
                                             10.0 * score_tenths);
            stats.bust_count = count(5);
            if (games)
                stats.bust_per_mille = static_cast<int>(1000LL * std::min<std::int64_t>(stats.bust_count, games) / games);
            stats.hands = count(6);
            stats.win_count = count(7);
            stats.deal_in_count = count(8);
            stats.call_count = count(9);
            stats.reach_count = count(10);
            if (stats.hands) {
                const auto rate = [&](int value) {
                    return static_cast<int>(1000LL * std::min(value, stats.hands) / stats.hands);
                };
                stats.win_per_mille = rate(stats.win_count);
                stats.deal_in_per_mille = rate(stats.deal_in_count);
                stats.call_per_mille = rate(stats.call_count);
                stats.reach_per_mille = rate(stats.reach_count);
            }
        }

        LobbyRank ReadOfficialLifetime(const std::string &text, LobbyStatistics &stats) {
            constexpr std::array targets{20, 20, 20, 20, 40, 60, 80, 100, 100, 100, 400, 800,
                                         1200, 1600, 2000, 2400, 2800, 3200, 3600, 4000};
            const auto fields = ProfileFields(text, 14);
            if (fields.size() < 3) return {};
            const int grade = std::max(0, ProfileInteger(fields[0]));
            stats.points = ProfileInteger(fields[1]);
            stats.rating = static_cast<int>(ProfileDecimal(fields[2]));
            if (static_cast<std::size_t>(grade) < targets.size()) stats.next_points = targets[grade];
            FillLifetimeStatistics(stats, fields, 3);
            return {grade, stats.rating};
        }

        std::optional<LobbyStatistics> ReadOfficialMonthly(const std::string &text) {
            const auto fields = ProfileFields(text, 21);
            if (fields.size() != 15 && fields.size() != 21) return std::nullopt;
            std::array<int, 4> places{};
            for (std::size_t i = 0; i < places.size(); ++i)
                places[i] = std::max(0, ProfileInteger(fields[i]));
            LobbyStatistics stats{};
            stats.total_score = ProfileDecimal(fields[4]);
            FillPlacements(stats, places, 100 * stats.total_score);
            stats.has_monthly_raw = true;
            stats.monthly_ranks.resize(fields.size() == 21 ? 13 : 9, 0);
            for (std::size_t i = 0; i < 9; ++i)
                stats.monthly_ranks[i] = std::max(0, ProfileInteger(fields[6 + i]));
            if (fields.size() == 21) {
                stats.has_amber_monthly = true;
                stats.total_income = ProfileDecimal(fields[15]);
                stats.total_chips = ProfileInteger(fields[16]);
                for (std::size_t i = 0; i < 4; ++i)
                    stats.monthly_ranks[9 + i] = std::max(0, ProfileInteger(fields[17 + i]));
            }
            return stats;
        }

        std::vector<int> DecimalList(const std::string &text, std::size_t limit) {
            std::vector<int> values;
            if (text.empty()) return values;
            std::istringstream fields(text);
            for (std::string field; std::getline(fields, field, ',');) {
                const auto value = Integer(field);
                if (value > 100000000 || values.size() == limit) Fail("Invalid self-hosted lobby list");
                values.push_back(static_cast<int>(value));
            }
            if (text.back() == ',') Fail("Incomplete self-hosted lobby list");
            return values;
        }

        OfflineLobbyProfile ServiceProfile(const NetworkMessage &message) {
            OfflineLobbyProfile profile{};
            profile.name = NetworkWide(Attribute(message, "nickname"));
            profile.membership_expiry = NetworkWide(Attribute(message, "expire"));
            profile.ranked_allowed = !Attribute(message, "account").empty();
            const auto number = [&](const std::string &key) {
                const auto value = Attribute(message, key);
                return value.empty() ? 0 : static_cast<int>(Integer(value.substr(0, value.find('.'))));
            };
            profile.membership_days = number("expiredays");
            profile.light_options = number("opt");
            for (int i = 0; i < 2; ++i) {
                const std::string n = i ? "3" : "4";
                auto &rank = i ? profile.three : profile.four;
                auto &stats = profile.lifetime[i];
                rank = {number("dan" + n), number("rating" + n)};
                stats.games = number("games" + n);
                stats.points = number("points" + n);
                stats.next_points = number("target" + n);
                const auto places = DecimalList(Attribute(message, "places" + n), 4);
                if (!stats.games || places.size() != 4) continue;
                std::int64_t total_places = 0;
                for (int p = 0; p < 4; ++p) {
                    total_places += (p + 1) * static_cast<std::int64_t>(places[p]);
                    stats.place_per_mille[p] = static_cast<int>(1000LL * places[p] / stats.games);
                }
                stats.average_place_hundredths = static_cast<int>(100 * total_places / stats.games);
            }
            return profile;
        }
    }

    LobbyStatistics ReadPrivateLobbyStatistics(std::string_view text) {
        LobbyStatistics stats{};
        FillLifetimeStatistics(stats, ProfileFields(text, 11), 0);
        return stats;
    }

    std::string LobbySession::RequestOfficialProfile(int type) {
        official_profile_type_ = type;
        if (snapshot.origin_lobby) snapshot.origin_lobby->profile.monthly.clear();
        return "<PXR V=\"" + std::to_string(type) + "\" />";
    }

    std::string LobbySession::OfficialProfilePoll() const {
        return "<PXR v=\"" + std::to_string(official_profile_type_) + "\" />";
    }

    void LobbySession::ReturnToOriginLobby() {
        const auto revision = snapshot.actions.revision + 1;
        game_ = {};
        actions_ = {};
        actions_.official = official;
        actions_.snapshot.enabled = interactive_games;
        actions_.snapshot.revision = revision;
        snapshot.game = {};
        snapshot.actions = actions_.snapshot;
        snapshot.exit_table.clear();
        snapshot.exit_allowed = snapshot.table_paused = snapshot.recovering = false;
        rejoin_received_ = recovery_go_ = official_start_ready_ = false;
        snapshot.native_window = 0;
        snapshot.target_lobby.clear();
        snapshot.phase = LobbyPhase::Lobby;
        ++snapshot.lobby_revision;
    }

    void LobbySession::FinishOfficialRecovery() {
        resume_connection = snapshot.recovering = snapshot.silent_recovery = false;
    }

    bool LobbySession::ApplyOfficialLobby(const NetworkMessage &message) {
        if (snapshot.phase == LobbyPhase::AwaitHelo) return false;
        // The official service uses RANKING for both the personal v2 list and contest rankings.
        if ((message.tag == "RANKING" || message.tag == "RANK") && message.attributes.count("v2")) {
            if (snapshot.origin_lobby && official_profile_type_ >= 0) {
                const auto stats = ReadOfficialMonthly(Attribute(message, "v2"));
                if (stats) snapshot.origin_lobby->profile.monthly[official_profile_type_] = *stats;
            }
            return true;
        }
        if (message.tag == "LN") {
            if (!snapshot.origin_lobby || official_profile_dirty_) {
                OfflineLobbyProfile profile{};
                profile.name = NetworkWide(snapshot.identity);
                profile.ranked_allowed = snapshot.identity != "NoName";
                const auto field = [&](const std::string &key) {
                    const auto it = snapshot.server.find(key);
                    return it == snapshot.server.end() ? std::string{} : it->second;
                };
                profile.membership_expiry = NetworkWide(field("expire"));
                profile.membership_days = field("expiredays").empty() ? !profile.membership_expiry.empty() :
                                          ProfileInteger(field("expiredays"));
                profile.light_options = std::max(0, ProfileInteger(field("opt")));
                profile.four = ReadOfficialLifetime(field("PF4"), profile.lifetime[0]);
                profile.three = ReadOfficialLifetime(field("PF3"), profile.lifetime[1]);
                constexpr const char *rating_keys[]{"PF01C", "PF02C", "PF03C", "PF11C",
                                                    "PF12C", "PF13C", "PF23C", "PF33C"};
                for (std::size_t i = 0; i < kAmberLifetimeProfiles.size(); ++i) {
                    const auto type = kAmberLifetimeProfiles[i];
                    const auto record = field("PF" + std::to_string(type));
                    const auto rating = field(rating_keys[i]);
                    if (record.empty() && rating.empty()) continue;
                    LobbyStatistics stats{};
                    const auto results = ProfileFields(record, 12);
                    const auto rating_fields = ProfileFields(rating, 3);
                    if (rating_fields.size() >= 2)
                        stats.rating = static_cast<int>(ProfileDecimal(rating_fields[1]));
                    FillLifetimeStatistics(stats, results, 1);
                    if (results.size() >= 2 && stats.games) {
                        constexpr int chip_values[]{0, 2, 5, 0, 2, 5, 5, 5};
                        const auto raw_chips = ProfileInteger(results[0]);
                        stats.total_chips = raw_chips;
                        const auto score_tenths = ProfileScoreTenths(results[1]);
                        const auto score = score_tenths / 10;
                        const auto chips = static_cast<std::int64_t>(raw_chips) * chip_values[i];
                        constexpr auto max_int = static_cast<std::int64_t>(std::numeric_limits<int>::max());
                        stats.average_g_hundredths = static_cast<int>(
                            std::clamp(10000LL * (score + chips) / stats.games, -max_int, max_int));
                        if (chip_values[i] > 0) {
                            stats.average_income = (static_cast<double>(score_tenths) * 0.1 +
                                                    static_cast<double>(chips)) / static_cast<double>(stats.games);
                            stats.average_chips = static_cast<double>(raw_chips) / static_cast<double>(stats.games);
                        }
                    }
                    profile.other_lifetime.emplace(type, stats);
                }
                if (!snapshot.origin_lobby) snapshot.origin_lobby.emplace(std::move(profile));
                else {
                    snapshot.origin_lobby->profile = std::move(profile);
                    snapshot.origin_lobby->Enter(kRankedLobby);
                }
                snapshot.lobby = "L0000";
                official_profile_dirty_ = false;
            }
            auto &lobby = *snapshot.origin_lobby;
            auto counts = lobby.counts();
            const auto decode = [&](const char *key, auto &array) {
                const auto value = Attribute(message, key);
                const auto values = value.empty() ? std::vector<int>{} : DecodeLobbyNumbers(value);
                array.fill(0);
                for (std::size_t i = 0; i < std::min(values.size(), array.size()); ++i)
                    array[i] = static_cast<unsigned>(std::max(values[i], 0));
            };
            decode("n", counts.totals);
            decode("j", counts.waiting);
            decode("g", counts.playing);
            lobby.SetCounts(lobby.room(), counts);
            if (snapshot.phase == LobbyPhase::AwaitLobby) snapshot.phase = LobbyPhase::Lobby;
            return true;
        }
        if (message.tag == "CHAT" && message.attributes.count("lobby") && snapshot.origin_lobby &&
            !game_.state.started) {
            const auto room = LobbyNumber(NetworkWide(Attribute(message, "lobby")));
            if (room) {
                snapshot.origin_lobby->Enter(*room);
                snapshot.lobby = NetworkUtf8(*room);
                snapshot.target_lobby.clear();
                snapshot.origin_pending.clear();
                snapshot.phase = LobbyPhase::Lobby;
                ++snapshot.lobby_revision;
            }
            return true;
        }
        return false;
    }

    bool LobbySession::ApplyOriginLobby(const NetworkMessage &message) {
        const auto &tag = message.tag;
        if (tag == "PROFILE") {
            snapshot.profile = message.attributes;
            if (snapshot.origin_lobby) snapshot.origin_lobby->profile = ServiceProfile(message);
            return true;
        }
        if (tag != "CHAT" && tag != "CS" && tag != "LN" && tag != "RESERVATIONS") return false;
        if (!message.attributes.count("epoch")) return true;
        const auto epoch = Integer(Attribute(message, "epoch"));
        const auto id = Attribute(message, "lobby");
        const auto rid = Attribute(message, "rid");
        const auto phase = snapshot.phase;
        const bool receipt = !snapshot.origin_pending.empty() && rid == std::to_string(origin_request_);
        if (tag == "CHAT") {
            const auto room = LobbyNumber(NetworkWide(id));
            if (!room || NetworkUtf8(*room) != id) Fail("Invalid self-hosted lobby identifier");
            if (phase == LobbyPhase::AwaitLobby && origin_epoch_ == 0 && epoch == 1 && rid.empty()) {
                snapshot.origin_lobby.emplace(ServiceProfile({"PROFILE", snapshot.profile}));
                snapshot.recovering = Attribute(message, "seated") == "1";
                if (!snapshot.recovering) resume_connection = false;
            } else if (phase == LobbyPhase::ChangingLobby && receipt && epoch == origin_epoch_ + 1 &&
                       id == snapshot.target_lobby) {
                snapshot.target_lobby.clear();
            } else if (phase == LobbyPhase::AwaitReturn && epoch == origin_epoch_ && id == snapshot.target_lobby && rid.empty()) {
                ReturnToOriginLobby();
            } else if (phase == LobbyPhase::Lobby && epoch == origin_epoch_ && id == snapshot.lobby && rid.empty()) {
                return true;
            } else return true;
            auto profile = snapshot.origin_lobby->profile;
            snapshot.origin_lobby.emplace(std::move(profile));
            snapshot.origin_lobby->Enter(*room);
            snapshot.lobby = id;
            origin_epoch_ = epoch;
            snapshot.origin_pending.clear();
            snapshot.origin_error.clear();
            snapshot.origin_clock = snapshot.origin_clock_at = 0;
            snapshot.phase = LobbyPhase::Lobby;
            ++snapshot.lobby_revision;
            return true;
        }
        if (epoch != origin_epoch_ || id != snapshot.lobby || !snapshot.origin_lobby ||
            (phase != LobbyPhase::Lobby && phase != LobbyPhase::Matching && phase != LobbyPhase::Cancelling &&
             phase != LobbyPhase::ChangingLobby)) return true;
        auto &lobby = *snapshot.origin_lobby;
        if (tag == "CS") {
            if (!lobby.contest()) return true;
            const auto number = [&](const char *key) { return Integer(Attribute(message, key)); };
            const auto players = number("players"), type = number("rule"), start = number("start"), end = number("end");
            if ((players != 3 && players != 4) || end <= start) Fail("Invalid self-hosted contest");
            lobby.SetContest(lobby.room(), {NetworkWide(Attribute(message, "title")), players, type, number("joinfee"),
                number("premiumonly") == 1, number("registered") == 1, number("blocked") == 1, start, end});
            snapshot.origin_clock = number("now");
            snapshot.origin_clock_at = GetTickCount64();
        } else if (tag == "LN") {
            const auto n = DecimalList(Attribute(message, "n"), 4);
            const auto j = DecimalList(Attribute(message, "j"), 113);
            const auto g = DecimalList(Attribute(message, "g"), 113);
            OfflineLobbyCounts counts;
            if (n.size() != counts.totals.size() || j.size() != counts.waiting.size() ||
                g.size() != counts.playing.size()) Fail("Invalid self-hosted lobby counts");
            std::copy(n.begin(), n.end(), counts.totals.begin());
            std::copy(j.begin(), j.end(), counts.waiting.begin());
            std::copy(g.begin(), g.end(), counts.playing.begin());
            lobby.SetCounts(lobby.room(), counts);
        } else if (rid.empty() || receipt) {
            const auto types = DecimalList(Attribute(message, "types"), 48);
            lobby.SetReservations(types);
            snapshot.origin_error = Attribute(message, "code");
            if (receipt) {
                lobby.DeclineMove();
                snapshot.origin_pending.clear();
                snapshot.target_lobby.clear();
            }
            if (snapshot.origin_pending.empty())
                snapshot.phase = lobby.reservations() ? LobbyPhase::Matching : LobbyPhase::Lobby;
        }
        return true;
    }

    std::vector<std::string> LobbySession::SubmitOriginLobby(const std::string &command, const std::wstring &value) {
        if (official && command == "monthly-profile") {
            const auto type = Integer(NetworkUtf8(value));
            if (!ValidMonthlyProfile(type)) return {};
            preferred_monthly_profile = type;
            const auto phase = snapshot.phase;
            if (!snapshot.origin_lobby || snapshot.recovering ||
                (phase != LobbyPhase::Lobby && phase != LobbyPhase::Matching) ||
                static_cast<int>(type) == official_profile_type_) return {};
            return {RequestOfficialProfile(static_cast<int>(type))};
        }
        if (!snapshot.origin_lobby || !snapshot.origin_pending.empty() ||
            (snapshot.phase != LobbyPhase::Lobby && snapshot.phase != LobbyPhase::Matching) || snapshot.recovering)
            return {};
        auto &lobby = *snapshot.origin_lobby;
        if (official) {
            if (command == "join") {
                const auto type = Integer(NetworkUtf8(value));
                if ((type & 0x810) == 0x810 || lobby.contest() || lobby.Reserved(type)) return {};
                const bool verified_practice = lobby.ranked() && (type == 1024 || type == 1552);
                bool reserved = verified_practice;
                for (unsigned tier = 0; tier < 4 && !reserved; ++tier)
                    for (unsigned slot = 0; slot < kRankedSlots && !reserved; ++slot)
                        if (lobby.Candidate(static_cast<RankedTier>(tier), slot).type == type)
                            reserved = lobby.Reserve(static_cast<RankedTier>(tier), slot);
                if (!reserved) return {};
                snapshot.match_type = type;
                snapshot.phase = LobbyPhase::Matching;
                return {"<JOIN t=\"" + std::to_string(Integer(snapshot.lobby.substr(1))) + "," +
                        std::to_string(type) + "\" />"};
            }
            if (command == "cancel" && lobby.reservations()) {
                lobby.Cancel();
                snapshot.phase = LobbyPhase::Lobby;
                return {"<JOIN />"};
            }
            if (command != "change" || !lobby.RequestMove(value)) return {};
            snapshot.target_lobby = NetworkUtf8(*lobby.pending());
            const auto &target = snapshot.target_lobby;
            snapshot.phase = LobbyPhase::ChangingLobby;
            return {target.front() == 'C' ? "<CS lobby=\"" + target + "\" />" :
                "<CHAT text=\"%2Flobby%20" + std::to_string(Integer(target.substr(1))) + "\" />"};
        }
        std::string wire;
        if (command == "change") {
            if (!lobby.RequestMove(value)) return {};
            snapshot.target_lobby = NetworkUtf8(*lobby.pending());
            const auto &target = snapshot.target_lobby;
            wire = target.front() == 'C' ? "<CS target=\"" + target + "\"" :
                "<CHAT text=\"%2Flobby%20" + std::to_string(Integer(target.substr(1))) + "\"";
            snapshot.phase = LobbyPhase::ChangingLobby;
        } else if (command == "join") {
            const auto type = Integer(NetworkUtf8(value));
            wire = lobby.contest() ? "<JOIN type=\"" + std::to_string(type) + "\"" :
                "<JOIN t=\"" + std::to_string(Integer(snapshot.lobby.substr(1))) + "," + std::to_string(type) + "\"";
            snapshot.phase = LobbyPhase::Matching;
        } else if (command == "cancel" && lobby.reservations()) {
            wire = "<JOIN";
            snapshot.phase = LobbyPhase::Cancelling;
        } else if (command == "register" && lobby.contest()) {
            wire = "<CSREG";
        } else return {};
        snapshot.origin_pending = command;
        snapshot.origin_error.clear();
        return {wire + " lobby=\"" + snapshot.lobby + "\" epoch=\"" + std::to_string(origin_epoch_) +
            "\" rid=\"" + std::to_string(++origin_request_) + "\" />"};
    }

    std::vector<std::string> LobbySession::SubmitLobby(std::uint64_t revision, const std::string &command,
                                                       unsigned value) {
        if (!match_games || !interactive_games || observe_games) Fail("Local matchmaking is disabled");
        if (revision != snapshot.lobby_revision) Fail("Stale lobby revision");
        if (official && command == "return" && snapshot.phase == LobbyPhase::MatchComplete) {
            ReturnToOriginLobby();
            official_game_id.clear();
            official_profile_type_ = -1;
            if (!official_identity_.empty()) {
                snapshot.phase = LobbyPhase::AwaitReturn;
                official_profile_dirty_ = true;
                return {"<HELO name=\"" + Escape(official_identity_) + "\" sx=\"" + official_gender_ + "\" tid=\"w0\" />"};
            }
            return {};
        }
        const auto roomName = [](unsigned room) {
            std::ostringstream out;
            out << 'L' << std::setw(4) << std::setfill('0') << room;
            return out.str();
        };
        const auto change = [](unsigned room) { return "<CHAT text=\"%2Flobby%20" + std::to_string(room) + "\" />"; };
        std::vector<std::string> replies;
        if (command == "change") {
            if (snapshot.phase != LobbyPhase::Lobby) Fail("Lobby change requires ready lobby");
            if (value != 0 && (value < 1000 || value > 9999)) Fail("Supported room is 0 or 1000..9999");
            replies = {change(value)};
            snapshot.room_settings.clear();
            room_ = value;
            snapshot.target_lobby = roomName(value);
            snapshot.phase = LobbyPhase::ChangingLobby;
        } else if (command == "match") {
            if (snapshot.phase != LobbyPhase::Lobby || snapshot.lobby != roomName(room_)) Fail(
                "Confirm a supported room before matching");
            auto queue = std::find_if(snapshot.queues.begin(), snapshot.queues.end(), [&](const ServerQueue &q) { return q.id == value; });
            if (!snapshot.catalog_ready || (!room_ && queue == snapshot.queues.end())) Fail("Select a server queue");
            if (!room_) {
                const auto rule = snapshot.rules.find(queue->rule);
                if (rule == snapshot.rules.end() || (rule->second.presentation != "riichi-v1" && rule->second.presentation != "mahjong-v1")) Fail("This rule needs a newer table renderer");
            } else {
                const auto rule = snapshot.rules.find(snapshot.room_settings.at("rule"));
                if (rule == snapshot.rules.end() || (rule->second.presentation != "riichi-v1" && rule->second.presentation != "mahjong-v1")) Fail("This rule needs a newer table renderer");
            }
            replies = {"<JOIN t=\"" + std::to_string(room_) + "," + std::to_string(value) + "\" />"};
            if (snapshot.native_service && room_) {
                if (!snapshot.room_settings.count("revision")) Fail("Confirm room settings before joining queue");
                replies = {
                    "<JOIN t=\"" + std::to_string(room_) + "," + std::to_string(value) + "\" revision=\"" + Escape(
                        snapshot.room_settings.at("revision")) + "\" />"
                };
            }
            snapshot.match_type = room_ ? Integer(snapshot.room_settings.at("type")) : queue->type;
            snapshot.match_result.clear();
            snapshot.phase = LobbyPhase::Matching;
            // JOIN selects family/length; local ROOM supplies the complete GO flags.
            if (!snapshot.native_service && !snapshot.room_settings.empty()) {
                const auto &room = snapshot.room_settings;
                snapshot.match_type |= (room.at("red_count") == "0" ? 2u : 0u) |
                        (room.at("open_tanyao") == "0" ? 4u : 0u) | (room.at("fast") == "1" ? 64u : 0u);
            }
            snapshot.last_settlement = {};
            snapshot.last_result_names = {};
            snapshot.queue_confirmed = false;
            game_.state.settlement = {};
            snapshot.game.settlement = {};
        } else if (command == "cancel") {
            if (snapshot.phase != LobbyPhase::Matching) Fail("No match request to cancel");
            replies = {"<JOIN />", change(room_)};
            snapshot.target_lobby = roomName(room_);
            snapshot.phase = LobbyPhase::Cancelling;
        } else if (command == "return") {
            if (snapshot.phase != LobbyPhase::MatchComplete || !snapshot.game.complete) Fail("Match is not complete");
            if (snapshot.origin_lobby) {
                replies = {"<BYE />"};
                snapshot.target_lobby = snapshot.lobby;
            } else {
                if (snapshot.lobby != roomName(room_)) Fail("Return requires a confirmed supported lobby");
                replies = {"<BYE />", change(room_)};
                snapshot.target_lobby = roomName(room_);
            }
            snapshot.phase = LobbyPhase::AwaitReturn;
            snapshot.variant_active = false;
            snapshot.variant_state.clear();
        } else Fail("Unknown lobby command");
        ++snapshot.lobby_revision;
        snapshot.native_error.clear();
        return replies;
    }

    std::vector<std::string> LobbySession::NativeRequest(const std::string &command, unsigned value,
                                                         const std::map<std::string, std::string> &settings) {
        if (!snapshot.native_service || snapshot.phase != LobbyPhase::Lobby || !snapshot.native_pending.empty()) Fail(
            "Native room request is not available");
        std::string tag;
        if (command == "create") tag = "ROOMCREATE";
        else if (command == "join") tag = "ROOMJOIN";
        else if (command == "leave") tag = "ROOMLEAVE";
        else if (command == "close") tag = "ROOMCLOSE";
        else if (command == "settings") tag = "SETROOM";
        else Fail("Unknown native request");
        std::string wire = "<" + tag + " rid=\"" + std::to_string(++snapshot.native_request) + "\"";
        if (command == "join") {
            if (value < 1000 || value > 9999) Fail("Invalid room number");
            wire += " room=\"" + std::to_string(value) + "\"";
        }
        if (command == "settings") {
            if (!snapshot.room_settings.count("revision")) Fail("Room revision missing");
            wire += " revision=\"" + Escape(snapshot.room_settings.at("revision")) + "\"";
        }
        if (command == "create" || command == "settings")
            for (const auto &item: settings) {
                if (item.first == "room" || item.first == "revision" || item.first == "owner" || item.first == "rid" || item.first == "type")
                    continue;
                if (item.first.empty() || !std::all_of(item.first.begin(), item.first.end(),
                                                       [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; })) Fail(
                    "Invalid setting key");
                wire += " " + item.first + "=\"" + Escape(item.second) + "\"";
            }
        snapshot.native_error.clear();
        snapshot.native_pending = command;
        return {wire + " />"};
    }

    std::vector<std::string> LobbySession::SubmitAction(std::uint64_t revision, std::size_t index) {
        if (snapshot.table_paused || snapshot.exit_pending) return {};
        if (snapshot.phase != LobbyPhase::Game) Fail("Action outside game session");
        if (snapshot.variant_active) {
            if (!snapshot.actions.enabled || snapshot.recovering || revision != snapshot.actions.revision ||
                index >= snapshot.actions.choices.size() || !snapshot.actions.pending.empty())
                Fail("Stale or pending platform action");
            const auto choice = snapshot.actions.choices[index];
            snapshot.actions.pending = choice.kind;
            snapshot.actions.choices.clear();
            return choice.messages;
        }
        auto next = actions_;
        const auto replies = next.Submit(revision, index);
        actions_ = std::move(next);
        snapshot.actions = actions_.snapshot;
        return replies;
    }

    std::vector<std::string> LobbySession::ExitGame(int choice) {
        if (official) {
            if (choice >= 0 || !snapshot.exit_allowed || snapshot.recovering || snapshot.phase != LobbyPhase::Game ||
                official_identity_.empty())
                return {};
            ReturnToOriginLobby();
            official_game_id.clear();
            official_profile_type_ = -1;
            snapshot.phase = LobbyPhase::AwaitReturn;
            official_profile_dirty_ = true;
            return {
                "<BYE />",
                "<HELO name=\"" + Escape(official_identity_) + "\" sx=\"" + official_gender_ + "\" tid=\"w0\" />"
            };
        }
        if (!snapshot.native_service || !snapshot.exit_allowed || snapshot.exit_table.empty() || snapshot.recovering ||
            snapshot.phase != LobbyPhase::Game)
            return {};
        if (choice < 0) {
            if (snapshot.exit_pending || snapshot.exit_voting) return {};
            snapshot.exit_pending = true;
            return {"<EXIT table=\"" + Escape(snapshot.exit_table) + "\" />"};
        }
        if (choice > 1 || !snapshot.exit_voting || snapshot.exit_choice >= 0 || snapshot.exit_ballot_pending) return {};
        snapshot.exit_ballot_pending = true;
        return {
            "<EXITVOTE table=\"" + Escape(snapshot.exit_table) + "\" id=\"" + std::to_string(snapshot.exit_vote_id) +
            "\" yes=\"" + std::to_string(choice) + "\" />"
        };
    }

    std::vector<std::string> LobbySession::ConfigureRoom(std::uint64_t revision,
                                                         const std::map<std::string, std::string> &settings) {
        if (!match_games || !interactive_games || observe_games || snapshot.phase != LobbyPhase::Lobby || revision !=
            snapshot.lobby_revision)
            Fail("Room configuration requires the current local lobby");
        if (snapshot.room_settings.empty()) Fail("Server does not advertise configurable rooms");
        std::string command = "<SETROOM";
        for (const auto &item: settings) {
            if (item.first == "room") continue;
            if (item.first.empty() || !std::all_of(item.first.begin(), item.first.end(),
                                                   [](char c) { return (c >= 'a' && c <= 'z') || c == '_'; }) ||
                item.second.size() > 32 || !std::all_of(item.second.begin(), item.second.end(), [](char c) {
                    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+';
                }))
                Fail("Invalid room setting encoding");
            command += " " + item.first + "=\"" + item.second + "\"";
        }
        return {command + " />"};
    }

    std::string NetworkJson(const std::string &value) {
        std::ostringstream out;
        out << '"';
        for (unsigned char c: value) {
            if (c == '"' || c == '\\') out << '\\' << c;
            else if (c < 32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << unsigned(c);
            else out << c;
        }
        out << '"';
        return out.str();
    }
} // namespace tenhou
