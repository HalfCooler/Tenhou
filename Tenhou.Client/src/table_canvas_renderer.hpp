#pragma once
#include "client_preferences.hpp"
#include "table_render_frame.hpp"
#include <windows.h>
#include <cstddef>
#include <filesystem>

namespace tenhou {
    class ITableCanvasRenderer {
    public:
        virtual ~ITableCanvasRenderer() = default;

        virtual void Configure(const std::filesystem::path &directory, int canvas_width) = 0;
        virtual void SetPreferences(const ClientPreferences &preferences, const std::filesystem::path &config,
                                    bool rebuild_background = true) = 0;
        virtual void Prepare() = 0;
        virtual void Invalidate() = 0;
        virtual void SetReferenceRendering(bool enabled) = 0;
        virtual std::size_t HudCacheHits() const = 0;

        virtual TileGeometryCatalog &EnsureGeometry(const std::vector<int> &red_tiles) = 0;
        virtual bool HasLoadedAssets() const = 0;

        virtual bool RenderToWindow(HWND, const TableRenderFrame &) { return false; }
        virtual void HideWindowSurface() {}
        virtual void RenderToDc(HDC dc, const TableRenderFrame &frame) = 0;
        virtual void RenderButtonToDc(HDC dc, const TableActionButton &button, unsigned dpi = 96) = 0;
    };
} // namespace tenhou
