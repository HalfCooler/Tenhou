#include "settlement_view.hpp"
#include "replay_effects.hpp"
#include "lobby_view.hpp"
#include "tile_shadow.hpp"
#include "wall_scene.hpp"
#include <commctrl.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <set>

using namespace tenhou;
namespace tenhou {
    void CheckCertificationPaint(HWND popup, const std::filesystem::path &resources);
    void RunTextTableChecks(HWND parent, const std::filesystem::path &resources);
    // Exercise the production receive/display boundary with packet bursts and a stalled UI timer.
    struct LobbyPresentationCheck {
        static void OfficialCalls(HWND parent, const std::filesystem::path &resources) {
            LobbyView view(parent, resources / "presentation-test.ini", [](const std::string &) {});
            view.ConfigureArtwork(resources, 1440);
            view.sequencer_.set_sound_enabled(false);
            view.sequencer_.set_animation_enabled(false);
            view.Show(true);
            LobbySession session;
            session.official = session.interactive_games = session.recover_games = true;
            session.preferred_monthly_profile = 9;
            session.Connected("ID00000000-00000000", 'M');
            const auto receive = [&](const char *wire) {
                const auto msg = ParseNetworkMessage(wire, session.official);
                const auto replies = session.Receive(msg);
                const auto &s = session.snapshot;
                const_cast<LobbySnapshot &>(view.client_->snapshot()) = s;
                view.OnGameEvent(msg, s);
                view.Update(s);
                return replies;
            };
            const auto require = [](bool ok, const char *message) { if (!ok) throw std::runtime_error(message); };
            require(receive("<HELO uname=\"Test&Partner%20Four\" "
                            "PF4=\"11,505,1674.45,-385.5,40,35,30,25,9,1000,212,147,348,206\" "
                            "PF3=\"12,125,1513.80,1234.9,85,75,40,0,15,1550,450,250,700,300\" "
                            "expire=\"20991231\" expiredays=\"90\" ratingscale=\"PF4=1.000000&PF3=1.000000\" "
                            "PF602=\"7,350,2,2,1,0,1,43,12,11,14,8\" PF02C=\"800,1672.46,4\" "
                            "rr=\"PF3=0,0&PF4=15000,0\" opt=\"3\" />") ==
                std::vector<std::string>{"<PXR V=\"9\" />"}, "Official lobby did not request the saved monthly category");
            receive("<LN n=\"A1A1A1A\" j=\"112A\" g=\"112A\" />");
            receive("<RANKING />");
            require(session.snapshot.phase == LobbyPhase::Lobby && session.snapshot.origin_lobby &&
                session.snapshot.identity == "Test&Partner Four" &&
                session.snapshot.origin_lobby->profile.membership_days == 90,
                "Official registered login did not reach the lobby with its profile");
            const auto &profile = session.snapshot.origin_lobby->profile;
            const auto &four = profile.lifetime[0];
            const auto &three = profile.lifetime[1];
            require(profile.four.code == 11 && profile.four.rating == 1674 && four.points == 505 &&
                four.next_points == 800 && four.games == 130 && four.hands == 1000 &&
                four.place_per_mille == std::array<int, 4>{307, 269, 230, 192} && four.bust_per_mille == 69 &&
                four.average_score_hundredths == -296 && four.average_place_hundredths == 230 &&
                four.win_per_mille == 212 && four.deal_in_per_mille == 147 &&
                four.call_per_mille == 348 && four.reach_per_mille == 206,
                "Official PF4 did not decode rank, game results and hand rates");
            require(profile.three.code == 12 && profile.three.rating == 1513 && three.points == 125 &&
                three.next_points == 1200 && three.games == 200 && three.hands == 1550 &&
                three.place_per_mille == std::array<int, 4>{425, 375, 200, 0} && three.bust_per_mille == 75 &&
                three.average_score_hundredths == 617 && three.average_place_hundredths == 177 &&
                three.win_per_mille == 290 && three.deal_in_per_mille == 161 &&
                three.call_per_mille == 451 && three.reach_per_mille == 193 &&
                profile.monthly.empty(),
                "Official PF3 did not decode rank, game results and hand rates");
            const auto &amber = profile.other_lifetime.at(602);
            require(amber.games == 5 && amber.rating == 1672 && amber.average_g_hundredths == 728000 &&
                amber.average_score_hundredths == 7000,
                "Amber lifetime results did not use their own placement, rating and chip values");
            const auto private_room = ReadPrivateLobbyStatistics("-450,1,0,0,1,0,20,4,2,6,8");
            require(private_room.games == 2 && private_room.average_score_hundredths == -22500 &&
                private_room.win_per_mille == 200 && private_room.reach_per_mille == 400,
                "Private-room configuration did not decode its own stats");
            require(session.OfficialProfilePoll() == "<PXR v=\"9\" />",
                "General four-player south selection did not set the polling type");
            // The production login capture contains an empty sixth field.
            receive("<RANKING v2=\"1,0,0,0,66.8,,0,4726,0,5027,0,0,0,0,29991\" />");
            receive("<RANKING rankingcs=\"fixture\" />");
            const auto &south = session.snapshot.origin_lobby->profile.monthly.at(9);
            require(south.games == 1 && south.place_per_mille[0] == 1000 &&
                south.average_score_hundredths == 6680 && south.average_place_hundredths == 100,
                "Official RANKING v2 did not populate the saved category or was overwritten by a contest ranking");
            require(session.SubmitOriginLobby("monthly-profile", L"9").empty(),
                "Selecting the current monthly category sent a redundant request");
            const auto phoenix = RankedMonthlyProfile(25, RankedTier::Phoenix);
            require(phoenix == 185 && session.SubmitOriginLobby("monthly-profile", L"185") ==
                    std::vector<std::string>{"<PXR V=\"185\" />"} &&
                session.snapshot.origin_lobby->profile.monthly.empty(),
                "Changing tier did not discard the earlier monthly result");
            receive("<RANK v2=\"4,6,0,0,-6.5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\" />");
            receive("<RANK v2=\"9,9,9\" />");
            const auto &three_south = session.snapshot.origin_lobby->profile.monthly.at(phoenix);
            require(three_south.games == 10 && three_south.average_score_hundredths == -65 &&
                three_south.average_place_hundredths == 160 &&
                session.SubmitOriginLobby("monthly-profile", L"65").empty(),
                "21-field or malformed monthly result handling differs");
            require(receive("<CHAT lobby=\"8141\" />") == std::vector<std::string>{"<PXR V=\"-1\" />"} &&
                session.OfficialProfilePoll() == "<PXR v=\"-1\" />" &&
                receive("<CHAT lobby=\"0000\" />") == std::vector<std::string>{"<PXR V=\"185\" />"},
                "Monthly subscription did not follow lobby changes");
            require(session.SubmitOriginLobby("join", L"2065").empty() &&
                session.snapshot.phase == LobbyPhase::Lobby, "Toutenkou changed official lobby state");
            require(receive("<GO type=\"9\" lobby=\"0\" />") == std::vector<std::string>{"<GOK />"},
                "Official game did not send GOK");
            receive("<UN n0=\"A\" n1=\"B\" n2=\"C\" n3=\"D\" />");
            require(receive("<TAIKYOKU oya=\"3\" />") == std::vector<std::string>{"<NEXTREADY />"},
                "Official match never acknowledged the first hand");
            require(view.match_intro_->Active(), "Official first-hand preparation was skipped");
            view.match_intro_->Finish();
            receive("<INIT seed=\"0,0,0,1,1,52\" ten=\"250,250,250,250\" oya=\"3\" "
                    "hai=\"4,8,12,16,17,18,20,24,32,36,40,44,48\" />");
            view.sequencer_.opening().Cancel();
            receive("<W />");
            receive("<G19 t=\"7\" />");
            view.ClearPresentationQueue();
            view.Update(session.snapshot);
            const auto &actions = session.snapshot.actions;
            const auto interaction = view.Interaction(session.snapshot);
            require(view.sequencer_.selection().group == "call" && interaction.selecting && interaction.playable.size() == 7,
                "Chi and pon candidates were not selectable together without a button");
            require(std::any_of(actions.choices.begin(), actions.choices.end(), [](const OnlineAction &a) {
                return a.kind == "open_kan";
            }), "Direct hand selection removed the open kan button action");
            view.sequencer_.selection().Pick(actions, 8);
            require(view.sequencer_.selection().Selectable(actions) == std::vector<int>({8, 12}) &&
                view.sequencer_.selection().selected == std::vector<int>{8}, "First picked tile did not constrain candidates");
            const auto chi = view.sequencer_.selection().Pick(actions, 12);
            require(chi < actions.choices.size() && actions.choices[chi].kind == "chi",
                "Two compatible hand tiles did not confirm chi");
            view.sequencer_.selection().Open(actions, "call");
            view.sequencer_.selection().Pick(actions, 17);
            const auto pon = view.sequencer_.selection().Pick(actions, 18);
            require(pon < actions.choices.size() && actions.choices[pon].kind == "pon",
                "Two matching hand tiles did not confirm pon");
            session.SubmitAction(actions.revision, chi);
            receive("<N who=\"0\" m=\"8583\" />"); // 3m-4m-[5m] from kamicha.
            view.ClearPresentationQueue();
            view.Update(session.snapshot);
            const auto discards = view.Interaction(session.snapshot);
            require(discards.restrict_discards && std::find(discards.playable.begin(), discards.playable.end(), 20) !=
                discards.playable.end() && std::none_of(discards.playable.begin(), discards.playable.end(), [](int tile) {
                    return tile == 4 || tile / 4 == 4;
                }), "Called tile and suji-kuikae tiles must be unavailable after chi");
            auto &recovering = const_cast<LobbySnapshot &>(view.client_->snapshot());
            recovering.recovering = recovering.silent_recovery = true;
            view.Update(recovering);
            require(!view.match_intro_->Active() && !view.LocalTableInput(),
                "In-process recovery exposed a preparation popup or live input");
            recovering.silent_recovery = false;
            view.Update(recovering);
            require(view.match_intro_->Resuming(), "Recovery at login did not show the resume preparation");
            view.Disconnect();
        }

