#include "network_protocol_internal.hpp"
#include "monotonic_clock.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>

namespace tenhou {
    using protocol_detail::Attribute;
    using protocol_detail::Digit;
    using protocol_detail::Escape;
    using protocol_detail::Fail;
    using protocol_detail::Integer;
    using protocol_detail::Letter;

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
                    if (const auto room = LobbyNumber(Attribute(message, "lobby")))
                        snapshot.lobby = *room;
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
                snapshot.certification = LocalizeCertification(DecodeNetworkText(notice));
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

} // namespace tenhou
