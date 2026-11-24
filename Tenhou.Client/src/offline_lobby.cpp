#include "offline_lobby.hpp"
#include <algorithm>
#include <iterator>

namespace tenhou {
    namespace {
        struct RankedSlot {
            unsigned type, count_base, tier_stride;
        };
        // 0x43F590 (type) and 0x428DD2 (LN index), in control ID order.
        constexpr RankedSlot kSlots[kRankedSlots]{
            {0x7, 3, 16}, {0xF, 7, 16}, {0x1, 0, 16}, {0x9, 4, 16},
            {0x41, 8, 16}, {0x49, 12, 16}, {0x1041, 9, 16}, {0x1049, 13, 16},
            {0x11, 64, 2}, {0x19, 65, 2}, {0x51, 72, 2}, {0x59, 73, 2}
        };
        constexpr unsigned kTierMask[]{0x00, 0x80, 0x20, 0xA0};

        bool Digits(std::wstring_view text) {
            return std::all_of(text.begin(), text.end(), [](wchar_t c) { return c >= L'0' && c <= L'9'; });
        }
    }

    std::optional<std::wstring> LobbyNumber(std::wstring input) {
        if (input == L"0" || input == L"0000" || input == kRankedLobby) return kRankedLobby;
        if (input.size() == 4 && Digits(input)) input.insert(input.begin(), L'L');
        if (input.size() == 5 && input.front() == L'L' && input[1] != L'0' &&
            Digits(std::wstring_view(input).substr(1))) return input;
        if (input.size() == 9 && input.front() == L'C' && Digits(std::wstring_view(input).substr(1))) return input;
        return std::nullopt;
    }

    std::wstring LocalizeCertification(std::wstring_view text) {
        constexpr std::wstring_view rank = L"認定段位/級位";
        constexpr std::wstring_view award =
            L"貴殿は天鳳において卓越した技能を遺憾なく発揮され優秀な成績をおさめられました。"
            L"今後もさらなる雀力向上に精進されますようここに段位/級位を認定し栄誉を称えます。";
        const auto award_length = [&](std::wstring_view source) -> std::size_t {
            std::size_t pos = 0;
            for (const wchar_t letter : award) {
                pos = source.find_first_not_of(L" 　\t\n\r", pos);
                if (pos == source.npos || source[pos++] != letter) return 0;
            }
            return pos;
        };
        std::wstring body;
        while (!text.empty()) {
            const auto newline = text.find_first_of(L"\r\n");
            const auto line = text.substr(0, newline);
            const auto first = line.find_first_not_of(L" 　\t");
            const auto last = line.find_last_not_of(L" 　\t");
            const auto content = first == line.npos ? std::wstring_view{} : line.substr(first, last - first + 1);
            // Match whole fields. A player's name can itself contain vocabulary used in the award.
            if (content.size() > 1 && content.back() == L'殿') {
                const auto name = content.substr(0, content.size() - 1);
                body += name.substr(0, name.find_last_not_of(L" 　\t") + 1);
                body += L"阁下：";
            } else if (content.substr(0, rank.size()) == rank) {
                const auto value = content.find_first_not_of(L" ：:\t　", rank.size());
                auto grade = std::wstring(value == content.npos ? std::wstring_view{} : content.substr(value));
                std::replace(grade.begin(), grade.end(), L'級', L'级');
                if (grade == L"天鳳位") grade = L"天凤位";
                body += L"认定段级位：" + grade;
            } else if (const auto length = content.empty() ? 0 : award_length(text)) {
                // The server can hard-wrap this paragraph anywhere within a sentence.
                body += L"您在天凤对局中展现了卓越的牌技，成绩优异。\n"
                        L"特此认定，以表彰您的成就。\n"
                        L"愿您再接再厉，精进牌艺。";
                text.remove_prefix(length);
                continue;
            } else if (content == L"天鳳段位認定協会") body += L"天凤段位认定协会";
            else if (content == L"天鳳サンマ漁業協会") body += L"天凤秋刀鱼渔业协会";
            else body += line;
            text.remove_prefix(line.size());
            if (!text.empty()) {
                const auto separator = text.front();
                text.remove_prefix(1);
                if (separator == L'\r' && !text.empty() && text.front() == L'\n') text.remove_prefix(1);
                body += L'\n';
            }
        }
        return body;
    }

