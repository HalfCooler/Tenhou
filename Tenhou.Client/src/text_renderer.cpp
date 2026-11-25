#include "text_renderer.hpp"
#include <usp10.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <list>
#include <set>
#include <sstream>
#include <stdexcept>

namespace tenhou {
    namespace {
        using namespace Gdiplus;
        constexpr double kPt = 96.0 / 72.0;
        constexpr std::size_t kLayoutLimit = 256, kRasterLimit = 64, kRasterBytes = 32 * 1024 * 1024;
        void Check(Status status, const char *message) {
            if (status != Ok) throw std::runtime_error(message);
        }
        REAL Real(double value) { return static_cast<REAL>(value); }
        std::uint16_t U16(const Bytes &b, std::size_t p) {
            if (p > b.size() || b.size() - p < 2) throw std::runtime_error("Truncated font cmap");
            return static_cast<std::uint16_t>((b[p] << 8) | b[p + 1]);
        }
        std::uint32_t U32(const Bytes &b, std::size_t p) {
            return (static_cast<std::uint32_t>(U16(b, p)) << 16) | U16(b, p + 2);
        }
        Bytes FontTable(const Bytes &font, std::uint32_t tag) {
            const auto count = U16(font, 4);
            for (std::size_t i = 0; i < count; ++i) {
                const auto p = 12 + i * 16;
                if (U32(font, p) != tag) continue;
                const std::size_t offset = U32(font, p + 8), size = U32(font, p + 12);
                if (offset > font.size() || size > font.size() - offset) throw std::runtime_error("Invalid font table");
                return Bytes(font.begin() + offset, font.begin() + offset + size);
            }
            throw std::runtime_error("Missing font table");
        }
        bool HasGlyph(const Bytes &cmap, std::uint32_t scalar) {
            const auto count = U16(cmap, 2);
            for (std::size_t i = 0; i < count; ++i) {
                const auto record = 4 + i * 8;
                const auto platform = U16(cmap, record), encoding = U16(cmap, record + 2);
                if (platform != 0 && !(platform == 3 && (encoding == 1 || encoding == 10))) continue;
                const std::size_t p = U32(cmap, record + 4);
                const auto format = U16(cmap, p);
                if (format == 12) {
                    const std::size_t groups = U32(cmap, p + 12);
                    for (std::size_t g = 0; g < groups; ++g) {
                        const auto start = U32(cmap, p + 16 + g * 12), end = U32(cmap, p + 20 + g * 12);
                        if (scalar >= start && scalar <= end)
                            return U32(cmap, p + 24 + g * 12) + scalar - start != 0;
                    }
                } else if (format == 4 && scalar <= 0xffff) {
                    const std::size_t segments = U16(cmap, p + 6) / 2, ends = p + 14;
                    const auto starts = ends + 2 * segments + 2, deltas = starts + 2 * segments;
                    const auto offsets = deltas + 2 * segments;
                    for (std::size_t s = 0; s < segments; ++s) {
                        const auto start = U16(cmap, starts + 2 * s), end = U16(cmap, ends + 2 * s);
                        if (scalar < start || scalar > end) continue;
                        const auto delta = U16(cmap, deltas + 2 * s), offset = U16(cmap, offsets + 2 * s);
                        if (!offset) return static_cast<std::uint16_t>(scalar + delta) != 0;
                        const auto glyph = U16(cmap, offsets + 2 * s + offset + 2 * (scalar - start));
                        return glyph != 0 && static_cast<std::uint16_t>(glyph + delta) != 0;
                    }
                }
            }
            return false;
        }
        std::vector<std::size_t> Clusters(std::wstring_view text) {
            std::vector<std::size_t> result{0};
            if (text.empty()) return result;
            SCRIPT_STRING_ANALYSIS analysis = nullptr;
            const auto length = static_cast<int>(text.size());
            if (FAILED(ScriptStringAnalyse(nullptr, text.data(), length, length * 3 / 2 + 16, -1, SSA_BREAK,
                                          0, nullptr, nullptr, nullptr, nullptr, nullptr, &analysis)))
                throw std::runtime_error("Cannot analyse text clusters");
            const auto *attributes = ScriptString_pLogAttr(analysis);
            if (!attributes) { ScriptStringFree(&analysis); throw std::runtime_error("Missing text cluster attributes"); }
            for (std::size_t i = 1; i < text.size(); ++i) {
                const auto c = text[i];
                std::uint32_t scalar = c;
                if (c >= 0xd800 && c <= 0xdbff && i + 1 < text.size() && text[i + 1] >= 0xdc00 && text[i + 1] <= 0xdfff)
                    scalar = 0x10000 + ((c - 0xd800) << 10) + text[i + 1] - 0xdc00;
                // Do not split a surrogate pair, variation sequence or joined cluster even on older Uniscribe.
                if (!attributes[i].fCharStop || (c >= 0xdc00 && c <= 0xdfff) ||
                    (scalar >= 0xfe00 && scalar <= 0xfe0f) || (scalar >= 0xe0100 && scalar <= 0xe01ef) ||
                    (scalar >= 0x1f3fb && scalar <= 0x1f3ff) || c == 0x200d || text[i - 1] == 0x200d) continue;
                result.push_back(i);
            }
            ScriptStringFree(&analysis);
            result.push_back(text.size());
            return result;
        }
        RectF Union(RectF a, const RectF &b) {
            if (a.Width <= 0 || a.Height <= 0) return b;
            if (b.Width <= 0 || b.Height <= 0) return a;
            RectF result; RectF::Union(result, a, b); return result;
        }
        Color Palette(TextColor key) {
            const auto index = static_cast<std::size_t>(key);
            if (index >= std::size(kTextPalette)) throw std::runtime_error("Invalid text color");
            const auto c = kTextPalette[index].rgba;
            return Color(c.a, c.r, c.g, c.b);
        }
        void Gradient(Graphics &g, RectF rect, const TextGradientStop *stops, std::size_t count, double alpha,
                      bool highlight = false) {
            std::vector<Color> colors;
            std::vector<REAL> positions;
            const auto interpolate = [](double position, const auto *nodes, std::size_t size, auto value) {
                for (std::size_t i = 1; i < size; ++i) {
                    if (position > nodes[i].position) continue;
                    const auto t = (position - nodes[i - 1].position) / (nodes[i].position - nodes[i - 1].position);
                    return value(nodes[i - 1]) * (1 - t) + value(nodes[i]) * t;
                }
                return value(nodes[size - 1]);
            };
            // The reference's gray values describe the result on white. Body/last row
            // are black alpha masks, not RGB gray rectangles. The penultimate row is
            // a separate light-gray highlight with the reference's fade-in envelope.
            // Sample the two interpolants together so premultiplied compositing also
            // preserves the original gray nodes between stops.
            for (unsigned i = 0; i <= 256; ++i) {
                const double position = i / 256.0;
                const auto gray = interpolate(position, stops, count, [](const auto &stop) { return double(stop.gray); });
                double coverage = (255 - gray) / 255;
                double channel = 0;
                if (highlight) {
                    coverage = std::max(coverage, interpolate(position, kInfoHighlightAlpha, std::size(kInfoHighlightAlpha),
                        [](const auto &stop) { return stop.alpha; }));
                    if (coverage > 0) channel = std::clamp((gray - 255 * (1 - coverage)) / coverage, 0.0, 255.0);
                }
                const auto c = static_cast<BYTE>(std::lround(channel));
                colors.emplace_back(static_cast<BYTE>(std::lround(coverage * alpha * 255)), c, c, c);
                positions.push_back(Real(position));
            }
            LinearGradientBrush brush(PointF(rect.X, rect.Y), PointF(rect.GetRight(), rect.Y), colors.front(), colors.back());
            Check(brush.SetInterpolationColors(colors.data(), positions.data(), static_cast<INT>(colors.size())), "Cannot set Info gradient");
            Check(g.FillRectangle(&brush, rect), "Cannot fill Info gradient");
        }
        // Three separable box passes approximate a Gaussian. The mask is generated at final device size.
        void Blur(Bitmap &bitmap, int radius) {
            if (radius <= 0) return;
            const int width = static_cast<int>(bitmap.GetWidth()), height = static_cast<int>(bitmap.GetHeight());
            BitmapData lock{};
            const Rect rect(0, 0, width, height);
            Check(bitmap.LockBits(&rect, ImageLockModeRead | ImageLockModeWrite, PixelFormat32bppPARGB, &lock), "Cannot lock shadow");
            std::vector<unsigned> a(static_cast<std::size_t>(width) * height), b(a.size());
            for (int y = 0; y < height; ++y) {
                const auto *row = reinterpret_cast<const std::uint32_t *>(static_cast<const BYTE *>(lock.Scan0) + y * lock.Stride);
                for (int x = 0; x < width; ++x) a[static_cast<std::size_t>(y) * width + x] = row[x] >> 24;
            }
            for (int pass = 0; pass < 3; ++pass) {
                for (int y = 0; y < height; ++y) {
                    unsigned sum = 0;
                    for (int x = 0; x <= std::min(radius, width - 1); ++x) sum += a[static_cast<std::size_t>(y) * width + x];
                    for (int x = 0; x < width; ++x) {
                        b[static_cast<std::size_t>(y) * width + x] = sum / static_cast<unsigned>(2 * radius + 1);
                        if (x - radius >= 0) sum -= a[static_cast<std::size_t>(y) * width + x - radius];
                        if (x + radius + 1 < width) sum += a[static_cast<std::size_t>(y) * width + x + radius + 1];
                    }
                }
                a.swap(b);
                for (int x = 0; x < width; ++x) {
                    unsigned sum = 0;
                    for (int y = 0; y <= std::min(radius, height - 1); ++y) sum += a[static_cast<std::size_t>(y) * width + x];
                    for (int y = 0; y < height; ++y) {
                        b[static_cast<std::size_t>(y) * width + x] = sum / static_cast<unsigned>(2 * radius + 1);
                        if (y - radius >= 0) sum -= a[static_cast<std::size_t>(y - radius) * width + x];
                        if (y + radius + 1 < height) sum += a[static_cast<std::size_t>(y + radius + 1) * width + x];
                    }
                }
                a.swap(b);
            }
            for (int y = 0; y < height; ++y) {
                auto *row = reinterpret_cast<std::uint32_t *>(static_cast<BYTE *>(lock.Scan0) + y * lock.Stride);
                for (int x = 0; x < width; ++x) row[x] = a[static_cast<std::size_t>(y) * width + x] << 24;
            }
            Check(bitmap.UnlockBits(&lock), "Cannot unlock shadow");
        }
    }

