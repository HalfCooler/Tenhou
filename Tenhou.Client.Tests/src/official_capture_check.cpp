#include <winsock2.h>
#include <ws2tcpip.h>
#include "network_client.hpp"
#include "ini_store.hpp"
#include "network_protocol.hpp"
#include "settlement_scene.hpp"
#include "table_scene.hpp"
#include "win32_utf.hpp"
#include <windows.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace tenhou;

namespace {
    void Require(bool ok, const std::string &message) {
        if (!ok) throw std::runtime_error(message);
    }

    std::string DecodeBase64(const std::string &input) {
        static const auto table = [] {
            std::array<int, 256> t{};
            t.fill(-1);
            for (int i = 0; i < 26; ++i) t[static_cast<unsigned char>('A' + i)] = i;
            for (int i = 0; i < 26; ++i) t[static_cast<unsigned char>('a' + i)] = 26 + i;
            for (int i = 0; i < 10; ++i) t[static_cast<unsigned char>('0' + i)] = 52 + i;
            t[static_cast<unsigned char>('+')] = 62;
            t[static_cast<unsigned char>('/')] = 63;
            return t;
        }();
        std::string out;
        out.reserve(input.size() * 3 / 4);
        int val = 0, valb = -8;
        for (unsigned char c: input) {
            if (c == '=') break;
            const int d = table[c];
            if (d < 0) throw std::runtime_error("Invalid Base64 character");
            val = (val << 6) + d;
            valb += 6;
            if (valb >= 0) {
                out.push_back(static_cast<char>((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return out;
    }

    std::string JsonStringField(const std::string &line, const std::string &key) {
        const std::string needle = "\"" + key + "\":\"";
        const auto pos = line.find(needle);
        if (pos == std::string::npos) return {};
        std::size_t i = pos + needle.size();
        std::string out;
        while (i < line.size() && line[i] != '"') {
            if (line[i] == '\\' && i + 1 < line.size()) {
                ++i;
                out.push_back(line[i]);
            } else {
                out.push_back(line[i]);
            }
            ++i;
        }
        return out;
    }

    int JsonIntField(const std::string &line, const std::string &key) {
        const std::string needle = "\"" + key + "\":";
        const auto pos = line.find(needle);
        if (pos == std::string::npos) return -1;
        return std::stoi(line.substr(pos + needle.size()));
    }

    struct RawEvent {
        int seq = 0;
        std::string kind;
        std::string direction;
        std::string bytes;
    };

    std::vector<RawEvent> LoadRawCapture(const std::filesystem::path &path) {
        std::ifstream file(path, std::ios::binary);
        Require(static_cast<bool>(file), "Cannot open raw capture: " + PathToUtf8(path));
        std::string line;
        Require(static_cast<bool>(std::getline(file, line)), "Empty capture file");
        Require(JsonStringField(line, "format") == "tenhou-origin-session-raw/1",
                "Expected raw capture format: " + PathToUtf8(path));
        std::vector<RawEvent> events;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            RawEvent ev;
            ev.seq = JsonIntField(line, "seq");
            ev.kind = JsonStringField(line, "kind");
            if (ev.kind == "frame") {
                ev.direction = JsonStringField(line, "direction");
                ev.bytes = DecodeBase64(JsonStringField(line, "data_b64"));
                Require(!ev.bytes.empty() && ev.bytes.back() == '\0',
                        "Raw frame missing trailing NUL at seq=" + std::to_string(ev.seq));
            }
            events.push_back(std::move(ev));
        }
        return events;
    }

    LobbySession MakeOfficialSession() {
        LobbySession session;
        session.official = true;
        session.origin_lobby = true;
        session.interactive_games = true;
        session.match_games = true;
        session.recover_games = true;
        session.preferred_monthly_profile = 17;
        return session;
    }

    void SeedLobby(LobbySession &session) {
        (void) session.Connected("ID00000000-00000000", 'M');
        (void) session.Receive(ParseNetworkMessage(
            "<HELO uname=\"Fixture\" PF4=\"11,445,1653.76,-57.5,40,38,38,39,11,1317,294,194,474,228\" "
            "PF3=\"11,115,1515.30,-2157.3,85,101,123,0,42,2355,638,439,707,518\" expire=\"20270527\" "
            "expiredays=\"189\" opt=\"3\" />", true));
        (void) session.Receive(ParseNetworkMessage("<LN n=\"A1A1A1A\" j=\"112A\" g=\"112A\" />", true));
    }

    void ReplayRawCapture(const std::filesystem::path &path, bool starts_with_helo,
                          bool expect_complete_return, bool expect_early_exit, bool expect_chips) {
        const auto events = LoadRawCapture(path);
        LobbySession session = MakeOfficialSession();
        NetworkFramer framer;
        bool connected = false;
        bool saw_prof = false;
        bool saw_saikai = false;
        bool saw_reinit = false;
        bool returned_to_lobby = false;
        bool submitted_return = false;
        bool submitted_exit = false;
        int inbound_frames = 0;
        int verified_actions = 0;
        int initial_pf3_games = -1;
        bool riichi_just_sent = false;
        SettlementState final_settlement;

        if (!starts_with_helo) {
            SeedLobby(session);
            connected = true;
        }

        const auto submit_matching = [&](const std::string &wire, int seq, const std::string &kind_hint = {}) {
            if (session.snapshot.phase != LobbyPhase::Game || !session.snapshot.actions.pending.empty() ||
                session.snapshot.game.awaiting_context)
                return;
            const auto &choices = session.snapshot.actions.choices;
            auto it = std::find_if(choices.begin(), choices.end(), [&](const OnlineAction &a) {
                if (!kind_hint.empty() && a.kind != kind_hint) return false;
                return !a.messages.empty() && a.messages.front() == wire;
            });
            Require(it != choices.end(),
                    "Action not offered by OnlineActions for " + wire + " at seq=" + std::to_string(seq));
            (void) session.SubmitAction(session.snapshot.actions.revision,
                                        static_cast<std::size_t>(it - choices.begin()));
            ++verified_actions;
        };

        for (const auto &ev: events) {
            if (ev.kind == "detach") {
                riichi_just_sent = false;
                continue;
            }
            if (ev.kind != "frame") continue;

            if (ev.direction == "out") {
                const auto raw_xml = ev.bytes.substr(0, ev.bytes.size() - 1);
                const auto msg = ParseNetworkMessage(raw_xml, true);
                if (msg.tag == "HELO") {
                    if (!connected) {
                        const auto out = session.Connected("ID00000000-00000000", 'M');
                        Require(out.size() == 1 && out[0].rfind("<HELO ", 0) == 0, "Expected outbound HELO");
                        connected = true;
                    } else if (session.snapshot.phase == LobbyPhase::MatchComplete) {
                        const auto out = session.SubmitLobby(session.snapshot.lobby_revision, "return");
                        Require(out.size() == 1 && out[0].rfind("<HELO ", 0) == 0,
                                "Official return did not send HELO");
                        Require(session.snapshot.phase == LobbyPhase::AwaitReturn,
                                "Official return did not enter AwaitReturn");
                        submitted_return = true;
                    }
                } else if (msg.tag == "BYE" && session.snapshot.phase == LobbyPhase::Game &&
                           session.snapshot.exit_allowed) {
                    const auto out = session.ExitGame();
                    Require(out.size() == 2 && out[0] == "<BYE />" && out[1].rfind("<HELO ", 0) == 0,
                            "Official practice exit did not send BYE + HELO");
                    Require(session.snapshot.phase == LobbyPhase::AwaitReturn,
                            "Official exit did not enter AwaitReturn");
                    submitted_exit = true;
                } else if (msg.tag == "JOIN" && msg.attributes.count("t") &&
                           (session.snapshot.phase == LobbyPhase::Lobby ||
                            session.snapshot.phase == LobbyPhase::Matching)) {
                    const auto &t = msg.attributes.at("t");
                    const auto comma = t.find(',');
                    if (comma != std::string::npos) {
                        const auto type_str = t.substr(comma + 1);
                        (void) session.SubmitOriginLobby("join", type_str);
                    }
                } else if (msg.tag == "NEXTREADY" && session.snapshot.game.ended) {
                    submit_matching("<NEXTREADY />", ev.seq, "next");
                } else if (msg.tag == "REACH") {
                    submit_matching(raw_xml, ev.seq, "riichi");
                    riichi_just_sent = true;
                } else if (msg.tag == "D") {
                    if (riichi_just_sent) {
                        riichi_just_sent = false;
                    } else {
                        submit_matching(raw_xml, ev.seq, "discard");
                    }
                } else if (msg.tag == "N") {
                    if (msg.attributes.count("type") && msg.attributes.at("type") == "4") {
                        // Closed kan: OnlineActions uses the lowest physical tile ID of the 4-of-a-kind;
                        // tenhou.exe may send any tile ID of the same kind.
                        if (session.snapshot.phase == LobbyPhase::Game && session.snapshot.actions.pending.empty() &&
                            !session.snapshot.game.awaiting_context) {
                            const int tile = std::stoi(msg.attributes.at("hai"));
                            const auto &choices = session.snapshot.actions.choices;
                            const auto it = std::find_if(choices.begin(), choices.end(), [&](const OnlineAction &a) {
                                return a.kind == "closed_kan" && !a.tiles.empty() && a.tiles.front() / 4 == tile / 4;
                            });
                            Require(it != choices.end(),
                                    "Closed kan choice missing at seq=" + std::to_string(ev.seq));
                            (void) session.SubmitAction(session.snapshot.actions.revision,
                                                        static_cast<std::size_t>(it - choices.begin()));
                            ++verified_actions;
                        }
                    } else {
                        submit_matching(raw_xml, ev.seq);
                    }
                }
                continue;
            }

            const auto frames = framer.Feed(ev.bytes.data(), ev.bytes.size());
            Require(frames.size() == 1, "Expected single framed message at seq=" + std::to_string(ev.seq));
            const auto msg = ParseNetworkMessage(frames[0], true);
            ++inbound_frames;

            // Simulate process-restart reconnection segment that begins directly at GO after a detach.
            if (msg.tag == "GO" && session.snapshot.game.started && !session.snapshot.game.complete) {
                session = MakeOfficialSession();
                session.resume_connection = true;
                session.official_game_id = "reconnect-fixture";
                (void) session.Connected("ID00000000-00000000", 'M');
            }

            const auto replies = session.Receive(msg);
            if (msg.tag == "PROF") {
                saw_prof = true;
                Require(!session.snapshot.game.complete, "Pre-AGARI PROF must not mark game complete early");
            }
            if (msg.tag == "SAIKAI") {
                saw_saikai = true;
                Require(replies.empty(), "Official SAIKAI must not transmit <REINIT />");
            }
            if (msg.tag == "REINIT") {
                saw_reinit = true;
                Require(session.snapshot.phase == LobbyPhase::Game && !session.snapshot.recovering,
                        "REINIT did not resume live game");
            }
            if (msg.tag == "LN" && initial_pf3_games < 0 && session.snapshot.origin_lobby) {
                initial_pf3_games = session.snapshot.origin_lobby->profile.lifetime[1].games;
            }
            if (session.snapshot.game.complete && final_settlement.receipts.empty()) {
                final_settlement = session.snapshot.game.settlement;
            }
            if ((submitted_return || submitted_exit) && msg.tag == "LN" &&
                session.snapshot.phase == LobbyPhase::Lobby) {
                returned_to_lobby = true;
            }
        }

        Require(inbound_frames > 0, "No inbound frames replayed");
        if (expect_complete_return) {
            Require(saw_prof, "Expected PROF in ranked capture");
            Require(submitted_return && returned_to_lobby, "Expected return-to-lobby HELO -> LN flow");
            Require(session.snapshot.origin_lobby && session.snapshot.phase == LobbyPhase::Lobby,
                    "Session did not return to official lobby");
            if (starts_with_helo && initial_pf3_games >= 0) {
                Require(session.snapshot.origin_lobby->profile.lifetime[1].games > initial_pf3_games,
                        "Post-match HELO did not refresh lifetime profile statistics");
            }
        }
        if (expect_early_exit) {
            Require(submitted_exit && returned_to_lobby, "Expected practice BYE + HELO -> LN return");
        }
        if (expect_chips) {
            Require(final_settlement.has_final && final_settlement.has_final_chips && final_settlement.has_ratio,
                    "Expected 16-item owari with chips and ratio");
            Require(final_settlement.ratio == std::array<int, 2>{5000, 10}, "Unexpected ratio");
            Require(final_settlement.final_chips[0] == 4 && final_settlement.final_chips[1] == -2 &&
                    final_settlement.final_chips[3] == -2, "Unexpected final chips");
            const std::array<std::string, 4> names{"HalfCool", "COM", "", "COM"};
            const auto round_scene = BuildSettlementScene(final_settlement, 0, names, 10, 14, true, true);
            const auto has_text = [](const SettlementScene &scene, const char *role, const std::string &value) {
                return std::any_of(scene.text.begin(), scene.text.end(), [&](const SettlementTextItem &t) {
                    return t.role == role && t.text == value;
                });
            };
            Require(has_text(round_scene, "chip_base", "+4枚") && has_text(round_scene, "chip_base", "-2枚"),
                    "Round settlement scene missing chip display");
            const auto owari_scene = BuildSettlementScene(final_settlement, final_settlement.receipts.size(),
                                                          names, 0, 0, true, true);
            Require(has_text(owari_scene, "final_chips", "+4枚") &&
                    has_text(owari_scene, "final_gold", "+13860G") &&
                    has_text(owari_scene, "final_gold", "-4980G") &&
                    has_text(owari_scene, "final_gold", "-8880G"),
                    "Final OWARI scene chip/gold values do not match origin formula");
        }
        std::cout << "PASS raw capture: " << PathToUtf8(path.filename())
                  << " (in=" << inbound_frames << " actions=" << verified_actions
                  << (saw_prof ? " PROF" : "")
                  << (saw_saikai ? " SAIKAI" : "")
                  << (saw_reinit ? " REINIT" : "")
                  << (returned_to_lobby ? " RETURNED" : "") << ")\n";
    }

    void CheckPrivateRoomRejoinAndSaikaiReconnect() {
        // Mirrors 个室 三食断赤 东风战（用于测试断线重连）.jsonl:
        // 1) Initial private-room matching with non-numeric REJOIN ticket -> GO -> UN -> TAIKYOKU -> NEXTREADY -> INIT
        LobbySession session = MakeOfficialSession();
        SeedLobby(session);
        const auto move = session.SubmitOriginLobby("change", "4409");
        Require(move == std::vector<std::string>{"<CHAT text=\"%2Flobby%204409\" />"}, "Expected private lobby move");
        (void) session.Receive(ParseNetworkMessage("<CHAT lobby=\"4409\" />", true));
        Require(session.snapshot.lobby == "L4409", "Did not enter L4409");

        const auto join = session.SubmitOriginLobby("join", "17");
        Require(join == std::vector<std::string>{"<JOIN t=\"4409,17\" />"}, "Expected private room JOIN");
        Require(session.snapshot.phase == LobbyPhase::Matching, "Expected Matching phase");

        const auto rejoin = session.Receive(ParseNetworkMessage("<REJOIN t=\"4409,17,r\" />", true));
        Require(rejoin == std::vector<std::string>{"<JOIN t=\"4409,17,r\" />"},
                "REJOIN did not echo non-numeric ticket");
        Require(session.snapshot.phase == LobbyPhase::Matching && !session.snapshot.recovering,
                "Matching REJOIN must not switch into recovery mode");

        const auto gok = session.Receive(ParseNetworkMessage(
            "<GO type=\"17\" lobby=\"4409\" gpid=\"gp-private-1\" />", true));
        Require(gok == std::vector<std::string>{"<GOK />"}, "Expected GOK");
        Require(!session.snapshot.exit_allowed, "Private gpid game should not mark practice exit_allowed");
        (void) session.Receive(ParseNetworkMessage(
            "<UN n0=\"Self\" n1=\"COM1\" n2=\"\" n3=\"COM3\" dan=\"11,0,0,0\" rate=\"1527.43,1500,1500,1500\" />",
            true));
        const auto ready = session.Receive(ParseNetworkMessage("<TAIKYOKU oya=\"3\" log=\"log-token\" />", true));
        Require(ready == std::vector<std::string>{"<NEXTREADY />"},
                "Private room TAIKYOKU after REJOIN did not send NEXTREADY");
        (void) session.Receive(ParseNetworkMessage("<CHAT />", true));
        (void) session.Receive(ParseNetworkMessage(
            "<INIT seed=\"0,0,0,5,0,104\" ten=\"350,350,0,350\" oya=\"3\" "
            "hai=\"53,61,120,71,84,36,123,58,90,91,51,88,46\" />", true));
        Require(session.snapshot.phase == LobbyPhase::Game && session.snapshot.game.initialized,
                "Private room did not reach live Game after INIT");

        // 2) Restart/reconnect with saved gpid: HELO(gpid) -> GO -> GOK -> UN -> SAIKAI (no REINIT out!) -> REINIT -> Nuki
        LobbySession resume = MakeOfficialSession();
        resume.resume_connection = true;
        resume.official_game_id = "gp-private-1";
        const auto helo = resume.Connected("ID00000000-00000000", 'M');
        Require(helo.size() == 1 && helo[0].find("gpid=\"gp-private-1\"") != std::string::npos,
                "Reconnect HELO missing gpid");
        const auto gok2 = resume.Receive(ParseNetworkMessage(
            "<GO type=\"17\" lobby=\"4409\" gpid=\"gp-private-1\" />", true));
        Require(gok2 == std::vector<std::string>{"<GOK />"}, "Reconnect GO did not send GOK");
        (void) resume.Receive(ParseNetworkMessage(
            "<UN n0=\"Self\" n1=\"COM1\" n2=\"\" n3=\"COM3\" dan=\"11,0,0,0\" rate=\"1527.43,1500,1500,1500\" />",
            true));
        const auto saikai = resume.Receive(ParseNetworkMessage("<SAIKAI oya=\"3\" />", true));
        Require(saikai.empty(), "SAIKAI must not send <REINIT />");
        Require(resume.snapshot.phase == LobbyPhase::AwaitRecovery, "SAIKAI should wait for server REINIT");
        (void) resume.Receive(ParseNetworkMessage(
            "<REINIT seed=\"0,0,0,5,0,104\" ten=\"350,350,0,350\" oya=\"3\" "
            "hai=\"36,46,51,53,58,61,71,84,88,90,91,120,123\" kawa0=\"83\" kawa1=\"37\" kawa3=\"134\" />", true));
        Require(resume.snapshot.phase == LobbyPhase::Game && !resume.snapshot.recovering,
                "REINIT did not complete reconnect");
        (void) resume.Receive(ParseNetworkMessage("<W />", true));
        (void) resume.Receive(ParseNetworkMessage("<g32 />", true));
        (void) resume.Receive(ParseNetworkMessage("<T72 t=\"128\" />", true));
        const auto &choices = resume.snapshot.actions.choices;
        const auto nuki = std::find_if(choices.begin(), choices.end(),
                                       [](const OnlineAction &a) { return a.kind == "nuki"; });
        Require(nuki != choices.end(), "Nuki action missing after REINIT + T72 t=128");
        const auto nuki_wire = resume.SubmitAction(resume.snapshot.actions.revision,
                                                   static_cast<std::size_t>(nuki - choices.begin()));
        Require(nuki_wire == std::vector<std::string>{"<N type=\"10\" />"},
                "Official nuki did not transmit <N type=\"10\" />");
        (void) resume.Receive(ParseNetworkMessage("<N who=\"0\" m=\"30752\" />", true));
        Require(resume.snapshot.actions.pending.empty() && resume.snapshot.game.players[0].nuki.size() == 1,
                "Server nuki confirmation failed");
        std::cout << "PASS private room REJOIN + SAIKAI reconnect + official N type=10\n";
    }

    void ReplayFullReconnectCapture(const std::filesystem::path &path) {
        // Replays 个室 三食断赤 东风战（完整断线重连再开）.jsonl across both process streams:
        // Stream 1: stale gpid HELO -> server HELO fallback -> L4409 -> JOIN -> GO(gpid) -> INIT -> D112 -> detach.
        // Stream 2: saved gpid HELO -> server GO directly -> GOK -> UN -> SAIKAI(ba,oya,sc) -> REINIT -> D131 -> BYE/e63.
        const auto events = LoadRawCapture(path);
        LobbySession session = MakeOfficialSession();
        NetworkFramer framer;
        std::string saved_gpid = "928CC0A7-50692F1D"; // Stream 1 stale gpid from #4.
        bool saw_stale_helo_fallback = false;
        bool saw_live_go_direct = false;
        bool saw_saikai = false;
        bool saw_reinit = false;
        bool saw_post_reinit_pon = false;
        bool saw_bye_seat1 = false;
        bool saw_bye_seat3 = false;
        int inbound_frames = 0;
        int verified_actions = 0;

        for (const auto &ev: events) {
            if (ev.kind == "detach") {
                session = MakeOfficialSession();
                continue;
            }
            if (ev.kind != "frame") continue;

            if (ev.direction == "out") {
                const auto raw_xml = ev.bytes.substr(0, ev.bytes.size() - 1);
                const auto msg = ParseNetworkMessage(raw_xml, true);
                if (msg.tag == "HELO") {
                    const auto gpid_it = msg.attributes.find("gpid");
                    Require(gpid_it != msg.attributes.end() && gpid_it->second == saved_gpid,
                            "Captured HELO gpid mismatch at seq=" + std::to_string(ev.seq));
                    session.resume_connection = true;
                    session.official_game_id = saved_gpid;
                    const auto out = session.Connected("ID00000000-00000000", 'M');
                    Require(out.size() == 1 &&
                            out[0] == "<HELO name=\"ID00000000-00000000\" gpid=\"" + saved_gpid + "\" sx=\"M\" tid=\"w0\" />",
                            "Outbound HELO(gpid) wire mismatch at seq=" + std::to_string(ev.seq));
                } else if (msg.tag == "CHAT" && msg.attributes.count("text")) {
                    const auto decoded = DecodeNetworkText(msg.attributes.at("text"));
                    if (decoded.rfind("/lobby ", 0) == 0 && session.snapshot.phase == LobbyPhase::Lobby) {
                        const auto room = decoded.substr(7);
                        const auto out = session.SubmitOriginLobby("change", room);
                        Require(out == std::vector<std::string>{raw_xml},
                                "Room change wire mismatch at seq=" + std::to_string(ev.seq));
                    }
                } else if (msg.tag == "JOIN" && msg.attributes.count("t")) {
                    const auto &t = msg.attributes.at("t");
                    const auto comma = t.find(',');
                    Require(comma != std::string::npos, "Expected lobby,type in JOIN");
                    const auto out = session.SubmitOriginLobby("join", t.substr(comma + 1));
                    Require(out == std::vector<std::string>{raw_xml},
                            "Private JOIN wire mismatch at seq=" + std::to_string(ev.seq));
                } else if (msg.tag == "D") {
                    const auto &choices = session.snapshot.actions.choices;
                    const auto it = std::find_if(choices.begin(), choices.end(), [&](const OnlineAction &a) {
                        return a.kind == "discard" && !a.messages.empty() && a.messages.front() == raw_xml;
                    });
                    Require(it != choices.end(), "Discard not offered for " + raw_xml + " at seq=" + std::to_string(ev.seq));
                    (void) session.SubmitAction(session.snapshot.actions.revision,
                                                static_cast<std::size_t>(it - choices.begin()));
                    ++verified_actions;
                }
                continue;
            }

            const auto frames = framer.Feed(ev.bytes.data(), ev.bytes.size());
            Require(frames.size() == 1, "Expected single framed message at seq=" + std::to_string(ev.seq));
            const auto msg = ParseNetworkMessage(frames[0], true);
            ++inbound_frames;

            const auto phase_before = session.snapshot.phase;
            const auto replies = session.Receive(msg);
            if (msg.tag == "HELO") {
                // Stale gpid fell back to normal lobby HELO: recovery must clear immediately!
                Require(!session.snapshot.recovering && session.official_game_id.empty(),
                        "Stale gpid HELO response did not immediately clear recovery state");
                saw_stale_helo_fallback = true;
            }
            if (msg.tag == "GO") {
                Require(replies == std::vector<std::string>{"<GOK />"}, "GO did not emit <GOK />");
                if (msg.attributes.count("gpid")) saved_gpid = msg.attributes.at("gpid");
                if (phase_before == LobbyPhase::AwaitHelo) {
                    saw_live_go_direct = true;
                    Require(session.snapshot.phase == LobbyPhase::AwaitRecovery && session.snapshot.lobby == "L4409",
                            "Direct GO in AwaitHelo did not enter AwaitRecovery with lobby L4409");
                }
            }
            if (msg.tag == "TAIKYOKU") {
                Require(replies == std::vector<std::string>{"<NEXTREADY />"}, "TAIKYOKU did not emit <NEXTREADY />");
            }
            if (msg.tag == "SAIKAI") {
                saw_saikai = true;
                Require(replies.empty(), "SAIKAI must not emit outbound <REINIT /> or <NEXTREADY />");
                Require(session.snapshot.phase == LobbyPhase::AwaitRecovery &&
                        session.snapshot.game.has_dealer && session.snapshot.game.dealer == 3 &&
                        session.snapshot.game.honba == 0 && session.snapshot.game.sticks == 0 &&
                        session.snapshot.game.players[0].score == 35000,
                        "SAIKAI did not record dealer/ba/sc while awaiting REINIT");
            }
            if (msg.tag == "REINIT") {
                saw_reinit = true;
                Require(session.snapshot.phase == LobbyPhase::Game && !session.snapshot.recovering &&
                        session.snapshot.game.players[3].nuki.size() == 2 &&
                        session.snapshot.game.players[0].discards.size() == 1 &&
                        session.snapshot.game.players[1].discards.size() == 1 &&
                        session.snapshot.game.players[3].discards.size() == 1,
                        "REINIT did not restore full table snapshot");
            }
            if (msg.tag == "e63") {
                saw_post_reinit_pon = std::any_of(session.snapshot.actions.choices.begin(),
                                                  session.snapshot.actions.choices.end(),
                                                  [](const OnlineAction &a) { return a.kind == "pon"; });
            }
            if (msg.tag == "BYE" && msg.attributes.count("who")) {
                if (msg.attributes.at("who") == "1") {
                    Require(!session.snapshot.game.players[1].connected, "BYE who=1 did not mark seat 1 offline");
                    saw_bye_seat1 = true;
                }
                if (msg.attributes.at("who") == "3") {
                    Require(!session.snapshot.game.players[3].connected, "BYE who=3 did not mark seat 3 offline");
                    saw_bye_seat3 = true;
                }
            }
        }

        Require(saw_stale_helo_fallback, "Missing stale-gpid HELO fallback");
        Require(saw_live_go_direct, "Missing direct GO on live gpid reconnect");
        Require(saw_saikai, "Missing SAIKAI");
        Require(saw_reinit, "Missing REINIT");
        Require(saw_post_reinit_pon, "Missing pon choice on post-REINIT e63 t=1");
        Require(verified_actions == 2, "Expected 2 verified discards (D112 + D131)");
        Require(saw_bye_seat1 && saw_bye_seat3, "Missing BYE seat disconnect tracking");
        std::cout << "PASS full reconnect raw capture: " << PathToUtf8(path.filename())
                  << " (in=" << inbound_frames << " actions=" << verified_actions
                  << " STALE_HELO_FALLBACK DIRECT_GO SAIKAI REINIT BYE)\n";
    }

    struct LoopbackServer {
        SOCKET listen_sock = INVALID_SOCKET;
        SOCKET client_sock = INVALID_SOCKET;
        unsigned port = 0;
        int accept_count = 0;
        bool peer_closed = false;
        NetworkFramer framer;

        LoopbackServer() {
            listen_sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            Require(listen_sock != INVALID_SOCKET, "Cannot create loopback listen socket");
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            addr.sin_port = 0;
            Require(::bind(listen_sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0,
                    "Cannot bind loopback socket");
            int len = sizeof(addr);
            Require(::getsockname(listen_sock, reinterpret_cast<sockaddr *>(&addr), &len) == 0,
                    "Cannot inspect loopback port");
            port = ntohs(addr.sin_port);
            Require(::listen(listen_sock, 4) == 0, "Cannot listen on loopback socket");
            u_long nonblock = 1;
            Require(::ioctlsocket(listen_sock, FIONBIO, &nonblock) == 0, "Cannot set listen nonblocking");
        }

        ~LoopbackServer() {
            CloseClient();
            if (listen_sock != INVALID_SOCKET) ::closesocket(listen_sock);
        }

        void CloseClient() {
            if (client_sock != INVALID_SOCKET) {
                ::closesocket(client_sock);
                client_sock = INVALID_SOCKET;
            }
            framer = {};
        }

        void SendFrame(const std::string &xml) {
            Require(client_sock != INVALID_SOCKET, "Loopback client not accepted");
            std::string wire = xml;
            wire.push_back('\0');
            std::size_t off = 0;
            while (off < wire.size()) {
                const int n = ::send(client_sock, wire.data() + off, static_cast<int>(wire.size() - off), 0);
                Require(n > 0, "Loopback send failed");
                off += static_cast<std::size_t>(n);
            }
        }

        bool WaitPeerClosed(unsigned wait_ms) {
            const ULONGLONG deadline = GetTickCount64() + wait_ms;
            while (GetTickCount64() < deadline) {
                MSG msg{};
                while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
                if (peer_closed) return true;
                if (client_sock != INVALID_SOCKET) {
                    char buf[64];
                    const int n = ::recv(client_sock, buf, sizeof(buf), 0);
                    if (n == 0) {
                        peer_closed = true;
                        CloseClient();
                        return true;
                    }
                }
                Sleep(10);
            }
            return peer_closed;
        }

        std::vector<std::string> Pump(unsigned wait_ms) {
            const ULONGLONG deadline = GetTickCount64() + wait_ms;
            std::vector<std::string> out;
            while (GetTickCount64() < deadline) {
                MSG msg{};
                while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
                if (client_sock == INVALID_SOCKET) {
                    SOCKET s = ::accept(listen_sock, nullptr, nullptr);
                    if (s != INVALID_SOCKET) {
                        u_long nonblock = 1;
                        ::ioctlsocket(s, FIONBIO, &nonblock);
                        client_sock = s;
                        peer_closed = false;
                        ++accept_count;
                    }
                }
                if (client_sock != INVALID_SOCKET) {
                    char buf[2048];
                    const int n = ::recv(client_sock, buf, sizeof(buf), 0);
                    if (n > 0) {
                        for (auto &f: framer.Feed(buf, static_cast<std::size_t>(n))) out.push_back(std::move(f));
                    } else if (n == 0) {
                        peer_closed = true;
                        CloseClient();
                    }
                }
                if (!out.empty()) return out;
                Sleep(10);
            }
            return out;
        }
    };

    void PumpUiUntil(unsigned wait_ms, const std::function<bool()> &done,
                     const std::function<std::string()> &context = {}) {
        const ULONGLONG deadline = GetTickCount64() + wait_ms;
        while (GetTickCount64() < deadline) {
            MSG msg{};
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            if (done()) return;
            Sleep(10);
        }
        const std::string detail = context ? context() : std::string{};
        Require(done(), "Timed out waiting for NetworkClient loopback state" +
                (detail.empty() ? "" : ": " + detail));
    }

    void CheckLoopbackOfficialReconnect() {
        const auto temp_dir = std::filesystem::temp_directory_path() / "tenhou-official-reconnect-test";
        std::error_code ec;
        std::filesystem::remove_all(temp_dir, ec);
        std::filesystem::create_directories(temp_dir);
        const auto config = temp_dir / "config.ini";
        const auto session_file = temp_dir / "official-session-10080-test.txt";
        WritePrivateProfileStringW(L"OfficialRecovery", session_file.filename().c_str(),
                                   L"928CC0A7-50692F1D", config.c_str());

        auto make_options = [&](unsigned port) {
            ConnectionOptions opt;
            opt.host = "127.0.0.1";
            opt.port = port;
            opt.identity = "ID00000000-00000000";
            opt.gender = 'M';
            opt.official = true;
            opt.origin_lobby = true;
            opt.interactive_games = true;
            opt.match_games = true;
            opt.recover_games = true;
            opt.auto_reconnect = false;
            opt.presentation = false;
            opt.config_file = config;
            opt.session_file = session_file;
            return opt;
        };

        auto read_saved_gpid = [&]() {
            return ReadIniString(config, "OfficialRecovery", PathToUtf8(session_file.filename()));
        };

        // Phase A: stale gpid -> <Z /> + <HELO gpid=stale> -> server <HELO> + <LN> -> immediately clears gpid & enters lobby -> GO saves live gpid.
        {
            NetworkClient client([](const LobbySnapshot &) {}, [](const std::string &) {});
            LoopbackServer server;
            client.Connect(make_options(server.port));

            std::vector<std::string> received;
            while (received.size() < 2) {
                auto batch = server.Pump(3000);
                Require(!batch.empty(), "Loopback server did not receive initial <Z /> and <HELO>");
                received.insert(received.end(), batch.begin(), batch.end());
            }
            Require(received[0] == "<Z />", "Official connect did not transmit initial <Z />");
            Require(received[1] == "<HELO name=\"ID00000000-00000000\" gpid=\"928CC0A7-50692F1D\" sx=\"M\" tid=\"w0\" />",
                    "Official stale-gpid HELO wire mismatch: " + received[1]);

            server.SendFrame("<HELO uname=\"HalfCool\" PF4=\"11,465,1674.24,-14.0,41,38,38,39,11,1322,295,194,476,229\" "
                             "PF3=\"11,145,1527.43,-2120.5,86,101,123,0,42,2359,640,440,708,519\" opt=\"3\"/>");
            server.SendFrame("<LN n=\"A1A1A1A\" j=\"112A\" g=\"112A\"/>");
            PumpUiUntil(3000, [&] {
                return client.snapshot().phase == LobbyPhase::Lobby && !client.snapshot().recovering;
            });
            Require(read_saved_gpid().empty(), "Stale gpid was not cleared from config.ini upon server HELO");

            // Drain PXR then change to L4409 and join type 17.
            (void) server.Pump(500);
            client.SubmitOriginLobby("change", "4409");
            auto ch = server.Pump(3000);
            Require(!ch.empty() && ch.back() == "<CHAT text=\"%2Flobby%204409\" />",
                    "Immediate lobby change after stale-gpid HELO failed");
            server.SendFrame("<CHAT lobby=\"4409\"/>");
            server.SendFrame("<LN n=\"BIK1C1C\" j=\"64C\" g=\"\"/>");
            PumpUiUntil(3000, [&] { return client.snapshot().lobby == "L4409"; });
            (void) server.Pump(300);
            client.SubmitOriginLobby("join", "17");
            auto jn = server.Pump(3000);
            Require(!jn.empty() && jn.back() == "<JOIN t=\"4409,17\" />", "Private room JOIN failed");
            server.SendFrame("<GO type=\"17\" lobby=\"4409\" gpid=\"E2B76CD8-4E2EA568\"/>");
            auto gok = server.Pump(3000);
            Require(!gok.empty() && gok.back() == "<GOK />", "GO did not emit <GOK />");
            PumpUiUntil(3000, [&] { return read_saved_gpid() == "E2B76CD8-4E2EA568"; });
            client.Disconnect();
        }

        // Phase B: fresh NetworkClient process loads saved live gpid -> <Z /> + <HELO gpid=live> -> GO -> GOK -> UN -> SAIKAI -> BYE -> REINIT.
        {
            NetworkClient client([](const LobbySnapshot &) {}, [](const std::string &) {});
            LoopbackServer server;
            client.Connect(make_options(server.port));

            std::vector<std::string> received;
            while (received.size() < 2) {
                auto batch = server.Pump(3000);
                Require(!batch.empty(), "Loopback server did not receive reconnect <Z /> and <HELO>");
                received.insert(received.end(), batch.begin(), batch.end());
            }
            Require(received[0] == "<Z />", "Official reconnect did not transmit initial <Z />");
            Require(received[1] == "<HELO name=\"ID00000000-00000000\" gpid=\"E2B76CD8-4E2EA568\" sx=\"M\" tid=\"w0\" />",
                    "Official reconnect HELO wire mismatch: " + received[1]);

            server.SendFrame("<GO type=\"17\" lobby=\"4409\" gpid=\"E2B76CD8-4E2EA568\"/>");
            auto gok = server.Pump(3000);
            Require(gok == std::vector<std::string>{"<GOK />"}, "Reconnect GO did not emit single <GOK />");

            server.SendFrame("<UN n0=\"%48%61%6C%66%43%6F%6F%6C\" n1=\"%4E%6F%4E%61%6D%65\" n2=\"\" n3=\"%4E%6F%4E%61%6D%65\" "
                             "dan=\"11,0,0,0\" rate=\"1527.43,1500.00,1500.00,1500.00\" sx=\"M,M,C,M\"/>");
            server.SendFrame("<SAIKAI ba=\"0,0\" oya=\"3\" sc=\"350,0,350,0,0,0,350,0\"/>");
            server.SendFrame("<BYE who=\"3\" />");
            PumpUiUntil(3000, [&] {
                return client.snapshot().phase == LobbyPhase::AwaitRecovery && client.snapshot().game.has_dealer &&
                       !client.snapshot().game.players[3].connected;
            });
            // Confirm client stayed silent after SAIKAI (no <REINIT /> or <NEXTREADY />).
            Require(server.Pump(150).empty(), "Client transmitted unexpected message while awaiting REINIT");

            server.SendFrame("<REINIT seed=\"0,0,0,0,5,110\" ten=\"350,350,0,350\" oya=\"3\" "
                             "hai=\"3,40,58,60,62,72,76,77,88,91,92,103,131\" m3=\"31520,31264\" "
                             "kawa0=\"112\" kawa1=\"128\" kawa3=\"1\"/>");
            PumpUiUntil(3000, [&] {
                return client.snapshot().phase == LobbyPhase::Game && !client.snapshot().recovering;
            });
            Require(client.snapshot().lobby == "L4409" &&
                    client.snapshot().game.players[3].nuki.size() == 2 &&
                    !client.snapshot().game.players[3].connected,
                    "Loopback REINIT state mismatch");
            client.Disconnect();
        }

        std::filesystem::remove_all(temp_dir, ec);
        std::cout << "PASS loopback NetworkClient official stale-gpid fallback + live-gpid SAIKAI/REINIT\n";
    }

    void CheckLoopbackOfficialOwariAndPracticeExit() {
        const auto temp_dir = std::filesystem::temp_directory_path() / "tenhou-official-owari-exit-test";
        std::error_code ec;
        std::filesystem::remove_all(temp_dir, ec);
        std::filesystem::create_directories(temp_dir);
        const auto config = temp_dir / "config.ini";
        const auto session_file = temp_dir / "official-session-10080-owari.txt";

        auto make_options = [&](unsigned port) {
            ConnectionOptions opt;
            opt.host = "127.0.0.1";
            opt.port = port;
            opt.identity = "ID00000000-00000000";
            opt.gender = 'M';
            opt.official = true;
            opt.origin_lobby = true;
            opt.interactive_games = true;
            opt.match_games = true;
            opt.recover_games = true;
            opt.auto_reconnect = false;
            opt.presentation = false;
            opt.config_file = config;
            opt.session_file = session_file;
            return opt;
        };

        auto read_saved_gpid = [&]() {
            return ReadIniString(config, "OfficialRecovery", PathToUtf8(session_file.filename()));
        };

        auto expect_z_helo = [](LoopbackServer &server, const char *label) {
            std::vector<std::string> received;
            while (received.size() < 2) {
                auto batch = server.Pump(3000);
                Require(!batch.empty(), std::string(label) + ": missing <Z /> + <HELO>");
                received.insert(received.end(), batch.begin(), batch.end());
            }
            Require(received[0] == "<Z />", std::string(label) + ": first frame was not <Z />");
            Require(received[1] == "<HELO name=\"ID00000000-00000000\" sx=\"M\" tid=\"w0\" />",
                    std::string(label) + ": unexpected HELO wire: " + received[1]);
        };

        NetworkClient client([](const LobbySnapshot &) {}, [](const std::string &) {});
        LoopbackServer server;
        client.Connect(make_options(server.port));
        expect_z_helo(server, "Initial official connect");
        Require(server.accept_count == 1, "Expected first TCP connection");

        auto state_desc = [&](const char *step) {
            return [&, step]() {
                return std::string(step) + " (phase=" + std::to_string(static_cast<int>(client.snapshot().phase)) +
                       " lobby=" + client.snapshot().lobby + " err=" + client.snapshot().error + ")";
            };
        };

        server.SendFrame("<HELO uname=\"TestPlayer\" PF4=\"11,465,1674.24,-14.0,41,38,38,39,11,1322,295,194,476,229\" "
                         "PF3=\"11,145,1527.43,-2120.5,86,101,123,0,42,2359,640,440,708,519\" opt=\"3\"/>");
        server.SendFrame("<LN n=\"A1A1A1A\" j=\"112A\" g=\"112A\"/>");
        PumpUiUntil(3000, [&] { return client.snapshot().phase == LobbyPhase::Lobby; }, state_desc("initial lobby"));
        (void) server.Pump(200);

        // 1) Private room match finishes with owari -> client closes game socket cleanly, stays in MatchComplete,
        //    and clicking END (SubmitLobby("return")) opens a fresh TCP connection with <Z /> + <HELO>.
        client.SubmitOriginLobby("change", "4409");
        auto ch = server.Pump(3000);
        Require(!ch.empty() && ch.back() == "<CHAT text=\"%2Flobby%204409\" />", "Private room change failed");
        server.SendFrame("<CHAT lobby=\"4409\"/>");
        server.SendFrame("<LN n=\"BIK1C1C\" j=\"64C\" g=\"\"/>");
        PumpUiUntil(3000, [&] { return client.snapshot().lobby == "L4409"; }, state_desc("enter L4409"));
        (void) server.Pump(200);

        client.SubmitOriginLobby("join", "17");
        auto jn = server.Pump(3000);
        Require(!jn.empty() && jn.back() == "<JOIN t=\"4409,17\" />", "Private room join failed");
        server.SendFrame("<GO type=\"17\" lobby=\"4409\" gpid=\"GP-OWARI-TEST\"/>");
        auto gok = server.Pump(3000);
        Require(!gok.empty() && gok.back() == "<GOK />", "Private room GO did not emit <GOK />");
        PumpUiUntil(3000, [&] { return read_saved_gpid() == "GP-OWARI-TEST"; }, state_desc("save gpid"));

        server.SendFrame("<UN n0=\"TestPlayer\" n1=\"COM1\" n2=\"\" n3=\"COM3\" dan=\"11,0,0,0\" rate=\"1500,1500,1500,1500\"/>");
        server.SendFrame("<TAIKYOKU oya=\"0\" log=\"owari-test\"/>");
        auto nr = server.Pump(3000);
        Require(!nr.empty() && nr.back() == "<NEXTREADY />", "Private room TAIKYOKU did not emit <NEXTREADY />");
        server.SendFrame("<INIT seed=\"0,0,0,1,1,56\" ten=\"350,350,0,350\" oya=\"0\" hai=\"0,4,8,12,16,20,24,28,32,36,40,44,48\"/>");
        PumpUiUntil(3000, [&] { return client.snapshot().phase == LobbyPhase::Game && client.snapshot().game.initialized; },
                    state_desc("private INIT"));

        server.SendFrame("<AGARI ba=\"0,0\" hai=\"0,4,8,12,16,20,24,28,32,36,40,44,48,52\" machi=\"52\" "
                         "ten=\"30,3900,0\" yaku=\"1,1\" doraHai=\"56\" who=\"0\" fromWho=\"1\" "
                         "sc=\"350,39,350,-39,0,0,350,0\" owari=\"389,29,311,-19,0,0,350,-10\"/>");
        PumpUiUntil(3000, [&] {
            return client.snapshot().phase == LobbyPhase::MatchComplete && client.snapshot().game.complete;
        }, state_desc("AGARI owari"));
        Require(read_saved_gpid().empty(), "Completed owari match did not clear saved OfficialRecovery gpid");
        Require(server.WaitPeerClosed(3000), "Official client did not close game TCP socket after owari");
        // Ensure closing the game socket left the client cleanly in MatchComplete (not Error).
        PumpUiUntil(200, [&] { return client.snapshot().phase == LobbyPhase::MatchComplete; }, state_desc("stay MatchComplete"));
        Require(client.snapshot().phase == LobbyPhase::MatchComplete && client.snapshot().error.empty(),
                "Closing owari socket disrupted MatchComplete state");

        client.SubmitLobby(client.snapshot().lobby_revision, "return");
        expect_z_helo(server, "END return-to-lobby fresh TCP reconnect");
        Require(server.accept_count == 2, "END did not open a fresh TCP connection to return to lobby");
        server.SendFrame("<HELO uname=\"TestPlayer\" PF4=\"11,465,1674.24,-14.0,41,38,38,39,11,1322,295,194,476,229\" "
                         "PF3=\"11,145,1527.43,-2120.5,86,101,123,0,42,2359,640,440,708,519\" opt=\"3\"/>");
        server.SendFrame("<LN n=\"A1A1A1A\" j=\"112A\" g=\"112A\"/>");
        PumpUiUntil(3000, [&] {
            return client.snapshot().phase == LobbyPhase::Lobby && !client.snapshot().recovering &&
                   client.snapshot().origin_lobby.has_value();
        }, state_desc("END returned lobby"));
        (void) server.Pump(200);

        // 2) Practice match mid-game EXIT -> same-socket <BYE /> + <HELO>, ignores stale in-flight game frames,
        //    returns to Lobby, and immediately re-enters the same practice match (type=1024).
        client.SubmitOriginLobby("join", "1024");
        auto pj = server.Pump(3000);
        Require(!pj.empty() && pj.back() == "<JOIN t=\"0,1024\" />", "Practice JOIN 1024 failed");
        server.SendFrame("<GO type=\"1024\" lobby=\"0\"/>");
        auto pgok = server.Pump(3000);
        Require(!pgok.empty() && pgok.back() == "<GOK />", "Practice GO did not emit <GOK />");
        server.SendFrame("<UN n0=\"TestPlayer\" n1=\"COM\" n2=\"COM\" n3=\"COM\" dan=\"11,0,0,0\" rate=\"1500,1500,1500,1500\"/>");
        server.SendFrame("<TAIKYOKU oya=\"0\"/>");
        auto pnr = server.Pump(3000);
        Require(!pnr.empty() && pnr.back() == "<NEXTREADY />", "Practice TAIKYOKU did not emit <NEXTREADY />");
        server.SendFrame("<INIT seed=\"0,0,0,1,1,52\" ten=\"250,250,250,250\" oya=\"0\" hai=\"0,4,8,12,16,20,24,28,32,36,40,44,48\"/>");
        PumpUiUntil(3000, [&] {
            return client.snapshot().phase == LobbyPhase::Game && client.snapshot().exit_allowed;
        }, state_desc("practice INIT"));

        client.ExitGame();
        std::vector<std::string> exit_frames;
        while (exit_frames.size() < 2) {
            auto batch = server.Pump(3000);
            Require(!batch.empty(), "Practice ExitGame did not transmit <BYE /> + <HELO>");
            exit_frames.insert(exit_frames.end(), batch.begin(), batch.end());
        }
        Require(exit_frames[0] == "<BYE />" &&
                exit_frames[1] == "<HELO name=\"ID00000000-00000000\" sx=\"M\" tid=\"w0\" />",
                "Practice ExitGame wire mismatch");
        Require(server.accept_count == 2, "Mid-game practice EXIT should reuse the active socket");

        // Send stale in-flight game frames during AwaitReturn before server <HELO> + <LN>.
        server.SendFrame("<U/>");
        server.SendFrame("<e55/>");
        server.SendFrame("<HELO uname=\"TestPlayer\" PF4=\"11,465,1674.24,-14.0,41,38,38,39,11,1322,295,194,476,229\" "
                         "PF3=\"11,145,1527.43,-2120.5,86,101,123,0,42,2359,640,440,708,519\" opt=\"3\"/>");
        server.SendFrame("<LN n=\"A1A1A1A\" j=\"112A\" g=\"112A\"/>");
        PumpUiUntil(3000, [&] { return client.snapshot().phase == LobbyPhase::Lobby; }, state_desc("practice EXIT returned lobby"));
        (void) server.Pump(200);

        // Immediately re-enter the exact same practice type 1024.
        client.SubmitOriginLobby("join", "1024");
        auto pj2 = server.Pump(3000);
        Require(!pj2.empty() && pj2.back() == "<JOIN t=\"0,1024\" />",
                "Re-entering practice 1024 immediately after EXIT failed");

        client.Disconnect();
        std::filesystem::remove_all(temp_dir, ec);
        std::cout << "PASS loopback NetworkClient official owari END reconnect + practice EXIT stale-frame & rejoin\n";
    }

    void CheckOriginAutoSort() {
        // Mirrors origin tenhou.exe.c:90593-90661 (FUN_00488220 "od eat 0") and ORIGIN.md observation #3:
        // Without manzu, suit order shifts to (pin, sou, man); when 9m arrives, 9m sorts after sou and before honors.
        std::array<int, 3> order{0, 1, 2};
        std::vector<int> hand{40, 44, 76, 80, 108, 124}; // 2p, 3p, 2s, 3s, East, Red (no manzu)
        UpdateOriginSuitOrder(order, hand);
        Require((order == std::array<int, 3>{2, 0, 1}), "Absent manzu did not move man suit behind pin and sou");
        SortOriginHand(hand, order);
        Require((hand == std::vector<int>{40, 44, 76, 80, 108, 124}), "Unexpected pin/sou/honor order");

        // Keep 9m (tile 32): all three suits now present, so suit order stays (pin, sou, man).
        hand.push_back(32);
        UpdateOriginSuitOrder(order, hand);
        Require((order == std::array<int, 3>{2, 0, 1}), "All-three-suit hand should preserve existing suit order");
        SortOriginHand(hand, order);
        Require((hand == std::vector<int>{40, 44, 76, 80, 32, 108, 124}),
                "9m did not sort after sou and before honors");

        // Discard all pinzu while keeping sou and 9m -> pin moves behind sou and man: (sou, man, pin).
        hand = {76, 80, 32, 108, 124};
        UpdateOriginSuitOrder(order, hand);
        Require((order == std::array<int, 3>{1, 2, 0}), "Absent pinzu did not move behind sou and man");
        SortOriginHand(hand, order);
        Require((hand == std::vector<int>{76, 80, 32, 108, 124}), "Expected sou, 9m, honors");
        std::cout << "PASS origin auto-sort suit order (od eat 0)\n";
    }
}

int main(int argc, char **argv) {
    try {
        Require(argc == 2, "Usage: tenhou_official_capture_check <origin-capture-dir>");
        const std::filesystem::path dir(argv[1]);
        const auto u8file = [&](const wchar_t *name) { return dir / name; };

        ReplayRawCapture(u8file(L"一般 三食断赤速 东风战.jsonl"), true, true, false, false);
        ReplayRawCapture(u8file(L"上级 三食断赤 东南战.jsonl"), true, true, false, false);
        ReplayRawCapture(u8file(L"上级 食断赤 东南战.jsonl"), false, true, false, false);
        ReplayRawCapture(u8file(L"试打 雀庄战 三若东祝 5.jsonl"), false, false, false, true);
        ReplayRawCapture(u8file(L"试打 雀庄战 若东祝 0（没终局）.jsonl"), false, false, true, false);
        ReplayFullReconnectCapture(u8file(L"个室 三食断赤 东风战（完整断线重连再开）.jsonl"));
        CheckPrivateRoomRejoinAndSaikaiReconnect();
        CheckLoopbackOfficialReconnect();
        CheckLoopbackOfficialOwariAndPracticeExit();
        CheckOriginAutoSort();
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
