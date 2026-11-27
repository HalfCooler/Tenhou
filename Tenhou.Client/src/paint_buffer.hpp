#pragma once
#include <windows.h>
#include "animation_clock.hpp"
#include "final_scene.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

namespace tenhou {
    // Buffer the whole client, including its background. Buffering only the table
    // still exposes the parent's background fill before the table is copied back.
    class PaintBuffer {
    public:
        PaintBuffer() = default;

        PaintBuffer(const PaintBuffer &) = delete;

        PaintBuffer &operator=(const PaintBuffer &) = delete;

        ~PaintBuffer() { Reset(); }

        template<class Draw>
        void Store(HDC target, int width, int height, Draw draw) { Compose(target, width, height, draw); }

        void CopyTo(const HDC target, const RECT region) const {
            if (!BitBlt(target, region.left, region.top, region.right - region.left, region.bottom - region.top,
                        memory_, region.left, region.top, SRCCOPY))
                throw std::runtime_error("Cannot restore retained background");
        }

        // Caller supplies conservative object-level invalidation. No full native
        // comparison or snapshot copy; the complete cached frame still serves print/exposure.
        template<class Draw>
        void RenderDirtyScaled(HDC target, int width, int height, const RECT destination,
                               const std::vector<RECT> &requested, Draw draw, const FinalScene *final_scene = nullptr) {
            if (width <= 0 || height <= 0 || IsRectEmpty(&destination)) return;

            const bool native_resize = width_ != width || height_ != height || !retained_valid_;
            Ensure(target, width, height);
            const int dw = destination.right - destination.left, dh = destination.bottom - destination.top;
            const bool resize = !scaled_ || scaled_width_ != dw || scaled_height_ != dh ||
                                scaled_left_ != destination.left || scaled_top_ != destination.top;
            std::vector<RECT> dirty = native_resize ? std::vector<RECT>{{0, 0, width, height}} : requested;
            try {
                for (const auto &rect: dirty)
                    Compose(target, width, height, [&](HDC dc) {
                        IntersectClipRect(dc, rect.left, rect.top, rect.right, rect.bottom);
                        draw(dc, rect);
                    });
                if (resize) {
                    scaled_ = std::make_unique<PaintBuffer>();
                    scaled_width_ = dw;
                    scaled_height_ = dh;
                    scaled_left_ = destination.left;
                    scaled_top_ = destination.top;
                }
                if (resize) dirty = {{0, 0, width, height}};
                std::vector<RECT> device_dirty;
                for (const auto &rect: dirty) {
                    const RECT clip{
                        std::max(0L, (rect.left - 2) * dw / width - 2), std::max(0L, (rect.top - 2) * dh / height - 2),
                        std::min(static_cast<LONG>(dw), (rect.right + 2) * dw / width + 3),
                        std::min(static_cast<LONG>(dh), (rect.bottom + 2) * dh / height + 3)
                    };
                    device_dirty.push_back(clip);
                    scaled_->Compose(target, dw, dh, [&](const HDC dc) {
                        IntersectClipRect(dc, clip.left, clip.top, clip.right, clip.bottom);
                        SetStretchBltMode(dc, HALFTONE);
                        SetBrushOrgEx(dc, -destination.left, -destination.top, nullptr);
                        if (!StretchBlt(dc, 0, 0, dw, dh, memory_, 0, 0, width, height, SRCCOPY))
                            throw std::runtime_error("Cannot scale retained region");
                    });
                }
                retained_valid_ = true;
                PresentFinal(target, width, height, destination, device_dirty, resize, final_scene);
            } catch (...) {
                retained_valid_ = false;
                scaled_.reset();
                throw;
            }
        }

        template<class Draw>
        void Render(const HDC target, const int width, const int height, Draw draw) {
            if (width <= 0 || height <= 0) return;

            Ensure(target, width, height);
            const int saved = SaveDC(memory_);
            if (!saved) throw std::runtime_error("Cannot save buffered paint context");
            try { draw(memory_); } catch (...) {
                RestoreDC(memory_, saved);
                throw;
            }
            RestoreDC(memory_, saved);
            // The target paint DC retains its update-region and child-window clip.
            // Publish only after the background, table, and HUD are all complete.

            if (!BitBlt(target, 0, 0, width, height, memory_, 0, 0, SRCCOPY))
                throw std::runtime_error("Cannot present buffered frame");
        }