    struct TextFontService::Impl {
        Bytes bytes, cmap;
        PrivateFontCollection collection;
        std::unique_ptr<FontFamily> family;
        bool private_source = false, bold = false;
    };
    TextFontService::TextFontService() : impl_(std::make_unique<Impl>()) {
        impl_->family = std::make_unique<FontFamily>(L"HGSeikaishotaiPRO");
        if (!impl_->family->IsAvailable() || !impl_->family->IsStyleAvailable(FontStyleRegular))
            throw std::runtime_error("HGSeikaishotaiPRO is not installed; select the resource font explicitly");
        HDC dc = CreateCompatibleDC(nullptr);
        if (!dc) throw std::runtime_error("Cannot create font coverage context");
        LOGFONTW description{}; description.lfHeight = -32; description.lfCharSet = DEFAULT_CHARSET;
        wcscpy_s(description.lfFaceName, L"HGSeikaishotaiPRO");
        const auto font = CreateFontIndirectW(&description);
        if (!font) { DeleteDC(dc); throw std::runtime_error("Cannot select installed text font"); }
        const auto previous = SelectObject(dc, font);
        const DWORD size = GetFontData(dc, 0x70616d63, 0, nullptr, 0); // 'cmap', GDI byte order.
        bool read = size != GDI_ERROR && size > 0;
        if (read) { impl_->cmap.resize(size); read = GetFontData(dc, 0x70616d63, 0, impl_->cmap.data(), size) == size; }
        SelectObject(dc, previous); DeleteObject(font);
        EnumFontFamiliesExW(dc, &description, [](const LOGFONTW *face, const TEXTMETRICW *, DWORD, LPARAM state) -> int {
            if (face->lfWeight >= FW_BOLD) *reinterpret_cast<bool *>(state) = true;
            return 1;
        }, reinterpret_cast<LPARAM>(&impl_->bold), 0);
        DeleteDC(dc);
        if (!read) throw std::runtime_error("Cannot read installed font cmap");
        impl_->bold = impl_->bold && impl_->family->IsStyleAvailable(FontStyleBold);
    }
    TextFontService::TextFontService(const ResourceManager &resources) : impl_(std::make_unique<Impl>()) {
        impl_->bytes = resources.Read(AssetId::TextFont);
        if (impl_->bytes.empty() || impl_->bytes.size() > static_cast<std::size_t>(INT_MAX))
            throw std::runtime_error("Invalid private text font");
        Check(impl_->collection.AddMemoryFont(impl_->bytes.data(), static_cast<INT>(impl_->bytes.size())), "Cannot load private text font");
        impl_->family = std::make_unique<FontFamily>(L"HGSeikaishotaiPRO", &impl_->collection);
        if (!impl_->family->IsAvailable() || !impl_->family->IsStyleAvailable(FontStyleRegular))
            throw std::runtime_error("Private text font has the wrong family or no regular face");
        impl_->cmap = FontTable(impl_->bytes, 0x636d6170);
        const auto head = FontTable(impl_->bytes, 0x68656164);
        impl_->bold = (U16(head, 44) & 1) != 0 && impl_->family->IsStyleAvailable(FontStyleBold);
        impl_->private_source = true;
    }
    TextFontService::~TextFontService() = default;
    const FontFamily &TextFontService::family() const { return *impl_->family; }
    bool TextFontService::native_bold() const { return impl_->bold; }
    bool TextFontService::private_font() const { return impl_->private_source; }
    std::vector<std::uint32_t> TextFontService::MissingGlyphs(std::wstring_view text) const {
        std::set<std::uint32_t> missing;
        for (std::size_t i = 0; i < text.size(); ++i) {
            std::uint32_t c = text[i];
            if (c >= 0xd800 && c <= 0xdbff && i + 1 < text.size() && text[i + 1] >= 0xdc00 && text[i + 1] <= 0xdfff)
                c = 0x10000 + ((c - 0xd800) << 10) + text[++i] - 0xdc00;
            if (c == '\n' || c == '\r' || c == '\t') continue;
            if (!HasGlyph(impl_->cmap, c)) missing.insert(c);
        }
        return {missing.begin(), missing.end()};
    }

