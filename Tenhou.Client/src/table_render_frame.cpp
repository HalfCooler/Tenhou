#include "table_render_frame.hpp"
#include <algorithm>
#include <string>

namespace tenhou {
    std::optional<TextKey> ActionButtonText(const unsigned control, const bool hot) {
        if (control == 5173) return hot ? TextKey::button_hover_exchange : TextKey::button_normal_exchange;
        constexpr TextKey missing_normal[]{TextKey::button_normal_man, TextKey::button_normal_pin, TextKey::button_normal_sou};
        constexpr TextKey missing_hover[]{TextKey::button_hover_man, TextKey::button_hover_pin, TextKey::button_hover_sou};
        if (control >= 5170 && control <= 5172) return hot ? missing_hover[control - 5170] : missing_normal[control - 5170];
        constexpr TextKey normal[]{TextKey::button_normal_chi, TextKey::button_normal_pon,
            TextKey::button_normal_kan, TextKey::button_normal_riichi, TextKey::button_normal_ron,
            TextKey::button_normal_tsumo, TextKey::button_normal_abort_draw, TextKey::button_normal_pass};
        constexpr TextKey hover[]{TextKey::button_hover_chi, TextKey::button_hover_pon,
            TextKey::button_hover_kan, TextKey::button_hover_riichi, TextKey::button_hover_ron,
            TextKey::button_hover_tsumo, TextKey::button_hover_abort_draw, TextKey::button_hover_pass};
        if (control == 5151) return hot ? TextKey::button_hover_nuki : TextKey::button_normal_nuki;
        if (control < 5121 || control > 5128) return {};
        return hot ? hover[control - 5121] : normal[control - 5121];
    }

    TableRenderFrame BuildOnlineTableRenderFrame(
        const int canvas_width, const CanvasRect bounds, const OnlineState &online, HandProjection &hand,
        const TileGeometryCatalog &geometry, const TablePresentation &presentation,
        const OpeningPresentation *opening, const TableInteraction *interaction,
        const ActionCaptions *captions, const bool settlement_presented, const bool relative_scores,
        const bool original_display, const bool interactive, const double now_anim_ms, const unsigned dpi) {
        TableRenderFrame frame;
        frame.canvas_width = canvas_width;
        frame.bounds = bounds;
        frame.destination = FitNativeCanvasBounds(bounds, canvas_width);
        frame.online_mode = true;
        frame.interactive = interactive;
        frame.original_display = original_display;
        frame.game_type = online.type;
        frame.red_tiles = (online.type & 2) == 0;
        frame.match_complete = online.complete;
        frame.captions = captions;
        frame.dpi = dpi;
        if (interaction) frame.interaction = *interaction;
        for (int seat = 0; seat < 4; ++seat) frame.raw_scores[seat] = online.players[seat].score;

        const bool opening_active = opening && opening->Active();
        frame.state = opening_active ? opening->Frame() : hand.Project(online);
        for (int seat = 0; seat < 4; ++seat)
            frame.state.players[seat].connected = online.players[seat].connected;
        if (!settlement_presented) {
            std::array<int, 4> scores{};
            for (int seat = 0; seat < 4; ++seat) scores[seat] = frame.state.players[seat].score;
            scores = ScoresBeforeSettlement(scores, online.settlement);
            for (int seat = 0; seat < 4; ++seat) frame.state.players[seat].score = scores[seat];
        }
        frame.state.relative_scores = relative_scores;

        const double unit = static_cast<double>(canvas_width) / 1024.0;
        hand.SetRaised(interaction ? interaction->selected : std::vector<int>{});
        const auto &raised = hand.raised();
        const unsigned placed = opening_active ? opening->placed() : static_cast<unsigned>(frame.state.wall.total);

        if (opening_active) frame.tiles = opening->Scene(geometry, frame.red_tiles);
        else if (presentation.Active()) frame.tiles = presentation.Frame(now_anim_ms);
        else {
            for (const auto &tile: BuildTableScene(frame.state, false)) {
                const auto origin = geometry.Origin(tile, frame.red_tiles);
                frame.tiles.push_back({tile, origin.first, origin.second});
            }
        }
        for (auto &tile: frame.tiles) {
            if (tile.tile.direction == 0 && std::string(tile.tile.role) == "hand" &&
                std::find(raised.begin(), raised.end(), tile.tile.tile) != raised.end()) {
                tile.y -= static_cast<int>(8 * unit);
            }
        }
        SortTableScene(frame.tiles, geometry, frame.red_tiles);
        if (original_display) InsertNearWall(frame.tiles, geometry, frame.state, frame.red_tiles, placed, true);
        if (opening_active) opening->SlideWall(frame.tiles);

        frame.tile_colors.reserve(frame.tiles.size());
        for (const auto &tile: frame.tiles) {
            if (!interaction) {
                frame.tile_colors.push_back(HandColor::Normal);
            } else {
                frame.tile_colors.push_back(HandProjection::ClassifyTile(
                    tile.tile, interactive, frame.state.ended, frame.state.variant, interaction->hovered,
                    interaction->playable, interaction->selected, interaction->selecting,
                    interaction->restrict_discards));
            }
        }
        return frame;
    }

    TableRenderFrame BuildReplayTableRenderFrame(
        const int canvas_width, const CanvasRect bounds, const ReplayState &session_state,
        const TileGeometryCatalog &geometry, const TablePresentation &presentation,
        const OpeningPresentation &opening, const ActionCaptions &captions,
        const bool red_tiles, const bool original_display, const bool animation_enabled, const bool relative_scores,
        const unsigned draw_end_ticks, const double now_anim_ms, const unsigned dpi) {
        TableRenderFrame frame;
        frame.canvas_width = canvas_width;
        frame.bounds = bounds;
        frame.destination = FitNativeCanvasBounds(bounds, canvas_width);
        frame.online_mode = false;
        frame.interactive = false;
        frame.original_display = original_display;
        frame.red_tiles = red_tiles;
        frame.captions = &captions;
        frame.dpi = dpi;

        const bool opening_active = opening.Active() && animation_enabled;
        frame.state = opening_active ? opening.Frame() : session_state;
        frame.game_type = frame.state.type;
        frame.state.relative_scores = relative_scores;
        if (draw_end_ticks) frame.state.draw_end_slots = 14 - draw_end_ticks / 2;

        const unsigned placed_count = opening_active ? opening.placed() : 136u;
        if (opening_active) frame.tiles = opening.Scene(geometry, red_tiles);
        else if (presentation.Active()) frame.tiles = presentation.Frame(now_anim_ms);
        else {
            for (const auto &tile: BuildTableScene(frame.state)) {
                const auto origin = geometry.Origin(tile, red_tiles);
                frame.tiles.push_back({tile, origin.first, origin.second});
            }
        }
        SortTableScene(frame.tiles, geometry, red_tiles);
        if (original_display) InsertNearWall(frame.tiles, geometry, frame.state, red_tiles, placed_count, true);
        if (opening_active) opening.SlideWall(frame.tiles);

        frame.tile_colors.assign(frame.tiles.size(), HandColor::Normal);
        return frame;
    }
} // namespace tenhou
