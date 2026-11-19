#include "offline_lobby.hpp"
#include "network_protocol.hpp"
#include <iostream>
#include <stdexcept>

namespace {
    void Check(bool condition, const char *message) {
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
        client.SubmitOriginLobby("change", L"C00112233");
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
        client.SubmitOriginLobby("change", L"L0000");
        accept("<CHAT lobby=\"L0000\" epoch=\"3\" rid=\"2\" />");
        accept("<RESERVATIONS lobby=\"L0000\" epoch=\"1\" types=\"1\" />");
        const std::string old_counts = "<LN lobby=\"L0000\" epoch=\"1\" n=\"7,0,1,0\" j=\"discarded\" g=\"discarded\" />";
        accept(old_counts);
        Check(!s.origin_lobby->reservations() && s.origin_lobby->counts().totals[0] == 0, "Old packets after re-entering the same room");
        client.SubmitOriginLobby("change", L"C00000001");
        accept("<RESERVATIONS lobby=\"L0000\" epoch=\"3\" rid=\"3\" types=\"\" code=\"LOBBY_NOT_FOUND\" />");
        Check(s.phase == LobbyPhase::Lobby && !s.origin_lobby->pending() && s.origin_error == "LOBBY_NOT_FOUND",
              "Rejected move left the lobby blocked");
        client.SubmitOriginLobby("join", L"1");
        accept("<RESERVATIONS lobby=\"L0000\" epoch=\"3\" rid=\"4\" types=\"1\" />");
        client.SubmitOriginLobby("join", L"129");
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
}

int main() {
    using namespace tenhou;
    try {
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
              !lobby.RequestMove(L"1234"), "Ranked type scopes reservations and locks lobby moves");
        lobby.Cancel();
        Check(!lobby.reservations() && !lobby.Candidate(RankedTier::Special, 9).reserved, "Cancel clears every tier");

        Check(LobbyNumber(L"1234") == L"L1234" && LobbyNumber(L"C00112233") == L"C00112233" &&
              !LobbyNumber(L"C112233") && !LobbyNumber(L"L0124") && !LobbyNumber(L"123abc"), "Room number grammar");
        Check(lobby.RequestMove(L"1234") && lobby.room() == kRankedLobby &&
              !lobby.ConfirmMove(kBroadcastLobby) && !lobby.Reserve(RankedTier::General, 2),
              "Pending room leaves current room intact");
        Check(lobby.ConfirmMove(L"L1234") && !lobby.SetCounts(kRankedLobby, FixtureCounts(kRankedLobby, 1)) &&
              lobby.SetCounts(L"L1234", FixtureCounts(L"L1234", 1)), "Confirmation and room-scoped counts");
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
        std::cout << "Offline lobby checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