    struct TextLayoutData {
        GraphicsPath path{FillModeWinding};
        TextStyle style{};
        TextMetrics metrics;
        RectF info_rect;
        double info_rule_height = 0;
        std::size_t serial = 0;
    };
    struct TextRenderer::RasterData {
        TextRasterPixels pixels;
        mutable Bitmap bitmap;

        RasterData(int width, int height)
            : pixels{width, height, Bytes(static_cast<std::size_t>(width) * height * 4)},
              bitmap(width, height, width * 4, PixelFormat32bppPARGB, pixels.bgra.data()) {}
    };

    struct TextRenderer::Impl {
        const TextFontService &font;
        TextRenderProfile profile;
        Bitmap measurement{1, 1, PixelFormat32bppPARGB};
        struct LayoutEntry { std::wstring text; TextStyleKey style; TextComposition composition; TextLayout layout; };
        struct RasterEntry {
            std::shared_ptr<const TextLayoutData> layout;
            std::array<REAL, 6> matrix;
            unsigned dpi;
            Rect bounds;
            std::shared_ptr<const RasterData> raster;
            std::size_t bytes;
        };
        std::list<LayoutEntry> layouts;
        std::list<RasterEntry> rasters;
        std::size_t serial = 0, raster_bytes = 0;
        std::size_t raster_builds = 0, raster_hits = 0;
        Impl(const TextFontService &f, TextRenderProfile p) : font(f), profile(p) {
            const double nonnegative[]{p.outline_width_pt, p.synthetic_bold_width_pt, p.shadow_blur_pt,
                p.info_padding_x_pt, p.info_padding_y_pt, p.info_rule_height_pt};
            for (const auto value : nonnegative)
                if (!std::isfinite(value) || value < 0 || value > 64) throw std::runtime_error("Invalid text effect parameter");
            const double signed_values[]{p.shadow_offset_x_pt, p.shadow_offset_y_pt, p.chi_secondary_gap_pt, p.chi_secondary_drop_pt};
            for (const auto value : signed_values)
                if (!std::isfinite(value) || std::abs(value) > 64) throw std::runtime_error("Invalid text effect offset");
            if (!std::isfinite(p.shadow_alpha) || p.shadow_alpha < 0 || p.shadow_alpha > 1 ||
                !std::isfinite(p.info_background_alpha) || p.info_background_alpha < 0 || p.info_background_alpha > 1 ||
                p.info_rule_height_pt <= 0) throw std::runtime_error("Invalid text alpha or gradient row height");
        }

