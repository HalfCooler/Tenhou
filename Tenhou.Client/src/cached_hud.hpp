#pragma once
#include "paint_buffer.hpp"
#include <map>
#include <string>

namespace tenhou {
    // Retain the exact GDI+ result, including transformed glow/outline rounding.
    // Black/white renders classify pixels as transparent, background-dependent or
    // opaque. Only the dependent pixels need an input comparison (the HUD region,
    // never a full native-frame scan). Opaque plate pixels occlude moving tiles.
    class CachedHud {
    public:
        std::size_t hits() const { return hits_; }

        void Invalidate() {
            masks_.clear();
            key_.clear();
            input_.clear();
            output_.clear();
            text_key_.clear();
            text_runs_.clear();
            text_valid_ = false;
        }

        // Cache HD layouts/runs, never a native-size text bitmap. The final scene
        // applies its current viewport/DPI and draws them in this exact position
        // among image commands. key includes text/style/numeric/animation state.
        template<class Build>
        void AppendText(FinalScene &scene, const std::string &key, Build build) {
            if (!text_valid_ || text_key_ != key) {
                auto runs = build();
                text_runs_ = std::move(runs);
                text_key_ = key;
                text_valid_ = true;
            } else ++hits_;
            for (const auto &run : text_runs_) scene.Text(run);
        }

        template<class Draw>
        void Paint(HDC target, int width, RECT region, const std::string &key, Draw draw) {
            DIBSECTION dib{};
            // These callers supply PaintBuffer's top-down DIB. GetObject reports an
            // absolute height on this Win32 driver; it cannot test scan direction.
            if (!GetObjectW(GetCurrentObject(target, OBJ_BITMAP), sizeof(dib), &dib) || !dib.dsBm.bmBits ||
                dib.dsBm.bmBitsPixel != 32 || dib.dsBm.bmWidth != width) {
                draw(target);
                return;
            }
            const int height = width * 3 / 4;
            const RECT canvas{ 0, 0, width, height };
            RECT clipped{};
            if (!IntersectRect(&clipped, &region, &canvas)) return;
            region = clipped;
            const int rw = region.right - region.left, rh = region.bottom - region.top;
            const auto count = static_cast<std::size_t>(rw) * rh;
            auto *pixels = static_cast<std::uint32_t *>(dib.dsBm.bmBits);
            const int stride = dib.dsBm.bmWidthBytes / 4;
            GdiFlush();
            const bool layout = width_ == width && Equal(region_, region);
            if (!layout) Invalidate();
            const auto mask = masks_.find(key);
            bool reusable = layout && key_ == key && output_.size() == count && mask != masks_.end();
            if (reusable) {
                ++hits_;
                for (int y = 0; y < rh && reusable; ++y)
                    for (int x = 0; x < rw; ++x) {
                        if (const auto i = static_cast<std::size_t>(y) * rw + x;
                            mask->second[i] == 1 && pixels[(y + region.top) * stride + x + region.left] != input_[i]) {
                            reusable = false;
                            break;
                        }
                    }
            }
            if (reusable) {
                for (int y = 0; y < rh; ++y)
                    for (int x = 0; x < rw; ++x) {
                        if (const auto i = static_cast<std::size_t>(y) * rw + x; mask->second[i])
                            pixels[(y + region.top) * stride + x + region.left] = output_[i];
                    }
                return;
            }
            // detail=1 marks the first classification for this key (the black/white
            // double render below); 0 marks a rebuild reusing a known mask.

            input_.resize(count);
            output_.resize(count);
            for (int y = 0; y < rh; ++y) std::copy_n(pixels + (y + region.top) * stride + region.left, rw,
                                                     input_.data() + static_cast<std::size_t>(y) * rw);
            draw(target);
            GdiFlush();
            for (int y = 0; y < rh; ++y) std::copy_n(pixels + (y + region.top) * stride + region.left, rw,
                                                     output_.data() + static_cast<std::size_t>(y) * rw);
            if (mask == masks_.end()) {
                std::vector<std::uint32_t> black(count), white(count);
                const auto render = [&](const bool bright, std::vector<std::uint32_t> &result) {
                    scratch_.Store(target, width, height, [&](HDC dc) {
                        const RECT all{0, 0, width, height};
                        FillRect(dc, &all, static_cast<HBRUSH>(GetStockObject(bright ? WHITE_BRUSH : BLACK_BRUSH)));
                        draw(dc);
                        GdiFlush();
                        DIBSECTION data{};
                        GetObjectW(GetCurrentObject(dc, OBJ_BITMAP), sizeof(data), &data);
                        const auto *source = static_cast<const std::uint32_t *>(data.dsBm.bmBits);
                        for (int y = 0; y < rh; ++y) std::copy_n(source + (y + region.top) * width + region.left, rw,
                                                                 result.data() + static_cast<std::size_t>(y) * rw);
                    });
                };
                render(false, black);
                render(true, white);
                std::vector<unsigned char> categories(count);
                for (std::size_t i = 0; i < count; ++i) {
                    const auto b = black[i] & 0xffffff, w = white[i] & 0xffffff;
                    categories[i] = b == 0 && w == 0xffffff ? 0 : b == w ? 2 : 1;
                }
                if (masks_.size() >= 8) masks_.erase(masks_.begin());
                masks_.emplace(key, std::move(categories));
            }
            width_ = width;
            region_ = region;
            key_ = key;
        }

    private:
        static bool Equal(const RECT a, const RECT b) {
            return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom;
        }

        PaintBuffer scratch_;
        std::map<std::string, std::vector<unsigned char> > masks_;
        std::vector<std::uint32_t> input_, output_;
        std::string key_;
        RECT region_{};
        int width_ = 0;
        std::size_t hits_ = 0;
        std::string text_key_;
        std::vector<SceneTextRun> text_runs_;
        bool text_valid_ = false;
    };
} // namespace tenhou
