#include "offline_lobby.hpp"
#include "network_protocol.hpp"
#include <iostream>
#include <stdexcept>

namespace {
    void Check(const bool condition, const char *message) {
        if (!condition) throw std::runtime_error(message);
    }

    void ServiceResponses() {
        using namespace tenhou;
        LobbySession client;
        client.require_native = client.match_games = client.interactive_games = client.origin_lobby = true;
        client.Connected("test_account", 'M');
        const auto accept = [&](const std::string &frame) { return client.Receive(ParseNetworkMessage(frame)); };
        const auto caps = accept("<HELO uname=\"TENHOU-PLATFORM\" version=\"1\" caps=\"origin-lobby\" />");
        Check(caps.size() == 1 && ParseNetworkMessage(caps.front()).attributes.at("lobby") == "origin", "Origin capability");
        accept("<PROFILE account=\"test_account\" nickname=\"Test\" dan4=\"10\" rating4=\"1640.40\" />");
        accept("<CHAT lobby=\"L0000\" epoch=\"1\" />");
        const auto &s = client.snapshot;
        Check(s.origin_lobby && s.phase == LobbyPhase::Lobby && s.origin_lobby->profile.four.rating == 1640,
              "Origin login and real profile");
        client.SubmitOriginLobby("change", "C00112233");
        const std::string contest = "<CS lobby=\"C00112233\" epoch=\"2\" title=\"Contest\" players=\"3\" rule=\"17\" "
            "joinfee=\"0\" premiumonly=\"0\" registered=\"1\" blocked=\"0\" start=\"1\" end=\"999\" now=\"10\" />";
        accept(contest);
        accept("<CHAT lobby=\"C00112233\" epoch=\"2\" rid=\"99\" />");
        Check(s.lobby == "L0000" && s.origin_lobby->pending() && !s.origin_lobby->Contest(), "Unrelated config or move receipt");
        accept("<CHAT lobby=\"C00112233\" epoch=\"2\" rid=\"1\" />");
        accept(contest);
        Check(s.lobby == "C00112233" && s.origin_lobby->ContestAction(10) == ContestStatus::Open, "Confirmed contest");
        accept("<RESERVATIONS lobby=\"L0000\" epoch=\"1\" types=\"1\" />");
        Check(!s.origin_lobby->reservations(), "Old reservation affected contest");
        client.SubmitOriginLobby("change", "L0000");
        accept("<CHAT lobby=\"L0000\" epoch=\"3\" rid=\"2\" />");
        accept("<RESERVATIONS lobby=\"L0000\" epoch=\"1\" types=\"1\" />");
        const std::string old_counts = "<LN lobby=\"L0000\" epoch=\"1\" n=\"7,0,1,0\" j=\"discarded\" g=\"discarded\" />";
        accept(old_counts);
        Check(!s.origin_lobby->reservations() && s.origin_lobby->counts().totals[0] == 0, "Old packets after re-entering the same room");
        client.SubmitOriginLobby("change", "C00000001");
        accept("<RESERVATIONS lobby=\"L0000\" epoch=\"3\" rid=\"3\" types=\"\" code=\"LOBBY_NOT_FOUND\" />");
        Check(s.phase == LobbyPhase::Lobby && !s.origin_lobby->pending() && s.origin_error == "LOBBY_NOT_FOUND",
              "Rejected move left the lobby blocked");
        client.SubmitOriginLobby("join", "1");
        accept("<RESERVATIONS lobby=\"L0000\" epoch=\"3\" rid=\"4\" types=\"1\" />");
        client.SubmitOriginLobby("join", "129");
        accept("<RESERVATIONS lobby=\"L0000\" epoch=\"3\" rid=\"5\" types=\"1,129\" />");
        Check(s.origin_lobby->reservations() == 2, "Multiple confirmed tickets");
        client.SubmitOriginLobby("cancel", {});
        accept("<UN n0=\"A\" n1=\"B\" n2=\"C\" n3=\"D\" sx=\"MMMM\" />");
        const auto reply = accept("<GO lobby=\"L0000\" reservation=\"129\" type=\"1\" ranked=\"1\" exit=\"0\" table=\"t\" />");
        accept("<RESERVATIONS lobby=\"L0000\" epoch=\"3\" rid=\"6\" types=\"\" />");
        Check(s.phase == LobbyPhase::Game && s.game.type == 1 && s.match_type == 129 && s.ranked &&
              s.origin_lobby->reservations() == 0 && s.origin_pending.empty() && reply == std::vector<std::string>{"<GOK />"},
              "Committed GO wins a pending cancel and uses presentation flags");
    }