        double AddText(GraphicsPath &path, std::wstring_view text, const TextStyle &style, double x = 0, double y = 0) {
            if (text.empty()) return 0;
            const auto em = Real(style.size_pt * kPt);
            const int face = style.bold && font.native_bold() ? FontStyleBold : FontStyleRegular;
            Font gdifont(&font.family(), em, face, UnitPixel);
            Check(gdifont.GetLastStatus(), "Cannot create text measurement font");
            StringFormat format(StringFormat::GenericTypographic());
            format.SetFormatFlags(format.GetFormatFlags() | StringFormatFlagsMeasureTrailingSpaces |
                                  StringFormatFlagsNoWrap | StringFormatFlagsNoFontFallback);
            Graphics measure(&measurement); measure.SetPageUnit(UnitPixel);
            RectF size;
            Check(measure.MeasureString(text.data(), static_cast<INT>(text.size()), &gdifont, PointF(), &format, &size), "Cannot measure text");
            GraphicsPath glyphs(FillModeWinding);
            double advance = size.Width;
            if (style.spacing_pt == 0) {
                Check(glyphs.AddString(text.data(), static_cast<INT>(text.size()), &font.family(), face, em,
                                      PointF(Real(x), Real(y)), &format), "Cannot build text outline");
            } else {
                const auto clusters = Clusters(text);
                const auto count = clusters.size() - 1;
                // Range positions come from the complete string, preserving mixed-script advances.
                // Each shaping/character cluster is passed intact to AddString.
                for (std::size_t base = 0; base < count; base += 32) {
                    const auto batch = std::min<std::size_t>(32, count - base);
                    CharacterRange ranges[32]; Region regions[32];
                    for (std::size_t j = 0; j < batch; ++j)
                        ranges[j] = CharacterRange(static_cast<INT>(clusters[base + j]), static_cast<INT>(clusters[base + j + 1] - clusters[base + j]));
                    Check(format.SetMeasurableCharacterRanges(static_cast<INT>(batch), ranges), "Cannot set cluster ranges");
                    Check(measure.MeasureCharacterRanges(text.data(), static_cast<INT>(text.size()), &gdifont,
                        RectF(0, 0, std::max(1.0f, size.Width + em * 2), em * 4), &format, static_cast<INT>(batch), regions), "Cannot measure text clusters");
                    for (std::size_t j = 0; j < batch; ++j) {
                        RectF range; Check(regions[j].GetBounds(&range, &measure), "Cannot read cluster bounds");
                        const auto index = base + j;
                        Check(glyphs.AddString(text.data() + clusters[index], static_cast<INT>(clusters[index + 1] - clusters[index]),
                            &font.family(), face, em, PointF(Real(x + range.X + index * style.spacing_pt * kPt), Real(y)), &format), "Cannot build spaced text outline");
                    }
                }
                if (count > 1) advance += (count - 1) * style.spacing_pt * kPt;
            }
            if (style.bold && !font.native_bold() && glyphs.GetPointCount() > 0) {
                // One shared synthetic-bold rule, applied geometrically before effects/measurement.
                auto wide = std::unique_ptr<GraphicsPath>(glyphs.Clone());
                Pen pen(Color(255, 0, 0, 0), Real(profile.synthetic_bold_width_pt * kPt)); pen.SetLineJoin(LineJoinRound);
                if (pen.GetWidth() > 0) {
                    Check(wide->Widen(&pen), "Cannot synthesize bold outlines");
                    Check(glyphs.AddPath(wide.get(), FALSE), "Cannot merge synthetic bold outline");
                }
            }
            if (glyphs.GetPointCount() > 0) Check(path.AddPath(&glyphs, FALSE), "Cannot append text outline");
            return advance;
        }
        TextLayout Layout(std::wstring_view text, TextStyleKey key, TextComposition composition) {
            if (text.size() > 4096 || text.find_first_of(L"\r\n\t") != std::wstring_view::npos)
                throw std::runtime_error("TextRenderer requires a single line of at most 4096 UTF-16 units");
            for (auto it = layouts.begin(); it != layouts.end(); ++it) {
                if (it->text == text && it->style == key && it->composition == composition) {
                    auto result = it->layout; layouts.splice(layouts.begin(), layouts, it); return result;
                }
            }
            const auto *style = FindTextStyle(key);
            if (!style) throw std::runtime_error("Invalid text style");
            if (style->font != L"HGSeikaishotaiPRO") throw std::runtime_error("Text style requires an unsupported font family");
            auto data = std::make_shared<TextLayoutData>(); data->style = *style; data->serial = ++serial;
            auto &metrics = data->metrics;
            const auto face = style->bold && font.native_bold() ? FontStyleBold : FontStyleRegular;
            const auto units = font.family().GetEmHeight(face);
            if (!units) throw std::runtime_error("Invalid font metrics");
            metrics.line_height = style->size_pt * kPt * font.family().GetLineSpacing(face) / units;
            if (composition == TextComposition::chi_selection) {
                metrics.advance = AddText(data->path, kChiSelection.main, *style);
                auto secondary = *style; secondary.size_pt = kChiSelection.secondary_size_pt.value(); secondary.spacing_pt = 0;
                const auto y = (style->size_pt - secondary.size_pt + profile.chi_secondary_drop_pt) * kPt;
                const auto x = metrics.advance + profile.chi_secondary_gap_pt * kPt;
                metrics.advance = x + AddText(data->path, kChiSelection.secondary, secondary, x, y);
            } else metrics.advance = AddText(data->path, text, *style);
            if (data->path.GetPointCount() > 0) Check(data->path.GetBounds(&metrics.ink), "Cannot measure text ink");
            metrics.effects = metrics.ink;
            if (style->outline != TextColor::none && metrics.effects.Width > 0)
                metrics.effects.Inflate(Real(profile.outline_width_pt * kPt / 2), Real(profile.outline_width_pt * kPt / 2));
            if (style->shadow && metrics.ink.Width > 0) {
                auto shadow = metrics.effects;
                shadow.Offset(Real(profile.shadow_offset_x_pt * kPt), Real(profile.shadow_offset_y_pt * kPt));
                shadow.Inflate(Real(3 * profile.shadow_blur_pt * kPt + 2), Real(3 * profile.shadow_blur_pt * kPt + 2));
                metrics.effects = Union(metrics.effects, shadow);
            }
            if (style->decoration == TextDecoration::info_gradient) {
                GraphicsPath reference;
                const double width = AddText(reference, L"国", *style) * kInfoWidthBaseAdvances;
                const double px = profile.info_padding_x_pt * kPt, py = profile.info_padding_y_pt * kPt;
                data->info_rule_height = profile.info_rule_height_pt * kPt;
                data->info_rect = RectF(0, 0, Real(width + 2 * px), Real(metrics.line_height + 2 * py + 2 * data->info_rule_height));
                Matrix move; move.Translate(Real(width + px - metrics.advance), Real(py));
                Check(data->path.Transform(&move), "Cannot position Info text");
                metrics.ink.Offset(Real(width + px - metrics.advance), Real(py));
                metrics.effects.Offset(Real(width + px - metrics.advance), Real(py));
                metrics.advance = data->info_rect.Width;
                metrics.effects = Union(metrics.effects, data->info_rect);
            }
            TextLayout result{metrics, data};
            layouts.push_front({std::wstring(text), key, composition, result});
            if (layouts.size() > kLayoutLimit) layouts.pop_back();
            return result;
        }
    };

