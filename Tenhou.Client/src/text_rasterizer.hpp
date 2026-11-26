#pragma once
#include "resource_pack.hpp"
#include "text_catalog.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

namespace tenhou {
    // Platform-neutral geometry and rasterization contracts for tenhou_text.
    // Free of <windows.h> / <gdiplus.h> so cross-platform backends (e.g. FreeType + HarfBuzz)
    // can implement ITextRasterizer alongside the Windows GDI+ reference TextRenderer.
    struct TextPointF {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct TextRectF {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    struct TextRectI {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    struct TextRasterPixels {
        int width = 0;
        int height = 0;
        Bytes bgra; // Top-down premultiplied BGRA.
    };

    struct TextCacheStats {
        std::size_t layouts = 0, rasters = 0, raster_bytes = 0;
        std::size_t layout_builds = 0, raster_builds = 0, raster_hits = 0;
    };

    struct TextLayoutMetrics {
        double advance = 0.0;
        double line_height = 0.0;
        TextRectF ink{};     // Visible glyph outlines in HD design pixels.
        TextRectF effects{}; // Outline, blur, shadow and decoration included.
    };

    struct TextLayoutData;

    struct PortableTextLayout {
        TextLayoutMetrics metrics{};
        std::shared_ptr<const TextLayoutData> data;
    };

    struct PortableTextContext {
        double native_layout_scale = 1.0; // SD = 1 / 1.4, HD = 1.
        double viewport_scale_x = 1.0;
        double viewport_scale_y = 1.0;
        double origin_x = 0.0;
        double origin_y = 0.0;
        unsigned dpi = 96;
        std::optional<TextRectF> device_clip;
    };

    struct PortableTextRun {
        PortableTextLayout layout;
        TextPointF anchor{};
        TextAlignment alignment = TextAlignment::left;
        std::array<double, 6> transform{1, 0, 0, 1, 0, 0};
        double opacity = 1.0;
    };

    struct PortableTextRaster {
        TextRectI bounds{};
        std::shared_ptr<const TextRasterPixels> pixels;
    };

    class ITextRasterizer {
    public:
        virtual ~ITextRasterizer() = default;

        virtual PortableTextLayout LayoutPortable(TextKey key) = 0;
        virtual PortableTextLayout LayoutPortable(std::string_view utf8_text, TextStyleKey style) = 0;
        virtual TextRectF MeasureBoundsPortable(const PortableTextContext &context,
                                                const PortableTextRun &run) const = 0;
        virtual PortableTextRaster RasterizePortable(const PortableTextContext &context,
                                                     const PortableTextRun &run) = 0;
        virtual TextCacheStats CacheStats() const = 0;
        virtual void ClearCaches() = 0;
    };
} // namespace tenhou
