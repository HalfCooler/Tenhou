#pragma once
#include <windows.h>
#include "animation_clock.hpp"
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
    PaintBuffer(const PaintBuffer&) = delete;
    PaintBuffer& operator=(const PaintBuffer&) = delete;
    ~PaintBuffer() { Reset(); }

    template<class Draw> void Store(HDC target, int width, int height, Draw draw) { Compose(target,width,height,draw); }
    void CopyTo(HDC target, RECT region) const {
        if (!BitBlt(target,region.left,region.top,region.right-region.left,region.bottom-region.top,
            memory_,region.left,region.top,SRCCOPY)) throw std::runtime_error("Cannot restore retained background");
    }
    // Caller supplies conservative object-level invalidation. No full native
    // comparison or snapshot copy; the complete cached frame still serves print/exposure.
    template<class Draw> void RenderDirtyScaled(HDC target, int width, int height, RECT destination,
        const std::vector<RECT>& requested, Draw draw) {
        if (width <= 0 || height <= 0 || IsRectEmpty(&destination)) return;
        
        const bool native_resize = width_ != width || height_ != height || !retained_valid_;
        Ensure(target,width,height);
        const int dw = destination.right-destination.left, dh = destination.bottom-destination.top;
        const bool resize = !scaled_ || scaled_width_ != dw || scaled_height_ != dh ||
            scaled_left_ != destination.left || scaled_top_ != destination.top;
        std::vector<RECT> dirty = native_resize ? std::vector<RECT>{{0,0,width,height}} : requested;
        try {
            for (const auto& rect : dirty) Compose(target,width,height,[&](HDC dc) {
                IntersectClipRect(dc,rect.left,rect.top,rect.right,rect.bottom); draw(dc,rect);
            });
            if (resize) {
                scaled_ = std::make_unique<PaintBuffer>(); scaled_width_ = dw; scaled_height_ = dh;
                scaled_left_ = destination.left; scaled_top_ = destination.top;
            }
            if (resize) dirty = {{0,0,width,height}};
            for (const auto& rect : dirty) {
                
                const RECT clip{std::max(0L,(rect.left-2)*dw/width-2),std::max(0L,(rect.top-2)*dh/height-2),
                    std::min(LONG(dw),(rect.right+2)*dw/width+3),std::min(LONG(dh),(rect.bottom+2)*dh/height+3)};
                scaled_->Compose(target,dw,dh,[&](HDC dc) {
                    IntersectClipRect(dc,clip.left,clip.top,clip.right,clip.bottom);
                    SetStretchBltMode(dc,HALFTONE); SetBrushOrgEx(dc,-destination.left,-destination.top,nullptr);
                    if (!StretchBlt(dc,0,0,dw,dh,memory_,0,0,width,height,SRCCOPY))
                        throw std::runtime_error("Cannot scale retained region");
                });
            }
            retained_valid_ = true;
        } catch (...) { retained_valid_ = false; scaled_.reset(); throw; }
        
        if (!BitBlt(target,destination.left,destination.top,dw,dh,scaled_->memory_,0,0,SRCCOPY))
            throw std::runtime_error("Cannot present retained frame");
    }

    template<class Draw> void Render(HDC target, int width, int height, Draw draw) {
        if (width <= 0 || height <= 0) return;
        
        Ensure(target, width, height);
        const int saved = SaveDC(memory_);
        if (!saved) throw std::runtime_error("Cannot save buffered paint context");
        try { draw(memory_); }
        catch (...) { RestoreDC(memory_, saved); throw; }
        RestoreDC(memory_, saved);
        // The target paint DC retains its update-region and child-window clip.
        // Publish only after the background, table, and HUD are all complete.
        
        if (!BitBlt(target, 0, 0, width, height, memory_, 0, 0, SRCCOPY))
            throw std::runtime_error("Cannot present buffered frame");
    }

    template<class Draw> void RenderScaled(HDC target, int width, int height, RECT destination, Draw draw) {
        if (width <= 0 || height <= 0 || destination.right <= destination.left || destination.bottom <= destination.top) return;
        
        Compose(target, width, height, draw);
        // GDI+ and GDI can batch writes to a DIB. Finish them before comparing
        // the native pixels, then retain the expensive HALFTONE result.
        GdiFlush();
        const int dw = destination.right-destination.left, dh = destination.bottom-destination.top;
        const bool resize = !scaled_ || scaled_width_ != dw || scaled_height_ != dh ||
            scaled_left_ != destination.left || scaled_top_ != destination.top;
        RECT dirty{0,0,width,height};
        if (!resize && previous_pixels_.size() == static_cast<std::size_t>(width)*height) {
            dirty = {width,height,0,0};
            for (int y = 0; y < height; ++y) {
                const auto row = static_cast<std::size_t>(y)*width;
                const auto* current = pixels_ + row;
                auto* previous = previous_pixels_.data() + row;
                if (std::memcmp(current, previous, static_cast<std::size_t>(width)*4) == 0) continue;
                int left = 0, right = width;
                while (left < right && current[left] == previous[left]) ++left;
                while (right > left && current[right-1] == previous[right-1]) --right;
                dirty.left = std::min(dirty.left, static_cast<LONG>(left));
                dirty.right = std::max(dirty.right, static_cast<LONG>(right));
                dirty.top = std::min(dirty.top, static_cast<LONG>(y)); dirty.bottom = y+1;
            }
        }
        if (resize) {
            scaled_ = std::make_unique<PaintBuffer>(); scaled_width_ = dw; scaled_height_ = dh;
            scaled_left_ = destination.left; scaled_top_ = destination.top;
        }
        if (dirty.right > dirty.left && dirty.bottom > dirty.top) {
            // Include the resampling footprint on both sides of a changed
            // pixel. Keep the full source/destination mapping while clipping;
            // scaling a cropped source would move the HALFTONE sample grid.
            RECT clip{std::max(0L, (dirty.left-2)*dw/width-2), std::max(0L, (dirty.top-2)*dh/height-2),
                std::min(static_cast<LONG>(dw), (dirty.right+2)*dw/width+3),
                std::min(static_cast<LONG>(dh), (dirty.bottom+2)*dh/height+3)};
            try {
                scaled_->Compose(target, dw, dh, [&](HDC scaled) {
                    IntersectClipRect(scaled,clip.left,clip.top,clip.right,clip.bottom);
                    SetStretchBltMode(scaled, HALFTONE);
                    SetBrushOrgEx(scaled, -destination.left, -destination.top, nullptr);
                    if (!StretchBlt(scaled,0,0,dw,dh,memory_,0,0,width,height,SRCCOPY))
                        throw std::runtime_error("Cannot scale buffered frame");
                });
                previous_pixels_.assign(pixels_,pixels_+static_cast<std::size_t>(width)*height);
            } catch (...) { previous_pixels_.clear(); throw; }
        }
        
        if (!BitBlt(target,destination.left,destination.top,dw,dh,scaled_->memory_,0,0,SRCCOPY))
            throw std::runtime_error("Cannot present scaled frame");
    }

    template<class Draw> void Paint(HWND window, Draw draw) {
        PAINTSTRUCT paint{};
        const HDC target = BeginPaint(window, &paint);
        try {
            RECT client{}; GetClientRect(window, &client);
            Render(target, client.right, client.bottom, draw);
        } catch (...) { EndPaint(window, &paint); throw; }
        EndPaint(window, &paint);
    }