    TextRenderContext TextRenderContext::Viewport(double native_width, double native_height,
                                                  const RectF &destination, bool sd, unsigned dpi) {
        if (native_width <= 0 || native_height <= 0 || destination.Width <= 0 || destination.Height <= 0)
            throw std::runtime_error("Invalid text viewport");
        TextRenderContext result;
        result.native_layout_scale = sd ? 1.0 / 1.4 : 1;
        result.viewport_scale_x = destination.Width / native_width;
        result.viewport_scale_y = destination.Height / native_height;
        result.origin_x = destination.X; result.origin_y = destination.Y;
        result.device_clip = destination; result.dpi = dpi;
        return result;
    }
    TextRenderer::TextRenderer(const TextFontService &font, TextRenderProfile profile) : impl_(std::make_unique<Impl>(font, profile)) {}
    TextRenderer::~TextRenderer() = default;
    TextLayout TextRenderer::Layout(TextKey key) {
        const auto *text = FindText(key);
        if (!text) throw std::runtime_error("Invalid text key");
        return impl_->Layout(text->label, text->style, text->composition);
    }
    TextLayout TextRenderer::Layout(std::wstring_view text, TextStyleKey style) { return impl_->Layout(text, style, TextComposition::plain); }

    SceneTextRun TextRenderer::Caption(TextKey key, PointF center, double scale, double opacity) {
        const auto *definition = FindText(key);
        if (!definition || definition->category != TextCategory::Caption)
            throw std::runtime_error("Caption requires a caption text key");
        if (!std::isfinite(scale) || scale <= 0) throw std::runtime_error("Invalid caption scale");
        auto layout = Layout(key);
        // Center the visible glyphs before applying the appearance scale; shadow must
        // not pull the text's anchor away from the other action captions.
        const auto &ink = layout.metrics.ink;
        return {layout, center, TextAlignment::left,
            {scale, 0, 0, scale, -(ink.X + ink.Width / 2) * scale, -(ink.Y + ink.Height / 2) * scale}, opacity};
    }

