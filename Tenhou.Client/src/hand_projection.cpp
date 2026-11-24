#include "hand_projection.hpp"
#include <algorithm>
#include <string_view>

namespace tenhou {
    ReplayState HandProjection::Project(const OnlineState &online) {
        auto state = OnlineTableState(online);
        const auto &hand = online.players[0].known_hand;
        const int drawn = online.players[0].drawn_tile;
        const bool new_hand = online.events < hand_events_ ||
                              (online.events != hand_events_ &&
                               (online.last_event == "INIT" || online.last_event == "REINIT"));
        if (new_hand) suit_order_ = {0, 1, 2};
        if (drawn != reordered_draw_) reordered_draw_ = -1;
        if (auto_sort_ || new_hand) {
            hand_order_ = hand;
            reordered_draw_ = -1;
            if (drawn >= 0) {
                const auto it = std::find(hand_order_.begin(), hand_order_.end(), drawn);
                if (it != hand_order_.end()) hand_order_.erase(it);
            }
            if (!online.variant) UpdateOriginSuitOrder(suit_order_, hand_order_);
            SortOriginHand(hand_order_, suit_order_);
            if (drawn >= 0 && std::find(hand.begin(), hand.end(), drawn) != hand.end())
                hand_order_.push_back(drawn);
        } else {
            hand_order_.erase(std::remove_if(hand_order_.begin(), hand_order_.end(), [&](int t) {
                return std::find(hand.begin(), hand.end(), t) == hand.end();
            }), hand_order_.end());
            for (int t: hand)
                if (std::find(hand_order_.begin(), hand_order_.end(), t) == hand_order_.end())
                    hand_order_.push_back(t);
            // Retain the actual visible order, including the separated draw at the end.
            // Otherwise disabling sort would move a retained low draw back into sorted order.
            const auto it = std::find(hand_order_.begin(), hand_order_.end(), drawn);
            if (it != hand_order_.end() && reordered_draw_ < 0) {
                hand_order_.erase(it);
                hand_order_.push_back(drawn);
            }
        }
        hand_events_ = online.events;
        if (!online.players[0].retired) state.players[0].hand = hand_order_;
        // FUN_00488650 reassigns the gap to the final slot after manual insertion.
        // This is a display projection; the authoritative drawn entity is unchanged.
        if (drawn >= 0 && reordered_draw_ == drawn && !hand_order_.empty())
            state.players[0].drawn_tile = hand_order_.back();
        state.draw_end_slots = draw_end_slots_;
        return state;
    }

    bool HandProjection::MoveHandTile(int tile, int target, const OnlineState &online,
                                      ReplayState *before, ReplayState *after) {
        const auto prior = Project(online);
        const auto from = std::find(hand_order_.begin(), hand_order_.end(), tile);
        const auto to = std::find(hand_order_.begin(), hand_order_.end(), target);
        if (from == hand_order_.end() || to == hand_order_.end() || from == to) return false;
        const auto index = to - hand_order_.begin();
        hand_order_.erase(from);
        hand_order_.insert(hand_order_.begin() + index, tile);
        auto_sort_ = false;
        reordered_draw_ = online.players[0].drawn_tile;
        const auto next = Project(online);
        if (before) *before = prior;
        if (after) *after = next;
        return true;
    }

    bool HandProjection::SetAutoSort(bool enabled, const OnlineState &online,
                                     ReplayState *before, ReplayState *after) {
        if (auto_sort_ == enabled) return false;
        const auto prior = Project(online);
        auto_sort_ = enabled;
        const auto next = Project(online);
        if (before) *before = prior;
        if (after) *after = next;
        return true;
    }

    std::pair<int, int> HandProjection::ActionAnchor(const OnlineState &online, const TileGeometryCatalog &geometry) {
        std::pair<int, int> anchor{0, 0};
        const bool red = (online.type & 2) == 0;
        for (const auto &t: BuildTableScene(Project(online), false))
            if (t.direction == 0 && std::string_view(t.role) == "hand") {
                const auto bounds = geometry.Bounds(t, red);
                anchor.first = std::max(anchor.first, bounds.x);
                anchor.second = std::max(anchor.second, bounds.y);
            }
        return anchor;
    }

    int HandProjection::HitTile(CanvasRect bounds, const OnlineState &online, TileGeometryCatalog &geometry,
                                int x, int y, const std::vector<PositionedTile> *animated_scene) {
        if (!geometry.loaded() || !online.initialized) return -1;
        const int w = geometry.width(), h = w * 3 / 4;
        const auto target = FitNativeCanvasBounds(bounds, w);
        if (x < target.left || x >= target.right || y < target.top || y >= target.bottom) return -1;
        const int px = (x - target.left) * w / (target.right - target.left);
        const int py = (y - target.top) * h / (target.bottom - target.top);
        int found = -1;
        const bool red = (online.type & 2) == 0;
        std::vector<PositionedTile> scene;
        if (animated_scene) scene = *animated_scene;
        else {
            for (const auto &t: BuildTableScene(Project(online), false))
                if (t.direction == 0 && std::string_view(t.role) == "hand") {
                    const auto origin = geometry.Origin(t, red);
                    const int ty = origin.second -
                                   (std::find(raised_.begin(), raised_.end(), t.tile) != raised_.end() ? 8 * w / 1024 : 0);
                    scene.push_back({t, origin.first, ty});
                }
        }
        SortTableScene(scene, geometry, red);
        for (const auto &placed: scene) {
            if (placed.flight || placed.tile.direction != 0 || std::string_view(placed.tile.role) != "hand") continue;
            if (geometry.OpaqueAt(placed.tile, red, px - placed.x, py - placed.y))
                found = placed.tile.tile;
        }
        return found;
    }

    HandColor HandProjection::ClassifyTile(const TableTile &tile, bool interactive, bool ended, bool variant,
                                           int hovered, const std::vector<int> &playable,
                                           const std::vector<int> &selected, bool selecting, bool restrict_discards) {
        if (!interactive || ended || tile.tile < 0 || tile.direction != 0 || std::string_view(tile.role) != "hand")
            return HandColor::Normal;
        const bool hover = tile.tile == hovered;
        const bool is_playable = std::find(playable.begin(), playable.end(), tile.tile) != playable.end();
        const bool is_selected = std::find(selected.begin(), selected.end(), tile.tile) != selected.end();
        if (variant) {
            if (is_selected) return HandColor::Exchange;
            if (selecting && is_playable) return HandColor::Hover;
        } else {
            if (is_selected) return hover ? HandColor::SelectedHover : HandColor::Selected;
            if (!is_playable && (selecting || restrict_discards))
                return hover ? HandColor::UnavailableHover : HandColor::Dim;
        }
        return hover ? (is_playable ? HandColor::Hover : HandColor::UnavailableHover) : HandColor::Normal;
    }
} // namespace tenhou