private:
    template<class Draw> void Compose(HDC target, int width, int height, Draw draw) {
        
        Ensure(target, width, height);
        const int saved = SaveDC(memory_);
        if (!saved) throw std::runtime_error("Cannot save buffered paint context");
        try { draw(memory_); }
        catch (...) { RestoreDC(memory_, saved); throw; }
        RestoreDC(memory_, saved);
    }
    void Reset() {
        if (memory_) {
            SelectObject(memory_, previous_);
            DeleteObject(bitmap_);
            DeleteDC(memory_);
        }
        memory_ = nullptr; bitmap_ = nullptr; previous_ = nullptr;
        pixels_ = nullptr; previous_pixels_.clear(); scaled_.reset();
        width_ = height_ = 0;
        retained_valid_ = false;
    }
    void Ensure(HDC target, int width, int height) {
        if (memory_ && width_ == width && height_ == height) return;
        const HDC memory = CreateCompatibleDC(target);
        // Keep a CPU-accessible 32-bit surface: GDI+ otherwise repeatedly
        // converts a device-dependent bitmap while composing alpha sprites.
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = width; info.bmiHeader.biHeight = -height;
        info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
        void* pixels = nullptr;
        const HBITMAP bitmap = CreateDIBSection(target, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        if (!memory || !bitmap) {
            if (memory) DeleteDC(memory);
            if (bitmap) DeleteObject(bitmap);
            throw std::runtime_error("Cannot allocate buffered frame");
        }
        const auto previous = SelectObject(memory, bitmap);
        if (!previous || previous == HGDI_ERROR) {
            DeleteObject(bitmap); DeleteDC(memory);
            throw std::runtime_error("Cannot select buffered frame");
        }
        Reset();
        memory_ = memory; bitmap_ = bitmap; previous_ = previous;
        pixels_ = static_cast<std::uint32_t*>(pixels);
        width_ = width; height_ = height;
        // Actual DIB/DC (re)creation happens only here, on a size change.
        
    }
    HDC memory_ = nullptr;
    HBITMAP bitmap_ = nullptr;
    HGDIOBJ previous_ = nullptr;
    int width_ = 0, height_ = 0;
    std::uint32_t* pixels_ = nullptr;
    std::vector<std::uint32_t> previous_pixels_;
    std::unique_ptr<PaintBuffer> scaled_;
    int scaled_width_ = 0, scaled_height_ = 0;
    LONG scaled_left_ = 0, scaled_top_ = 0;
    bool retained_valid_ = false;
};
} // namespace tenhou
