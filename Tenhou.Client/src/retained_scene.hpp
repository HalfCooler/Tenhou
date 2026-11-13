#pragma once
#include "paint_buffer.hpp"
#include "table_scene.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <sstream>

namespace tenhou {
    inline bool Overlaps(RECT a, RECT b) {
        RECT intersection{};
        return IntersectRect(&intersection, &a, &b) != FALSE;
    }

    inline void ClipGraphics(Gdiplus::Graphics &graphics, RECT rect) {
        graphics.SetClip(Gdiplus::Rect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top));
    }

    // Include the GDI+ boundary pixel when restoring the old and new positions.
    inline RECT TileBounds(const PositionedTile &placed, int width, int height) {
        return {placed.x - 2, placed.y - 2, placed.x + width + 2, placed.y + height + 2};
    }

    // Original 00485b40 bounds overlay unions at 400 pixels. Scale that native
    // bound with the selected canvas; fall back to a full frame for complex damage.
    class DirtyRegions {
    public:
        DirtyRegions(int width, int height) : width_(width), height_(height) {
        }

        void Add(RECT rect) {
            RECT limits{0, 0, width_, height_}, clipped{};
            if (!IntersectRect(&clipped, &rect, &limits)) return;
            rect = clipped;
            for (std::size_t i = 0; i < rectangles.size();) {
                const auto &existing = rectangles[i];
                if (rect.left >= existing.left && rect.top >= existing.top && rect.right <= existing.right && rect.
                    bottom <= existing.bottom) return;
                if (existing.left >= rect.left && existing.top >= rect.top && existing.right <= rect.right && existing.
                    bottom <= rect.bottom) {
                    rectangles.erase(rectangles.begin() + i);
                    continue;
                }
                RECT united{};
                UnionRect(&united, &rect, &rectangles[i]);
                if (Overlaps(rect, rectangles[i]) && united.right - united.left <= 400 * width_ / 1024 &&
                    united.bottom - united.top <= 400 * width_ / 1024) {
                    rect = united;
                    rectangles.erase(rectangles.begin() + i);
                    i = 0;
                } else ++i;
            }
            rectangles.push_back(rect);
            if (rectangles.size() > 32) rectangles = {{0, 0, width_, height_}};
        }

        std::vector<RECT> rectangles;

    private:
        int width_, height_;
    };

    // Metadata excludes tile positions, which have their own old/new rectangles.
    // A metadata change conservatively invalidates the complete foreground.
    inline std::string TableMetadata(const ReplayState &state) {
        std::ostringstream out;
        out << state.relative_scores << ':' << state.type << ':' << state.furiten << ':' << state.initialized << ':' <<
                state.ended << ':' << state.round << ':' << state.honba << ':'
                << state.sticks << ':' << state.dealer << ':' << state.turn << ':' << state.result.size() << ':' <<
                state.result;
        out << ":seats=" << state.seat_mask;
        if (state.variant) out << ":variant:" << state.wall.total << ':' << state.wall.front << ':' << state.wall.tail;
        for (const auto &p: state.players) out << ':' << p.connected << ':' << p.score << ':' << p.reach << ':' << p.
                                           name.size() << ':' << p.name << ':' << p.nuki.size();
        out << ":winners=" << state.winners.size();
        for (auto winner: state.winners) out << ':' << winner;
        out << ":indicators=" << state.dora_indicators.size();
        for (auto indicator: state.dora_indicators) out << ':' << indicator;
        return out.str();
    }

    inline std::string WallMetadata(const ReplayState &state, unsigned placed, bool original, bool red) {
        const auto &w = state.wall;
        std::ostringstream out;
        out << original << ':' << red << ':' << state.initialized << ':' << placed << ':' << w.known << ':' << w.offset
                << ':' << w.front << ':' << w.replacements << ':' << w.unavailable << ':' << w.total << ':' << w.tail << ':' << w.dead_wall;
        out << ":indicators=" << state.dora_indicators.size();
        for (auto indicator: state.dora_indicators) out << ':' << indicator;
        return out.str();
    }

    class RetainedScene {
    public:
        struct Damage {
            RECT bounds{};
            std::array<const void *, 3> shadows{};
        };

        void SetReferenceRendering(bool enabled) {
            reference_ = enabled;
            Invalidate();
        }

        void Invalidate() {
            valid_ = false;
            backdrop_key_.clear();
        }

        template<class Backdrop, class Foreground>
        void Render(HDC target, int width, RECT destination,
                    const std::vector<PositionedTile> &scene, TableArtwork &artwork, bool red,
                    const std::string &backdrop_key, const std::string &metadata, Backdrop background,
                    Foreground foreground, RECT overlay = {},
                    const std::vector<unsigned> &appearances = {}, const std::vector<RECT> &metadata_regions = {},
                    const std::vector<RECT> &atomic_layers = {}, const std::vector<Damage> &damage = {}) {
            const int height = width * 3 / 4;
            const bool reference = reference_;
            const bool base_changed = !valid_ || width_ != width || backdrop_key != backdrop_key_;
            DirtyRegions dirty(width, height);
            std::vector<Item> current;
            current.reserve(scene.size());
            for (const auto &tile: scene) {
                const auto &image = artwork.Get(tile.tile, red);
                const auto index = current.size();
                current.push_back({
                    tile,
                    index < damage.size()
                        ? damage[index].bounds
                        : TileBounds(tile, image.image.width, image.image.height),
                    index < appearances.size() ? appearances[index] : 0,
                    index < damage.size() ? damage[index].shadows : std::array<const void *, 3>{}
                });
            }
            if (reference || base_changed) dirty.rectangles = {{0, 0, width, height}};
            else {
                if (metadata != metadata_) {
                    if (metadata_regions.empty()) dirty.rectangles = {{0, 0, width, height}};
                    else for (auto rect: metadata_regions) dirty.Add(rect);
                }
                const auto count = std::max(current.size(), previous_.size());
                for (std::size_t i = 0; i < count; ++i) {
                    if (i < current.size() && i < previous_.size() && Same(current[i], previous_[i])) continue;
                    if (i < current.size()) dirty.Add(current[i].bounds);
                    if (i < previous_.size()) dirty.Add(previous_[i].bounds);
                }
            }
            if (!dirty.rectangles.empty()) {
                dirty.Add(overlay);
                dirty.Add(previous_overlay_);
            }
            // A transformed GDI+ image can round a boundary sample differently when
            // its glow is cut by a clip. Restore/repaint an intersecting HUD group
            // as a whole, retaining exact full-render rasterization at both sizes.
            if (!atomic_layers.empty()) {
                DirtyRegions coherent(width, height);
                for (auto rect: dirty.rectangles) {
                    for (const auto &layer: atomic_layers)
                        if (Overlaps(rect, layer)) {
                            RECT united{};
                            UnionRect(&united, &rect, &layer);
                            rect = united;
                        }
                    coherent.Add(rect);
                }
                dirty.rectangles = std::move(coherent.rectangles);
            }

            try {
                if (base_changed && !reference) {
                    backdrop_.Store(target, width, height, background);
                }
                native_.RenderDirtyScaled(target, width, height, destination, dirty.rectangles,
                                          [&](HDC dc, RECT region) {
                                              if (reference) background(dc);
                                              else backdrop_.CopyTo(dc, region);
                                              foreground(dc, region);
                                          });
            } catch (...) {
                Invalidate();
                throw;
            }
            previous_ = std::move(current);
            width_ = width;
            valid_ = true;
            backdrop_key_ = backdrop_key;
            metadata_ = metadata;
            previous_overlay_ = overlay;
        }

        static bool Reference() {
            return false;
        }

    private:
        struct Item {
            PositionedTile tile;
            RECT bounds;
            unsigned appearance = 0;
            std::array<const void *, 3> shadows{};
        };

        static bool Same(const Item &a, const Item &b) {
            const auto &x = a.tile;
            const auto &y = b.tile;
            return a.appearance == b.appearance && a.shadows == b.shadows && EqualRect(&a.bounds, &b.bounds) && x.x == y
                   .x && x.y == y.y && x.tile.family == y.tile.family && x.tile.slot == y.tile.slot &&
                   x.tile.tile == y.tile.tile && x.tile.direction == y.tile.direction && x.tile.drawn == y.tile.drawn &&
                   x.tile.riichi == y.tile.riichi;
        }

        PaintBuffer backdrop_, native_;
        std::vector<Item> previous_;
        std::string backdrop_key_, metadata_;
        bool valid_ = false;
        int width_ = 0;
        bool reference_ = Reference();
        RECT previous_overlay_{};
    };
} // namespace tenhou