    void OfficialProfileAndServerErrorChecks() {
        using namespace tenhou;
        // 1. FormatServerError mapping and fatal classification.
        const auto dup = FormatServerError(1004, {});
        Check(dup.caption == "Error" && dup.fatal && !dup.message.empty(), "ERR 1004 must be fatal Error dialog");
        const auto non_fatal = FormatServerError(1019, {});
        Check(non_fatal.caption == "Error" && !non_fatal.fatal, "ERR 1019 must be non-fatal");
        const auto special = FormatServerError(1012, {});
        Check(special.caption != "Error" && !special.fatal, "ERR 1012 uses special caption");
        const auto custom = FormatServerError(400, "该账号已在其他窗口登录");
        Check(custom.caption == "Error" && custom.fatal &&
              custom.message == "该账号已在其他窗口登录", "Self-hosted ERR wire text preserved");

        // 2. Official duplicate login <ERR code="1004" /> throws ServerRejection.
        {
            LobbySession official;
            official.official = official.origin_lobby = true;
            official.Connected("ID00000000-00000000", 'M');
            bool caught = false;
            try {
                official.Receive(ParseNetworkMessage("<ERR code=\"1004\" />"));
            } catch (const ServerRejection &e) {
                caught = (e.code == 1004 && e.wire_text.empty());
            }
            Check(caught, "Official ERR 1004 must throw ServerRejection with code 1004");
        }

        // 3. Self-hosted duplicate login <ERR code="400" text="..." /> throws ServerRejection.
        {
            LobbySession native;
            native.require_native = native.origin_lobby = true;
            native.Connected("dup_account", 'M');
            bool caught = false;
            try {
                native.Receive(ParseNetworkMessage("<ERR code=\"400\" text=\"该账号已在其他窗口登录\" />"));
            } catch (const ServerRejection &e) {
                caught = (e.code == 400 && e.wire_text == "该账号已在其他窗口登录");
            }
            Check(caught, "Self-hosted duplicate login ERR must throw ServerRejection with wire text");
        }

        // 4. Official HELO + 15-field & 21-field RANKING v2 + PF602 + non-fatal ERR 1019.
        {
            LobbySession official;
            official.official = official.origin_lobby = true;
            official.preferred_monthly_profile = 17;
            official.Connected("ID00000000-00000000", 'M');
            official.Receive(ParseNetworkMessage(
                "<HELO uname=\"HalfCool\" auth=\"20261124-00000000\" "
                "PF4=\"11,465,1674.00,0,41,38,38,39,11,800,178,117,288,138\" "
                "PF3=\"11,175,1538.00,-2052.6,87,101,123,0,42,1000,272,186,300,220\" "
                "PF602=\"15,-60.0,4,3,2,1,1,80,20,10,24,16\" PF02C=\"0,1620.00,0\" "
                "expire=\"20270527\" />"));
            official.Receive(ParseNetworkMessage("<LN n=\"B\" j=\"A\" g=\"A\" />"));
            Check(official.snapshot.phase == LobbyPhase::Lobby && official.snapshot.origin_lobby,
                  "Official session entered lobby");
            const auto &amber = official.snapshot.origin_lobby->profile.other_lifetime.at(602);
            Check(amber.games == 10 && amber.average_income && *amber.average_income == -3.0 &&
                  amber.average_chips && *amber.average_chips == 1.5,
                  "PF602 average_income and average_chips parsed");

            // 15-field monthly ranking (HalfCool Figure 2: 2+0+0=2 games, +104 945位, +60 837位, R1538 25098位).
            official.Receive(ParseNetworkMessage("<RANKING v2=\"2,0,0,0,104,,0,945,0,837,0,0,0,0,25098\" />"));
            const auto &m15 = official.snapshot.origin_lobby->profile.monthly.at(17);
            Check(m15.has_monthly_raw && !m15.has_amber_monthly && m15.games == 2 &&
                  m15.place_counts == std::array<int, 4>{2, 0, 0, 0} && m15.total_score == 104.0 &&
                  m15.monthly_ranks.size() == 9 && m15.monthly_ranks[1] == 945 &&
                  m15.monthly_ranks[3] == 837 && m15.monthly_ranks[8] == 25098,
                  "15-field RANKING v2 parsed accurately");

            // 21-field Amber monthly ranking.
            official.Receive(ParseNetworkMessage(
                "<RANKING v2=\"4,3,2,1,55,,12,10,20,30,40,50,60,70,80,150,25,101,102,103,104\" />"));
            const auto &m21 = official.snapshot.origin_lobby->profile.monthly.at(17);
            Check(m21.has_monthly_raw && m21.has_amber_monthly && m21.games == 10 &&
                  m21.total_income == 150.0 && m21.total_chips == 25 &&
                  m21.monthly_ranks.size() == 13 && m21.monthly_ranks[0] == 12 &&
                  m21.monthly_ranks[9] == 101 && m21.monthly_ranks[12] == 104,
                  "21-field Amber RANKING v2 parsed accurately");

            // Non-fatal ERR 1019 in Lobby must not disconnect or throw.
            const auto before_receipt = official.snapshot.server_error_receipt;
            official.Receive(ParseNetworkMessage("<ERR code=\"1019\" />"));
            Check(official.snapshot.phase == LobbyPhase::Lobby &&
                  official.snapshot.server_error_receipt > before_receipt &&
                  official.snapshot.server_error_code == 1019 &&
                  !official.snapshot.server_error_fatal,
                  "Non-fatal official ERR 1019 recorded without disconnecting");
        }
    }
}

