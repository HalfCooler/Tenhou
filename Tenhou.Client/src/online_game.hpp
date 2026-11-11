#pragma once
#include "replay.hpp"
#include <array>
#include <string>

namespace tenhou {
    struct NetworkMessage;

    struct OnlinePlayer {
        std::string name;
        char gender = '?';
        bool connected = true;
        std::vector<int> known_hand;
        std::vector<int> deal_hand;
        int hidden_count = 0, score = 25000, reach = 0, drawn_tile = -1;
        bool draw_pending = false;
        std::vector<ReplayDiscard> discards;
        std::vector<ReplayMeld> melds;
        std::vector<int> nuki;
    };

    struct OnlineState {
        WallState wall;
        bool started = false, initialized = false, ended = false, complete = false;
        bool awaiting_context = false; // REINIT does not identify the next actor/draw.
        bool furiten = false;
        unsigned type = 0;
        unsigned seat_mask = 15;
        bool open_hands = false;
        unsigned local_action_wait_ms = 0;
        std::vector<int> red_tiles{16, 52, 88};
        int round = 0, honba = 0, sticks = 0, dealer = 0, turn = -1, last_discard_seat = -1;
        std::uint64_t events = 0;
        std::array<OnlinePlayer, 4> players;
        std::vector<int> dora_indicators, winners;
        std::string result, last_event, final_scores;
        SettlementState settlement;
    };

    class OnlineGame {
    public:
        static bool Handles(const std::string &tag);

        void Receive(const NetworkMessage &message); // Failure leaves the state unchanged.
        OnlineState state;

    private:
        void Apply(const NetworkMessage &message);

        void Validate() const;
    };

    // A rendering projection only: unknown entities become -1 back-face tiles.
    ReplayState OnlineTableState(const OnlineState &state);

    std::string OnlineStateJson(const OnlineState &state);
} // namespace tenhou
