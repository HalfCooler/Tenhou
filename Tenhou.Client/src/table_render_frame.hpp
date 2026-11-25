#pragma once
#include "action_caption_state.hpp"
#include "hand_projection.hpp"
#include "replay_effects.hpp"
#include "tile_geometry.hpp"
#include "wall_geometry.hpp"
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace tenhou {
    std::optional<TextKey> ActionButtonText(unsigned control, bool hot);

    enum class TableActionButtonStyle : unsigned {
        Sprite440,
        OriginalExit,
        PushButton
    };

    struct TableActionButton {
        int id = 0;
        CanvasRect viewport_rect{};
        bool enabled = true;
        bool hot = false;
        bool pressed = false;
        bool focused = false;
        TableActionButtonStyle style = TableActionButtonStyle::Sprite440;
        std::wstring label;
    };

    // Pure C++ frame description consumed identically by GdiTableRenderer and D2DTableRenderer.
    struct TableRenderFrame {
        int canvas_width = 1440;
        CanvasRect bounds{};
        CanvasRect destination{};
        ReplayState state;
        unsigned game_type = 0;
        bool red_tiles = true;
        bool original_display = true;
        bool interactive = false;
        bool online_mode = false;
        bool match_complete = false;
        std::array<int, 4> raw_scores{};
        std::vector<PositionedTile> tiles;
        std::vector<HandColor> tile_colors;
        const ActionCaptions *captions = nullptr;
        std::optional<TableInteraction> interaction;
        std::vector<TableActionButton> buttons;
        unsigned dpi = 0;
    };

    TableRenderFrame BuildOnlineTableRenderFrame(
        int canvas_width, CanvasRect bounds, const OnlineState &online, HandProjection &hand,
        const TileGeometryCatalog &geometry, const TablePresentation &presentation,
        const OpeningPresentation *opening, const TableInteraction *interaction,
        const ActionCaptions *captions, bool settlement_presented, bool relative_scores,
        bool original_display, bool interactive, double now_anim_ms, unsigned dpi = 0);

    TableRenderFrame BuildReplayTableRenderFrame(
        int canvas_width, CanvasRect bounds, const ReplayState &session_state,
        const TileGeometryCatalog &geometry, const TablePresentation &presentation,
        const OpeningPresentation &opening, const ActionCaptions &captions,
        bool red_tiles, bool original_display, bool animation_enabled, bool relative_scores,
        unsigned draw_end_ticks, double now_anim_ms, unsigned dpi = 0);
} // namespace tenhou
