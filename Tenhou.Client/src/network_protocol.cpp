#include "network_protocol.hpp"
#include <windows.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <set>

namespace tenhou {
namespace {
[[noreturn]] void Fail(const char* message) { throw std::runtime_error(message); }
bool Space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
bool Letter(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }
bool Digit(char c) { return c >= '0' && c <= '9'; }
bool Name(char c) { return Letter(c) || Digit(c) || c == '-' || c == '.' || c == ':'; }
unsigned Integer(const std::string& value, int base = 10) {
    unsigned result = 0;
    const auto p = std::from_chars(value.data(), value.data() + value.size(), result, base);
    if (value.empty() || p.ec != std::errc() || p.ptr != value.data() + value.size()) Fail("Invalid protocol number");
    return result;
}
void Validate(const std::string& value) {
    (void)NetworkWide(value);
    for (unsigned char c : value) if ((c < 32 && c != 9 && c != 10 && c != 13) || c == 127)
        Fail("Invalid control character in protocol text");
}
std::string Entities(const std::string& value) {
    std::string out;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '&') { out += value[i]; continue; }
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
            if (!code || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff) || code == 0xfffe || code == 0xffff)
                Fail("Invalid XML scalar");
            std::wstring wide;
            if (code <= 0xffff) wide += static_cast<wchar_t>(code);
            else { code -= 0x10000; wide += static_cast<wchar_t>(0xd800 + (code >> 10)); wide += static_cast<wchar_t>(0xdc00 + (code & 1023)); }
            out += NetworkUtf8(wide);
        } else Fail("Unsupported XML entity");
        i = end;
    }
    Validate(out);
    return out;
}
std::string Attribute(const NetworkMessage& message, const std::string& key) {
    const auto found = message.attributes.find(key);
    return found == message.attributes.end() ? "" : found->second;
}
std::string Escape(const std::string& value) {
    std::string out;
    for (char c : value) {
        if (c == '&') out += "&amp;";
        else if (c == '"') out += "&quot;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else out += c;
    }
    return out;
}
} // namespace

