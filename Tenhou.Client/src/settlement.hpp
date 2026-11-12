#pragma once
#include <array>
#include <map>
#include <string>
#include <vector>

namespace tenhou {
    struct ReplayState;

    struct SettlementYaku {
        int id = 0, han = 0;
    };

    // Immutable display receipts. These never change live hand visibility or pay scores.
    struct SettlementReceipt {
        unsigned seat_mask = 15;
        std::string tag, reason;
        int who = -1, from = -1, winning_tile = -1;
        bool has_hand = false, has_melds = false, has_yaku = false, has_yakuman = false, has_ten = false;
        std::vector<int> hand, melds, dora, ura, yakuman, ten, ba;
        std::vector<SettlementYaku> yaku;
        std::array<std::vector<int>, 4> revealed;
        std::array<bool, 4> has_revealed{};
        std::array<int, 4> base{}, delta{}; // points, converted once from sc's hundreds.
    };

    struct SettlementState {
        std::vector<int> red_tiles{16, 52, 88};
        unsigned seat_mask = 15;
        std::vector<SettlementReceipt> receipts;
        bool has_final = false;
        std::array<double, 4> final_scores{}, final_points{};
    };

    void AppendSettlement(SettlementState &state, const std::string &tag,
                          const std::map<std::string, std::string> &attributes, unsigned seat_mask = 15);

    void ValidateSettlementDisclosure(const SettlementReceipt &receipt, const ReplayState &table);

    std::string SettlementJson(const SettlementState &state);

    std::string SettlementText(const SettlementState &state, std::size_t selection,
                               const std::array<std::string, 4> &names, bool red = true, bool totals = true);

    std::string SettlementLabel(const SettlementReceipt &receipt);

    std::string SettlementYakuName(int id);

    // Presentation order and the zero-hit ura row never alter the receipt or score.
    std::vector<SettlementYaku> SettlementDisplayYaku(const SettlementReceipt &receipt);
} // namespace tenhou
