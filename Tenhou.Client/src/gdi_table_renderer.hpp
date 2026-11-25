#pragma once
#include "cached_hud.hpp"
#include "original_display.hpp"
#include "retained_scene.hpp"
#include "table_canvas_renderer.hpp"
#include "table_scene.hpp"
#include "tile_shadow.hpp"

namespace tenhou {
    void PaintTableInteraction(HDC dc, const TableRenderFrame &frame, TableArtwork &artwork,
                               bool grayscale = false);

    class GdiTableRenderer final : public ITableCanvasRenderer {
    public:
        GdiTableRenderer();
        ~GdiTableRenderer() override;

        void Configure(const std::filesystem::path &directory, int canvas_width) override;
        void SetPreferences(const ClientPreferences &preferences, const std::filesystem::path &config,
                            bool rebuild_background = true) override;
        void Prepare() override;
        void Invalidate() override;
        void SetReferenceRendering(bool enabled) override;
        std::size_t HudCacheHits() const override { return hud_cache_.hits(); }

        TileGeometryCatalog &EnsureGeometry(const std::vector<int> &red_tiles) override;
        bool HasLoadedAssets() const override { return artwork_.width() != 0 && !background_.bgra.empty(); }

        void RenderToDc(HDC dc, const TableRenderFrame &frame) override;
        void RenderButtonToDc(HDC dc, const TableActionButton &button, unsigned dpi = 96) override;

    private:
        std::filesystem::path directory_;
        std::filesystem::path config_;
        int width_ = 1440;
        ClientPreferences preferences_;
        TableArtwork artwork_;
        Image background_;
        DisplayArtwork display_artwork_;
        RetainedScene native_buffer_;
        CachedHud hud_cache_;
        TileShadowLayer shadow_layer_;
        HFONT ui_font_ = nullptr;
        HFONT exit_font_ = nullptr;
        bool reference_rendering_ = RetainedScene::Reference();
    };
} // namespace tenhou