int main() {
    using namespace tenhou;
    try {
        Check(LocalizeCertification("認定段位/級位：１級\r\n天鳳段位認定協会 殿\r\n天鳳サンマ漁業協会") ==
              "认定段级位：１级\n天鳳段位認定協会阁下：\n天凤秋刀鱼渔业协会",
              "Certificate must translate ranks and issuers while preserving player names");
        Check(LocalizeCertification("Additional award text") == "Additional award text",
              "Unknown certificate text must be preserved");

        Check(FormatFixedTrunc(-1.239, 2, false, nullptr) == "-1.23" &&
              FormatRate(0.2799) == ".279" && FormatRate(0) == ".000" && FormatRate(1) == "1.00",
              "Profile values truncate and keep the original rate precision");
        Check(FormatSignedAdaptive(9.999, 2) == "+9.99" && FormatSignedAdaptive(-10.99, 2) == "-10.9" &&
              FormatSignedAdaptive(100.9, 2) == "+100" && FormatSignedAdaptive(4.56, 1) == "+4.5",
              "Signed profile precision follows magnitude and the field limit");
        Check(FormatExpiry("20261125") == "2026/11/25" && FormatExpiry("") == "----/--/--" &&
              FormatExpiry("2026/11/25") == "2026/11/25" && RankGradeName(10) == "初段",
              "Profile expiry and grade use the same display values");
        auto profile = FixtureProfile();
        profile.other_lifetime[kAmberLifetimeProfiles[0]].rating = 1888;
        Check(MonthlyCorrespondingRating(profile, 17) == profile.three.rating &&
              MonthlyCorrespondingRating(profile, kAmberMonthlyProfiles[0]) == 1888 &&
              !LifetimeProfileLabel(kRankedFourProfile).empty() && !RankedMonthlyLabel(17).empty(),
              "Monthly choices refer to the corresponding lifetime categories");
        const auto clipboard = FormatProfileClipboardText(profile, {}, kRankedFourProfile,
                                                           kDefaultMonthlyProfile, false, "2026/11/25 00:00:00");
        Check(clipboard.find("2026/11/25 00:00:00\r\n") != std::string::npos &&
              clipboard.find(LifetimeProfileCaption(kRankedFourProfile)) != std::string::npos &&
              clipboard.find(MonthlyProfileCaption(kDefaultMonthlyProfile)) != std::string::npos,
              "Clipboard export includes the caller's timestamp and profile captions");

        OfflineLobbyTiming timing;
        timing.BeginMove(1000);
        Check(!timing.MoveReady(1249) && timing.MoveReady(1250), "Local move confirmation deadline");
        Check(timing.Hover(2, 1000) == OfflineLobbyTiming::HoverEffect::Schedule &&
              !timing.AdvanceRules(1499) && timing.AdvanceRules(1500) && timing.rule_slot() == 2,
              "Rule tooltip opens after the hover delay");
        Check(timing.Hover(3, 1500) == OfflineLobbyTiming::HoverEffect::Redraw && timing.rule_slot() == 3 &&
              timing.Hover(-1, 1600) == OfflineLobbyTiming::HoverEffect::Schedule &&
              !timing.AdvanceRules(2099) && timing.AdvanceRules(2100) && timing.rule_slot() == -1,
              "Visible rules switch immediately and hide after the leave delay");
        timing.Hover(3, 2200);
        timing.ClearRules();
        Check(!timing.AdvanceRules(2700), "Clearing rules cancels queued timer messages");
        using Phase = OfflineLobbyTiming::CertificatePhase;
        timing.ResetCertificate(0);
        Check(timing.CertificateFrame(0) == std::pair<float, float>{1.f, .01f} &&
              timing.CertificateStatus(1979) == Phase::Opening && timing.CertificateStatus(1980) == Phase::Ready,
              "Certification fade gates its OK button");
        Check(timing.CloseCertificate(2000) && !timing.CloseCertificate(2001) &&
              timing.CertificateFrame(2020) == std::pair<float, float>{.8f, .8f} &&
              timing.CertificateStatus(2419) == Phase::Closing && timing.CertificateStatus(2420) == Phase::Finished,
              "Certification closes using stepped zoom and opacity");

        OfflineLobby lobby(FixtureProfile());
        constexpr unsigned base[]{7, 15, 1, 9, 0x41, 0x49, 0x1041, 0x1049, 0x11, 0x19, 0x51, 0x59};
        constexpr unsigned mask[]{0, 0x80, 0x20, 0xA0};
        for (unsigned p = 0; p < 4; ++p) {
            const unsigned index[]{16 * p + 3, 16 * p + 7, 16 * p, 16 * p + 4, 16 * p + 8, 16 * p + 12,
                                   16 * p + 9, 16 * p + 13, 64 + 2 * p, 65 + 2 * p, 72 + 2 * p, 73 + 2 * p};
            for (unsigned i = 0; i < kRankedSlots; ++i) {
                const auto candidate = lobby.Candidate(static_cast<RankedTier>(p), i);
                Check(candidate.type == (base[i] | mask[p]) && candidate.count_slot == index[i], "48 ranked slots");
            }
        }
        Check(RankedEligible(RankedTier::General, {13, 1799}, 0) &&
              !RankedEligible(RankedTier::General, {13, 1800}, 0), "General rank boundary");
        Check(!RankedEligible(RankedTier::Advanced, {8, 1500}, 59) &&
              RankedEligible(RankedTier::Advanced, {8, 1500}, 60), "Advanced membership threshold");
        Check(RankedEligible(RankedTier::Advanced, {16, 1999}, 0) &&
              !RankedEligible(RankedTier::Advanced, {16, 2000}, 90), "Advanced excludes phoenix");
        Check(!RankedEligible(RankedTier::Special, {12, 1800}, 90) &&
              RankedEligible(RankedTier::Special, {13, 1800}, 0), "Special rank boundary");
        Check(!RankedEligible(RankedTier::Phoenix, {15, 2000}, 90) &&
              RankedEligible(RankedTier::Phoenix, {16, 2000}, 0), "Phoenix rank boundary");
        Check(lobby.Candidate(RankedTier::General, 6).visible && !lobby.Candidate(RankedTier::Advanced, 6).visible,
              "Light option is per tier");
        Check(!lobby.Reserve(RankedTier::General, 8) && !lobby.Reserve(RankedTier::Special, 2),
              "Four- and three-player eligibility are independent");
        Check(lobby.Reserve(RankedTier::General, 2) && !lobby.Reserve(RankedTier::General, 2) &&
              lobby.Reserve(RankedTier::Special, 9) && lobby.reservations() == 2, "Cross-page unique reservations");
        Check(lobby.Candidate(RankedTier::General, 2).reserved && !lobby.Candidate(RankedTier::Advanced, 2).reserved &&
              !lobby.RequestMove("1234"), "Ranked type scopes reservations and locks lobby moves");
        lobby.Cancel();
        Check(!lobby.reservations() && !lobby.Candidate(RankedTier::Special, 9).reserved, "Cancel clears every tier");

        Check(LobbyNumber("1234") == "L1234" && LobbyNumber("C00112233") == "C00112233" &&
              !LobbyNumber("C112233") && !LobbyNumber("L0124") && !LobbyNumber("123abc"), "Room number grammar");
        Check(lobby.RequestMove("1234") && lobby.room() == kRankedLobby &&
              !lobby.ConfirmMove(kBroadcastLobby) && !lobby.Reserve(RankedTier::General, 2),
              "Pending room leaves current room intact");
        Check(lobby.ConfirmMove("L1234") && !lobby.SetCounts(kRankedLobby, FixtureCounts(kRankedLobby, 1)) &&
              lobby.SetCounts("L1234", FixtureCounts("L1234", 1)), "Confirmation and room-scoped counts");
        Check(lobby.Candidate(RankedTier::General, 8).visible && !lobby.Candidate(RankedTier::General, 6).visible,
              "Private room slots");
        Check(lobby.Reserve(RankedTier::General, 2), "Private room reservation");
        lobby.Cancel();

        lobby.SetContest(kEventLobbies[0], FixtureContest(0, 100));
        Check(lobby.RequestMove(kEventLobbies[0]) && !lobby.Contest(), "Configuration does not confirm a room");
        Check(lobby.ConfirmMove(kEventLobbies[0]) && lobby.ContestAction(100) == ContestStatus::Register &&
              !lobby.ReserveContest(100), "Registration required");
        Check(lobby.RegisterContest(100) && !lobby.RegisterContest(100) &&
              lobby.ReserveContest(100) && lobby.ContestAction(100) == ContestStatus::Reserved, "Contest reservation");
        Check(lobby.ContestAction(400) == ContestStatus::Closed && lobby.reservations() == 1,
              "Deadline closes the contest while its reservation can still be cancelled");
        lobby.Cancel();
        Check(lobby.ContestAction(399) == ContestStatus::Open && lobby.ContestAction(400) == ContestStatus::Closed &&
              !lobby.ReserveContest(400), "Contest end is exclusive");
        Check(lobby.RequestMove(kEventLobbies[1]) && lobby.ConfirmMove(kEventLobbies[1]) &&
              lobby.ContestAction(200) == ContestStatus::Loading, "Contest can be confirmed before configuration");
        lobby.SetContest(kEventLobbies[1], FixtureContest(1, 200));
        Check(lobby.ContestAction(209) == ContestStatus::BeforeStart && lobby.ContestAction(210) == ContestStatus::Open,
              "Contest start is inclusive");
        Check(lobby.RequestMove(kEventLobbies[2]) && lobby.ConfirmMove(kEventLobbies[2]), "Third contest confirmation");
        lobby.SetContest(kEventLobbies[2], FixtureContest(2, 210));
        Check(lobby.ContestAction(211) == ContestStatus::Blocked && !lobby.RegisterContest(211), "Contest blocked");
        ServiceResponses();
        OfficialProfileAndServerErrorChecks();
        std::cout << "Offline lobby checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