        static void OfficialPracticeExitStopsOpening(HWND parent, const std::filesystem::path &resources) {
            LobbyView view(parent, resources / "presentation-test.ini", [](const std::string &) {});
            view.EnableServiceLobby([] {}, [] {});
            view.ConfigureArtwork(resources, 1440);
            view.sequencer_.set_sound_enabled(false);
            view.sequencer_.set_animation_enabled(true);
            view.Show(true);
            LobbySession session;
            session.official = session.origin_lobby = session.interactive_games = session.match_games = true;
            session.Connected("ID00000000-00000000", 'M');
            const auto receive = [&](const char *wire) {
                const auto msg = ParseNetworkMessage(wire, true);
                (void) session.Receive(msg);
                const_cast<LobbySnapshot &>(view.client_->snapshot()) = session.snapshot;
                view.OnGameEvent(msg, session.snapshot);
                view.Update(session.snapshot);
            };
            const auto require = [](bool ok, const char *message) { if (!ok) throw std::runtime_error(message); };
            receive("<HELO uname=\"桐山\" PF4=\"9,0,1500,0,0,0,0,0,0,0,0,0,0,0\" "
                    "PF3=\"11,0,1500,0,0,0,0,0,0,0,0,0,0,0\" opt=\"3\" />");
            receive("<LN n=\"A1A1A1A\" j=\"112A\" g=\"112A\" />");
            const HWND service_viewport = FindWindowExW(view.window_, nullptr, L"TENHOU_REBUILD_OFFLINE_VIEW", nullptr);
            require(service_viewport && IsWindowVisible(service_viewport),
                    "Official service lobby viewport was not shown in Lobby phase");

            const HWND popup = GetDlgItem(service_viewport, 0xbc0);
            require(popup && !IsWindowVisible(popup), "Ordinary HELO opened certification");
            const HWND panel = FindWindowExW(service_viewport, nullptr, L"TENHOU_REBUILD_OFFLINE_LOBBY", nullptr);
            LOGFONTW font{};
            GetObjectW(reinterpret_cast<HFONT>(SendDlgItemMessageW(panel, 0xfa4, WM_GETFONT, 0, 0)), sizeof(font), &font);
            require(std::wstring(font.lfFaceName) == L"MS UI Gothic" && font.lfHeight == -12, "Lobby font changed");

            receive("<GO type=\"1024\" lobby=\"0\" />");
            receive("<UN n0=\"桐山\" n1=\"COM\" n2=\"COM\" n3=\"COM\" />");
            receive("<TAIKYOKU oya=\"0\" />");
            if (view.match_intro_->Active()) view.match_intro_->Finish();
            receive("<INIT seed=\"0,0,0,1,1,52\" ten=\"250,250,250,250\" oya=\"0\" "
                    "hai=\"0,4,8,12,16,20,24,28,32,36,40,44,48\" />");
            require(view.sequencer_.opening().Active() && !IsWindowVisible(service_viewport),
                    "Practice opening wall animation did not start cleanly");

            // Pressing EXIT during 码牌 enters AwaitReturn; opening animation must stop immediately,
            // and the service lobby must stay hidden until <HELO> + <LN> finishes returning to Lobby.
            const auto exit_replies = session.ExitGame();
            require(exit_replies.size() == 2 && exit_replies[0] == "<BYE />" &&
                    exit_replies[1].rfind("<HELO ", 0) == 0,
                    "Official practice exit did not emit BYE + HELO");
            const_cast<LobbySnapshot &>(view.client_->snapshot()) = session.snapshot;
            view.Update(session.snapshot);
            view.OpeningTick(AnimationNow() + 100);
            require(!view.sequencer_.opening().Active() && view.sequencer_.deferred_sounds().empty(),
                    "Pressing EXIT during wall setup did not stop opening animation/audio cues");
            require(!IsWindowVisible(service_viewport),
                    "Service lobby became clickable before AwaitReturn completed");

            // Stale in-flight game frames during AwaitReturn must be ignored, then HELO + LN restores lobby.
            receive("<U />");
            receive("<e55 />");
            require(session.snapshot.phase == LobbyPhase::AwaitReturn && !IsWindowVisible(service_viewport),
                    "Stale game frames during AwaitReturn corrupted return transition");
            receive("<HELO uname=\"桐山\" nintei=\"認定段位/級位 : 初段%0D%0A%0D%0A桐山%20殿%0A%0A"
                    "貴殿は天鳳において卓越した技能を遺憾なく発揮され優秀な%0A成績をおさめられました。"
                    "今後もさらなる雀力向上に精進されますようここに段位/級位を認定し栄誉を称えます。"
                    "%0A%0A2026年11月24日%0A天鳳段位認定協会\" PF4=\"10,0,1500,0,0,0,0,0,0,0,0,0,0,0\" "
                    "PF3=\"11,0,1500,0,0,0,0,0,0,0,0,0,0,0\" opt=\"3\" />");
            receive("<LN n=\"A1A1A1A\" j=\"112A\" g=\"112A\" />");
            require(session.snapshot.phase == LobbyPhase::Lobby && IsWindowVisible(service_viewport),
                    "Official lobby did not reopen after EXIT return completed");
            const auto &notice = session.snapshot.certification;
            require(notice.find(L"认定段级位：初段\n\n桐山阁下：\n\n您在天凤") == 0 &&
                    notice.find(L"精进牌艺。\n\n2026年11月24日\n天凤段位认定协会") != notice.npos,
                    "The certificate lost its translation or server-supplied fields");
            RECT rect{};
            GetClientRect(popup, &rect);
            require(IsWindowVisible(popup) && !IsWindowEnabled(panel) && rect.right == 812 && rect.bottom == 658,
                    "Award did not open above the lobby at original HD size");
            const auto wait_for = [&](HWND child, bool visible) {
                const auto until = GetTickCount64() + 4000;
                while ((IsWindowVisible(child) != FALSE) != visible && GetTickCount64() < until) {
                    MSG msg{};
                    if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) DispatchMessageW(&msg);
                    Sleep(10);
                }
                require((IsWindowVisible(child) != FALSE) == visible, "Certificate animation did not complete");
            };
            const HWND ok = GetDlgItem(popup, IDOK);
            wait_for(ok, true);
            CheckCertificationPaint(popup, resources);
            SendMessageW(ok, BM_CLICK, 0, 0);
            wait_for(popup, false);
            view.Update(session.snapshot);
            require(!IsWindowVisible(popup) && IsWindowEnabled(panel), "Acknowledged certificate reopened");
            view.Disconnect();
        }