        template<class Draw>
        void RenderScaled(HDC target, int width, int height, const RECT destination, Draw draw,
                          const FinalScene *final_scene = nullptr) {
            if (width <= 0 || height <= 0 || destination.right <= destination.left || destination.bottom <= destination.
                top) return;

            Compose(target, width, height, draw);
            // GDI+ and GDI can batch writes to a DIB. Finish them before comparing
            // the native pixels, then retain the expensive HALFTONE result.
            GdiFlush();
            const int dw = destination.right - destination.left, dh = destination.bottom - destination.top;
            const bool resize = !scaled_ || scaled_width_ != dw || scaled_height_ != dh ||
                                scaled_left_ != destination.left || scaled_top_ != destination.top;
            RECT dirty{0, 0, width, height};
            if (!resize && previous_pixels_.size() == static_cast<std::size_t>(width) * height) {
                dirty = {width, height, 0, 0};
                for (int y = 0; y < height; ++y) {
                    const auto row = static_cast<std::size_t>(y) * width;
                    const auto *current = pixels_ + row;
                    auto *previous = previous_pixels_.data() + row;
                    if (std::memcmp(current, previous, static_cast<std::size_t>(width) * 4) == 0) continue;
                    int left = 0, right = width;
                    while (left < right && current[left] == previous[left]) ++left;
                    while (right > left && current[right - 1] == previous[right - 1]) --right;
                    dirty.left = std::min(dirty.left, static_cast<LONG>(left));
                    dirty.right = std::max(dirty.right, static_cast<LONG>(right));
                    dirty.top = std::min(dirty.top, static_cast<LONG>(y));
                    dirty.bottom = y + 1;
                }
            }
            if (resize) {
                scaled_ = std::make_unique<PaintBuffer>();
                scaled_width_ = dw;
                scaled_height_ = dh;
                scaled_left_ = destination.left;
                scaled_top_ = destination.top;
            }
            std::vector<RECT> device_dirty;
            if (dirty.right > dirty.left && dirty.bottom > dirty.top) {
                // Include the resampling footprint on both sides of a changed
                // pixel. Keep the full source/destination mapping while clipping;
                // scaling a cropped source would move the HALFTONE sample grid.
                RECT clip{
                    std::max(0L, (dirty.left - 2) * dw / width - 2), std::max(0L, (dirty.top - 2) * dh / height - 2),
                    std::min(static_cast<LONG>(dw), (dirty.right + 2) * dw / width + 3),
                    std::min(static_cast<LONG>(dh), (dirty.bottom + 2) * dh / height + 3)
                };
                device_dirty.push_back(clip);
                try {
                    scaled_->Compose(target, dw, dh, [&](const HDC scaled) {
                        IntersectClipRect(scaled, clip.left, clip.top, clip.right, clip.bottom);
                        SetStretchBltMode(scaled, HALFTONE);
                        SetBrushOrgEx(scaled, -destination.left, -destination.top, nullptr);
                        if (!StretchBlt(scaled, 0, 0, dw, dh, memory_, 0, 0, width, height, SRCCOPY))
                            throw std::runtime_error("Cannot scale buffered frame");
                    });
                    previous_pixels_.assign(pixels_, pixels_ + static_cast<std::size_t>(width) * height);
                } catch (...) {
                    previous_pixels_.clear();
                    throw;
                }
            }
            try { PresentFinal(target, width, height, destination, device_dirty, resize, final_scene); }
            catch (...) {
                final_composite_.reset(); final_stamps_.clear();
                throw;
            }
        }

        template<class Draw>
        void Paint(const HWND window, Draw draw) {
            PAINTSTRUCT paint{};
            const HDC target = BeginPaint(window, &paint);
            try {
                RECT client{};
                GetClientRect(window, &client);
                Render(target, client.right, client.bottom, draw);
            } catch (...) {
                EndPaint(window, &paint);
                throw;
            }
            EndPaint(window, &paint);
        }

