#include "wall_geometry.hpp"
#include <algorithm>
#include <array>

namespace tenhou {
    unsigned WallStackOrder(const WallState &wall, const int seat, const int stack) {
        unsigned index = 0;
        for (int side = 0; side < 4; ++side) {
            const int stacks = wall.SideTiles(side) / 2;
            index += std::min(stacks, stack + (side < seat ? 1 : 0));
        }
        return index;
    }

    TableTile WallTile(const WallState &wall, const int seat, const int stack, const bool stacked) {
        const int stacks = wall.SideTiles(seat) / 2;
        const int resource = std::min(stack, 16) + (stacks < 17 ? (17 - stacks) / 2 : 0);
        TableTile tile{'y', resource + (stacked ? 0 : 17), -1, seat};
        tile.wall_stack = stack;
        tile.wall_stacks = stacks;
        tile.wall_extra = std::max(0, stack - 16);
        tile.role = "wall";
        return tile;
    }

    std::vector<TableTile> BuildWallScene(const ReplayState &state, const unsigned placed) {
        std::vector<TableTile> scene;
        const auto &wall = state.wall;
        if (!state.initialized || !wall.known) return scene;
        std::array<std::array<bool, 36>, 4> alive{};
        for (int seat = 0; seat < 4; ++seat)
            for (int stack = 0; stack < wall.SideTiles(seat) / 2; ++stack) {
                const unsigned order = WallStackOrder(wall, seat, stack) * 2;
                alive[seat][stack * 2] = order + 1 < placed;
                alive[seat][stack * 2 + 1] = order < placed;
            }
        const auto remove = [&](const int index) {
            const auto p = wall.Position(index);
            alive[p.first][p.second] = false;
        };
        for (int i = 0; i < std::max(wall.front, wall.initial_removed); ++i) remove(i);
        if (wall.dead_wall)
            for (int i = 0; i < wall.replacements; ++i) remove((135 - i) ^ 1);
        else
            for (int i = 0; i < wall.tail; ++i) remove(wall.total - 1 - i);
        std::array<std::array<int, 17>, 4> indicators{};
        for (auto &seat: indicators) seat.fill(-1);
        for (std::size_t i = 0; wall.dead_wall && i < state.dora_indicators.size() && i < 5; ++i) {
            const auto p = wall.Position(wall.indicator_index - static_cast<int>(i) * 2);
            indicators[p.first][p.second / 2] = state.dora_indicators[i];
        }
        for (int seat = 0; seat < 4; ++seat)
            for (int stack = 0; stack < wall.SideTiles(seat) / 2; ++stack) {
                const bool top = alive[seat][stack * 2], bottom = alive[seat][stack * 2 + 1];
                if (!top && !bottom) continue;
                const bool stacked = wall.dead_wall ? top : top && bottom;
                auto tile = WallTile(wall, seat, stack, stacked);
                if (top && wall.dead_wall) tile.tile = indicators[seat][stack];
                tile.role = tile.tile < 0 ? "wall" : "wall_indicator";
                // FUN_0049cab0 / FUN_004902f0: split at physical index 120,
                // then move the tail stacks by the literal per-seat PE offsets.
                const auto tail = wall.Position(120);
                if (wall.dead_wall && placed == 136 && tail.second / 2 != 0 && seat == tail.first &&
                    stack >= tail.second / 2) {
                    constexpr int dx[]{4, -1, -4, -1}, dy[]{0, -4, 0, 4};
                    tile.dx = dx[seat];
                    tile.dy = dy[seat];
                }
                scene.push_back(tile);
            }
        return scene;
    }

    void InsertNearWall(std::vector<PositionedTile> &scene, const TileGeometryCatalog &geometry,
                        const ReplayState &state, const bool red, const unsigned placed, const bool include_far) {
        std::vector<PositionedTile> near_wall;
        if (include_far) {
            std::vector<PositionedTile> far_wall;
            for (const auto &tile: BuildWallScene(state, placed))
                if (tile.direction != 0) {
                    const auto origin = geometry.Origin(tile, red);
                    far_wall.push_back({tile, origin.first, origin.second});
                }
            SortTableScene(far_wall, geometry, red);
            std::vector<PositionedTile> wall;
            for (const auto &t: far_wall) {
                if (t.tile.tile >= 0 && t.tile.slot < 17) {
                    auto lower = t.tile;
                    lower.slot += 17;
                    lower.tile = -1;
                    const auto origin = geometry.Origin(lower, red);
                    wall.push_back({lower, origin.first, origin.second});
                }
                wall.push_back(t);
            }
            scene.insert(scene.begin(), wall.begin(), wall.end());
        }
        for (const auto &tile: BuildWallScene(state, placed))
            if (tile.direction == 0) {
                const auto origin = geometry.Origin(tile, red);
                near_wall.push_back({tile, origin.first, origin.second});
            }
        if (near_wall.empty()) return;
        SortTableScene(near_wall, geometry, red);
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
                const auto origin = geometry.Origin(lower, red);
                wall.push_back({lower, origin.first, origin.second});
            }
            wall.push_back(t);
        }
        scene.insert(end, wall.begin(), wall.end());
    }
} // namespace tenhou