    bool RankedEligible(RankedTier tier, LobbyRank rank, int membership_days) {
        const bool special = rank.code >= 13 && rank.rating >= 1800;
        const bool phoenix = rank.code >= 16 && rank.rating >= 2000;
        switch (tier) {
            case RankedTier::General: return !special;
            case RankedTier::Advanced: return (rank.code >= 9 || membership_days >= 60) && !phoenix;
            case RankedTier::Special: return special;
            case RankedTier::Phoenix: return phoenix;
        }
        return false;
    }

    unsigned RankedMonthlyProfile(unsigned type, RankedTier tier) {
        const auto rule = (type & 0x600) ? (type & 0x8) | 1 : type & 0x1F;
        return rule | kTierMask[static_cast<unsigned>(tier)];
    }

    bool ValidMonthlyProfile(unsigned type) {
        if (std::find(kAmberMonthlyProfiles.begin(), kAmberMonthlyProfiles.end(), type) != kAmberMonthlyProfiles.end())
            return true;
        const auto rule = type & ~0xA0u;
        return std::find(kRankedMonthlyProfiles.begin(), kRankedMonthlyProfiles.end(), rule) != kRankedMonthlyProfiles.end();
    }

    OfflineLobbyProfile FixtureProfile() {
        LobbyStatistics four{153, 1656, 475, 800, {254, 252, 247, 247}, 222, 147, 357, 175, 71, -53, 249, 1300};
        four.place_counts = {39, 39, 38, 37};
        four.total_score = -81.1;
        LobbyStatistics three{98, 1842, 210, 1600, {363, 330, 307, 0}, 289, 135, 302, 223, 83, 215, 194, 820};
        three.place_counts = {36, 32, 30, 0};
        three.total_score = 210.7;
        LobbyStatistics monthly{16, 1668, 0, 0, {312, 250, 250, 188}, 250, 125, 312, 188, 62, 312, 231, 0};
        monthly.place_counts = {5, 4, 4, 3};
        monthly.total_score = 50.0;
        monthly.has_monthly_raw = true;
        monthly.monthly_ranks = {0, 1240, 0, 1180, 0, 0, 0, 0, 18420};
        return {L"OfflineDemo", L"2027/02/17", 90, 0x05, {11, 1656}, {13, 1842},
            {{four, three}}, {{kDefaultMonthlyProfile, monthly}}, true, {}};
    }

    OfflineLobbyCounts FixtureCounts(const std::wstring &room, std::uint64_t seconds) {
        OfflineLobbyCounts counts;
        if (room.empty() || room.front() == L'C') return counts;
        const bool ranked = room == kRankedLobby;
        const auto tick = static_cast<unsigned>(seconds % 32);
        unsigned playing = 0;
        for (unsigned i = 0; i < counts.waiting.size(); ++i) {
            counts.waiting[i] = (i + tick) % (ranked ? 8 : 3);
            counts.playing[i] = (7 * i + 3 * tick) % (ranked ? 151 : 15);
            counts.totals[2] += counts.waiting[i];
            playing += counts.playing[i];
        }
        counts.totals[0] = counts.totals[2] + playing;
        counts.totals[3] = playing / 8;
        return counts;
    }