    private:
        // scaled_ always contains the clean image base. The final buffer is restored
        // from it before replaying commands, so alpha/effects never accumulate.
        void PresentFinal(const HDC target, const int width, const int height, const RECT destination,
                          const std::vector<RECT> &base_dirty, const bool resize, const FinalScene *scene) {
            const int dw = destination.right - destination.left, dh = destination.bottom - destination.top;
            if (!scene && !final_composite_) {
                if (!BitBlt(target, destination.left, destination.top, dw, dh, scaled_->memory_, 0, 0, SRCCOPY))
                    throw std::runtime_error("Cannot present scaled frame");
                return;
            }
            const int ui_w = width > 1440 ? 1440 : width;
            const int ui_h = width > 1440 ? ui_w * 3 / 4 : height;
            auto current = scene ? scene->Snapshot(ui_w, ui_h, dw, dh) : std::vector<FinalSceneStamp>{};
            RECT dirty{};
            const auto add = [&](const RECT r) {
                if (IsRectEmpty(&r)) return;
                if (IsRectEmpty(&dirty)) dirty = r;
                else { RECT united{}; UnionRect(&united, &dirty, &r); dirty = united; }
            };
            for (auto r : base_dirty) add(r);
            if (resize || !final_composite_) dirty = {0, 0, dw, dh};
            else for (std::size_t i = 0; i < std::max(current.size(), final_stamps_.size()); ++i) {
                if (i < current.size() && i < final_stamps_.size() && current[i].key == final_stamps_[i].key &&
                    current[i].layout == final_stamps_[i].layout && EqualRect(&current[i].bounds, &final_stamps_[i].bounds)) continue;
                if (i < current.size()) add(current[i].bounds);
                if (i < final_stamps_.size()) add(final_stamps_[i].bounds);
            }
            // Replay intersecting commands as complete atomic groups. In particular,
            // transformed image filtering must not change at a dirty clip boundary.
            bool expanded;
            do {
                const auto before = dirty;
                for (const auto &command : current) {
                    RECT intersection{};
                    if (IntersectRect(&intersection, &dirty, &command.bounds)) add(command.bounds);
                }
                expanded = !EqualRect(&before, &dirty);
            } while (expanded);
            try {
                if (resize || !final_composite_) final_composite_ = std::make_unique<PaintBuffer>();
                if (!IsRectEmpty(&dirty)) final_composite_->Compose(target, dw, dh, [&](const HDC dc) {
                    IntersectClipRect(dc, dirty.left, dirty.top, dirty.right, dirty.bottom);
                    scaled_->CopyTo(dc, dirty);
                    if (scene) scene->Paint(dc, ui_w, ui_h, dw, dh, dirty);
                });
                final_stamps_ = std::move(current);
                if (!BitBlt(target, destination.left, destination.top, dw, dh, final_composite_->memory_, 0, 0, SRCCOPY))
                    throw std::runtime_error("Cannot present final scene");
            } catch (...) {
                final_composite_.reset(); final_stamps_.clear();
                throw;
            }
        }

        template<class Draw>
        void Compose(const HDC target, const int width, const int height, Draw draw) {
            Ensure(target, width, height);
            const int saved = SaveDC(memory_);
            if (!saved) throw std::runtime_error("Cannot save buffered paint context");
            try { draw(memory_); } catch (...) {
                RestoreDC(memory_, saved);
                throw;
            }
            RestoreDC(memory_, saved);
        }

        void Reset() {
            if (memory_) {
                SelectObject(memory_, previous_);
                DeleteObject(bitmap_);
                DeleteDC(memory_);
            }
            memory_ = nullptr;
            bitmap_ = nullptr;
            previous_ = nullptr;
            pixels_ = nullptr;
            previous_pixels_.clear();
            scaled_.reset();
            final_composite_.reset();
            final_stamps_.clear();
            width_ = height_ = 0;
            retained_valid_ = false;
        }

        void Ensure(const HDC target, const int width, const int height) {
            if (memory_ && width_ == width && height_ == height) return;
            const HDC memory = CreateCompatibleDC(target);
            // Keep a CPU-accessible 32-bit surface: GDI+ otherwise repeatedly
            // converts a device-dependent bitmap while composing alpha sprites.
            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = width;
            info.bmiHeader.biHeight = -height;
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            void *pixels = nullptr;
            const HBITMAP bitmap = CreateDIBSection(target, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
            if (!memory || !bitmap) {
                if (memory) DeleteDC(memory);
                if (bitmap) DeleteObject(bitmap);
                throw std::runtime_error("Cannot allocate buffered frame");
            }
            const auto previous = SelectObject(memory, bitmap);
            if (!previous || previous == HGDI_ERROR) {
                DeleteObject(bitmap);
                DeleteDC(memory);
                throw std::runtime_error("Cannot select buffered frame");
            }
            Reset();
            memory_ = memory;
            bitmap_ = bitmap;
            previous_ = previous;
            pixels_ = static_cast<std::uint32_t *>(pixels);
            width_ = width;
            height_ = height;
            // Actual DIB/DC (re)creation happens only here, on a size change.
        }

        HDC memory_ = nullptr;
        HBITMAP bitmap_ = nullptr;
        HGDIOBJ previous_ = nullptr;
        int width_ = 0, height_ = 0;
        std::uint32_t *pixels_ = nullptr;
        std::vector<std::uint32_t> previous_pixels_;
        std::unique_ptr<PaintBuffer> scaled_;
        std::unique_ptr<PaintBuffer> final_composite_;
        std::vector<FinalSceneStamp> final_stamps_;
        int scaled_width_ = 0, scaled_height_ = 0;
        LONG scaled_left_ = 0, scaled_top_ = 0;
        bool retained_valid_ = false;
    };
} // namespace tenhou