    namespace {
        struct DeviceTextGeometry {
            std::array<REAL, 6> elements{};
            Rect device;
        };
        DeviceTextGeometry TextGeometry(const TextRenderContext &context, const SceneTextRun &run) {
            if (!run.layout.data || run.opacity <= 0 || run.layout.metrics.effects.Width <= 0 ||
                run.layout.metrics.effects.Height <= 0) return {};
            const auto &data = *run.layout.data;
            const auto sx = context.native_layout_scale * context.viewport_scale_x, sy = context.native_layout_scale * context.viewport_scale_y;
            if (!std::isfinite(sx) || !std::isfinite(sy) || sx <= 0 || sy <= 0 || !std::isfinite(run.opacity))
                throw std::runtime_error("Invalid text render context");
            const auto &t = run.transform;
            const double align = run.alignment == TextAlignment::center ? -data.metrics.advance / 2 :
                                 run.alignment == TextAlignment::right ? -data.metrics.advance : 0;
            std::array<REAL, 6> elements{Real(t[0] * sx), Real(t[1] * sy), Real(t[2] * sx), Real(t[3] * sy),
                Real(context.origin_x + (run.anchor.X + t[4] + align * t[0]) * sx),
                Real(context.origin_y + (run.anchor.Y + t[5] + align * t[1]) * sy)};
            for (const auto value : elements) if (!std::isfinite(value)) throw std::runtime_error("Invalid text affine transform");
            Matrix matrix(elements[0], elements[1], elements[2], elements[3], elements[4], elements[5]);
            if (!matrix.IsInvertible()) return {};
            auto bounds = data.metrics.effects;
            PointF corners[]{PointF(bounds.X, bounds.Y), PointF(bounds.GetRight(), bounds.Y),
                             PointF(bounds.X, bounds.GetBottom()), PointF(bounds.GetRight(), bounds.GetBottom())};
            Check(matrix.TransformPoints(corners, 4), "Cannot transform text bounds");
            REAL left = corners[0].X, right = left, top = corners[0].Y, bottom = top;
            for (const auto p : corners) { left = std::min(left, p.X); right = std::max(right, p.X); top = std::min(top, p.Y); bottom = std::max(bottom, p.Y); }
            // Shadow box support is rounded in device pixels. Include its rounding allowance after rotation/shear.
            const REAL guard = data.style.shadow ? 12.0f : 2.0f;
            left = std::floor(left - guard); top = std::floor(top - guard);
            right = std::ceil(right + guard); bottom = std::ceil(bottom + guard);
            if (left < -1000000 || top < -1000000 || right > 1000000 || bottom > 1000000 ||
                right - left > 16384 || bottom - top > 16384 || (right - left) * (bottom - top) > 16 * 1024 * 1024)
                throw std::runtime_error("Text raster exceeds bounded allocation");
            const Rect device(static_cast<INT>(left), static_cast<INT>(top), static_cast<INT>(right - left), static_cast<INT>(bottom - top));
            return {elements, device};
        }
    }
    RectF TextRenderer::Bounds(const TextRenderContext &context, const SceneTextRun &run) {
        const auto device = TextGeometry(context, run).device;
        return RectF(Real(device.X), Real(device.Y), Real(device.Width), Real(device.Height));
    }
    std::shared_ptr<const TextRenderer::RasterData> TextRenderer::GetRaster(
        const TextRenderContext &context, const SceneTextRun &run, Rect &device) {
        auto geometry = TextGeometry(context, run);
        device = geometry.device;
        if (device.IsEmptyArea()) return {};
        auto elements = geometry.elements;
        const auto &data = *run.layout.data;
        const auto left = Real(device.X), top = Real(device.Y);
        // Cache translation only by subpixel phase; integer movement can reuse the same raster.
        elements[4] -= left; elements[5] -= top;
        auto found = impl_->rasters.end();
        for (auto it = impl_->rasters.begin(); it != impl_->rasters.end(); ++it)
            if (it->layout == run.layout.data && it->matrix == elements && it->dpi == context.dpi &&
                it->bounds.Width == device.Width && it->bounds.Height == device.Height) { found = it; break; }
        if (found == impl_->rasters.end()) {
            ++impl_->raster_builds;
            auto raster = std::make_shared<RasterData>(device.Width, device.Height);
            Check(raster->bitmap.GetLastStatus(), "Cannot allocate text raster");
            Matrix local(elements[0], elements[1], elements[2], elements[3], elements[4], elements[5]);
            Graphics g(&raster->bitmap); g.SetPageUnit(UnitPixel); g.Clear(Color(0, 0, 0, 0));
            g.SetSmoothingMode(SmoothingModeAntiAlias); g.SetPixelOffsetMode(PixelOffsetModeHalf);
            Check(g.SetTransform(&local), "Cannot set text raster transform");
            if (data.style.decoration == TextDecoration::info_gradient) {
                auto body = data.info_rect; body.Height -= Real(data.info_rule_height * 2);
                const auto alpha = impl_->profile.info_background_alpha;
                Gradient(g, body, kInfoBodyGradient, std::size(kInfoBodyGradient), alpha);
                RectF rule(0, body.Height, body.Width, Real(data.info_rule_height));
                Gradient(g, rule, kInfoPenultimateGradient, std::size(kInfoPenultimateGradient), alpha, true);
                rule.Y += rule.Height;
                Gradient(g, rule, kInfoLastGradient, std::size(kInfoLastGradient), alpha);
            }
            if (data.style.shadow) {
                Bitmap shadow(device.Width, device.Height, PixelFormat32bppPARGB);
                Check(shadow.GetLastStatus(), "Cannot allocate text shadow");
                {
                    Graphics mask(&shadow); mask.SetPageUnit(UnitPixel); mask.Clear(Color(0, 0, 0, 0)); mask.SetSmoothingMode(SmoothingModeAntiAlias);
                    auto path = std::unique_ptr<GraphicsPath>(data.path.Clone());
                    Matrix offset; offset.Translate(Real(impl_->profile.shadow_offset_x_pt * kPt), Real(impl_->profile.shadow_offset_y_pt * kPt));
                    Check(path->Transform(&offset), "Cannot offset shadow"); Check(path->Transform(&local), "Cannot transform shadow");
                    SolidBrush brush(Color(static_cast<BYTE>(std::lround(impl_->profile.shadow_alpha * 255)), 0, 0, 0));
                    Check(mask.FillPath(&brush, path.get()), "Cannot fill text shadow mask");
                }
                const auto scale = std::max(std::hypot(elements[0], elements[1]), std::hypot(elements[2], elements[3]));
                const auto radius = static_cast<int>(std::ceil(impl_->profile.shadow_blur_pt * kPt * scale));
                Blur(shadow, radius);
                g.ResetTransform(); Check(g.DrawImage(&shadow, 0, 0), "Cannot composite shadow"); g.SetTransform(&local);
            }
            if (data.style.outline != TextColor::none && impl_->profile.outline_width_pt > 0) {
                Pen pen(Palette(data.style.outline), Real(impl_->profile.outline_width_pt * kPt)); pen.SetLineJoin(LineJoinRound);
                Check(g.DrawPath(&pen, &data.path), "Cannot draw text outline");
            }
            SolidBrush fill(Palette(data.style.fill)); Check(g.FillPath(&fill, &data.path), "Cannot fill text outline");
            g.Flush(FlushIntentionSync);
            const auto bytes = static_cast<std::size_t>(device.Width) * device.Height * 4;
            impl_->rasters.push_front({run.layout.data, elements, context.dpi, device, std::move(raster), bytes});
            impl_->raster_bytes += bytes; found = impl_->rasters.begin();
        } else {
            ++impl_->raster_hits;
            impl_->rasters.splice(impl_->rasters.begin(), impl_->rasters, found); found = impl_->rasters.begin();
        }
        auto result = found->raster;
        // Oversized individual rasters may be drawn once but are not retained.
        while (impl_->rasters.size() > kRasterLimit || impl_->raster_bytes > kRasterBytes) {
            impl_->raster_bytes -= impl_->rasters.back().bytes; impl_->rasters.pop_back();
        }
        return result;
    }