    OfflineContest FixtureContest(unsigned event, std::uint64_t seconds) {
        // Scenario data only. The three IDs are the original menu destinations.
        switch (event) {
            case 0: return {{}, 4, 0x9, 10, false, false, false, seconds, seconds + 300};
            case 1: return {{}, 3, 0x11, 0, false, true, false, seconds + 10, seconds + 40};
            default: return {{}, 4, 0x1, 0, true, false, true, seconds, seconds + 300};
        }
    }

    bool OfflineLobby::RequestMove(std::wstring room) {
        auto id = LobbyNumber(std::move(room));
        if (!id || pending_ || *id == room_ || !reservations_.empty()) return false;
        pending_ = std::move(id);
        return true;
    }

    bool OfflineLobby::ConfirmMove(const std::wstring &room) {
        if (!pending_ || *pending_ != room) return false;
        Enter(room);
        return true;
    }

    void OfflineLobby::Enter(std::wstring room) {
        room_ = std::move(room);
        pending_.reset();
        counts_ = {};
        Cancel();
    }

    void OfflineLobby::SetReservations(const std::vector<int> &types) {
        Cancel();
        for (int type : types) reservations_.emplace(room_, type);
    }

    void OfflineLobby::SetContest(const std::wstring &room, OfflineContest contest) {
        const auto id = LobbyNumber(room);
        if (id && id->front() == L'C') contests_.insert_or_assign(*id, std::move(contest));
    }

    bool OfflineLobby::SetCounts(const std::wstring &room, OfflineLobbyCounts sample) {
        if (room != room_) return false;
        counts_ = std::move(sample);
        return true;
    }

    bool OfflineLobby::Reserved(unsigned type) const { return reservations_.count({room_, type}) != 0; }

    LobbyCandidate OfflineLobby::Candidate(RankedTier tier, unsigned slot) const {
        const auto page = static_cast<unsigned>(tier);
        if (slot >= std::size(kSlots) || page >= std::size(kTierMask) || contest()) return {};
        const auto &definition = kSlots[slot];
        const bool three = (definition.type & 0x10) != 0;
        const bool light = (definition.type & 0x1000) != 0;
        const bool eligible = !ranked() || (profile.ranked_allowed &&
            RankedEligible(tier, three ? profile.three : profile.four, profile.membership_days));
        const unsigned type = definition.type | (ranked() ? kTierMask[page] : 0);
        const unsigned index = definition.count_base + (ranked() ? page : 0) * definition.tier_stride;
        return {type, index, eligible && (!light || (ranked() && (profile.light_options & (1u << page)))),
                Reserved(type)};
    }

    bool OfflineLobby::Reserve(RankedTier tier, unsigned slot) {
        const auto candidate = Candidate(tier, slot);
        if (pending_ || !candidate.visible || candidate.reserved) return false;
        return reservations_.emplace(room_, candidate.type).second;
    }

    const OfflineContest *OfflineLobby::Contest(const std::wstring &room) const {
        const auto it = contests_.find(room);
        return it == contests_.end() ? nullptr : &it->second;
    }

    ContestStatus OfflineLobby::ContestAction(std::uint64_t now) const {
        const auto *config = Contest();
        if (!config) return ContestStatus::Loading;
        if (now >= config->end) return ContestStatus::Closed;
        if (config->blocked || (config->premium_only && profile.membership_days <= 0)) return ContestStatus::Blocked;
        if (Reserved(config->rule)) return ContestStatus::Reserved;
        if (!config->registered) return config->fee ? ContestStatus::Register : ContestStatus::Unregistered;
        return now < config->start ? ContestStatus::BeforeStart : ContestStatus::Open;
    }

    bool OfflineLobby::RegisterContest(std::uint64_t now) {
        if (pending_ || ContestAction(now) != ContestStatus::Register) return false;
        contests_.at(room_).registered = true;
        return true;
    }

    bool OfflineLobby::ReserveContest(std::uint64_t now) {
        if (pending_ || ContestAction(now) != ContestStatus::Open) return false;
        return reservations_.emplace(room_, Contest()->rule).second;
    }
}
