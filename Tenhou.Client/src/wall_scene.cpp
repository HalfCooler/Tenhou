#include "wall_scene.hpp"
#include <stdexcept>

namespace tenhou {
    void PaintWall(Gdiplus::Graphics &graphics, TableArtwork &artwork, const ReplayState &state, const bool red,
                   const unsigned placed, const bool include_near) {
        std::vector<PositionedTile> scene;
        for (const auto &tile: BuildWallScene(state, placed)) {
            if (!include_near && tile.direction == 0) continue;
            const auto origin = artwork.geometry().Origin(tile, red);
            scene.push_back({tile, origin.first, origin.second});
        }
        SortTableScene(scene, artwork, red);
        const auto paint = [&](const TableTile &tile) {
            const auto &item = artwork.Get(tile, red);
            Gdiplus::Bitmap image(item.image.width, item.image.height, item.image.width * 4,
                                  PixelFormat32bppARGB, const_cast<BYTE *>(item.image.bgra.data()));
            const auto offset = artwork.Offset(tile);
            if (graphics.DrawImage(&image, item.x + offset.x, item.y + offset.y) != Gdiplus::Ok)
                throw std::runtime_error("Cannot draw original wall tile");
        };
        for (const auto &placed_tile: scene) {
            const auto &tile = placed_tile.tile;
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