std::wstring NetworkWide(const std::string& value) {
    if (value.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (!size) Fail("Invalid UTF-8 protocol text");
    std::wstring out(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), out.data(), size);
    return out;
}
std::string NetworkUtf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (!size) Fail("Invalid Unicode input");
    std::string out(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), out.data(), size, nullptr, nullptr);
    return out;
}
std::string DecodeNetworkText(const std::string& value) {
    std::string out;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '%') out += value[i];
        else {
            if (i + 2 >= value.size()) Fail("Invalid percent encoding");
            out += static_cast<char>(Integer(value.substr(i + 1, 2), 16)); i += 2;
        }
    }
    Validate(out); return out;
}
NetworkMessage ParseNetworkMessage(const std::string& xml) {
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
    spaces(); expect('<'); message.tag = name();
    for (;;) {
        const bool spaced = p < xml.size() && Space(xml[p]); spaces();
        if (p < xml.size() && xml[p] == '/') { ++p; expect('>'); break; }
        // Some original server messages use <TAG ...> without a closing slash.
        if (p < xml.size() && xml[p] == '>') { ++p; break; }
        if (!spaced) Fail("Missing attribute whitespace");
        const auto key = name(); spaces(); expect('='); spaces();
        if (p == xml.size() || (xml[p] != '\'' && xml[p] != '"')) Fail("Unquoted protocol attribute");
        const char quote = xml[p++]; const auto start = p;
        while (p < xml.size() && xml[p] != quote) { if (xml[p] == '<') Fail("Invalid protocol attribute"); ++p; }
        if (p == xml.size()) Fail("Truncated protocol attribute");
        if (message.attributes.size() >= 64 || !message.attributes.emplace(key, Entities(xml.substr(start, p - start))).second)
            Fail("Duplicate or excessive protocol attributes");
        ++p;
    }
    spaces(); if (p != xml.size()) Fail("Trailing or nested protocol XML");
    return message;
}
std::vector<std::string> NetworkFramer::Feed(const char* bytes, std::size_t count) {
    std::vector<std::string> messages;
    for (std::size_t i = 0; i < count; ++i) {
        if (bytes[i] == '\0') {
            if (!pending_.empty()) { messages.push_back(std::move(pending_)); pending_.clear(); }
        } else {
            if (pending_.size() == kNetworkMessageLimit) Fail("Network message exceeds 64 KiB limit");
            pending_ += bytes[i];
        }
    }
    return messages;
}
std::string AuthenticationResponse(const std::string& challenge) {
    if (challenge.empty() || challenge.size() > 128 || !std::all_of(challenge.begin(), challenge.end(), [](char c) {
            return Letter(c) || Digit(c) || c == '-' || c == '.'; })) Fail("Invalid authentication token");
    if (challenge.size() != 17 || challenge[8] != '-') return challenge; // Original opaque-token fallback.
    if (!std::all_of(challenge.begin(), challenge.begin() + 8, Digit)) Fail("Invalid authentication challenge prefix");
    static constexpr std::array<std::uint32_t, 12> table{{4129170786u, 3225465664u, 643717713u, 3921069994u,
        3593408605u, 38016083u, 2763975236u, 3634488961u, 3889429448u, 568446438u, 3275163606u, 4107603335u}};
    const auto index = Integer("2" + challenge.substr(2, 6)) % (60u - static_cast<unsigned char>(challenge[7]));
    const auto value = Integer(challenge.substr(9), 16) ^ table[index];
    std::ostringstream out; out << challenge.substr(0, 9) << std::hex << std::setfill('0') << std::setw(8) << value;
    return out.str();
}
std::vector<int> DecodeLobbyNumbers(const std::string& value) {
    // Original 004c6630: base-52 letter values and decimal runs of comma separators.
    if (value.empty() || value.size() > 4096) Fail("Invalid lobby statistics");
    std::string numbers; unsigned number = 0, commas = 0; int sign = 1;
    auto append = [&](const std::string& part) { if (numbers.size() + part.size() > 16384) Fail("Lobby statistics exceed limit"); numbers += part; };
    for (char c : value) {
        if (c == '-') sign = -1;
        else if (Letter(c) && c != '_') {
            if (commas) { append(std::string(commas, ',')); commas = 0; }
            const unsigned digit = static_cast<unsigned>(c >= 'a' ? c - 'a' + 26 : c - 'A');
            if (number > (2147483647u - digit) / 52) Fail("Lobby statistic overflow");
            number = number * 52 + digit;
        } else if (Digit(c)) {
            if (number) append(std::to_string(sign * static_cast<int>(number)));
            number = 0; sign = 1;
            commas = commas * 10 + static_cast<unsigned>(c - '0');
            if (commas > 1024) Fail("Lobby statistic run exceeds limit");
        } else Fail("Invalid lobby statistic encoding");
    }
    append(std::to_string(sign * static_cast<int>(number)));
    std::vector<int> result; std::size_t start = 0;
    for (;;) {
        const auto end = numbers.find(',', start); const auto token = numbers.substr(start, end - start);
        int n = 0;
        if (!token.empty()) {
            const auto parsed = std::from_chars(token.data(), token.data() + token.size(), n);
            if (parsed.ec != std::errc() || parsed.ptr != token.data() + token.size()) Fail("Invalid decoded statistic");
        }
        result.push_back(n); if (result.size() > 1024) Fail("Too many lobby statistics");
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return result;
}
const char* LobbyPhaseName(LobbyPhase phase) {
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
std::vector<std::string> LobbySession::Connected(const std::string& identity, char gender, std::uint64_t revision) {
    if (match_games && (!interactive_games || observe_games)) Fail("Matchmaking requires local interactive mode without read-only observation");
    // The fixed original accepts NoName or a 19-character registered ID, not arbitrary guest names.
    const bool registered = identity.size() == 19 && identity.compare(0, 2, "ID") == 0 &&
        std::all_of(identity.begin(), identity.end(), [](char c) { return (Letter(c) && c != '_') || Digit(c) || c == '-'; });
    const bool local_account=require_native && identity.size()>=3 && identity.size()<=32 && std::all_of(identity.begin(),identity.end(),[](char c){return Letter(c) || Digit(c) || c=='-';});
    if ((identity != "NoName" && !registered && !local_account) || (gender != 'M' && gender != 'F'))
        Fail("请输入本机账号，或使用 NoName 访客登录");
    snapshot = {}; game_ = {}; actions_ = {}; actions_.snapshot.enabled = interactive_games; snapshot.actions = actions_.snapshot; snapshot.phase = LobbyPhase::AwaitHelo;
    rejoin_received_ = recovery_go_ = false;
    actions_.snapshot.revision = revision; snapshot.actions = actions_.snapshot;
    snapshot.recovering = resume_connection;
    snapshot.matchmaking = match_games; snapshot.lobby_revision = revision; room_ = 0;
    return {"<HELO name=\"" + Escape(identity) + "\" sx=\"" + gender + "\" tid=\"w0\" />"};
}
std::vector<std::string> LobbySession::Receive(const NetworkMessage& message) {
    auto next = *this; auto replies = next.Apply(message); *this = std::move(next); return replies;
}
std::vector<std::string> LobbySession::Apply(const NetworkMessage& message) {
    const auto& tag = message.tag;
    if (snapshot.phase == LobbyPhase::Disconnected || snapshot.phase == LobbyPhase::Error || snapshot.phase == LobbyPhase::Resolving || snapshot.phase == LobbyPhase::Connecting)
        Fail("Message outside connected session");
    ++snapshot.received; snapshot.last_tag = tag;
    if (tag == "ERR") {
        if(message.attributes.count("text")) Fail(Attribute(message,"text").c_str());
        const auto code = Attribute(message, "code");
        if (!code.empty() && code.size() <= 10 && std::all_of(code.begin(), code.end(), Digit))
            throw std::runtime_error("Server rejected session (ERR code=" + code + ")");
        Fail("Server rejected session (ERR)");
    }
    if (tag == "BYE" && !message.attributes.count("who")) Fail("Server ended session (BYE)");
    const auto phase = snapshot.phase;
    if (snapshot.native_service) {
        if(tag=="PROFILE") { snapshot.profile=message.attributes; return {}; }
        if(tag=="REPLAYBEGIN" || tag=="REPLAYCHUNK" || tag=="REPLAYEND" || tag=="REPLAYERROR") return {};
        if(message.attributes.count("window")) snapshot.native_window=Integer(Attribute(message,"window"));
        if(tag=="SESSION") {
            const auto token=Attribute(message,"token");
            if(token.size()!=64 || !std::all_of(token.begin(),token.end(),[](char c) { return Digit(c) || (c>='a' && c<='f'); })) Fail("Invalid native session token");
            snapshot.native_token=token; native_token=token;
            snapshot.native_request=Integer(Attribute(message,"rid")); return {};
        }
        if(tag=="ACCEPT" || tag=="REJECT") {
            if(snapshot.native_pending.empty() || Integer(Attribute(message,"rid"))!=snapshot.native_request) Fail("Unexpected native request receipt");
            snapshot.native_error=tag=="REJECT"?Attribute(message,"code"):"";
            if(tag=="REJECT") snapshot.native_pending.clear();
            if(tag=="ACCEPT") {
                room_=Integer(Attribute(message,"room"));
                if(room_!=0 && (room_<1000 || room_>9999)) Fail("Invalid allocated room");
                std::ostringstream name; name<<'L'<<std::setw(4)<<std::setfill('0')<<room_; snapshot.lobby=name.str(); snapshot.room_settings.clear();
                if(!room_) snapshot.native_pending.clear();
            }
            ++snapshot.lobby_revision; return {};
        }
        if(tag=="QUEUED") {
            if(phase!=LobbyPhase::Matching || Integer(Attribute(message,"room"))!=room_ || Integer(Attribute(message,"type"))!=snapshot.match_type) Fail("Queue receipt mismatch");
            snapshot.queue_confirmed=true; return {};
        }
        if(tag=="QUEUEREJECT") {
            if(phase!=LobbyPhase::Matching) Fail("Queue rejection outside matching");
            snapshot.phase=LobbyPhase::Lobby; snapshot.queue_confirmed=false; snapshot.native_error=Attribute(message,"code"); ++snapshot.lobby_revision; return {};
        }
        if(tag=="RESTORE") {
            game_={}; actions_={}; actions_.snapshot.enabled=interactive_games;
            snapshot.game={}; snapshot.actions=actions_.snapshot; snapshot.recovering=true; snapshot.phase=LobbyPhase::Lobby; return {};
        }
        if(tag=="RESTORED") {
            snapshot.recovering=false; resume_connection=false;
            snapshot.phase=snapshot.game.complete?LobbyPhase::MatchComplete:snapshot.game.started?LobbyPhase::Game:LobbyPhase::Lobby;
            return {};
        }
        if(tag=="EXITVOTE") {
            if(Attribute(message,"table")!=snapshot.exit_table) return {};
            snapshot.exit_voting=Attribute(message,"show")=="1"; snapshot.exit_pending=false;
            if(snapshot.exit_voting) {
                snapshot.exit_vote_id=Integer(Attribute(message,"id")); snapshot.exit_vote_ms=Integer(Attribute(message,"ms"));
                snapshot.exit_total=Integer(Attribute(message,"total")); snapshot.exit_yes=Integer(Attribute(message,"yes")); snapshot.exit_no=Integer(Attribute(message,"no"));
                const auto choice=Attribute(message,"choice"); snapshot.exit_choice=choice=="-1"?-1:static_cast<int>(Integer(choice));
                if(!snapshot.exit_vote_id || snapshot.exit_vote_ms>30000 || snapshot.exit_total<2 || snapshot.exit_total>4 ||
                    snapshot.exit_yes+snapshot.exit_no>snapshot.exit_total || snapshot.exit_choice>1) Fail("Invalid exit vote snapshot");
            }
            snapshot.exit_ballot_pending=false; return {};
        }
        if(tag=="TABLEEND") {
            if(Attribute(message,"table")!=snapshot.exit_table || Attribute(message,"reason")!="exit") Fail("Unexpected table end");
            const auto revision=snapshot.actions.revision+1;
            game_={}; actions_={}; actions_.snapshot.enabled=interactive_games; actions_.snapshot.revision=revision;
            snapshot.game={}; snapshot.actions=actions_.snapshot; snapshot.last_settlement={}; snapshot.last_result_names={};
            snapshot.exit_table.clear(); snapshot.exit_allowed=snapshot.exit_pending=snapshot.exit_voting=snapshot.exit_ballot_pending=false;
            snapshot.table_paused=snapshot.recovering=false; snapshot.native_window=0;
            snapshot.phase=LobbyPhase::Lobby; snapshot.target_lobby.clear(); snapshot.match_result="exit"; ++snapshot.lobby_revision;
            return {};
        }
        if(tag=="PAUSED" || tag=="TABLEERROR") { snapshot.table_paused=tag=="TABLEERROR" || Attribute(message,"show")=="1"; return {}; }
        if(tag=="READYREQUEST") { return {Attribute(message,"kind")=="gok"?"<GOK />":"<STARTREADY />"}; }
        if(tag=="DRAINING") { snapshot.native_error="DRAINING"; return {}; }
    }
    if (tag == "ROOM" && match_games && phase == LobbyPhase::Lobby) {
        if (Attribute(message,"room") != std::to_string(room_)) Fail("Room settings do not match current lobby");
        for(const char* key : {"revision","match_type","length","time","fast","advanced","starting_points","target_points","bankruptcy","ancient","red_count","open_tanyao","minimum","open_hands"})
            if(!message.attributes.count(key) || message.attributes.at(key).size()>32) Fail("Incomplete room settings");
        const auto& kind=message.attributes.at("match_type");const auto& length=message.attributes.at("length");
        if((kind!="standard3" && kind!="standard4") || (length!="single" && length!="east" && length!="south")) Fail("Unsupported room settings");
        snapshot.room_settings = message.attributes;
        if(snapshot.native_service && Attribute(message,"rid")==std::to_string(snapshot.native_request)) snapshot.native_pending.clear();
        ++snapshot.lobby_revision;
        return {};
    }
    const bool waiting_lobby = phase == LobbyPhase::Cancelling || phase == LobbyPhase::ChangingLobby || phase == LobbyPhase::AwaitReturn;
    // A known lobby identifier is the controlled-service confirmation barrier.
    // LN statistics and CS without a matching lobby cannot acknowledge a request.
    if (match_games && (tag == "CS" || tag == "CHAT") && message.attributes.count("lobby")) {
        const auto lobby = Attribute(message, "lobby");
        if (!waiting_lobby && (phase == LobbyPhase::AwaitLobby || phase == LobbyPhase::Lobby) &&
            lobby.size() == 5 && lobby[0] == 'L' && std::all_of(lobby.begin() + 1, lobby.end(), Digit)) {
            const auto room = Integer(lobby.substr(1));
            if (room == 0 || room >= 1000) { if(room_!=room) snapshot.room_settings.clear(); room_ = room; }
        }
        if (waiting_lobby && lobby == snapshot.target_lobby) {
            const auto revision = snapshot.actions.revision + 1;
            game_ = {}; actions_ = {}; actions_.snapshot.enabled = interactive_games; actions_.snapshot.revision = revision;
            snapshot.game = {}; snapshot.actions = actions_.snapshot; snapshot.phase = LobbyPhase::Lobby;
            snapshot.target_lobby.clear(); ++snapshot.lobby_revision;
        }
    }
    if (tag == "REJOIN") {
        if (!recover_games || !resume_connection || !interactive_games || rejoin_received_ || recovery_go_ || snapshot.phase != LobbyPhase::Lobby)
            Fail("Unexpected or disabled REJOIN");
        const auto type = Attribute(message, "t");
        if (type.empty() || type.size() > 64) Fail("Invalid REJOIN t");
        std::size_t start = 0; int fields = 0;
        for (;;) {
            const auto end = type.find(',', start);
            if (Integer(type.substr(start, end == std::string::npos ? end : end - start)) > 65535 || ++fields > 4)
                Fail("Invalid REJOIN t range");
            if (end == std::string::npos) break;
            start = end + 1;
        }
        rejoin_received_ = true;
        return {"<JOIN t=\"" + type + "\" />"};
    }
    if (tag == "REINIT" && !recover_games) Fail("Online rejoin is not supported in this stage");
    if ((observe_games || interactive_games) && OnlineGame::Handles(tag)) {
        if (match_games && !snapshot.recovering) {
            if (tag == "GO") {
                if (phase != LobbyPhase::Matching && phase != LobbyPhase::Cancelling) Fail("GO outside an active match request");
                if (Integer(Attribute(message, "type")) != snapshot.match_type) Fail("GO rule differs from requested match");
                snapshot.target_lobby.clear(); ++snapshot.lobby_revision;
            } else if (tag == "UN" || tag == "TAIKYOKU") {
                if (phase != LobbyPhase::Matching && phase != LobbyPhase::Cancelling && phase != LobbyPhase::Game && phase != LobbyPhase::AwaitRecovery)
                    Fail("Game identity outside matchmaking/game");
                if (!game_.state.started && tag == "UN") {
                    game_.Receive(message); snapshot.game = game_.state; return {};
                }
            } else if (phase != LobbyPhase::Game && phase != LobbyPhase::AwaitRecovery && !(phase == LobbyPhase::MatchComplete && tag == "PROF"))
                Fail("Game event outside active game");
        }
        if (snapshot.phase != LobbyPhase::Lobby && snapshot.phase != LobbyPhase::Matching && snapshot.phase != LobbyPhase::Cancelling &&
            snapshot.phase != LobbyPhase::MatchComplete && snapshot.phase != LobbyPhase::AwaitRecovery && snapshot.phase != LobbyPhase::Game) Fail("Game message before lobby confirmation");
        if (!snapshot.native_service && resume_connection && snapshot.recovering && tag != "GO" && tag != "UN" && tag != "TAIKYOKU" && tag != "REINIT")
            Fail("Expected REINIT before resumed game events");
        if (tag == "REINIT" && (!game_.state.started || (resume_connection && !recovery_go_))) Fail("REINIT before recovery GO");
        const auto before = game_.state;
        auto game_message=message;
        if(snapshot.native_service) game_message.attributes.erase("window");
        if(snapshot.native_service && tag=="GO") {
            snapshot.exit_table=Attribute(message,"table"); snapshot.exit_allowed=Attribute(message,"exit")=="1";
            snapshot.ranked=Attribute(message,"ranked")=="1";
            snapshot.exit_pending=false;
            for(const char* key:{"table","exit","ranked"}) game_message.attributes.erase(key);
        }
        game_.Receive(game_message);
        actions_.Receive(game_message, before, game_.state);
        snapshot.game = game_.state; snapshot.actions = actions_.snapshot;
        if (game_.state.started) snapshot.phase = snapshot.recovering ? LobbyPhase::AwaitRecovery : LobbyPhase::Game;
        if (tag == "REINIT") { snapshot.recovering = false; snapshot.phase = LobbyPhase::Game; resume_connection = false; }
        if (match_games && game_.state.complete && (tag == "PROF" || phase == LobbyPhase::MatchComplete)) {
            snapshot.phase = LobbyPhase::MatchComplete;
            snapshot.last_settlement = game_.state.settlement;
            snapshot.last_result_red = (game_.state.type & 2) == 0;
            snapshot.last_result_dealer=game_.state.dealer;
            snapshot.last_result_honba=game_.state.honba; snapshot.last_result_sticks=game_.state.sticks;
            for (int i = 0; i < 4; ++i) snapshot.last_result_names[i] = game_.state.players[i].name;
            if (phase != LobbyPhase::MatchComplete) {
                ++snapshot.lobby_revision;
                snapshot.match_result = "Match complete | " + game_.state.result;
                for (int i = 0; i < 4; ++i) snapshot.match_result += " | Seat " + std::to_string(i) + ": " + std::to_string(game_.state.players[i].score);
                if (!game_.state.final_scores.empty()) snapshot.match_result += " | Final scores / points: " + game_.state.final_scores;
            }
        }
        if (snapshot.native_service && snapshot.recovering) return {};
        if (tag == "GO" && resume_connection) {
            recovery_go_ = true;
            if (interactive_games) return {"<GOK />", "<REINIT />"};
        }
        if (interactive_games && tag == "GO") return {"<GOK />"};
        return {};
    }
    if (tag == "GO" || tag == "TAIKYOKU" || tag == "INIT" || tag == "REINIT")
        Fail("Online game/rejoin is not supported in this stage");
    if (tag == "HELO") {
        if (snapshot.phase != LobbyPhase::AwaitHelo) Fail("Unexpected repeated HELO");
        if (!Attribute(message, "closing").empty()) Fail("Server is closing");
        snapshot.server = message.attributes; snapshot.server.erase("auth");
        snapshot.identity = DecodeNetworkText(Attribute(message, "uname"));
        if (snapshot.identity.size() > 1024) Fail("Server identity exceeds limit");
        snapshot.phase = LobbyPhase::AwaitLobby;
        if(Attribute(message,"uname")=="LOCAL-NATIVE" && Attribute(message,"version")=="26") {
            snapshot.native_service=true;
            std::string credentials;
            if(!native_password.empty() || native_register) credentials=" password=\""+Escape(native_password)+"\" nickname=\""+Escape(native_nickname)+"\" register=\""+(native_register?"1":"0")+"\"";
            native_password.clear(); native_nickname.clear(); native_register=false;
            return {"<CAPS version=\"26\" token=\""+Escape(native_token)+"\" sx=\""+native_gender+"\" presentation=\""+(presentation?"1":"0")+"\""+credentials+" />"};
        }
        if(require_native) Fail("The configured endpoint is not a Stage 26 native service");
        if (message.attributes.count("auth")) return {"<AUTH val=\"" + AuthenticationResponse(Attribute(message, "auth")) + "\"/>"};
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
                if (it->first.compare(0, 3, "CS.") == 0) it = snapshot.server.erase(it); else ++it;
            }
            for (const auto& item : message.attributes) if (item.first != "auth") snapshot.server["CS." + item.first] = item.second;
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
std::vector<std::string> LobbySession::SubmitLobby(std::uint64_t revision, const std::string& command, unsigned value) {
    if (!match_games || !interactive_games || observe_games) Fail("Local matchmaking is disabled");
    if (revision != snapshot.lobby_revision) Fail("Stale lobby revision");
    const auto roomName = [](unsigned room) {
        std::ostringstream out; out << 'L' << std::setw(4) << std::setfill('0') << room; return out.str();
    };
    const auto change = [](unsigned room) { return "<CHAT text=\"%2Flobby%20" + std::to_string(room) + "\" />"; };
    std::vector<std::string> replies;
    if (command == "change") {
        if (snapshot.phase != LobbyPhase::Lobby) Fail("Lobby change requires ready lobby");
        if (value != 0 && (value < 1000 || value > 9999)) Fail("Supported room is 0 or 1000..9999");
        replies = {change(value)};
        snapshot.room_settings.clear();
        room_ = value; snapshot.target_lobby = roomName(value); snapshot.phase = LobbyPhase::ChangingLobby;
    } else if (command == "match") {
        if (snapshot.phase != LobbyPhase::Lobby || snapshot.lobby != roomName(room_)) Fail("Confirm a supported room before matching");
        const std::set<unsigned> native_rules{1,9,65,73,17,25,81,89,7,71,23,87};
        if (snapshot.native_service ? (!room_ && !native_rules.count(value)) : (value != 1 && value != 9 && value != 17 && value != 25)) Fail("Unsupported game rule preset");
        replies = {"<JOIN t=\"" + std::to_string(room_) + "," + std::to_string(value) + "\" />"};
        if(snapshot.native_service && room_) {
            if(!snapshot.room_settings.count("revision")) Fail("Confirm room settings before joining queue");
            replies={"<JOIN t=\""+std::to_string(room_)+","+std::to_string(value)+"\" revision=\""+Escape(snapshot.room_settings.at("revision"))+"\" />"};
        }
        snapshot.match_type = value; snapshot.match_result.clear(); snapshot.phase = LobbyPhase::Matching;
        // JOIN selects family/length; local ROOM supplies the complete GO flags.
        if (!snapshot.native_service && !snapshot.room_settings.empty()) {
            const auto& room=snapshot.room_settings;
            snapshot.match_type |= (room.at("red_count")=="0"?2u:0u) |
                (room.at("open_tanyao")=="0"?4u:0u) | (room.at("fast")=="1"?64u:0u);
        }
        snapshot.last_settlement = {}; snapshot.last_result_names = {};
        snapshot.queue_confirmed=false;
        game_.state.settlement = {}; snapshot.game.settlement = {};
    } else if (command == "cancel") {
        if (snapshot.phase != LobbyPhase::Matching) Fail("No match request to cancel");
        replies = {"<JOIN />", change(room_)};
        snapshot.target_lobby = roomName(room_); snapshot.phase = LobbyPhase::Cancelling;
    } else if (command == "return") {
        if (snapshot.phase != LobbyPhase::MatchComplete || !snapshot.game.complete) Fail("Match is not complete");
        if (snapshot.lobby != roomName(room_)) Fail("Return requires a confirmed supported lobby");
        replies = {"<BYE />", change(room_)};
        snapshot.target_lobby = roomName(room_); snapshot.phase = LobbyPhase::AwaitReturn;
    } else Fail("Unknown lobby command");
    ++snapshot.lobby_revision;
    snapshot.native_error.clear();
    return replies;
}
std::vector<std::string> LobbySession::NativeRequest(const std::string& command,unsigned value,const std::map<std::string,std::string>& settings) {
    if(!snapshot.native_service || snapshot.phase!=LobbyPhase::Lobby || !snapshot.native_pending.empty()) Fail("Native room request is not available");
    std::string tag;
    if(command=="create") tag="ROOMCREATE"; else if(command=="join") tag="ROOMJOIN";
    else if(command=="leave") tag="ROOMLEAVE"; else if(command=="close") tag="ROOMCLOSE"; else if(command=="settings") tag="SETROOM"; else Fail("Unknown native request");
    std::string wire="<"+tag+" rid=\""+std::to_string(++snapshot.native_request)+"\"";
    if(command=="join") { if(value<1000 || value>9999) Fail("Invalid room number"); wire+=" room=\""+std::to_string(value)+"\""; }
    if(command=="settings") { if(!snapshot.room_settings.count("revision")) Fail("Room revision missing"); wire+=" revision=\""+Escape(snapshot.room_settings.at("revision"))+"\""; }
    if(command=="create" || command=="settings") for(const auto& item:settings) {
        if(item.first=="room" || item.first=="revision" || item.first=="owner" || item.first=="rid") continue;
        if(item.first.empty() || !std::all_of(item.first.begin(),item.first.end(),[](char c) { return (c>='a'&&c<='z') || c=='_'; })) Fail("Invalid setting key");
        wire+=" "+item.first+"=\""+Escape(item.second)+"\"";
    }
    snapshot.native_error.clear(); snapshot.native_pending=command; return {wire+" />"};
}
std::vector<std::string> LobbySession::SubmitAction(std::uint64_t revision, std::size_t index) {
    if(snapshot.table_paused || snapshot.exit_pending) return {};
    if (snapshot.phase != LobbyPhase::Game) Fail("Action outside game session");
    auto next = actions_; const auto replies = next.Submit(revision, index);
    actions_ = std::move(next); snapshot.actions = actions_.snapshot; return replies;
}
std::vector<std::string> LobbySession::ExitGame(int choice) {
    if(!snapshot.native_service || !snapshot.exit_allowed || snapshot.exit_table.empty() || snapshot.recovering ||
        snapshot.phase!=LobbyPhase::Game) return {};
    if(choice<0) {
        if(snapshot.exit_pending || snapshot.exit_voting) return {};
        snapshot.exit_pending=true; return {"<EXIT table=\""+Escape(snapshot.exit_table)+"\" />"};
    }
    if(choice>1 || !snapshot.exit_voting || snapshot.exit_choice>=0 || snapshot.exit_ballot_pending) return {};
    snapshot.exit_ballot_pending=true;
    return {"<EXITVOTE table=\""+Escape(snapshot.exit_table)+"\" id=\""+std::to_string(snapshot.exit_vote_id)+"\" yes=\""+std::to_string(choice)+"\" />"};
}
std::vector<std::string> LobbySession::ConfigureRoom(std::uint64_t revision, const std::map<std::string,std::string>& settings) {
    if (!match_games || !interactive_games || observe_games || snapshot.phase!=LobbyPhase::Lobby || revision!=snapshot.lobby_revision)
        Fail("Room configuration requires the current local lobby");
    if (snapshot.room_settings.empty()) Fail("Server does not advertise configurable rooms");
    std::string command="<SETROOM";
    for (const auto& item : settings) {
        if (item.first=="room") continue;
        if (item.first.empty() || !std::all_of(item.first.begin(),item.first.end(),[](char c){return (c>='a' && c<='z') || c=='_';}) ||
            item.second.size()>32 || !std::all_of(item.second.begin(),item.second.end(),[](char c){return (c>='a' && c<='z') || (c>='0' && c<='9') || c=='+';}))
            Fail("Invalid room setting encoding");
        command+=" "+item.first+"=\""+item.second+"\"";
    }
    return {command+" />"};
}
std::string NetworkJson(const std::string& value) {
    std::ostringstream out; out << '"';
    for (unsigned char c : value) {
        if (c == '"' || c == '\\') out << '\\' << c;
        else if (c < 32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << unsigned(c);
        else out << c;
    }
    out << '"'; return out.str();
}
} // namespace tenhou
