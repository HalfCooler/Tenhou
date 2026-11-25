#pragma once
#include "table_scene.hpp"
#include "wall_geometry.hpp"
#include <objidl.h>
#include <gdiplus.h>

namespace tenhou {
    void PaintWall(Gdiplus::Graphics &graphics, TableArtwork &artwork,
                   const ReplayState &state, bool red, unsigned placed = 136, bool include_near = true);

    inline void InsertNearWall(std::vector<PositionedTile> &scene, TableArtwork &artwork,
                               const ReplayState &state, bool red, unsigned placed = 136, bool include_far = false) {
        InsertNearWall(scene, artwork.geometry(), state, red, placed, include_far);
    }
} // namespace tenhou