    TextRaster TextRenderer::Rasterize(const TextRenderContext &context, const SceneTextRun &run) {
        Rect bounds;
        const auto raster = GetRaster(context, run, bounds);
        if (!raster) return {};
        return {bounds, std::shared_ptr<const TextRasterPixels>(raster, &raster->pixels)};
    }

    RectF TextRenderer::Draw(Graphics &target, const TextRenderContext &context, const SceneTextRun &run) {
        Rect device;
        const auto raster = GetRaster(context, run, device);
        if (!raster) return {};
        const auto saved = target.Save();
        try {
            // This API consumes physical device coordinates; ignore any inherited world transform.
            Check(target.ResetTransform(), "Cannot reset text target transform");
            Check(target.SetPageUnit(UnitPixel), "Cannot set text target units");
            Check(target.SetPageScale(1), "Cannot set text target page scale");
            target.SetCompositingMode(CompositingModeSourceOver);
            target.SetInterpolationMode(InterpolationModeNearestNeighbor);
            target.SetPixelOffsetMode(PixelOffsetModeHalf);
            if (context.device_clip) Check(target.SetClip(*context.device_clip, CombineModeIntersect), "Cannot clip text target");
            ColorMatrix opacity{1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, Real(std::clamp(run.opacity, 0.0, 1.0)), 0, 0, 0, 0, 0, 1};
            ImageAttributes attributes; Check(attributes.SetColorMatrix(&opacity), "Cannot set text opacity");
            Check(target.DrawImage(&raster->bitmap, device, 0, 0, device.Width, device.Height, UnitPixel, &attributes), "Cannot composite text raster");
        } catch (...) { target.Restore(saved); throw; }
        target.Restore(saved);
        return RectF(Real(device.X), Real(device.Y), Real(device.Width), Real(device.Height));
    }
    TextCacheStats TextRenderer::CacheStats() const {
        return {impl_->layouts.size(), impl_->rasters.size(), impl_->raster_bytes,
                impl_->serial, impl_->raster_builds, impl_->raster_hits};
    }
    void TextRenderer::ClearCaches() { impl_->layouts.clear(); impl_->rasters.clear(); impl_->raster_bytes = 0; }
}
