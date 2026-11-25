#pragma once
#include "table_canvas_renderer.hpp"
#include <memory>

namespace tenhou {
    class D2DTableRenderer final : public ITableCanvasRenderer {
    public:
        explicit D2DTableRenderer(bool warp = false);
        ~D2DTableRenderer() override;

        void Configure(const std::filesystem::path &directory, int canvas_width) override;
        void SetPreferences(const ClientPreferences &preferences, const std::filesystem::path &config,
                            bool rebuild_background = true) override;
        void Prepare() override;
        void Invalidate() override;
        void SetReferenceRendering(bool enabled) override;
        std::size_t HudCacheHits() const override;

        TileGeometryCatalog &EnsureGeometry(const std::vector<int> &red_tiles) override;
        bool HasLoadedAssets() const override;
        bool RenderToWindow(HWND parent, const TableRenderFrame &frame) override;
        void HideWindowSurface() override;
        void RenderToDc(HDC dc, const TableRenderFrame &frame) override;
        void RenderButtonToDc(HDC dc, const TableActionButton &button, unsigned dpi = 96) override;

        std::size_t PresentedFrames() const;
        std::size_t DeviceGeneration() const;
        // Exercises the same invalidation/retry path used by device removal.
        void SimulateDeviceLoss();

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
} // namespace tenhou
