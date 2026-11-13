#pragma once
#include "online_actions.hpp"
#include <windows.h>

namespace tenhou {
    std::string ActionGroup(const std::string &kind);

    struct CandidateTile {
        int tile = -1;
        bool context = false;
    };

    std::vector<CandidateTile> BuildCandidateTiles(const OnlineAction &choice, const OnlineState &game);

    struct ActionPlacement {
        int button = 0;
        RECT rect{};
    };

    // Original 00497c0e..00497cf6: hand anchor, 65/45 offsets, 110 stride,
    // reverse creation order and 1.4 resource scale for the 1440 canvas.
    std::vector<ActionPlacement> BuildActionPlacements(const ActionSnapshot &actions, int width, POINT hand_anchor);

    struct ActionSelection {
        std::uint64_t revision = 0;
        std::string group;
        std::vector<std::size_t> indices;
        std::vector<int> selected;

        void Clear();

        void Sync(const ActionSnapshot &actions);

        void Open(const ActionSnapshot &actions, const std::string &wanted);

        std::size_t UniqueCall(const ActionSnapshot &actions, bool red) const;

        bool HandSelection() const { return group == "chi" || group == "pon" || group == "exchange"; }

        std::size_t Confirm(const ActionSnapshot &actions) const;

        std::vector<int> Selectable(const ActionSnapshot &actions) const;

        std::size_t Pick(const ActionSnapshot &actions, int tile);

        bool Active() const { return !group.empty(); }
    };
} // namespace tenhou
