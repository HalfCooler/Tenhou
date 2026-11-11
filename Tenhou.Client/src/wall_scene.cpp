#include "wall_scene.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace tenhou {
    std::vector<TableTile> BuildWallScene(const ReplayState &state, unsigned placed) {
        std::vector<TableTile> scene;
        const auto &wall = state.wall;
        if (!state.initialized || !wall.known) return scene;
        std::array < std::array < bool, 34 >, 4 > alive{};
        for (int seat = 0; seat < 4; ++seat)
            for (int slot = 0; slot < 34; ++slot)
                // Preparation builds the four sides simultaneously, one tile per side.
                alive[seat][slot] = static_cast<unsigned>((slot ^ 1) * 4 + seat) < placed;
        const auto remove = [&](int index) {
            const auto p = wall.Position(index);
            alive[p.first][p.second] = false;
        };
        for (int i = 0; i < std::max(wall.front, wall.initial_removed); ++i) remove(i);
        for (int i = 0; i < wall.replacements; ++i) remove((135 - i) ^ 1);
        std::array < std::array < int, 17 >, 4 > indicators{};
        for (auto &seat: indicators) seat.fill(-1);
        for (std::size_t i = 0; i < state.dora_indicators.size() && i < 5; ++i) {
            const auto p = wall.Position(wall.indicator_index - static_cast<int>(i) * 2);
            indicators[p.first][p.second / 2] = state.dora_indicators[i];
        }
        for (int seat = 0; seat < 4; ++seat)
            for (int stack = 0; stack < 17; ++stack) {
                const bool top = alive[seat][stack * 2], bottom = alive[seat][stack * 2 + 1];
                if (!top && !bottom) continue;
                TableTile tile{'y', stack + (top ? 0 : 17), top ? indicators[seat][stack] : -1, seat};
                tile.role = tile.tile < 0 ? "wall" : "wall_indicator";
                // FUN_0049cab0 / FUN_004902f0: split at physical index 120,
                // then move the tail stacks by the literal per-seat PE offsets.
                const auto tail = wall.Position(120);
                if (placed == 136 && tail.second / 2 != 0 && seat == tail.first && stack >= tail.second / 2) {
                    constexpr int dx[]{4, -1, -4, -1}, dy[]{0, -4, 0, 4};
                    tile.dx = dx[seat];
                    tile.dy = dy[seat];
                }
                scene.push_back(tile);
            }
        return scene;
    }

    void InsertNearWall(std::vector<PositionedTile> &scene, TableArtwork &artwork,
                        const ReplayState &state, bool red, unsigned placed, bool include_far) {
        std::vector<PositionedTile> near_wall;
        if (include_far) {
            std::vector<PositionedTile> far_wall;
            for (const auto &tile: BuildWallScene(state, placed))
                if (tile.direction != 0) {
                    const auto &im = artwork.Get(tile, red);
                    const auto offset = TableTileOffset(tile, artwork.width());
                    far_wall.push_back({tile, im.x + offset.x, im.y + offset.y});
                }
            // Order whole stacks before expanding the indicator's lower sprite.
            // Sorting the lower sprite independently can put it above its own face
            // when their bottom edges tie (notably y15/y32 in direction 3).
            SortTableScene(far_wall, artwork, red);
            std::vector<PositionedTile> wall;
            for (const auto &t: far_wall) {
                if (t.tile.tile >= 0 && t.tile.slot < 17) {
                    auto lower = t.tile;
                    lower.slot += 17;
                    lower.tile = -1;
                    const auto &im = artwork.Get(lower, red);
                    const auto offset = TableTileOffset(lower, artwork.width());
                    wall.push_back({lower, im.x + offset.x, im.y + offset.y});
                }
                wall.push_back(t);
            }
            scene.insert(scene.begin(), wall.begin(), wall.end());
        }
        for (const auto &tile: BuildWallScene(state, placed))
            if (tile.direction == 0) {
                const auto &item = artwork.Get(tile, red);
                const auto offset = TableTileOffset(tile, artwork.width());
                near_wall.push_back({tile, item.x + offset.x, item.y + offset.y});
            }
        if (near_wall.empty()) return;
        SortTableScene(near_wall, artwork, red);
        const auto local_hand = [](const PositionedTile &t) {
            return t.tile.direction == 0 && (t.tile.family == 't' || t.tile.family == 'n');
        };
        const auto end = std::stable_partition(scene.begin(), scene.end(), [&](const PositionedTile &t) {
            return !local_hand(t);
        });
        std::vector<PositionedTile> wall;
        for (const auto &t: near_wall) {
            if (t.tile.tile >= 0 && t.tile.slot < 17) {
                auto lower = t.tile;
                lower.slot += 17;
                lower.tile = -1;
                const auto &item = artwork.Get(lower, red);
                const auto offset = TableTileOffset(lower, artwork.width());
                wall.push_back({lower, item.x + offset.x, item.y + offset.y});
            }
            wall.push_back(t);
        }
        scene.insert(end, wall.begin(), wall.end());
    }

    void PaintWall(Gdiplus::Graphics &graphics, TableArtwork &artwork, const ReplayState &state, bool red,
                   unsigned placed, bool include_near) {
        std::vector<PositionedTile> scene;
        for (const auto &tile: BuildWallScene(state, placed)) {
            if (!include_near && tile.direction == 0) continue;
            const auto &item = artwork.Get(tile, red);
            const auto offset = TableTileOffset(tile, artwork.width());
            scene.push_back({tile, item.x + offset.x, item.y + offset.y});
        }
        SortTableScene(scene, artwork, red);
        const auto paint = [&](const TableTile &tile) {
            const auto &item = artwork.Get(tile, red);
            Gdiplus::Bitmap image(item.image.width, item.image.height, item.image.width * 4,
                                  PixelFormat32bppARGB, const_cast<BYTE *>(item.image.bgra.data()));
            const auto offset = TableTileOffset(tile, artwork.width());
            if (graphics.DrawImage(&image, item.x + offset.x, item.y + offset.y) != Gdiplus::Ok)
                throw std::runtime_error("Cannot draw original wall tile");
        };
        for (const auto &placed_tile: scene) {
            const auto &tile = placed_tile.tile;
            // Back y0..16 sprites already include both layers, but face sprites
            // contain only the upper tile. Restore its lower back before the face.
            if (tile.tile >= 0 && tile.slot < 17) {
                auto lower = tile;
                lower.slot += 17;
                lower.tile = -1;
                paint(lower);
            }
            paint(tile);
        }
        if (state.initialized && !state.wall.known) {
            const auto text = std::string("Wall unavailable: ") + state.wall.unavailable;
            const std::wstring wide(text.begin(), text.end());
            Gdiplus::Font font(L"Segoe UI", 14.0f * artwork.width() / 1024);
            Gdiplus::SolidBrush brush(Gdiplus::Color(255, 255, 215, 140));
            graphics.DrawString(wide.c_str(), -1, &font, Gdiplus::PointF(24.0f, 100.0f), &brush);
        }
    }
} // namespace tenhou
