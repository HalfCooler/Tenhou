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

        struct BenchStats {
            std::size_t texture_uploads = 0;
            std::size_t texture_upload_bytes = 0;
            std::size_t hud_rebuilds = 0;
            std::size_t interaction_rebuilds = 0;
            std::size_t shadow_rebuilds = 0;
            std::size_t static_base_hits = 0;
            std::size_t static_base_rebuilds = 0;
        };

        std::size_t PresentedFrames() const;
        std::size_t DeviceGeneration() const;
        bool UsingHardware() const;
        void SetSyncInterval(unsigned interval) const;
        BenchStats Stats() const;
        void ResetStats() const;
        // Exercises the same invalidation/retry path used by device removal.
        void SimulateDeviceLoss() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
} // namespace tenhou
