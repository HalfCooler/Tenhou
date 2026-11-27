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
        // Raw counts and monthly ranking fields preserved from original PF* / RANKING v2 wire frames.
        std::array<int, 4> place_counts{};
        int bust_count = 0, win_count = 0, deal_in_count = 0, call_count = 0, reach_count = 0;
        double total_score = 0.0;
        int total_chips = 0;
        bool has_monthly_raw = false;
        bool has_amber_monthly = false;
        double total_income = 0.0;
        std::optional<double> average_income = std::nullopt;
        std::optional<double> average_chips = std::nullopt;
        std::vector<int> monthly_ranks;
    };

    inline constexpr unsigned kPrivateThreeProfile = 1, kPrivateFourProfile = 2;
    inline constexpr unsigned kRankedThreeProfile = 3, kRankedFourProfile = 4;
    inline constexpr unsigned kAmberRatingProfile = 690, kAmberGProfile = 691;
    inline constexpr unsigned kDefaultMonthlyProfile = 1;
    inline constexpr std::array<unsigned, 8> kRankedMonthlyProfiles{7, 3, 1, 15, 11, 9, 17, 25};
    inline constexpr std::array<unsigned, 8> kAmberMonthlyProfiles{0x421, 0x221, 0x621, 0x429,
                                                                   0x229, 0x629, 0x631, 0x639};
    inline constexpr std::array<unsigned, 8> kAmberLifetimeProfiles{601, 602, 603, 611, 612, 613, 623, 633};

    std::string LocalizeCertification(std::string_view text);

    unsigned RankedMonthlyProfile(unsigned type, RankedTier tier);
    bool ValidMonthlyProfile(unsigned type);

    struct OfflineLobbyProfile {
        std::string name, membership_expiry;
        int membership_days;
        unsigned light_options;
        LobbyRank four, three;
        std::array<LobbyStatistics, 2> lifetime;
        std::map<unsigned, LobbyStatistics> monthly;
        bool ranked_allowed = true;
        std::map<unsigned, LobbyStatistics> other_lifetime;
    };

    std::string AmberProfileLabel(std::size_t index);
    std::string LifetimeProfileLabel(unsigned type);
    std::string LifetimeProfileCaption(unsigned type);
    std::string RankedMonthlyLabel(unsigned type);
    std::string MonthlyProfileCaption(unsigned type);
    std::string FormatFixedTrunc(double value, int decimals, bool strip_zero, const char *plus_prefix);
    std::string FormatSignedAdaptive(double value, int max_decimals);
    std::string FormatRate(double value, int decimals = 3);
    std::string FormatExpiry(std::string_view raw);
    std::string RankGradeName(int code);
    int MonthlyCorrespondingRating(const OfflineLobbyProfile &profile, unsigned monthly_type);
    std::string FormatProfileClipboardText(const OfflineLobbyProfile &profile,
                                           const std::array<LobbyStatistics, 2> &private_lifetime,
                                           unsigned lifetime, unsigned monthly, bool official,
                                           std::string_view timestamp);

    class OfflineLobbyTiming {
    public:
        static constexpr unsigned kMoveDelay = 250, kRulesDelay = 500, kCertificateTick = 20;
        enum class HoverEffect { None, Schedule, Redraw };
        enum class CertificatePhase { Opening, Ready, Closing, Finished };

        void BeginMove(const std::uint64_t now) { move_at_ = now + kMoveDelay; }
        bool MoveReady(const std::uint64_t now) const { return now >= move_at_; }

        HoverEffect Hover(int slot, std::uint64_t now);
        void ClearRules();
        bool AdvanceRules(std::uint64_t now);
        int hovered() const { return hovered_; }
        int rule_slot() const { return rule_slot_; }

        void ResetCertificate(std::uint64_t now);
        bool CloseCertificate(std::uint64_t now);
        bool certificate_closing() const { return certificate_close_.has_value(); }
        CertificatePhase CertificateStatus(std::uint64_t now) const;
        std::pair<float, float> CertificateFrame(std::uint64_t now) const;

    private:
        std::uint64_t move_at_ = 0, certificate_start_ = 0;
        std::optional<std::uint64_t> rules_at_, certificate_close_;
        int hovered_ = -1, rule_slot_ = -1;
    };

    struct OfflineLobbyCounts {
        std::array<unsigned, 4> totals{};
        std::array<unsigned, 113> waiting{}, playing{};
    };

    struct OfflineContest {
        std::string title;
        unsigned players, rule, fee;
        bool premium_only, registered, blocked;
        std::uint64_t start, end;
    };

    enum class ContestStatus { Loading, Blocked, Register, Unregistered, BeforeStart, Open, Closed, Reserved };

    inline constexpr char kRankedLobby[] = "L0000";
    inline constexpr char kBroadcastLobby[] = "L8141";
    inline constexpr std::array<const char *, 3> kEventLobbies{"C00112233", "C00223344", "C00334455"};
    inline constexpr unsigned kRankedSlots = 12;
    inline constexpr unsigned kParlorSlots = 8;

    std::optional<std::string> LobbyNumber(std::string input);
    bool RankedEligible(RankedTier tier, LobbyRank rank, int membership_days);
    OfflineLobbyProfile FixtureProfile();
    OfflineLobbyCounts FixtureCounts(const std::string &room, std::uint64_t seconds);
    OfflineContest FixtureContest(unsigned event, std::uint64_t seconds);

    class OfflineLobby {
    public:
        explicit OfflineLobby(OfflineLobbyProfile account) : profile(std::move(account)) {}

        bool RequestMove(std::string room);
        bool ConfirmMove(const std::string &room);
        void Enter(std::string room);
        void DeclineMove() { pending_.reset(); }
        void SetContest(const std::string &room, OfflineContest contest);
        bool SetCounts(const std::string &room, OfflineLobbyCounts sample);

        LobbyCandidate Candidate(RankedTier tier, unsigned slot) const;
        bool Reserve(RankedTier tier, unsigned slot);
        const OfflineContest *Contest(const std::string &room) const;
        const OfflineContest *Contest() const { return Contest(room_); }
        ContestStatus ContestAction(std::uint64_t now) const;
        bool RegisterContest(std::uint64_t now);
        bool ReserveContest(std::uint64_t now);
        void SetReservations(const std::vector<int> &types);
        void Cancel() { reservations_.clear(); }

        const std::string &room() const { return room_; }
        const std::optional<std::string> &pending() const { return pending_; }
        bool ranked() const { return room_ == kRankedLobby; }
        bool contest() const { return !room_.empty() && room_.front() == 'C'; }
        std::size_t reservations() const { return reservations_.size(); }
        bool Reserved(unsigned type) const;
        const OfflineLobbyCounts &counts() const { return counts_; }
        OfflineLobbyProfile profile;

    private:
        std::string room_ = kRankedLobby;
        std::optional<std::string> pending_;
        OfflineLobbyCounts counts_;
        std::map<std::string, OfflineContest> contests_;
        std::set<std::pair<std::string, unsigned>> reservations_;
    };
} // namespace tenhou
