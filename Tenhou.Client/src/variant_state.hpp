#pragma once
#include "online_actions.hpp"
#include <map>

namespace tenhou {
    enum class VariantPhase { New, Exchange, Missing, Turn, Claims, Ended, Complete };
    struct VariantEvent {
        std::uint64_t seq = 0;
        std::string kind;
        int actor = -1, tile = -1, source = -1;
    };

    struct VariantPlayer {
        std::vector<int> hand, river, flowers, winning;
        std::vector<ReplayMeld> melds;
        int hidden = 0, drawn = -1, missing = -1, score = 0;
        bool retired = false;
    };

    struct VariantState {
        bool initialized = false;
        VariantPhase phase = VariantPhase::New;
        int local = 0, turn = -1, dealer = 0, hand_number = 0, total_hands = 0;
        int wall = 0, wall_total = 108, wall_front = 0, wall_tail = 0, wall_offset = 0;
        int last_actor = -1, last_tile = -1;
        std::uint64_t seq = 0;
        std::uint64_t event_seq = 0;
        std::vector<VariantEvent> events;
        SettlementState settlement;
        std::string rule, direction, result, history;
        std::array<VariantPlayer, 4> players;
        int Relative(int absolute) const { return (absolute - local + 4) % 4; }
        bool Preparing() const { return phase == VariantPhase::Exchange || phase == VariantPhase::Missing; }
        bool Sichuan() const { return rule == "sichuan-bloodbattle"; }
    };

    struct VariantEventCursor {
        std::uint64_t applied = 0;
        bool aligned = false, resynchronized = false;
        std::vector<VariantEvent> Consume(const VariantState &state, bool recovering);
    };

    // Parsing is atomic at the LobbySession boundary. Malformed offers never reach input or painting.
    VariantState ParseVariantState(const std::map<std::string, std::string> &fields);
    std::vector<OnlineAction> ParseVariantActions(const std::string &encoded, const std::string &table,
                                                 std::uint64_t window, int tile_limit);
    OnlineState VariantTableState(const VariantState &state, const std::map<std::string, std::string> &identity);
}