        static void RecoveredContestReturn(HWND parent, const std::filesystem::path &resources) {
            LobbyView view(parent, resources / "presentation-test.ini", [](const std::string &) {});
            view.EnableServiceLobby([] {}, [] {});
            view.ConfigureArtwork(resources, 1440); view.sequencer_.set_sound_enabled(false); view.Show(true);
            LobbySession session;
            session.require_native = session.origin_lobby = session.interactive_games = session.match_games = true;
            session.Connected("NoName", 'M');
            auto receive = [&](const char *frame) {
                const auto message = ParseNetworkMessage(frame);
                session.Receive(message);
                const auto &snapshot = session.snapshot;
                const_cast<LobbySnapshot &>(view.client_->snapshot()) = snapshot;
                view.OnGameEvent(message, snapshot);
                view.Update(snapshot);
            };
            receive("<HELO uname=\"TENHOU-PLATFORM\" version=\"1\" caps=\"origin-lobby\" />");
            receive("<CHAT lobby=\"C00112233\" epoch=\"1\" seated=\"1\" />");
            receive("<RESTORE />");
            receive("<UN n0=\"A\" n1=\"B\" n2=\"C\" n3=\"D\" sx=\"MMMM\" />");
            receive("<GO type=\"9\" lobby=\"C00112233\" reservation=\"9\" table=\"contest-check\" />");
            receive("<INIT seed=\"7,0,0,1,1,52\" ten=\"250,250,250,250\" oya=\"3\" hai=\"0,4,8,12,16,20,24,28,32,36,40,44,48\" />");
            receive("<RYUUKYOKU sc=\"250,0,250,0,250,0,250,0\" owari=\"250,20,250,10,250,-10,250,-20\" />");
            receive("<PROF />");
            receive("<RESTORED />");

            const HWND settlement = GetDlgItem(view.window_, 5400);
            if (view.sequencer_.displaying() || !view.sequencer_.show_results() || !IsWindowVisible(settlement))
                throw std::runtime_error("Recovered contest result did not use the live snapshot");
            session.SubmitLobby(session.snapshot.lobby_revision, "return");
            receive("<CHAT lobby=\"C00112233\" epoch=\"1\" seated=\"0\" />");
            if (session.snapshot.phase != LobbyPhase::Lobby || session.snapshot.last_settlement.receipts.empty() ||
                view.sequencer_.show_results() || IsWindowVisible(settlement))
                throw std::runtime_error("The recovered contest result stayed visible after return");
            view.Disconnect();
        }

