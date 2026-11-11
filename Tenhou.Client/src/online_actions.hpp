#pragma once
#include "online_game.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace tenhou {
    struct OnlineAction {
        std::string kind, label;
        std::vector<int> tiles;
        std::vector<std::string> messages;
    };

    struct ActionSnapshot {
        bool enabled = false;
        std::uint64_t revision = 0;
        std::string pending;
        std::vector<OnlineAction> choices;
    };

    // A request controller, never an authority for scores, yaku or hidden tiles.
    class OnlineActions {
    public:
        ActionSnapshot snapshot;

        void Receive(const NetworkMessage &message, const OnlineState &before, const OnlineState &after);

        std::vector<std::string> Submit(std::uint64_t revision, std::size_t index);

    private:
        OnlineAction pending_;

        void Offer(const OnlineState &state, unsigned hint);
    };

    std::string ActionStateJson(const ActionSnapshot &state);
} // namespace tenhou
