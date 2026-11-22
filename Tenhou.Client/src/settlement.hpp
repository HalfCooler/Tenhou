#pragma once
#include "text_catalog.hpp"
#include <array>
#include <map>
#include <string>
#include <vector>

namespace tenhou {
    struct ReplayState;

    struct SettlementYaku {
        int id = 0, han = 0;
    };
    struct SettlementEntry {
        std::string name;
        int value = 0, count = 1;
    };
    SettlementEntry ParseSettlementEntry(const std::string &text);
    struct SettlementDisplayEntry {
        std::string name;
        const RulePatternDefinition *pattern;
        int count;
        long long total_value;
    };
    SettlementDisplayEntry ResolveSettlementEntry(const SettlementEntry &entry, std::string_view rule);
    std::string SettlementEntryLabel(const SettlementEntry &entry, const std::string &rule = {});
    struct SettlementMeld {
        std::string kind;
        std::vector<int> tiles;
        int source = -1, called = -1, added = -1;
    };

    // Immutable display receipts. These never change live hand visibility or pay scores.
    struct SettlementReceipt {
        bool generic = false;
        std::string rule;
        std::vector<int> flowers;
        int fan = 0;
        int points = 0; // Server-supplied displayed payout; delta retains all actual transfers.
        std::vector<SettlementEntry> entries;
        std::vector<SettlementMeld> public_melds;
        unsigned seat_mask = 15;
        std::string tag, reason;
        int who = -1, from = -1, winning_tile = -1;
        bool has_hand = false, has_melds = false, has_yaku = false, has_yakuman = false, has_ten = false;
        std::vector<int> hand, melds, dora, ura, yakuman, ten, ba;
        std::vector<SettlementYaku> yaku;
        std::vector<SettlementYaku> chip_yaku;
        std::array<std::vector<int>, 4> revealed;
        std::array<bool, 4> has_revealed{};
        std::array<int, 4> base{}, delta{}; // points, converted once from sc's hundreds.
        bool has_chips = false;
        std::array<int, 4> chip_base{}, chip_delta{};
    };

    struct SettlementState {
        std::vector<int> red_tiles{16, 52, 88};
        unsigned seat_mask = 15;
        std::vector<SettlementReceipt> receipts;
        bool has_final = false;
        std::array<double, 4> final_scores{}, final_points{};
        bool has_final_chips = false;
        std::array<int, 4> final_chips{};
        bool has_ratio = false;
        std::array<int, 2> ratio{};
    };

    // Keep immediate transfers (e.g. Sichuan kongs), hiding only receipt payments.
    std::array<int, 4> ScoresBeforeSettlement(std::array<int, 4> scores, const SettlementState &settlement);

    void AppendSettlement(SettlementState &state, const std::string &tag,
                          const std::map<std::string, std::string> &attributes, unsigned seat_mask = 15);

    void ValidateSettlementDisclosure(const SettlementReceipt &receipt, const ReplayState &table);

    std::string SettlementJson(const SettlementState &state);

    std::string SettlementText(const SettlementState &state, std::size_t selection,
                               const std::array<std::string, 4> &names, bool red = true, bool totals = true);

    std::string SettlementLabel(const SettlementReceipt &receipt);

    std::string SettlementYakuName(int id);
    std::optional<TextKey> SettlementYakuKey(int id);

    // Presentation order and the zero-hit ura row never alter the receipt or score.
    std::vector<SettlementYaku> SettlementDisplayYaku(const SettlementReceipt &receipt);
} // namespace tenhou