        static void ProtocolOpening(HWND parent, const std::filesystem::path &resources, const std::string &rule) {
            LobbyView view(parent, resources / "presentation-test.ini", [](const std::string &) {});
            view.ConfigureArtwork(resources, 1440); view.sequencer_.set_sound_enabled(false); view.Show(true);
            LobbySession session; session.interactive_games = true;
            session.snapshot.native_service = true; session.snapshot.phase = LobbyPhase::Lobby;
            auto receive = [&](const NetworkMessage &message) {
                session.Receive(message);
                const auto &snapshot = session.snapshot;
                const_cast<LobbySnapshot &>(view.client_->snapshot()) = snapshot;
                view.OnGameEvent(message, snapshot); view.Update(snapshot);
            };
            auto require = [](bool ok, const char *message) { if (!ok) throw std::runtime_error(message); };
            receive({"UN", {{"dan", "11,0,0,0"}}});
            receive({"VGO", {{"rule", rule}, {"table", "opening-check"}}});
            const bool sichuan = rule == "sichuan-bloodbattle";
            std::map<std::string, std::string> fields{
                {"phase", sichuan ? "exchange" : "turn"}, {"seat", "0"}, {"dealer", "0"}, {"turn", "0"},
                {"hand_number", "1"}, {"total_hands", "8"}, {"seq", "1"}, {"event_seq", "1"},
                {"events", "1:start:0:-1:-1"}, {"wall_total", sichuan ? "108" : "144"},
                {"wall", sichuan ? "56" : "92"}, {"wall_front", "52"}, {"window", "1"},
                {"scores", "50000,50000,50000,50000"}, {"hand0", "0,4,8,12,16,20,24,28,32,36,40,44,48"},
                {"hidden0", "13"}, {"hidden1", "13"}, {"hidden2", "13"}, {"hidden3", "13"},
                {"actions", sichuan ? "exchange:0,4,8;exchange:4,8,12" : "discard:0;discard:4"}};
            const auto button = [&](int id) -> std::optional<TableActionButton> {
                for (const auto &item: view.Buttons())
                    if (item.id == id) return item;
                return std::nullopt;
            };
            for (int hand = 1; hand <= 2; ++hand) {
                fields["hand_number"] = std::to_string(hand); fields["seq"] = fields["event_seq"] = std::to_string(hand);
                fields["events"] = std::to_string(hand) + ":start:0:-1:-1";
                receive({"PREPARE", {}}); receive({"VSTATE", fields});
                require(view.Snapshot().game.players[0].rank == 11 && view.Snapshot().game.players[1].rank == 0,
                    "Variant snapshot lost the relative seat ranks");
                require(view.sequencer_.opening().Active() && view.sequencer_.variant_hand() == hand, "Protocol opening did not install the new hand");
                require(view.match_intro_->Active() == (hand == 1), "First match introduction was skipped or repeated between hands");
                require(!button(5173), "Exchange confirmation appeared before dealing/selection");
                if (hand == 1) {
                    const auto steps = view.sequencer_.opening().steps();
                    receive({"PAUSED", {{"show", "1"}, {"offline", "2"}}});
                    require(!view.Snapshot().game.players[1].connected && view.Snapshot().game.players[2].connected,
                        "Paused match did not mark the disconnected seat");
                    receive({"PAUSED", {{"show", "0"}, {"offline", "0"}}});
                    require(view.Snapshot().game.players[1].connected, "Resumed match retained an offline seat");
                    view.OpeningTick(AnimationNow() + 100);
                    require(view.sequencer_.opening().steps() == steps && view.match_intro_->Active(),
                        "Deal started before the match introduction was confirmed");
                }
                receive({"READYACK", {{"table", "opening-check"}, {"kind", "STARTREADY"}}});
                receive({"START", {}});
                if (hand == 1) {
                    require(view.match_intro_->Active(), "Server START flashed away the local match introduction");
                    view.match_intro_->Finish();
                }
                require(session.snapshot.actions.choices.size() == 2 && !view.sequencer_.start_barrier(),
                    "Ready acknowledgement erased the initial/next-hand actions");
                if (sichuan) require(view.sequencer_.selection().Active() && view.sequencer_.selection().indices.size() == 2,
                    "Ready acknowledgement invalidated the exchange selection");
                const auto now = AnimationNow();
                for (int tick = 1; tick <= 400 && view.sequencer_.opening().Active(); ++tick)
                    view.OpeningTick(now + tick * kTableTickMs);
                require(!view.sequencer_.opening().Active() && !button(5173),
                    "Exchange confirmation appeared without three selected tiles");
                if (sichuan) {
                    for (int tile: {0, 4, 8}) view.sequencer_.selection().Pick(view.Snapshot().actions, tile);
                    view.Arrange();
                    require(button(5173) && button(5173)->enabled,
                        "A legal three-tile exchange has no confirmation button");
                }
            }
            if (sichuan) {
                const auto revision = view.Snapshot().actions.revision;
                view.sequencer_.pressed_tile() = 8; view.sequencer_.pressed_revision() = revision;
                fields["seq"] = fields["event_seq"] = "3";
                fields["events"] = "2:start:0:-1:-1;3:exchange:1:-1:-1";
                receive({"VSTATE", fields});
                require(view.Snapshot().actions.revision == revision && view.sequencer_.selection().selected == std::vector<int>({0,4,8}) &&
                    view.sequencer_.pressed_tile() == 8 && view.sequencer_.selection().Confirm(view.Snapshot().actions) == 0,
                    "Peer exchange interrupted the local selection or mouse press");
                fields["phase"] = "missing"; fields["actions"] = "dingque:0;dingque:1;dingque:2"; fields["window"] = "2";
                fields["seq"] = fields["event_seq"] = "4";
                fields["events"] += ";4:exchange:2:-1:-1";
                receive({"VSTATE", fields});
                require(view.Snapshot().actions.revision != revision && !view.sequencer_.selection().Active(),
                    "New preparation phase retained an old exchange request");
                const auto suit = button(5170);
                require(suit && suit->enabled, "Missing-suit action has no enabled canvas button");
                const auto &rect = suit->viewport_rect;
                const auto point = MAKELPARAM(rect.left + 1, rect.top + 1);
                SendMessageW(view.window_, WM_LBUTTONDOWN, MK_LBUTTON, point);
                require(view.sequencer_.pressed_action() == 5170 && GetCapture() == view.window_,
                    "Canvas button did not capture the press");
                const auto missing_revision = view.sequencer_.button_revision();
                fields["seq"] = fields["event_seq"] = "5";
                fields["events"] += ";5:dingque:3:-1:-1";
                receive({"VSTATE", fields});
                require(view.Snapshot().actions.revision == missing_revision && view.sequencer_.button_revision() == missing_revision &&
                    view.sequencer_.pressed_action() == 5170 && button(5170)->enabled,
                    "Peer dingque interrupted a pressed suit button");
                fields["window"] = "3";
                receive({"VSTATE", fields});
                require(view.Snapshot().actions.revision != missing_revision && view.sequencer_.pressed_action() == -1 &&
                    GetCapture() != view.window_, "Changed server window retained a stale button capture");
                SendMessageW(view.window_, WM_LBUTTONUP, 0, point);
                require(view.Snapshot().actions.pending.empty(), "Stale release submitted a current action");

                const auto current_suit = button(5170);
                require(current_suit && current_suit->enabled, "New preparation window lost its suit action");
                const auto &current_rect = current_suit->viewport_rect;
                SendMessageW(view.window_, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(current_rect.left + 1, current_rect.top + 1));
                SendMessageW(view.window_, WM_CANCELMODE, 0, 0);
                SetFocus(view.window_);
                require(view.HandleKey(VK_SPACE) && view.keyboard_button_ == 5170,
                        "Space did not arm the focused canvas button");
                auto &live = const_cast<LobbySnapshot &>(view.client_->snapshot());
                // Drive the full command path with no socket payload in this fixture.
                for (auto &choice: live.actions.choices) choice.messages.clear();
                SendMessageW(view.window_, WM_KEYUP, VK_SPACE, 0);
                require(live.actions.pending == "dingque" && view.keyboard_button_ == -1,
                        "Space release did not submit the focused suit action once");
            }
            view.Disconnect();
        }
        static void Run(HWND parent, const std::filesystem::path &resources) {
            LobbyView view(parent, resources / "presentation-test.ini", [](const std::string &) {});
            view.ConfigureArtwork(resources, 1440); view.sequencer_.set_sound_enabled(false); view.Show(true);
            LobbySnapshot snapshot; snapshot.variant_active = true; snapshot.phase = LobbyPhase::Game;
            snapshot.actions.enabled = true;
            auto &state = snapshot.variant; state.initialized = true; state.rule = "sichuan-bloodbattle";
            state.local = state.dealer = state.turn = 0; state.hand_number = 1; state.seq = state.event_seq = 1;
            state.phase = VariantPhase::Exchange; state.wall_front = 52; state.wall = 56;
            state.events = {{1, "start", 0, -1, -1}};
            for (auto &player: state.players) player.hidden = 13;
            state.players[0].hand = {0,4,8,12,16,20,24,28,32,36,40,44,48};
            auto receive = [&](const std::string &tag) {
                snapshot.game = VariantTableState(state, {});
                const_cast<LobbySnapshot &>(view.client_->snapshot()) = snapshot;
                view.OnGameEvent({tag, {}}, snapshot); view.Update(snapshot);
            };
            auto require = [](bool ok, const char *message) { if (!ok) throw std::runtime_error(message); };
            receive("VGO"); receive("PREPARE"); receive("VSTATE");
            require(view.sequencer_.opening().Active() && view.sequencer_.start_barrier() && view.sequencer_.displayed().game.events == 1,
                "Variant initial snapshot deadlocked behind PREPARE");
            require(view.match_intro_->Active(), "Variant first hand skipped the match introduction");
            view.match_intro_->Finish();
            receive("START"); require(!view.sequencer_.start_barrier(), "Variant START did not release opening");
            view.sequencer_.opening().Cancel();
            state.phase = VariantPhase::Turn; state.turn = 1; state.wall_front = 53; state.wall = 55;
            state.players[1].hidden = 14; state.seq = state.event_seq = 2;
            state.events.push_back({2, "draw", 1, -1, -1}); receive("VSTATE");
            require(view.sequencer_.displayed().game.events == 2 && view.table_.Animating(), "Variant draw did not animate");
            state.phase = VariantPhase::Claims; state.last_actor = 1; state.last_tile = 52;
            state.players[1].hidden = 13; state.players[1].river = {52}; state.seq = state.event_seq = 3;
            state.events.push_back({3, "discard", 1, 52, -1}); receive("VSTATE");
            require(view.sequencer_.displayed().game.events == 2 && view.sequencer_.queue().size() == 1,
                "Packet burst replaced the visible opponent draw");
            view.table_.CancelAnimation();
            for (int i = 0; i < 41; ++i) view.PresentationTick();
            require(view.sequencer_.displayed().game.events == 2, "Opponent draw lost its think wait");
            view.PresentationTick(); require(view.sequencer_.displayed().game.events == 3 && view.table_.Animating(),
                "Queued discard never animated after draw wait");
            snapshot.table_paused = true; receive("PAUSED");
            const auto wait = view.sequencer_.pause_ticks(); view.PresentationTick();
            require(view.sequencer_.pause_ticks() == wait, "Paused table advanced its event queue");
            snapshot.table_paused = false; snapshot.recovering = true; receive("VSTATE");
            require(view.sequencer_.queue().empty() && !view.table_.Animating() && !view.sequencer_.captions().Active(),
                "Recovery replayed queued motion or declarations");
            snapshot.recovering = false; receive("RESTORED");
            ++state.hand_number; ++state.seq; ++state.event_seq; state.phase = VariantPhase::Exchange;
            state.events.push_back({state.event_seq, "start", 0, -1, -1});
            receive("PREPARE"); receive("VSTATE");
            require(view.sequencer_.displayed().variant.hand_number == 2 && view.sequencer_.opening().Active(),
                "Next-hand PREPARE deadlocked behind the previous displayed hand");
            receive("START"); view.sequencer_.opening().Cancel();
            state.rule = "guobiao"; state.wall_total = 144; state.wall_front = 53; state.wall_tail = 2; state.wall = 89;
            state.phase = VariantPhase::Turn; ++state.hand_number; ++state.seq; ++state.event_seq;
            state.players[0].flowers = {136,140}; state.events.push_back({state.event_seq, "start", 0, -1, -1});
            receive("PREPARE"); receive("VSTATE"); receive("START");
            require(view.sequencer_.opening_flowers()[0], "Fresh guobiao deal lost its automatic flower caption");
            const auto now = AnimationNow();
            for (int tick = 1; tick <= 400 && view.sequencer_.opening().Active(); ++tick)
                view.OpeningTick(now + tick * kTableTickMs);
            require(!view.sequencer_.opening().Active() && view.sequencer_.captions().Frames().size() == 1 && view.sequencer_.captions().Frames()[0].resource == 480,
                "Opening flowers never appeared at the end of the deal");
            snapshot.recovering = true; receive("VSTATE");
            require(!view.sequencer_.captions().Active() && !view.sequencer_.opening_flowers()[0], "Recovery repeated the opening flower effect");
            view.Disconnect();
        }
    };
    struct SettlementPresentationCheck {
        static void Run(HWND parent) {
            auto require = [](bool ok, const char *message) { if (!ok) throw std::runtime_error(message); };
            SettlementView result(parent); result.SetAnimationEnabled(false); result.SetSoundEnabled(false);
            SettlementState state; SettlementReceipt win; win.generic = true; win.who = win.from = 0;
            win.entries = {{"清一色"}}; auto second = win; second.who = second.from = 1;
            auto draw = win; draw.who = draw.from = -1; state.receipts = {win, second, draw};
            int advanced = 0; result.SetConfirmHandler([&] { ++advanced; });
            result.Set(state, {}, false); result.SetContinuation(true); result.SetRoundFinished(true); result.Show(true);
            const auto expire = [&] {
                require(result.sequencer_.confirm_deadline() != 0 && result.sequencer_.confirm_seconds() == 5,
                    "A variant receipt has no fresh five-second OK countdown");
                result.sequencer_.SetConfirmDeadlineForTest(GetTickCount64()); result.ConfirmTick();
            };
            result.SetConfirmEnabled(false);
            require(!result.sequencer_.confirm_deadline(), "Disabled confirmation retained a running countdown");
            result.SetConfirmEnabled(true);
            for (std::size_t i = 0; i < state.receipts.size(); ++i) {
                require(result.sequencer_.selected() == i && advanced == 0, "Countdown skipped a player or submitted next hand early");
                expire();
            }
            require(advanced == 1, "Last receipt did not automatically continue the hand");
            result.Show(false); require(!result.sequencer_.confirm_deadline(), "Hidden results retained the OK countdown");
            // The final match hand and every page of a long fan list also advance.
            state.receipts = {win}; state.receipts[0].entries.resize(kSettlementPatternsPerPage + 1, {"花牌"});
            result.SetContinuation(false); result.Set(state, {}, false); result.Show(true);
            expire(); require(result.sequencer_.pattern_page() == 1 && advanced == 1, "Fan page did not auto-advance");
            expire(); require(advanced == 2, "Final hand without continuation had no automatic OK");
            result.Show(false); result.SetRoundFinished(false); result.Show(true);
            require(!result.sequencer_.confirm_deadline(), "History browsing started a live-hand countdown");
            result.Show(false);
        }

        static void RiichiEntrance(HWND parent) {
            SettlementState state;
            SettlementReceipt win;
            win.who = 0; win.has_yaku = true; win.dora = {80}; win.yaku = {{23, 1}};
            state.receipts = {win};
            for (const int canvas: {1024, 1440}) {
                SettlementLayout layout; layout.width = canvas; layout.yaku_entrance = .4;
                const auto one = BuildSettlementScene(state, 0, {}, 0, 0, false, false, layout);
                state.receipts[0].yaku = {{1,1}, {2,1}, {7,1}, {8,1}, {52,1}, {53,0}, {54,1}};
                const auto seven = BuildSettlementScene(state, 0, {}, 2, 0, false, false, layout);
                const auto settled = BuildSettlementScene(state, 0, {}, 7, 0, true, true, layout);
                if (seven.panel.bottom - seven.panel.top - (one.panel.bottom - one.panel.top) !=
                    6 * (canvas == 1024 ? 24 : 34) || !EqualRect(&seven.panel, &settled.panel))
                    throw std::runtime_error("Riichi card does not follow the full receipt's yaku count");
                if (std::count_if(seven.text.begin(), seven.text.end(), [](const SettlementTextItem &item) {
                    return (item.role == "yaku" || item.role == "han") && item.entrance == .4;
                }) != 2) throw std::runtime_error("The current yaku and han must animate together");
                state.receipts = {win};
            }
            SettlementView result(parent);
            result.SetSoundEnabled(false);
            result.Set(state, {}, false);
            result.Show(true);
            result.Present();
            for (int frame = 0; frame < 30; ++frame) result.PresentationTick();
            if (result.sequencer_.shown_yaku() != 1 || result.sequencer_.yaku_entrance_frame() != 0)
                throw std::runtime_error("Riichi entrance did not start at frame zero");
            for (int frame = 0; frame < 10; ++frame) result.PresentationTick();
            if (result.sequencer_.yaku_entrance_frame() != 10 || !result.Animating())
                throw std::runtime_error("Riichi entrance did not settle before totals");
            result.Show(false);
        }
    };
}
namespace {
    void Check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
    void Geometry(const std::filesystem::path &resources) {
        TableArtwork art; art.Load(resources, 1440);
        ReplayState state; state.initialized = state.variant = true;
        for (int seat: {1, 3}) {
            auto &player = state.players[seat]; player.hand.assign(13, -1);
            player.retired = true; player.winning_tile = 4;
        }
        const auto scene = BuildTableScene(state);
        for (const auto &winning: scene) if (std::string(winning.role) == "winning") {
            const auto &face = art.Get(winning); const auto shift = art.Offset(winning);
            RECT win{face.x + shift.x, face.y + shift.y,
                face.x + shift.x + face.image.width, face.y + shift.y + face.image.height};
            for (const auto &back: scene) if (back.direction == winning.direction && std::string(back.role) == "retired") {
                const auto &image = art.Get(back); const auto offset = art.Offset(back);
                RECT hand{image.x + offset.x, image.y + offset.y,
                    image.x + offset.x + image.image.width, image.y + offset.y + image.image.height}, intersection{};
                Check(!IntersectRect(&intersection, &win, &hand), "Side winner overlaps the retired hand");
            }
        }
    }

