#pragma once
#include "table_scene.hpp"
#include <objidl.h>
#include <gdiplus.h>

namespace tenhou {
    TableTile WallTile(const WallState &wall, int seat, int stack, bool stacked);
    unsigned WallStackOrder(const WallState &wall, int seat, int stack);
    // Full stacks, remaining lower tiles, and only explicitly supplied indicators.
    std::vector<TableTile> BuildWallScene(const ReplayState &state, unsigned placed = 136);

    void PaintWall(Gdiplus::Graphics &graphics, TableArtwork &artwork,
                   const ReplayState &state, bool red, unsigned placed = 136, bool include_near = true);

    // Original 00476fe0 orders k0 -> y0 -> n0 -> t0. Keep the near
    // wall in foreground damage tracking so rivers cannot paint over its face.
    void InsertNearWall(std::vector<PositionedTile> &scene, TableArtwork &artwork,
                        const ReplayState &state, bool red, unsigned placed = 136, bool include_far = false);
} // namespace tenhou
