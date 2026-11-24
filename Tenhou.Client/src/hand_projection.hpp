#pragma once
#include "online_game.hpp"
#include "tile_geometry.hpp"
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace tenhou {
    enum class HandColor : unsigned {
        Normal,
        Hover,
        UnavailableHover,
        Selected,
        Dim,
        SelectedHover,
        Exchange
    };

    // Pure C++ hand ordering, manual drag insertion, hit-testing, and tile highlight projection.
    class HandProjection {
    public:
        ReplayState Project(const OnlineState &online);

        bool MoveHandTile(int tile, int target, const OnlineState &online,
                          ReplayState *before = nullptr, ReplayState *after = nullptr);

        bool SetAutoSort(bool enabled, const OnlineState &online,
                         ReplayState *before = nullptr, ReplayState *after = nullptr);

        bool auto_sort() const { return auto_sort_; }

        void SetRaised(std::vector<int> tiles) { raised_ = std::move(tiles); }
        const std::vector<int> &raised() const { return raised_; }
        void ClearRaised() { raised_.clear(); }

        void SetDrawEndSlots(unsigned slots) { draw_end_slots_ = slots; }
        unsigned draw_end_slots() const { return draw_end_slots_; }

        std::pair<int, int> ActionAnchor(const OnlineState &online, const TileGeometryCatalog &geometry);

        int HitTile(CanvasRect bounds, const OnlineState &online, TileGeometryCatalog &geometry,
                    int x, int y, const std::vector<PositionedTile> *animated_scene = nullptr);

        static HandColor ClassifyTile(const TableTile &tile, bool interactive, bool ended, bool variant,
                                      int hovered, const std::vector<int> &playable,
                                      const std::vector<int> &selected, bool selecting, bool restrict_discards);

    private:
        std::vector<int> hand_order_;
        std::array<int, 3> suit_order_{0, 1, 2};
        std::vector<int> raised_;
        std::uint64_t hand_events_ = 0;
        int reordered_draw_ = -1;
        bool auto_sort_ = true;
        unsigned draw_end_slots_ = 14;
    };
} // namespace tenhou