    void RiichiFlights(const std::filesystem::path &resources) {
        TableArtwork art;
        for (const int width: {1024, 1440}) {
            if (width == 1024 && (!std::filesystem::exists(resources / "packs/_obj0.dat") ||
                                  !std::filesystem::exists(resources / "packs/_obj1.dat"))) continue;
            art.Load(resources, width);
            const auto origin = [&](const TableTile &tile) {
                const auto &image = art.Get(tile);
                const auto offset = art.Offset(tile);
                return std::make_pair(image.x + offset.x, image.y + offset.y);
            };
            ReplayState before;
            before.initialized = true;
            before.wall.Init(0, 1, 1);
            before.players[0].hand = {0,4,8,12,16,20,24,28,32,36,40,44,48};
            auto after = before;
            after.wall.Draw();
            after.players[0].hand.push_back(56);
            after.players[0].drawn_tile = 56;
            const auto draw = BuildReplayTransition(before, after, {ReplayEventKind::Draw, "T56", {}, 0, 56},
                                                    0, width, origin, true, &art);
            Check(draw.duration >= 195 && draw.duration <= 210, "Draw lost its native tick duration");
            const auto first_frame = draw.Frame(0), before_tick = draw.Frame(14.99), next_tick = draw.Frame(15);
            const auto &flying = first_frame.back();
            Check(flying.tile.tile == 56 && flying.flight && flying.flight->sprite.tile == -1 &&
                  flying.flight->count == 4 && flying.flight->opacity == .08f, "Draw did not fly a four-copy back");
            Check(flying.x == before_tick.back().x && flying.y == before_tick.back().y &&
                  (flying.x != next_tick.back().x || flying.y != next_tick.back().y),
                  "Draw interpolated between native ticks");
            const auto &trail = art.GetFlight(*flying.flight, true).image;
            unsigned peak = 0;
            for (std::size_t i = 3; i < trail.bgra.size(); i += 4) peak = std::max(peak, unsigned(trail.bgra[i]));
            const auto terminal = draw.Frame(draw.duration - 1).back();
            Check(peak > 20 && peak < 90 && terminal.flight && terminal.x == terminal.flight->landing_x &&
                  terminal.y == terminal.flight->landing_y && !draw.Frame(draw.duration).back().flight,
                  "Composite alpha or final hand swap is wrong");

            before = after;
            after.players[0].hand.erase(after.players[0].hand.begin() + 1);
            after.players[0].drawn_tile = -1;
            after.players[0].discards.push_back({4, false, true, false});
            const auto discard = BuildReplayTransition(before, after, {ReplayEventKind::Discard, "D4", {}, 0, 4},
                                                       0, width, origin, true, &art);
            Check(discard.duration >= 135 && discard.duration <= 150 && discard.Frame(0).back().flight &&
                  discard.Frame(0).back().flight->sprite.slot == 40 && discard.Frame(0).back().flight->cover_landing,
                  "Discard lost its original speed, sideways art or landing cover");

            before.players[0].hand.back() = 120;
            before.players[0].drawn_tile = 120;
            after = before;
            after.players[0].hand.pop_back();
            after.players[0].drawn_tile = -1;
            after.players[0].nuki.push_back(120);
            const auto nuki = BuildReplayTransition(before, after, {ReplayEventKind::Meld, "N", {}, 0},
                                                    0, width, origin, true, &art);
            const auto nuki_frame = nuki.Frame(0);
            Check(nuki_frame.back().flight && nuki_frame.back().flight->count == 4 &&
                  nuki_frame.back().flight->opacity == .16f && !nuki_frame.back().flight->cover_landing,
                  "North extraction lost its four-copy flight");
        }
    }
    void Settlement(HWND parent) {
        SettlementState state; SettlementReceipt receipt;
        receipt.generic = true; receipt.who = receipt.from = 0; receipt.fan = 3;
        receipt.entries = {{"清一色"}, {"根×1"}}; state.receipts = {receipt};
        SettlementView result(parent); std::vector<unsigned> sounds;
        result.SetSoundPlayer([&](unsigned id) { sounds.push_back(id); });
        result.SetSoundEnabled(true);
        result.Set(state, {}, false); result.Show(true);
        const auto window = GetDlgItem(parent, 5400);
        Check(result.Animating() && sounds == std::vector<unsigned>{101}, "Generic settlement has no opening sound or wait");
        for (int i = 0; i < 29; ++i) SendMessageW(window, WM_TIMER, 1, 0);
        Check(sounds.size() == 1, "First pattern skipped the opening wait");
        SendMessageW(window, WM_TIMER, 1, 0);
        Check(sounds == std::vector<unsigned>({101, 109}), "First generic pattern is silent");
        for (int i = 0; i < 55; ++i) SendMessageW(window, WM_TIMER, 1, 0);
        Check(sounds == std::vector<unsigned>({101, 109, 109}), "Second generic pattern is silent");
        for (int i = 0; i < 12; ++i) SendMessageW(window, WM_TIMER, 1, 0);
        Check(!result.Animating(), "Generic settlement never finished");
        result.Show(false); result.SetSoundEnabled(false); sounds.clear(); result.Show(true);
        for (int i = 0; i < 120; ++i) SendMessageW(window, WM_TIMER, 1, 0);
        Check(sounds.empty() && !result.Animating(), "Muted settlement played audio or blocked completion");
        result.Show(false); result.SetSoundEnabled(true); result.Show(true); sounds.clear();
        result.SetPaused(true); SendMessageW(window, WM_TIMER, 1, 0);
        Check(sounds.empty(), "Paused settlement advanced");
        result.SetPaused(false); result.Show(false);
        SendMessageW(window, WM_TIMER, 1, 0); Check(sounds.empty(), "Hidden settlement replayed audio");
        // Browsing live receipts must not skip earlier winners when the hand ends.
        result.SetAnimationEnabled(false); result.SetSoundEnabled(false); result.SetContinuation(true);
        SettlementState ordered;
        SettlementReceipt first; first.generic = true; first.who = first.from = 0;
        auto second = first; second.who = second.from = 1;
        auto draw = first; draw.who = draw.from = -1;
        ordered.receipts = {first, second, draw}; result.Set(ordered, {}, false);
        const auto choices = GetDlgItem(window, 5401);
        SendMessageW(choices, CB_SETCURSEL, 1, 0);
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(5401, CBN_SELCHANGE), reinterpret_cast<LPARAM>(choices));
        result.SetRoundFinished(true); result.Show(true);
        Check(SendMessageW(choices, CB_GETCURSEL, 0, 0) == 0, "Hand-end results did not rewind to the first winner");
        int advanced = 0; result.SetConfirmHandler([&] { ++advanced; });
        for (int i = 0; i < 2; ++i) {
            SendMessageW(window, WM_COMMAND, 5404, 0);
            Check(SendMessageW(choices, CB_GETCURSEL, 0, 0) == i + 1 && advanced == 0,
                "Next hand was submitted before all winners and the draw receipt");
        }
        SendMessageW(window, WM_COMMAND, 5404, 0);
        Check(advanced == 1, "Last combined draw receipt did not release confirmation");
        result.Show(false); result.SetConfirmHandler({});
    }

    void Guobiao(HWND parent, const std::filesystem::path &resources) {
        TableArtwork art; art.Load(resources, 1440);
        for (const auto &rule: {"sichuan-bloodbattle", "guobiao"}) {
            const int rounds = std::string(rule) == "guobiao" ? 16 : 8;
            ReplayState hud_state; hud_state.variant = true; hud_state.variant_rule = rule;
            for (int round = 0; round < rounds; ++round) {
                hud_state.round = round;
                const auto hud = BuildOriginalHud(hud_state, 1440);
                Check(FindText(hud.text[0].key)->origin_id == 407u + round / 4 && FindText(hud.text[1].key)->origin_id == 403u + round % 4 &&
                    hud.text[2].key == TextKey::center_hand_unit, "Variant round title does not reuse riichi wind/number/hand artwork");
            }
            ReplayState opening_state; opening_state.variant = opening_state.initialized = true;
            opening_state.wall.InitVariant(rounds == 8 ? 108 : 144, 52, 0, 0);
            const std::vector<int> order{48,0,44,4,40,8,36,12,32,16,28,20,24};
            for (auto &player: opening_state.players) player.hand.assign(13, -1);
            opening_state.players[0].hand = order; std::sort(opening_state.players[0].hand.begin(), opening_state.players[0].hand.end());
            opening_state.players[0].deal_hand = order;
            OpeningPresentation opening; opening.Begin(opening_state, true, &art);
            const auto stack = WallTile(opening_state.wall, 1, 0, true);
            constexpr int landing = 900;
            opening.Tick();
            auto wall_scene = BuildWallScene(opening.Frame(), opening.placed());
            Check(wall_scene.size() == 1 && wall_scene.front().direction == 0,
                "Opening did not place one complete stack at a time");
            for (int step = 0; step <= 5; ++step) {
                opening.Tick();
                std::vector<PositionedTile> wall{{stack, landing, 200}};
                opening.SlideWall(wall);
                Check(wall.front().x == landing + 160 - 40 * std::min(step, 4) && wall.front().y == 200 &&
                      wall.front().flight && wall.front().flight->count == 2 && wall.front().flight->opacity == .08f,
                    "Wall stack did not follow the original four-step flight");
            }
            opening.Tick(opening.wall_tiles() / 2 - opening.steps());
            Check(opening.Frame().turn == -1 && opening.dealt() == 0, "Wall setup highlighted a dealing seat");
            while (opening.Active() && !opening.dealt()) opening.Tick();
            Check(opening.Frame().players[0].hand == std::vector<int>(order.begin(), order.begin() + 4),
                "Variant first deal packet arrived already sorted");
            std::vector<int> actors;
            int last = -1;
            const auto first_packet = opening.steps();
            std::vector<POINT> flight;
            while (opening.Active()) {
                const int actor = opening.Frame().turn;
                if (actor >= 0 && actor != last) actors.push_back(actor);
                if (opening.steps() - first_packet < 3) {
                    const auto scene = opening.Scene(art);
                    const auto tile = std::find_if(scene.begin(), scene.end(), [&](const PositionedTile &item) {
                        return item.tile.direction == 0 && item.tile.tile == order.front();
                    });
                    Check(tile != scene.end() && tile->flight && tile->flight->count == 4 &&
                          tile->flight->sprite.family == 'n' && tile->flight->sprite.tile == -1,
                          "Dealing flight lost the first tile or used a visible face");
                    flight.push_back({tile->x, tile->y});
                }
                last = actor;
                opening.Tick();
            }
            Check(flight.size() == 3 && (flight[0].x != flight[2].x || flight[0].y != flight[2].y)
                && std::abs(flight[2].x - 2 * flight[1].x + flight[0].x) <= 1
                && std::abs(flight[2].y - 2 * flight[1].y + flight[0].y) <= 1,
                "Deal flight did not advance at a constant step");
            Check(actors == std::vector<int>{0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3},
                "Dealing highlight did not follow each packet's recipient");
            auto scene = opening.Scene(art); std::vector<std::pair<int,int>> slots;
            for (const auto &tile: scene) if (tile.tile.direction == 0 && std::string(tile.tile.role) == "hand")
                slots.emplace_back(tile.tile.slot, tile.tile.tile);
            std::sort(slots.begin(), slots.end());
            for (std::size_t i = 0; i < slots.size(); ++i)
                Check(slots[i].second == static_cast<int>(i) * 4, "Variant opening did not sort after the full deal");
            Check(slots.size() == 13, "Variant sort omitted dealt entities");
        }
        ReplayState table; table.initialized = table.variant = true;
        table.wall.InitVariant(144, 0, 0, 0);
        for (int seat = 0; seat < 4; ++seat) {
            auto &player = table.players[seat]; player.hand.assign(10, -1);
            if (!seat) player.hand = {4,8,12,16,20,24,28,32,36,40};
            player.flowers = {136 + seat, 140 + seat};
            ReplayMeld meld; meld.kind = MeldKind::Pon; meld.from = (seat + 1) % 4;
            meld.tiles = {0,1,2}; meld.called_tile = 2; player.melds = {meld};
        }
        const auto table_scene = BuildTableScene(table);
        Check(std::count_if(table_scene.begin(), table_scene.end(), [](const TableTile &t) {
            return std::string(t.role) == "flower"; }) == 4, "Flowers are not public corner counters");
        for (const auto &tile: table_scene) if (std::string(tile.role) == "flower") {
            Check(tile.tile == 136 + tile.direction && tile.slot == 0, "Counter did not retain the first flower");
            for (const auto &meld: table_scene) if (meld.direction == tile.direction && std::string(meld.role) == "meld")
                Check(meld.slot % 20 > 0, "Flower occupies a meld placeholder");
        }
        for (int seat = 0; seat < 4; ++seat) {
            std::set<Bytes> faces;
            for (int entity = 136; entity < 144; ++entity) faces.insert(art.Get({'n', 0, entity, seat}).image.bgra);
            Check(faces.size() == 8, "Flower faces alias each other in the artwork cache");
        }
        std::vector<PositionedTile> wall;
        const auto full = BuildWallScene(table, 144);
        Check(full.size() == 72, "144-tile wall has fewer than 72 stacks");
        for (const auto &tile: full) {
            const auto &im = art.Get(tile); const auto offset = art.Offset(tile);
            RECT rect{im.x + offset.x, im.y + offset.y, im.x + offset.x + im.image.width, im.y + offset.y + im.image.height};
            Check(rect.left >= 0 && rect.top >= 0 && rect.right <= 1440 && rect.bottom <= 1080, "Eighteenth stack leaves the canvas");
            for (const auto &other: table_scene) {
                const auto &image = art.Get(other); const auto shift = art.Offset(other);
                RECT target{image.x + shift.x, image.y + shift.y, image.x + shift.x + image.image.width, image.y + shift.y + image.image.height}, intersection{};
                bool opaque_overlap = false;
                if (IntersectRect(&intersection, &rect, &target))
                    for (LONG y = intersection.top; y < intersection.bottom; ++y)
                        for (LONG x = intersection.left; x < intersection.right; ++x) {
                            const auto a = (std::size_t(y - rect.top) * im.image.width + x - rect.left) * 4 + 3;
                            const auto b = (std::size_t(y - target.top) * image.image.width + x - target.left) * 4 + 3;
                            if (im.image.bgra[a] > 32 && image.image.bgra[b] > 32) opaque_overlap = true;
                        }
                if (opaque_overlap) throw std::runtime_error("Full wall direction=" + std::to_string(tile.direction) +
                    " stack=" + std::to_string(tile.wall_stack) + " overlaps " + other.role + " direction=" + std::to_string(other.direction) +
                    " slot=" + std::to_string(other.slot) + " area=" + std::to_string(intersection.right - intersection.left) + "x" + std::to_string(intersection.bottom - intersection.top));
            }
            auto command = tile;
            if (tile.wall_stack == 17) command = WallTile(table.wall, tile.direction, 17, false);
            const auto &image = art.Get(command); const auto shift = art.Offset(command);
            wall.push_back({command, image.x + shift.x, image.y + shift.y});
        }
        const auto shadows = BuildTileShadows(wall, art, ShadowPack::Embedded());
        Check(shadows.size() == 72, "Eighteenth lower tile lost its shadow to the seventeenth stack");
        for (const auto &shadow: shadows) Check(shadow.ground != nullptr, "Extended wall lacks a ground shadow");
        ReplayState before; before.initialized = before.variant = true; before.wall.InitVariant(144, 52, 0, 0);
        before.players[0].hand = {0,4,8,12,16,20,24,28,32,36,40,44,48};
        auto after = before; after.wall.InitVariant(144, 53, 3, 0); after.players[0].hand.push_back(52); after.players[0].drawn_tile = 52;
        ReplayEvent draw; draw.kind = ReplayEventKind::Draw; draw.who = 0;
        const auto origin = [&](const TableTile &tile) {
            const auto &image = art.Get(tile); const auto shift = art.Offset(tile);
            return std::pair<int,int>{image.x + shift.x, image.y + shift.y};
        };
        const auto transition = BuildReplayTransition(before, after, draw, 12, 1440, origin);
        const auto position = before.wall.Position(141); // Two tail flowers followed by the ordinary replacement.
        const auto expected = origin(WallTile(before.wall, position.first, position.second / 2, position.second % 2 == 0));
        const auto moving = std::find_if(transition.tiles.begin(), transition.tiles.end(), [](const AnimatedTile &tile) { return tile.tile.tile == 52; });
        Check(moving != transition.tiles.end() && moving->from_x == expected.first && moving->from_y == expected.second,
            "Consecutive flowers draw the replacement from the first tail flower instead of the final tile");
        std::size_t patterns = 0;
        for (const auto &pattern: kTextRulePatterns) if (pattern.rule == "guobiao") {
            ++patterns; Check(FindTextRulePattern("guobiao", pattern.server_name) == &pattern, "Unmapped guobiao fan");
        }
        Check(patterns == 82 && !FindTextRulePattern("guobiao", "../secret"), "Pattern catalogue is incomplete or accepts paths");
        SettlementState state; SettlementReceipt receipt;
        receipt.generic = true; receipt.rule = "guobiao"; receipt.who = receipt.from = 0; receipt.fan = 24;
        for (int i = 0; i < 30; ++i) receipt.entries.push_back(ParseSettlementEntry("花牌×2"));
        state.receipts = {receipt};
        for (std::size_t page = 0; page < 3; ++page) {
            SettlementLayout layout; layout.pattern_page = page;
            const auto scene = BuildSettlementScene(state, 0, {}, static_cast<std::size_t>(-1), 0, true, true, layout);
            Check(std::count_if(scene.text.begin(), scene.text.end(), [](const SettlementTextItem &text) {
                return text.role == "variant_pattern"; }) == (page == 2 ? 2 : 14), "Pattern paging omitted or duplicated entries");
            Check(std::any_of(scene.text.begin(), scene.text.end(), [](const SettlementTextItem &text) {
                return text.role == "variant_fan" && text.text == "24 飜"; }), "Guobiao uses the wrong fan unit");
        }
        SettlementReceipt sichuan; sichuan.generic = true; sichuan.rule = "sichuan-bloodbattle";
        sichuan.who = sichuan.from = 0; sichuan.fan = 2; sichuan.points = 2000;
        sichuan.entries = {{"底和", 1, 1}, {"根", 1, 1}};
        const auto sc = BuildSettlementScene(SettlementState{{}, 15, {sichuan}}, 0, {});
        Check(std::count_if(sc.text.begin(), sc.text.end(), [](const SettlementTextItem &t) {
            return t.role == "variant_value" && t.text == "1 飜"; }) == 2, "Sichuan per-pattern values are absent");
        Check(std::any_of(sc.text.begin(), sc.text.end(), [](const SettlementTextItem &t) {
            return t.role == "variant_fan" && t.text == "2 飜 2000 点"; }), "Sichuan total lost the displayed payment");
        sichuan.who = sichuan.from = -1; sichuan.entries.clear(); sichuan.reason = "查花猪、查大叫、退税";
        const auto drawn = BuildSettlementScene(SettlementState{{}, 15, {sichuan}}, 0, {});
        Check(std::count_if(drawn.text.begin(), drawn.text.end(), [](const SettlementTextItem &item) {
            return item.role == "reason" && item.text == "流局" && !item.spans.empty(); }) == 1 &&
            drawn.score_panels.size() == 4, "Variant draw does not share the riichi title and four payment panels");
        auto riichi_draw = sichuan; riichi_draw.generic = false; riichi_draw.reason.clear();
        const auto compact = BuildSettlementScene(SettlementState{{}, 15, {riichi_draw}}, 0, {});
        Check(EqualRect(&drawn.panel, &compact.panel) && drawn.panel.bottom - drawn.panel.top == 300 &&
            std::none_of(drawn.text.begin(), drawn.text.end(), [](const SettlementTextItem &t) { return t.role == "variant_reason"; }),
            "Draw window is oversized or still contains the long variant reason");
        for (const auto &panel: drawn.score_panels)
            Check(panel.left >= drawn.panel.left && panel.right <= drawn.panel.right && panel.top >= drawn.panel.top &&
                panel.bottom <= drawn.confirm.top, "Compact draw payment card escaped the panel");
        for (const auto &rule: {"", "sichuan-bloodbattle", "guobiao"}) {
            SettlementState payments; SettlementReceipt a, b;
            a.rule = b.rule = rule; a.who = 0; a.delta = {3000,-1000,-1000,-1000};
            b.who = 1; b.delta = {-2000,2000,0,0}; payments.receipts = {a,b};
            Check(ScoresBeforeSettlement({52000,50000,49000,49000}, payments) == std::array<int,4>{51000,49000,50000,50000},
                "Win payments leaked onto the table or immediate kong points were hidden");
        }
        Check(SettlementText(state, 0, {}).find("花牌×2") != std::string::npos, "Text details lost pattern repetitions");
        SettlementView result(parent); result.SetAnimationEnabled(false); result.Set(state, {}, false); result.Show(true);
        const auto window = GetDlgItem(parent, 5400);
        Check(!IsWindowEnabled(GetDlgItem(window, 5405)) && IsWindowEnabled(GetDlgItem(window, 5406)), "Paging controls have the wrong initial state");
        Check(result.HandleKey(VK_NEXT) && result.HandleKey(VK_NEXT), "PageDown is not routed to pattern paging");
        Check(IsWindowEnabled(GetDlgItem(window, 5405)) && !IsWindowEnabled(GetDlgItem(window, 5406)), "Last pattern page is inaccessible");
        result.Show(false);
    }
}
int main(int argc, char **argv) {
    HWND parent = nullptr;
    ULONG_PTR gdiplus = 0;
    try {
        Check(argc == 2, "Usage: tenhou_presentation_check RESOURCES");
        Gdiplus::GdiplusStartupInput input;
        Check(Gdiplus::GdiplusStartup(&gdiplus, &input, nullptr) == Gdiplus::Ok, "Cannot start GDI+");
        INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES}; InitCommonControlsEx(&controls);
        Geometry(argv[1]);
        RiichiFlights(argv[1]);
        parent = CreateWindowExW(0, L"STATIC", L"Presentation checks", WS_POPUP,
            -32000, -32000, 1200, 850, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        Check(parent != nullptr, "Cannot create test parent"); ShowWindow(parent, SW_SHOWNOACTIVATE);
        Settlement(parent); SettlementPresentationCheck::Run(parent); SettlementPresentationCheck::RiichiEntrance(parent);
        Guobiao(parent, argv[1]);
        LobbyPresentationCheck::OfficialCalls(parent, argv[1]);
        LobbyPresentationCheck::OfficialPracticeExitStopsOpening(parent, argv[1]);
        LobbyPresentationCheck::RecoveredContestReturn(parent, argv[1]);
        LobbyPresentationCheck::ProtocolOpening(parent, argv[1], "sichuan-bloodbattle");
        LobbyPresentationCheck::ProtocolOpening(parent, argv[1], "guobiao");
        LobbyPresentationCheck::Run(parent, argv[1]);
        RunTextTableChecks(parent, argv[1]);
        DestroyWindow(parent); parent = nullptr;
        Gdiplus::GdiplusShutdown(gdiplus);
        std::cout << "PASS guobiao flowers, 18-stack geometry/shadows, replacement origins, 82 fan mappings, PNG overrides/paging; shared presentation and recovery\n";
        return 0;
    } catch (const std::exception &error) {
        if (parent) DestroyWindow(parent);
        if (gdiplus) Gdiplus::GdiplusShutdown(gdiplus);
        std::cerr << error.what() << '\n'; return 1;
    }
}
