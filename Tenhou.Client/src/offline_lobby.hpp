#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tenhou {
    enum class RankedTier : unsigned { General, Advanced, Special, Phoenix };

    struct LobbyCandidate {
        unsigned type;
        unsigned count_slot;
        bool visible;
        bool reserved;
    };

    struct LobbyRank {
        int code, rating;
    };

    struct LobbyStatistics {
        int games, rating, points, next_points;
        std::array<int, 4> place_per_mille;
        int win_per_mille, deal_in_per_mille, call_per_mille, reach_per_mille, bust_per_mille;
        int average_score_hundredths, average_place_hundredths;
        int hands = 0;
        std::optional<int> average_g_hundredths = std::nullopt;
    };

    inline constexpr unsigned kPrivateThreeProfile = 1, kPrivateFourProfile = 2;
    inline constexpr unsigned kRankedThreeProfile = 3, kRankedFourProfile = 4;
    inline constexpr unsigned kAmberRatingProfile = 690, kAmberGProfile = 691;
    inline constexpr unsigned kDefaultMonthlyProfile = 1;
    inline constexpr std::array<unsigned, 8> kRankedMonthlyProfiles{7, 3, 1, 15, 11, 9, 17, 25};
    inline constexpr std::array<unsigned, 8> kAmberMonthlyProfiles{0x421, 0x221, 0x621, 0x429,
                                                                   0x229, 0x629, 0x631, 0x639};
    inline constexpr std::array<unsigned, 8> kAmberLifetimeProfiles{601, 602, 603, 611, 612, 613, 623, 633};

    unsigned RankedMonthlyProfile(unsigned type, RankedTier tier);
    bool ValidMonthlyProfile(unsigned type);

    struct OfflineLobbyProfile {
        std::wstring name, membership_expiry;
        int membership_days;
        unsigned light_options;
        LobbyRank four, three;
        std::array<LobbyStatistics, 2> lifetime;
        std::map<unsigned, LobbyStatistics> monthly;
        bool ranked_allowed = true;
        std::map<unsigned, LobbyStatistics> other_lifetime;
    };

    struct OfflineLobbyCounts {
        std::array<unsigned, 4> totals{};
        std::array<unsigned, 113> waiting{}, playing{};
    };

    struct OfflineContest {
        std::wstring title;
        unsigned players, rule, fee;
        bool premium_only, registered, blocked;
        std::uint64_t start, end;
    };

    enum class ContestStatus { Loading, Blocked, Register, Unregistered, BeforeStart, Open, Closed, Reserved };

    inline constexpr wchar_t kRankedLobby[] = L"L0000";
    inline constexpr wchar_t kBroadcastLobby[] = L"L8141";
    inline constexpr std::array<const wchar_t *, 3> kEventLobbies{L"C00112233", L"C00223344", L"C00334455"};
    inline constexpr unsigned kRankedSlots = 12;
    inline constexpr unsigned kParlorSlots = 8;

    std::optional<std::wstring> LobbyNumber(std::wstring input);
    bool RankedEligible(RankedTier tier, LobbyRank rank, int membership_days);
    OfflineLobbyProfile FixtureProfile();
    OfflineLobbyCounts FixtureCounts(const std::wstring &room, std::uint64_t seconds);
    OfflineContest FixtureContest(unsigned event, std::uint64_t seconds);

    class OfflineLobby {
    public:
        explicit OfflineLobby(OfflineLobbyProfile account) : profile(std::move(account)) {}

        bool RequestMove(std::wstring room);
        bool ConfirmMove(const std::wstring &room);
        void Enter(std::wstring room);
        void DeclineMove() { pending_.reset(); }
        void SetContest(const std::wstring &room, OfflineContest contest);
        bool SetCounts(const std::wstring &room, OfflineLobbyCounts sample);

        LobbyCandidate Candidate(RankedTier tier, unsigned slot) const;
        bool Reserve(RankedTier tier, unsigned slot);
        const OfflineContest *Contest(const std::wstring &room) const;
        const OfflineContest *Contest() const { return Contest(room_); }
        ContestStatus ContestAction(std::uint64_t now) const;
        bool RegisterContest(std::uint64_t now);
        bool ReserveContest(std::uint64_t now);
        void SetReservations(const std::vector<int> &types);
        void Cancel() { reservations_.clear(); }

        const std::wstring &room() const { return room_; }
        const std::optional<std::wstring> &pending() const { return pending_; }
        bool ranked() const { return room_ == kRankedLobby; }
        bool contest() const { return room_.front() == L'C'; }
        std::size_t reservations() const { return reservations_.size(); }
        bool Reserved(unsigned type) const;
        const OfflineLobbyCounts &counts() const { return counts_; }
        OfflineLobbyProfile profile;

    private:
        std::wstring room_ = kRankedLobby;
        std::optional<std::wstring> pending_;
        OfflineLobbyCounts counts_;
        std::map<std::wstring, OfflineContest> contests_;
        std::set<std::pair<std::wstring, unsigned>> reservations_;
    };
}
