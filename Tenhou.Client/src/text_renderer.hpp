#pragma once
#include "resource_manager.hpp"
#include "text_catalog.hpp"
#include "text_render_profile.hpp"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tenhou {
    // The caller owns GDI+ startup; the font service must outlive its renderer/layouts.
    // This service never silently substitutes another family.
    class TextFontService {
    public:
        TextFontService(); // Installed HGSeikaishotaiPRO for development.
        explicit TextFontService(const ResourceManager &resources); // Catalogued private font.
        ~TextFontService();
        TextFontService(const TextFontService &) = delete;
        TextFontService &operator=(const TextFontService &) = delete;
        const Gdiplus::FontFamily &family() const;
        bool native_bold() const;
        bool private_font() const;
        // Unicode scalar values, including supplementary planes; no fallback fonts.
        std::vector<std::uint32_t> MissingGlyphs(std::string_view utf8_text) const;
    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

    struct TextMetrics {
        double advance = 0, line_height = 0;
        Gdiplus::RectF ink;     // Visible glyph outlines in HD design pixels.
        Gdiplus::RectF effects; // Outline, blur, shadow and decoration included.
    };
    struct TextLayoutData;
    struct TextLayout {
        TextMetrics metrics;
        std::shared_ptr<const TextLayoutData> data;
    };
    // Physical destination pixels are already DPI-adjusted by the window owner.
    // DPI is recorded for cache invalidation, never multiplied into the size again.
    struct TextRenderContext {
        double native_layout_scale = 1; // SD = 1 / 1.4, HD = 1.
        double viewport_scale_x = 1, viewport_scale_y = 1;
        double origin_x = 0, origin_y = 0;
        unsigned dpi = 96;
        std::optional<Gdiplus::RectF> device_clip;
        static TextRenderContext Viewport(double native_width, double native_height,
                                          const Gdiplus::RectF &physical_destination, bool sd, unsigned dpi = 96);
    };
    struct SceneTextRun {
        TextLayout layout;
        Gdiplus::PointF anchor;
        TextAlignment alignment = TextAlignment::left;
        // GDI+ affine order: m11,m12,m21,m22,dx,dy, applied around the logical anchor.
        std::array<double, 6> transform{1, 0, 0, 1, 0, 0};
        double opacity = 1;
    };
    struct TextRasterPixels {
        int width, height;
        Bytes bgra; // Top-down premultiplied BGRA.
    };

    struct TextRaster {
        Gdiplus::Rect bounds;
        std::shared_ptr<const TextRasterPixels> pixels;
    };

    struct TextCacheStats {
        std::size_t layouts = 0, rasters = 0, raster_bytes = 0;
        std::size_t layout_builds = 0, raster_builds = 0, raster_hits = 0;
    };

    // UI-thread service. Layouts are reusable; transparent rasters are bounded and background-independent.
    class TextRenderer {
    public:
        explicit TextRenderer(const TextFontService &font, TextRenderProfile profile = {});
        ~TextRenderer();
        TextRenderer(const TextRenderer &) = delete;
        TextRenderer &operator=(const TextRenderer &) = delete;
        TextLayout Layout(TextKey key);
        TextLayout Layout(std::string_view utf8_text, TextStyleKey style);
        // All action captions, including completion, share this visual-center anchor.
        SceneTextRun Caption(TextKey key, Gdiplus::PointF center, double scale = 1, double opacity = 1);
        // Conservative physical raster bounds; uses exactly the same geometry as Draw.
        static Gdiplus::RectF Bounds(const TextRenderContext &context, const SceneTextRun &run);
        TextRaster Rasterize(const TextRenderContext &context, const SceneTextRun &run);
        Gdiplus::RectF Draw(Gdiplus::Graphics &target, const TextRenderContext &context, const SceneTextRun &run);
        TextCacheStats CacheStats() const;
        void ClearCaches();
    private:
        struct RasterData;
        std::shared_ptr<const RasterData> GetRaster(const TextRenderContext &context, const SceneTextRun &run,
                                                     Gdiplus::Rect &device);
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
