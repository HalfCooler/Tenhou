// Manual P1 specimen exporter. Intentionally not registered as an automated test.
#include "text_renderer.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

using namespace tenhou;
using namespace Gdiplus;
namespace {
    struct GdiSession {
        ULONG_PTR token = 0;
        GdiSession() {
            GdiplusStartupInput input;
            if (GdiplusStartup(&token, &input, nullptr) != Ok) throw std::runtime_error("GDI+ startup failed");
        }
        ~GdiSession() { GdiplusShutdown(token); }
    };
    struct Options {
        std::filesystem::path output, resources = L"build/Tenhou.Client.Tests/resources";
        bool resource_font = false;
        unsigned dpi = 96;
        std::vector<int> widths{1440, 1024, 1279};
    };
    Options Parse(int argc, wchar_t **argv) {
        Options result;
        for (int i = 1; i < argc; ++i) {
            const std::wstring arg = argv[i];
            if (i + 1 >= argc) throw std::runtime_error("Every option needs a value");
            const std::wstring value = argv[++i];
            if (arg == L"--output") result.output = value;
            else if (arg == L"--resources") result.resources = value;
            else if (arg == L"--font-source") {
                if (value != L"installed" && value != L"resources") throw std::runtime_error("Font source: installed or resources");
                result.resource_font = value == L"resources";
            } else if (arg == L"--dpi" || arg == L"--width") {
                std::size_t used = 0; const int n = std::stoi(value, &used);
                if (used != value.size()) throw std::runtime_error("Invalid numeric argument");
                if (arg == L"--dpi") {
                    if (n < 96 || n > 384) throw std::runtime_error("DPI must be 96..384");
                    result.dpi = static_cast<unsigned>(n);
                } else {
                    if (n < 640 || n > 2880) throw std::runtime_error("Width must be 640..2880");
                    result.widths = {n};
                }
            } else throw std::runtime_error("Unknown option");
        }
        if (result.output.empty()) throw std::runtime_error("Usage: tenhou_text_samples --output DIR [--font-source installed|resources] [--resources DIR] [--dpi 96] [--width 1440]");
        return result;
    }
    CLSID PngEncoder() {
        UINT count = 0, bytes = 0;
        if (GetImageEncodersSize(&count, &bytes) != Ok || !bytes) throw std::runtime_error("No image encoders");
        std::vector<BYTE> buffer(bytes);
        auto *encoders = reinterpret_cast<ImageCodecInfo *>(buffer.data());
        if (GetImageEncoders(count, bytes, encoders) != Ok) throw std::runtime_error("Cannot inspect encoders");
        for (UINT i = 0; i < count; ++i) if (wcscmp(encoders[i].MimeType, L"image/png") == 0) return encoders[i].Clsid;
        throw std::runtime_error("No PNG encoder");
    }
    void Label(Graphics &g, std::wstring_view text, REAL x, REAL y, REAL size = 16) {
        Font font(L"Segoe UI", size, FontStyleRegular, UnitPixel);
        SolidBrush brush(Color(255, 205, 215, 220));
        g.DrawString(text.data(), static_cast<INT>(text.size()), &font, PointF(x, y), &brush);
    }
    void Box(Graphics &g, const RectF &rect, Color color) {
        Pen pen(color, 1); g.DrawRectangle(&pen, rect);
    }
    class Page {
    public:
        Bitmap bitmap;
        Graphics graphics;
        double scale;
        TextRenderContext context;
        Page(int width, int design_height, unsigned dpi) :
            bitmap(width, static_cast<int>(std::ceil(design_height * width / 1440.0)), PixelFormat32bppPARGB),
            graphics(&bitmap), scale(width / 1440.0), context(TextRenderContext::Viewport(1440, design_height,
                RectF(0, 0, static_cast<REAL>(width), static_cast<REAL>(bitmap.GetHeight())), false, dpi)) {
            if (bitmap.GetLastStatus() != Ok) throw std::runtime_error("Cannot allocate specimen page");
            bitmap.SetResolution(static_cast<REAL>(dpi), static_cast<REAL>(dpi));
            graphics.SetPageUnit(UnitPixel); graphics.Clear(Color(255, 24, 39, 43));
        }
        void Text(std::wstring_view text, double x, double y, double size = 16) {
            Label(graphics, text, static_cast<REAL>(x * scale), static_cast<REAL>(y * scale), static_cast<REAL>(size * scale));
        }
        void Cell(double x, double y, double width, double height, TextColor fill) {
            const auto &color = kTextPalette[static_cast<std::size_t>(fill)].rgba;
            const bool dark_ink = color.r + color.g + color.b < 400;
            SolidBrush brush(dark_ink ? Color(255, 225, 227, 215) : Color(255, 32, 72, 67));
            graphics.FillRectangle(&brush, static_cast<REAL>(x * scale), static_cast<REAL>(y * scale),
                                   static_cast<REAL>(width * scale), static_cast<REAL>(height * scale));
        }
        void Save(const std::filesystem::path &path, const CLSID &encoder) {
            graphics.Flush(FlushIntentionSync);
            if (bitmap.Save(path.c_str(), &encoder) != Ok) throw std::runtime_error("Cannot save specimen PNG");
        }
    };
    TextLayout Sample(TextRenderer &renderer, unsigned origin) {
        const auto *definition = FindTextByOrigin(origin);
        if (!definition) throw std::runtime_error("Unknown specimen origin");
        return renderer.Layout(definition->key);
    }
    void LayoutRow(Page &page, TextRenderer &renderer, const TextLayout &layout, double x, double y,
                   bool bounds, double opacity = 1, std::array<double, 6> transform = {1, 0, 0, 1, 0, 0}) {
        SceneTextRun run{layout, PointF(static_cast<REAL>(x), static_cast<REAL>(y)), TextAlignment::left, transform, opacity};
        const auto device = renderer.Draw(page.graphics, page.context, run);
        if (bounds) {
            Box(page.graphics, device, Color(255, 255, 118, 91));
            // Ink diagnostic here applies to untransformed samples only; the affine samples show device effects.
            if (transform == std::array<double, 6>{1, 0, 0, 1, 0, 0}) {
                auto ink = layout.metrics.ink;
                ink.X = static_cast<REAL>((ink.X + x) * page.scale); ink.Y = static_cast<REAL>((ink.Y + y) * page.scale);
                ink.Width *= static_cast<REAL>(page.scale); ink.Height *= static_cast<REAL>(page.scale);
                Box(page.graphics, ink, Color(255, 98, 233, 161));
            }
        }
    }
    void Overview(const Options &options, TextRenderer &renderer, int width, bool bounds, const CLSID &encoder) {
        Page page(width, 1900, options.dpi);
        page.Text(L"P1 / HD design 1440 / device " + std::to_wstring(width) + L" / DPI " + std::to_wstring(options.dpi), 24, 14, 22);
        page.Text(L"Human-reviewed P1 profile. Green: ink / orange: device effect allocation.", 24, 48);
        std::size_t row = 0;
        for (const auto &style : kTextStyles) {
            const auto found = std::find_if(std::begin(kTextDefinitions), std::end(kTextDefinitions),
                [&](const TextDefinition &text) { return text.style == style.key; });
            if (found == std::end(kTextDefinitions)) continue;
            const double y = 90 + row++ * 76.0;
            page.Text(L"style " + std::to_wstring(static_cast<unsigned>(style.key)) + L" / " + std::to_wstring(found->origin_id) +
                L" / " + std::to_wstring(static_cast<int>(style.size_pt)) + L"pt", 24, y + 12);
            page.Cell(300, y, 1100, 70, style.fill);
            LayoutRow(page, renderer, renderer.Layout(found->key), 316, y, bounds);
        }
        double y = 1250;
        page.Text(L"459 / main 24pt + secondary 12pt", 24, y);
        LayoutRow(page, renderer, Sample(renderer, 459), 700, y - 4, bounds);
        y += 85;
        page.Text(L"595-598 hover / 599-602 normal", 24, y);
        for (unsigned i = 0; i < 4; ++i) {
            LayoutRow(page, renderer, Sample(renderer, 595 + i), 50 + i * 340.0, y + 32, bounds);
            LayoutRow(page, renderer, Sample(renderer, 599 + i), 50 + i * 340.0, y + 82, bounds);
        }
        y += 160;
        page.Text(L"Dynamic digits, ASCII minus, multiplier / full-string layout", 24, y);
        LayoutRow(page, renderer, renderer.Layout("-123456 +789 24飜 70符 32000点 4枚 ×12", TextStyleKey::yaku), 30, y + 32, bounds);
        page.Text(L"Caption scale / fade (each rendered at its own final size)", 24, y + 95);
        for (const auto &caption : {renderer.Caption(TextKey::caption_complete, PointF(250, static_cast<REAL>(y + 160)), .65, 1),
                                   renderer.Caption(TextKey::caption_complete, PointF(720, static_cast<REAL>(y + 160)), 1, .6),
                                   renderer.Caption(TextKey::caption_complete, PointF(1180, static_cast<REAL>(y + 160)), 1.15, .25)}) {
            const auto device = renderer.Draw(page.graphics, page.context, caption);
            if (bounds) Box(page.graphics, device, Color(255, 255, 118, 91));
        }
        const TextDefinition *longest = nullptr;
        for (const auto &text : kTextDefinitions)
            if (text.category == TextCategory::Yaku && (!longest || text.label.size() > longest->label.size())) longest = &text;
        page.Text(L"Longest fixed yaku / exact catalog wording", 24, 1760);
        if (longest) LayoutRow(page, renderer, renderer.Layout(longest->key), 30, 1790, bounds);
        const auto name = "overview-" + std::to_string(width) + (bounds ? "-bounds.png" : ".png");
        page.Save(options.output / name, encoder);
    }
    void Transforms(const Options &options, TextRenderer &renderer, int width, const CLSID &encoder) {
        Page page(width, 1080, options.dpi);
        page.Text(L"Final-size prototype / rotation, shear, clip, opacity / device " + std::to_wstring(width), 24, 20, 22);
        const auto seat = Sample(renderer, 416);
        const std::array<std::array<double, 6>, 4> transforms{{{1, .06, .04, 1, 0, 0}, {0, -1, 1, 0, 0, 0},
            {-1, 0, .04, -1, 0, 0}, {0, 1, -1, 0, 0, 0}}};
        const PointF anchors[]{PointF(640, 750), PointF(320, 580), PointF(800, 280), PointF(1080, 470)};
        for (std::size_t i = 0; i < transforms.size(); ++i)
            LayoutRow(page, renderer, seat, anchors[i].X, anchors[i].Y, true, 1, transforms[i]);
        for (unsigned i = 0; i < 5; ++i) {
            const auto *text = FindTextByOrigin(566 + i);
            LayoutRow(page, renderer, renderer.Layout(text->key), 590, 350 + i * 49.0, true);
        }
        auto clip = page.context;
        clip.device_clip = RectF(static_cast<REAL>(80 * page.scale), static_cast<REAL>(850 * page.scale),
                                  static_cast<REAL>(550 * page.scale), static_cast<REAL>(110 * page.scale));
        Box(page.graphics, *clip.device_clip, Color(255, 230, 190, 50));
        // The original x=50 left-aligned crop deliberately cut off this caption.
        // Use the same visible-center anchor as all real action captions instead.
        auto clipped = renderer.Caption(TextKey::caption_complete, PointF(355, 905));
        renderer.Draw(page.graphics, clip, clipped);
        page.Text(L"Caption centered in clip rectangle. Right: SD compatibility (native 1/1.4).", 80, 990);
        auto sd = TextRenderContext::Viewport(1024, 768, RectF(static_cast<REAL>(780 * page.scale), static_cast<REAL>(850 * page.scale),
            static_cast<REAL>(560 * page.scale), static_cast<REAL>(420 * page.scale)), true, options.dpi);
        sd.device_clip = page.context.device_clip;
        renderer.Draw(page.graphics, sd, SceneTextRun{Sample(renderer, 459), PointF(0, 0)});
        page.Save(options.output / ("transforms-" + std::to_string(width) + ".png"), encoder);
    }
    void InfoLayer(const Options &options, TextRenderer &renderer, int width, const CLSID &encoder) {
        const auto layout = renderer.Layout(TextKey::info_general);
        const auto scale = width / 1440.0;
        Bitmap bitmap(static_cast<INT>(std::ceil((layout.metrics.advance + 24) * scale)),
            static_cast<INT>(std::ceil((layout.metrics.effects.GetBottom() + 24) * scale)), PixelFormat32bppPARGB);
        if (bitmap.GetLastStatus() != Ok) throw std::runtime_error("Cannot allocate transparent Info specimen");
        bitmap.SetResolution(static_cast<REAL>(options.dpi), static_cast<REAL>(options.dpi));
        {
            Graphics graphics(&bitmap); graphics.SetPageUnit(UnitPixel); graphics.Clear(Color(0, 0, 0, 0));
            TextRenderContext context; context.viewport_scale_x = context.viewport_scale_y = scale; context.dpi = options.dpi;
            renderer.Draw(graphics, context, SceneTextRun{layout, PointF(12, 12)});
            graphics.Flush(FlushIntentionSync);
        }
        if (bitmap.Save((options.output / ("info-layer-" + std::to_string(width) + ".png")).c_str(), &encoder) != Ok)
            throw std::runtime_error("Cannot save transparent Info specimen");
    }
    void Catalog(const Options &options, TextRenderer &renderer, int width, const CLSID &encoder) {
        constexpr std::size_t per_page = 24;
        for (std::size_t start = 0; start < std::size(kTextDefinitions); start += per_page) {
            Page page(width, 1510, options.dpi);
            page.Text(L"All 296 fixed labels / page " + std::to_wstring(start / per_page + 1) + L" / OriginId, actual pt size", 24, 15, 22);
            for (std::size_t index = start; index < std::min(start + per_page, std::size(kTextDefinitions)); ++index) {
                const auto &text = kTextDefinitions[index]; const auto *style = FindTextStyle(text.style);
                const auto cell = index - start;
                const double x = 24 + (cell % 2) * 710.0, y = 65 + (cell / 2) * 119.0;
                page.Text(std::to_wstring(text.origin_id) + L" / " + std::to_wstring(static_cast<int>(style->size_pt)) + L"pt", x, y);
                page.Cell(x, y + 24, 688, 90, style->fill);
                auto context = page.context;
                context.device_clip = RectF(static_cast<REAL>(x * page.scale), static_cast<REAL>((y + 24) * page.scale),
                    static_cast<REAL>(688 * page.scale), static_cast<REAL>(90 * page.scale));
                renderer.Draw(page.graphics, context, SceneTextRun{renderer.Layout(text.key), PointF(static_cast<REAL>(x + 6), static_cast<REAL>(y + 24))});
            }
            std::ostringstream name; name << "catalog-" << width << '-' << std::setw(2) << std::setfill('0') << (start / per_page + 1) << ".png";
            page.Save(options.output / name.str(), encoder);
        }
    }
    std::string Scalars(const std::vector<std::uint32_t> &values) {
        if (values.empty()) return "none";
        std::ostringstream text;
        for (const auto value : values) text << "U+" << std::hex << std::uppercase << value << ' ';
        return text.str();
    }
}
int wmain(int argc, wchar_t **argv) {
    try {
        const auto options = Parse(argc, argv);
        GdiSession gdiplus;
        ResourceManager resources(options.resources);
        auto font = options.resource_font ? std::make_unique<TextFontService>(resources) : std::make_unique<TextFontService>();
        TextRenderer renderer(*font);
        const auto encoder = PngEncoder();
        std::filesystem::create_directories(options.output);
        for (const auto width : options.widths) {
            Overview(options, renderer, width, false, encoder);
            Overview(options, renderer, width, true, encoder);
            Transforms(options, renderer, width, encoder);
            InfoLayer(options, renderer, width, encoder);
            Catalog(options, renderer, width, encoder);
        }
        std::ofstream metrics(options.output / "metrics.csv", std::ios::binary);
        metrics << "origin_id,size_pt,spacing_pt,advance,line_height,ink_x,ink_y,ink_w,ink_h,effect_x,effect_y,effect_w,effect_h\n";
        std::string fixed;
        for (const auto &text : kTextDefinitions) {
            fixed += text.label;
            const auto *style = FindTextStyle(text.style); const auto m = renderer.Layout(text.key).metrics;
            metrics << text.origin_id << ',' << style->size_pt << ',' << style->spacing_pt << ',' << m.advance << ',' << m.line_height << ','
                << m.ink.X << ',' << m.ink.Y << ',' << m.ink.Width << ',' << m.ink.Height << ','
                << m.effects.X << ',' << m.effects.Y << ',' << m.effects.Width << ',' << m.effects.Height << '\n';
        }
        const auto stats = renderer.CacheStats();
        std::ofstream report(options.output / "README.md", std::ios::binary);
        report << "# P1 manual specimens\n\nUser completed visual review; caption centering and transparent Info layering corrected.\n\n"
            << "- Font: HGSeikaishotaiPRO (" << (font->private_font() ? "private resource" : "installed") << ")\n"
            << "- Bold: " << (font->native_bold() ? "native face" : "shared geometric emboldening") << "\n"
            << "- Profile: " << kTextRenderProfileStatus << "\n- DPI metadata: " << options.dpi << " (no additional size multiplier)\n"
            << "- Catalog source SHA-256: " << kTextSourceSha256 << "\n"
            << "- Compiled catalog JSON SHA-256: " << kTextRenderCatalogSha256 << "\n"
            << "- Compiled rendering profile SHA-256: " << kTextRenderProfileSha256 << "\n"
            << "- Fixed-label missing glyphs: " << Scalars(font->MissingGlyphs(fixed)) << "\n"
            << "- Dynamic repertoire missing glyphs: " << Scalars(font->MissingGlyphs("0123456789 -+飜符点枚×N未知役種")) << "\n"
            << "- Diagnostic U+2212 coverage: " << Scalars(font->MissingGlyphs("\u2212")) << " (production samples use ASCII '-')\n"
            << "- Cache after export: " << stats.layouts << " layouts, " << stats.rasters << " rasters, " << stats.raster_bytes << " bytes\n\n"
            << "overview: all styles, 459, button pairs, dynamic digits, scaled/faded captions.\n\n"
            << "transforms: four-seat affine transforms, shared-width Info, clipping and SD mapping.\n\n"
            << "info-layer: RGBA PNG on a fully transparent canvas; inspect its alpha/checkerboard in an image editor.\n\n"
            << "catalog: all 296 fixed definitions in OriginId order, including unused labels.\n\n"
            << "metrics.csv: shared layout measurements in HD design pixels; these are diagnostics, not acceptance results.\n";
        for (const auto width : options.widths) report << "\n![Overview " << width << "](overview-" << width << ".png)\n";
        if (!metrics || !report) throw std::runtime_error("Cannot write specimen diagnostics");
        std::cout << "P1 delivery specimens exported.\n";
        return 0;
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
