#include "offline_lobby.hpp"
#include "text_catalog.hpp"
#include <algorithm>
#include <cmath>
#include <cwchar>
#include <iterator>
#include <numeric>

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

        using Label = TextLabelKey;
        constexpr Label kTiers[]{Label::lobby_general, Label::lobby_advanced, Label::lobby_special, Label::lobby_phoenix};
        constexpr Label kParlorRows[]{Label::lobby_parlor_0, Label::lobby_parlor_2, Label::lobby_parlor_5,
                                      Label::lobby_parlor_three};
        const wchar_t *Text(Label label) { return kTextLabels[static_cast<std::size_t>(label)].label.data(); }

        struct ProfileChoice {
            unsigned type;
            Label label;
        };
        constexpr ProfileChoice kLifetimeChoices[]{
            {kRankedFourProfile, Label::lobby_lifetime_four}, {kRankedThreeProfile, Label::lobby_lifetime_three},
            {kPrivateFourProfile, Label::lobby_private_four}, {kPrivateThreeProfile, Label::lobby_private_three},
            {kAmberRatingProfile, Label::lobby_amber_rating}, {kAmberGProfile, Label::lobby_amber_g}
        };
        constexpr Label kRankedMonthlyRules[]{
            Label::lobby_no_tanyao, Label::lobby_four_tanyao, Label::lobby_red, Label::lobby_three_red
        };
        std::wstring ProfileCaption(Label period, const std::wstring &category) {
            return L"■" + std::wstring(Text(period)) + L" / " + category;
        }

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

    std::wstring AmberProfileLabel(std::size_t index) {
        const auto length = (index < 3 || index == 6) ? Label::lobby_east : Label::lobby_south;
        return std::wstring(Text(length)) + L" " + Text(kParlorRows[index < 6 ? index % 3 : 3]);
    }

    std::wstring LifetimeProfileLabel(unsigned type) {
        for (const auto &choice: kLifetimeChoices)
            if (type == choice.type) return Text(choice.label);
        const auto it = std::find(kAmberLifetimeProfiles.begin(), kAmberLifetimeProfiles.end(), type);
        if (it != kAmberLifetimeProfiles.end())
            return std::wstring(Text(Label::lobby_amber)) + L" " + AmberProfileLabel(it - kAmberLifetimeProfiles.begin());
        return {};
    }

    std::wstring RankedMonthlyLabel(unsigned type) {
        const auto length = (type & 8) ? Label::lobby_south : Label::lobby_east;
        const auto rule = (type & 0x10) ? 3 : (type & 4) ? 0 : (type & 2) ? 1 : 2;
        return std::wstring(Text(length)) + L" " + Text(kRankedMonthlyRules[rule]);
    }

    std::wstring MonthlyProfileCaption(unsigned type) {
        const auto amber = std::find(kAmberMonthlyProfiles.begin(), kAmberMonthlyProfiles.end(), type);
        if (amber != kAmberMonthlyProfiles.end())
            return ProfileCaption(Label::lobby_month, std::wstring(Text(Label::lobby_amber)) + L" " +
                                  AmberProfileLabel(amber - kAmberMonthlyProfiles.begin()));
        const unsigned tier = ((type & 0x80) ? 1 : 0) + ((type & 0x20) ? 2 : 0);
        return ProfileCaption(Label::lobby_month, std::wstring(Text(kTiers[tier])) + L"桌 " + RankedMonthlyLabel(type));
    }

    std::wstring LifetimeProfileCaption(unsigned type) {
        return ProfileCaption(Label::lobby_lifetime, LifetimeProfileLabel(type));
    }

    // Exact FUN_00434290 (tenhou.exe.c:34390): %f -> split '.' -> truncate/pad frac -> optional strip leading '0' & '+' prefix.
    std::wstring FormatFixedTrunc(double value, int decimals, bool strip_zero, const wchar_t *plus_prefix) {
        const float f = static_cast<float>(value);
        wchar_t buffer[128]{};
        std::swprintf(buffer, std::size(buffer), L"%f", static_cast<double>(f));
        std::wstring text(buffer);
        std::wstring integer_part, frac_part;
        const auto dot = text.find(L'.');
        if (dot == std::wstring::npos) integer_part = text;
        else {
            integer_part = text.substr(0, dot);
            frac_part = text.substr(dot + 1);
        }
        if (strip_zero) {
            if (integer_part == L"0") integer_part.clear();
            else if (integer_part == L"-0") integer_part = L"-";
        }
        if (decimals <= 0) {
            frac_part.clear();
        } else {
            if (frac_part.size() > static_cast<std::size_t>(decimals))
                frac_part.resize(static_cast<std::size_t>(decimals));
            while (frac_part.size() < static_cast<std::size_t>(decimals))
                frac_part.push_back(L'0');
        }
        std::wstring prefix;
        if (plus_prefix && f > 0.0f) prefix = plus_prefix;
        if (decimals <= 0) return prefix + integer_part;
        return prefix + integer_part + L"." + frac_part;
    }

    // Exact FUN_00434650 (tenhou.exe.c:34532): signed adaptive decimals (|int|<10 -> 2, <100 -> 1, >=100 -> 0, capped).
    std::wstring FormatSignedAdaptive(double value, int max_decimals) {
        const int abs_int = std::abs(static_cast<int>(static_cast<float>(value)));
        int decimals = abs_int >= 100 ? 0 : abs_int >= 10 ? 1 : 2;
        if (decimals > max_decimals) decimals = max_decimals;
        return FormatFixedTrunc(value, decimals, false, L"+");
    }

    // Exact FUN_004346b0 (tenhou.exe.c:34557): rate without leading zero (e.g. .279, .000, 1.00).
    std::wstring FormatRate(double value, int decimals) {
        const float f = static_cast<float>(value);
        return FormatFixedTrunc(f, decimals - (f == 1.0f ? 1 : 0), true, nullptr);
    }

    // Exact 0042a978..0042a9d0: YYYYMMDD -> YYYY/MM/DD, empty -> ----/--/--.
    std::wstring FormatExpiry(std::wstring_view raw) {
        if (raw.empty()) return L"----/--/--";
        if (raw.size() == 8 && std::all_of(raw.begin(), raw.end(), [](wchar_t c) { return c >= L'0' && c <= L'9'; }))
            return std::wstring(raw.substr(0, 4)) + L"/" + std::wstring(raw.substr(4, 2)) + L"/" +
                   std::wstring(raw.substr(6, 2));
        return std::wstring(raw);
    }

    // Exact FUN_004748b0 (0..20 rank grade names in Chinese).
    std::wstring RankGradeName(int code) {
        static constexpr const wchar_t *kGrades[]{
            L"新人", L"9级", L"8级", L"7级", L"6级", L"5级", L"4级", L"3级", L"2级", L"1级",
            L"初段", L"二段", L"三段", L"四段", L"五段", L"六段", L"七段", L"八段", L"九段", L"十段", L"天凤位"
        };
        if (code >= 0 && code < static_cast<int>(std::size(kGrades))) return kGrades[code];
        return code > 20 ? L"天凤位" : L"新人";
    }

    int MonthlyCorrespondingRating(const OfflineLobbyProfile &profile, unsigned monthly_type) {
        if ((monthly_type & 0x600) != 0) {
            const auto it = std::find(kAmberMonthlyProfiles.begin(), kAmberMonthlyProfiles.end(), monthly_type);
            if (it != kAmberMonthlyProfiles.end()) {
                const auto life_id = kAmberLifetimeProfiles[static_cast<std::size_t>(it - kAmberMonthlyProfiles.begin())];
                const auto found = profile.other_lifetime.find(life_id);
                return found == profile.other_lifetime.end() ? 0 : found->second.rating;
            }
            return 0;
        }
        return (monthly_type & 0x10) ? profile.three.rating : profile.four.rating;
    }

    std::wstring FormatProfileClipboardText(const OfflineLobbyProfile &profile,
                                           const std::array<LobbyStatistics, 2> &private_lifetime,
                                           unsigned lifetime, unsigned monthly, bool official,
                                           std::wstring_view timestamp) {
        std::wstring out;
        out += L"【天凤麻将】\r\n";
        out += L"作成时间: " + std::wstring(timestamp) + L"\r\n";
        out += L"玩家名: " + profile.name + L"\r\n";
        out += std::wstring(Text(Label::lobby_membership_expiry)) + L": " + FormatExpiry(profile.membership_expiry) + L"\r\n\r\n";

        const bool ranked = lifetime == kRankedFourProfile || lifetime == kRankedThreeProfile;
        const bool private_room = lifetime == kPrivateFourProfile || lifetime == kPrivateThreeProfile;
        const bool three = lifetime == kRankedThreeProfile || lifetime == kPrivateThreeProfile ||
                           lifetime == kAmberLifetimeProfiles[6] || lifetime == kAmberLifetimeProfiles[7];
        const LobbyStatistics empty{};
        const auto extra = profile.other_lifetime.find(lifetime);
        const auto &all = ranked ? profile.lifetime[three] : private_room ? private_lifetime[three] :
                          extra != profile.other_lifetime.end() ? extra->second : empty;
        out += ProfileCaption(Label::lobby_lifetime, LifetimeProfileLabel(lifetime)) + L"\r\n";
        if (ranked && profile.ranked_allowed) {
            const auto &rank = three ? profile.three : profile.four;
            const auto target = official && !all.next_points ? std::wstring{} : L"/" + std::to_wstring(all.next_points);
            out += RankGradeName(rank.code) + L" " + std::to_wstring(all.points) + target +
                   L"pt R" + std::to_wstring(rank.rating) + L"\r\n";
        }
        if (all.games > 0) {
            out += std::wstring(Text(Label::lobby_games)) + L": " + std::to_wstring(all.games) + L"\r\n";
            out += std::wstring(Text(Label::lobby_average_score)) + L": " +
                   FormatSignedAdaptive(static_cast<double>(all.average_score_hundredths) / 100.0, 2) + L"\r\n";
            out += std::wstring(Text(Label::lobby_average_place)) + L": " +
                   FormatFixedTrunc(static_cast<double>(all.average_place_hundredths) / 100.0, 2, false, nullptr) + L"\r\n";
            constexpr Label places[]{Label::lobby_place_1, Label::lobby_place_2, Label::lobby_place_3, Label::lobby_place_4};
            const auto raw_games = std::accumulate(all.place_counts.begin(), all.place_counts.end(), 0);
            for (int i = 0; i < (three ? 3 : 4); ++i) {
                const double rate = raw_games == all.games
                    ? static_cast<double>(all.place_counts[i]) / static_cast<double>(all.games)
                    : static_cast<double>(all.place_per_mille[i]) / 1000.0;
                out += std::wstring(Text(places[i])) + L": " + FormatRate(rate, 3) + L"\r\n";
            }
            const double bust = raw_games == all.games && all.bust_count > 0
                ? static_cast<double>(all.bust_count) / static_cast<double>(all.games)
                : static_cast<double>(all.bust_per_mille) / 1000.0;
            out += std::wstring(Text(Label::lobby_bust)) + L": " + FormatRate(bust, 3) + L"\r\n";
            if (all.hands > 0) {
                const auto hand_rate = [&](int count, int permille) {
                    return count > 0 || all.win_count > 0 || all.call_count > 0
                        ? static_cast<double>(count) / static_cast<double>(all.hands)
                        : static_cast<double>(permille) / 1000.0;
                };
                out += std::wstring(Text(Label::lobby_wins)) + L": " + FormatRate(hand_rate(all.win_count, all.win_per_mille), 3) + L"\r\n";
                out += std::wstring(Text(Label::lobby_deal_in)) + L": " + FormatRate(hand_rate(all.deal_in_count, all.deal_in_per_mille), 3) + L"\r\n";
                out += std::wstring(Text(Label::lobby_call)) + L": " + FormatRate(hand_rate(all.call_count, all.call_per_mille), 3) + L"\r\n";
                out += std::wstring(Text(Label::lobby_reach)) + L": " + FormatRate(hand_rate(all.reach_count, all.reach_per_mille), 3) + L"\r\n";
            }
        }
        out += L"\r\n" + MonthlyProfileCaption(monthly) + L"\r\n";
        const auto month = profile.monthly.find(monthly);
        if (month != profile.monthly.end() && month->second.games > 0) {
            const auto &m = month->second;
            const bool m_three = (monthly & 0x10) != 0;
            const auto &p = m.place_counts;
            if (m_three) {
                out += std::wstring(Text(Label::lobby_games)) + L": " + std::to_wstring(p[0]) + L"+" +
                       std::to_wstring(p[1]) + L"+" + std::to_wstring(p[2]) + L" = " + std::to_wstring(m.games) + L"\r\n";
            } else {
                out += std::wstring(Text(Label::lobby_games)) + L": " + std::to_wstring(p[0]) + L"+" +
                       std::to_wstring(p[1]) + L"+" + std::to_wstring(p[2]) + L"+" + std::to_wstring(p[3]) +
                       L" = " + std::to_wstring(m.games) + L"\r\n";
            }
            const auto rank_suffix = [&](std::size_t idx) -> std::wstring {
                if (idx < m.monthly_ranks.size() && m.monthly_ranks[idx] > 0)
                    return L" " + std::to_wstring(m.monthly_ranks[idx]) + Text(Label::lobby_rank_unit);
                return {};
            };
            const double score = m.has_monthly_raw ? m.total_score : (static_cast<double>(m.average_score_hundredths) * m.games / 100.0);
            out += std::wstring(Text(Label::lobby_monthly_total)) + Text(Label::lobby_monthly_score) + L": " +
                   FormatSignedAdaptive(score, 1) + rank_suffix(1) + L"\r\n";
            out += std::wstring(Text(Label::lobby_monthly_average)) + Text(Label::lobby_monthly_score) + L": " +
                   FormatSignedAdaptive(score / m.games, 2) + rank_suffix(2) + L"\r\n";
            const double uma = m_three ? (p[0] * 30.0 - p[2] * 30.0) : (p[0] * 30.0 + p[1] * 10.0 - p[2] * 10.0 - p[3] * 30.0);
            const double avg_place = static_cast<double>(p[0] + p[1] * 2 + p[2] * 3 + p[3] * 4) / m.games;
            out += std::wstring(Text(Label::lobby_monthly_total)) + Text(Label::lobby_monthly_place) + L": " +
                   FormatFixedTrunc(uma, 0, false, L"+") + rank_suffix(3) + L"\r\n";
            out += std::wstring(Text(Label::lobby_monthly_average)) + Text(Label::lobby_monthly_place) + L": " +
                   FormatFixedTrunc(avg_place, 2, false, nullptr) + rank_suffix(4) + L"\r\n";
        }
        return out;
    }

    OfflineLobbyTiming::HoverEffect OfflineLobbyTiming::Hover(int slot, std::uint64_t now) {
        if (hovered_ == slot) return HoverEffect::None;
        hovered_ = slot;
        rules_at_.reset();
        if (slot >= 0 && rule_slot_ >= 0) {
            rule_slot_ = slot;
            return HoverEffect::Redraw;
        }
        if (slot < 0 && rule_slot_ < 0) return HoverEffect::None;
        rules_at_ = now + kRulesDelay;
        return HoverEffect::Schedule;
    }

    void OfflineLobbyTiming::ClearRules() {
        rules_at_.reset();
        hovered_ = rule_slot_ = -1;
    }

    bool OfflineLobbyTiming::AdvanceRules(std::uint64_t now) {
        if (!rules_at_ || now < *rules_at_) return false;
        rules_at_.reset();
        rule_slot_ = hovered_;
        return true;
    }

    void OfflineLobbyTiming::ResetCertificate(std::uint64_t now) {
        certificate_start_ = now;
        certificate_close_.reset();
    }

    bool OfflineLobbyTiming::CloseCertificate(std::uint64_t now) {
        if (certificate_close_) return false;
        certificate_close_ = now;
        return true;
    }

    OfflineLobbyTiming::CertificatePhase OfflineLobbyTiming::CertificateStatus(std::uint64_t now) const {
        if (certificate_close_)
            return now - *certificate_close_ >= 420 ? CertificatePhase::Finished : CertificatePhase::Closing;
        return now - certificate_start_ >= 1980 ? CertificatePhase::Ready : CertificatePhase::Opening;
    }

    std::pair<float, float> OfflineLobbyTiming::CertificateFrame(std::uint64_t now) const {
        if (certificate_close_) {
            const float zoom = static_cast<float>(std::pow(.8, (now - *certificate_close_) / kCertificateTick));
            return {zoom, zoom};
        }
        return {1.f, std::min(1.f, (1 + (now - certificate_start_) / kCertificateTick) * .01f)};
    }
}
