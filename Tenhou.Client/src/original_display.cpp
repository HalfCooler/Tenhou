#include "original_display.hpp"
#include "resource_manager.hpp"
#include "shared_assets.hpp"
#include "table_scene.hpp"
#include "text_catalog.hpp"
#include "window_layout.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tenhou {
    bool HudInteraction::Handle(const HWND window, const RECT board, const int width, const UINT message, const LPARAM point) {
        if (message == WM_TIMER) {
            KillTimer(window, 9);
            if (inside_) return false;
            const bool changed = relative_;
            relative_ = false;
            return changed;
        }
        bool inside = false;
        if (message != WM_MOUSELEAVE) {
            const auto target = FitNativeCanvas(board, width);
            const double fit = static_cast<double>(target.right - target.left) / width;
            const auto hud = BuildOriginalHud({}, width);
            const double x = (static_cast<short>(LOWORD(point)) - target.left) / fit;
            const double y = (static_cast<short>(HIWORD(point)) - target.top) / fit;
            inside = x > hud.panel[0] + 7 && x < hud.panel[0] + hud.panel[2] - 7 &&
                     y > hud.panel[1] && y < hud.panel[1] + hud.panel[3];
        }
        const bool changed = inside && !relative_;
        if (inside) {
            KillTimer(window, 9);
            relative_ = true;
        } else if (inside_) SetTimer(window, 9, 750, nullptr);
        inside_ = inside;
        return changed;
    }

    RECT OriginalAutoButtonRect(const int width, const int index) {
        // 0047d657..0047d77a: right offsets 2.15,3.15,4.15,5.15.
        // Native controls are 80x22 (1024) and 110x37 (1440); higher tiers scale linearly from 1440.
        if (width == 1024) {
            const int x = static_cast<int>(1024 - 88 * (2.15 + index));
            const int y = 768 - 32;
            return {x, y, x + 80, y + 22};
        }
        const int x1440 = static_cast<int>(1440 - 123 * (2.15 + index));
        const int y1440 = 1080 - 57;
        if (width == 1440) return {x1440, y1440, x1440 + 110, y1440 + 37};
        const double ratio = static_cast<double>(width) / 1440.0;
        const int x = static_cast<int>(std::lround(x1440 * ratio));
        const int y = static_cast<int>(std::lround(y1440 * ratio));
        return {x, y, x + static_cast<int>(std::lround(110 * ratio)), y + static_cast<int>(std::lround(37 * ratio))};
    }

    namespace {
        using Affine = std::array<float, 6>;

        Affine Multiply(const Affine &a, const Affine &b) {
            return {
                a[0] * b[0] + a[1] * b[2], a[0] * b[1] + a[1] * b[3],
                a[2] * b[0] + a[3] * b[2], a[2] * b[1] + a[3] * b[3],
                a[4] * b[0] + a[5] * b[2] + b[4], a[4] * b[1] + a[5] * b[3] + b[5]
            };
        }

        Affine Rotation(const float degrees) {
            const double angle = degrees * 3.14159265358979323846 / 180;
            return {
                static_cast<float>(std::cos(angle)), static_cast<float>(std::sin(angle)),
                static_cast<float>(-std::sin(angle)), static_cast<float>(std::cos(angle)), 0, 0
            };
        }

        void Transform(Gdiplus::Graphics &g, const Affine &a, const float sx = 1.0f, const float sy = 1.0f) {
            Gdiplus::Matrix matrix(a[0], a[1], a[2], a[3], a[4], a[5]);
            if (sx != 1.0f || sy != 1.0f) matrix.Scale(sx, sy, Gdiplus::MatrixOrderAppend);
            if (g.SetTransform(&matrix) != Gdiplus::Ok) throw std::runtime_error(
                "Cannot set original display transform");
        }

        std::wstring Wide(const std::string &value) {
            const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
            std::wstring out(size, L' ');
            if (size) MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), out.data(), size);
            return out;
        }

        // Exact 2-pass separable Box/Area downsampler on premultiplied BGRA.
        // Preserves thin lines and small dots cleanly across both integer (4x/8x) and fractional ratios.
        Image DownsampleAreaPremul(const Image &src, const int dst_w, const int dst_h) {
            if (dst_w <= 0 || dst_h <= 0 || src.width <= 0 || src.height <= 0)
                throw std::runtime_error("Invalid display sprite downsample dimensions");
            if (dst_w == src.width && dst_h == src.height) return src;
            // Horizontal pass: src (src.width x src.height) -> mid (dst_w x src.height) in premultiplied float BGRA.
            std::vector<float> mid(static_cast<std::size_t>(dst_w) * src.height * 4, 0.0f);
            const double scale_x = static_cast<double>(src.width) / static_cast<double>(dst_w);
            for (int dx = 0; dx < dst_w; ++dx) {
                const double x0 = dx * scale_x;
                const double x1 = (dx + 1) * scale_x;
                const int ix0 = std::max(0, static_cast<int>(std::floor(x0)));
                const int ix1 = std::min(src.width - 1, static_cast<int>(std::ceil(x1)) - 1);
                const double inv_area = 1.0 / (x1 - x0);
                for (int sy = 0; sy < src.height; ++sy) {
                    double b = 0, g = 0, r = 0, a = 0;
                    const std::uint8_t *row = src.bgra.data() + static_cast<std::size_t>(sy) * src.width * 4;
                    for (int sx = ix0; sx <= ix1; ++sx) {
                        const double w = std::min(x1, static_cast<double>(sx + 1)) - std::max(x0, static_cast<double>(sx));
                        if (w <= 0.0) continue;
                        const std::uint8_t *p = row + static_cast<std::size_t>(sx) * 4;
                        const double alpha = static_cast<double>(p[3]) / 255.0;
                        const double wa = w * alpha;
                        b += p[0] * wa;
                        g += p[1] * wa;
                        r += p[2] * wa;
                        a += p[3] * w;
                    }
                    float *out = mid.data() + (static_cast<std::size_t>(sy) * dst_w + dx) * 4;
                    out[0] = static_cast<float>(b * inv_area);
                    out[1] = static_cast<float>(g * inv_area);
                    out[2] = static_cast<float>(r * inv_area);
                    out[3] = static_cast<float>(a * inv_area);
                }
            }
            // Vertical pass: mid (dst_w x src.height) -> dst (dst_w x dst_h) unpremultiplied 8-bit BGRA.
            Image dst;
            dst.width = dst_w;
            dst.height = dst_h;
            dst.bgra.resize(static_cast<std::size_t>(dst_w) * dst_h * 4);
            const double scale_y = static_cast<double>(src.height) / static_cast<double>(dst_h);
            for (int dy = 0; dy < dst_h; ++dy) {
                const double y0 = dy * scale_y;
                const double y1 = (dy + 1) * scale_y;
                const int iy0 = std::max(0, static_cast<int>(std::floor(y0)));
                const int iy1 = std::min(src.height - 1, static_cast<int>(std::ceil(y1)) - 1);
                const double inv_area = 1.0 / (y1 - y0);
                std::uint8_t *dst_row = dst.bgra.data() + static_cast<std::size_t>(dy) * dst_w * 4;
                for (int dx = 0; dx < dst_w; ++dx) {
                    double b = 0, g = 0, r = 0, a = 0;
                    for (int sy = iy0; sy <= iy1; ++sy) {
                        const double w = std::min(y1, static_cast<double>(sy + 1)) - std::max(y0, static_cast<double>(sy));
                        if (w <= 0.0) continue;
                        const float *p = mid.data() + (static_cast<std::size_t>(sy) * dst_w + dx) * 4;
                        b += p[0] * w;
                        g += p[1] * w;
                        r += p[2] * w;
                        a += p[3] * w;
                    }
                    b *= inv_area;
                    g *= inv_area;
                    r *= inv_area;
                    a *= inv_area;
                    std::uint8_t *out = dst_row + static_cast<std::size_t>(dx) * 4;
                    const int alpha_u8 = std::clamp(static_cast<int>(std::lround(a)), 0, 255);
                    out[3] = static_cast<std::uint8_t>(alpha_u8);
                    if (a > 0.5) {
                        const double inv_a = 255.0 / a;
                        out[0] = static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(b * inv_a)), 0, 255));
                        out[1] = static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(g * inv_a)), 0, 255));
                        out[2] = static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(r * inv_a)), 0, 255));
                    } else {
                        out[0] = out[1] = out[2] = 0;
                    }
                }
            }
            return dst;
        }
    }

    DisplaySpriteSize OriginalDisplaySpriteSize(const unsigned id, const int design_width) {
        DisplaySpriteSize hd{};
        switch (id) {
            case 401:
            case 402: hd = {7, 13}; break;
            case 440: hd = {140, 51}; break;
            case 466: hd = {114, 14}; break;
            case 589: hd = {244, 172}; break;
            case 591: hd = {728, 330}; break;
            case 592: hd = {590, 430}; break;
            default: return {};
        }
        if (design_width == 1024) {
            return {
                std::max(1, static_cast<int>(hd.width / 1.4)),
                std::max(1, static_cast<int>(hd.height / 1.4))
            };
        }
        return hd;
    }

    void DisplayArtwork::Caption(Gdiplus::Graphics &g, const std::wstring &text, float x, float y,
                                 float em, const OriginalHudLayout &hud, float width) {
        const auto key = std::make_tuple(text, x, y, em, width);
        auto found = captions_.find(key);
        if (found == captions_.end()) {
            Gdiplus::FontFamily family(L"System");
            const auto *selected = family.IsAvailable() ? &family : Gdiplus::FontFamily::GenericSansSerif();
            auto path = std::make_shared<Gdiplus::GraphicsPath>();
            Gdiplus::StringFormat format;
            format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
            format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
            const Gdiplus::RectF bounds(x, y, width, em * 1.7f);
            if (width > 0) path->AddString(text.c_str(), -1, selected, Gdiplus::FontStyleRegular, em, bounds, &format);
            else path->AddString(text.c_str(), -1, selected, Gdiplus::FontStyleRegular, em, Gdiplus::PointF(x, y),
                                 &format);
            if (captions_.size() >= 128) captions_.clear();
            found = captions_.emplace(key, std::move(path)).first;
        }
        Gdiplus::Pen outline(Gdiplus::Color(hud.outline_color), hud.outline_width);
        outline.SetLineJoin(Gdiplus::LineJoinRound);
        Gdiplus::SolidBrush fill(Gdiplus::Color(hud.text_color));
        g.DrawPath(&outline, found->second.get());
        g.FillPath(&fill, found->second.get());
    }

    OriginalHudLayout BuildOriginalHud(const ReplayState &state, const int width) {
        if (!IsSupportedTableWidth(width)) throw std::runtime_error("Unsupported native display canvas");
        OriginalHudLayout out;
        // Original HUD uses exactly 1.4 at 1440; 1920/2640 scale linearly from 1440.
        const float ratio = width == 1024 ? 1.0f : static_cast<float>(width) / 1440.0f;
        out.scale = width == 1024 ? 1.0f : 1.4f * ratio;
        out.outline_width = 2 * out.scale;
        out.center = {width / 2.0f, width * 3 / 8.0f - (width == 1024 ? 52.0f : 75.0f * ratio) - ratio};
        const int panel_width = width == 1024 ? 174 : static_cast<int>(std::lround(244.0 * ratio));
        const int panel_height = width == 1024 ? 123 : static_cast<int>(std::lround(172.0 * ratio));
        const int pad_w = static_cast<int>(std::lround(18.0 * ratio));
        const int pad_h = static_cast<int>(std::lround(20.0 * ratio));
        out.panel = {
            (width - panel_width - pad_w) / 2 + pad_w / 2,
            (width * 3 / 4 - panel_height - pad_h) / 2 - (width == 1024 ? 52 : static_cast<int>(std::lround(75.0 * ratio))) + pad_h / 2,
            panel_width, panel_height
        };
        const Affine base{1, 0, 0, 1, out.center[0], out.center[1]};
        out.seats[0] = base;
        out.seats[1] = Multiply({1, 0, 0, 1, 0, 23 * out.scale},
                                Multiply(Rotation(-91.6f), Multiply({1, .03f, 0, 1, 0, 0}, base)));
        out.seats[2] = Multiply(Rotation(-180), base);
        out.seats[3] = Multiply({1, 0, 0, 1, 0, 23 * out.scale},
                                Multiply(Rotation(-268.4f), Multiply({1, -.03f, 0, 1, 0, 0}, base)));
        out.sticks = out.seats;
        out.sticks[1] = Multiply({.9f, 0, 0, 1, 0, 0}, Multiply(Rotation(-.6f), out.seats[1]));
        out.sticks[3] = Multiply({.9f, 0, 0, 1, 0, 0}, Multiply(Rotation(.6f), out.seats[3]));
        const auto scaled = [&](const float n) { return static_cast<int>(n * out.scale); };
        constexpr TextKey winds[]{TextKey::center_east, TextKey::center_south, TextKey::center_west, TextKey::center_north};
        constexpr TextKey hands[]{TextKey::center_hand_one, TextKey::center_hand_two, TextKey::center_hand_three, TextKey::center_hand_four};
        out.text.push_back({winds[(state.round / 4) % 4], scaled(-30), scaled(-17)});
        out.text.push_back({hands[state.round % 4], scaled(-10), scaled(-17)});
        out.text.push_back({TextKey::center_hand_unit, scaled(10), scaled(-17)});
        constexpr TextKey active[]{TextKey::seat_self_east, TextKey::seat_self_south, TextKey::seat_self_west, TextKey::seat_self_north};
        constexpr TextKey inactive[]{TextKey::seat_black_east, TextKey::seat_black_south, TextKey::seat_black_west, TextKey::seat_black_north};
        for (int i = 0; i < 4; ++i) {
            if (!SeatPresent(state.seat_mask, i)) continue;
            const unsigned wind = static_cast<unsigned>(SeatWind(state.seat_mask, state.dealer, i));
            out.text.push_back({
                state.turn == i ? active[wind] : inactive[wind],
                static_cast<int>(6 * ratio - 58 * out.scale), scaled(28), i
            });
            if (state.players[i].reach == 2) out.sprites.push_back({466, 0, scaled(57), i, true});
        }
        return out;
    }

    RECT FitNativeCanvas(const RECT bounds, const int width) {
        const auto fitted = FitNativeCanvasBounds({bounds.left, bounds.top, bounds.right, bounds.bottom}, width);
        return {fitted.left, fitted.top, fitted.right, fitted.bottom};
    }

    void DisplayArtwork::Configure(const std::filesystem::path &directory, const int width) {
        text_renderer_.reset();
        text_font_.reset();
        variants_.Configure(directory);
        directory_ = directory;
        canvas_width_ = IsSupportedTableWidth(width) ? width : UiDesignWidth(width);
        width_ = UiDesignWidth(canvas_width_);
        pack_.reset();
        images_.clear();
        captions_.clear();
    }

    DisplaySpriteSize DisplayArtwork::LogicalSize(const unsigned id) const {
        return OriginalDisplaySpriteSize(id, width_);
    }

    const Image *DisplayArtwork::FindSprite(unsigned id) {
        const auto *source = variants_.GetSprite(id);
        if (!source) return nullptr;
        const auto logical = LogicalSize(id);
        if (logical.width > 0 && logical.height > 0) {
            const double scale = canvas_width_ == width_ ? 1.0 : static_cast<double>(canvas_width_) / static_cast<double>(width_);
            const int target_w = std::max(1, static_cast<int>(std::lround(logical.width * scale)));
            const int target_h = std::max(1, static_cast<int>(std::lround(logical.height * scale)));
            if (source->width > target_w || source->height > target_h) {
                return &images_.emplace(
                    id, DownsampleAreaPremul(*source, std::min(source->width, target_w), std::min(source->height, target_h))
                ).first->second;
            }
            return &images_.emplace(id, *source).first->second;
        }
        if (width_ != 1024) return &images_.emplace(id, *source).first->second;
        const int dst_w = std::max(1, static_cast<int>(source->width / 1.4));
        const int dst_h = std::max(1, static_cast<int>(source->height / 1.4));
        return &images_.emplace(id, DownsampleAreaPremul(*source, dst_w, dst_h)).first->second;
    }

    const Image &DisplayArtwork::Get(unsigned id) {
        if (!FindTextImageByOrigin(id)) throw std::runtime_error("Unsupported display image ID " + std::to_string(id));
        const auto cached = images_.find(id);
        if (cached != images_.end()) return cached->second;
        if (const auto *custom = FindSprite(id)) return *custom;
        if (!pack_) {
            pack_ = SharedAssets().GetDisplayPack(ResourceManager(directory_).Path(AssetId::DisplayPack), width_);
        }
        const auto found = std::find_if(pack_->entries.begin(), pack_->entries.end(),
                                        [&](const Resource &r) { return r.id == id; });
        if (found == pack_->entries.end()) throw std::runtime_error(
            "Missing original display sprite " + std::to_string(id));
        auto decoded = DecodeImage(*found);
        if (width_ == 1024 && pack_->signature == "res2") {
            const int dst_w = std::max(1, static_cast<int>(decoded.width / 1.4));
            const int dst_h = std::max(1, static_cast<int>(decoded.height / 1.4));
            return images_.emplace(id, DownsampleAreaPremul(decoded, dst_w, dst_h)).first->second;
        }
        return images_.emplace(id, std::move(decoded)).first->second;
    }

    float DisplayArtwork::MeasureText(const std::string &text, const float em) {
        Gdiplus::Bitmap bitmap(1, 1, PixelFormat32bppARGB);
        Gdiplus::Graphics graphics(&bitmap);
        Gdiplus::FontFamily family(L"System");
        Gdiplus::Font font(family.IsAvailable() ? &family : Gdiplus::FontFamily::GenericSansSerif(), em,
                           Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::RectF rect;
        const auto value = Wide(text);
        graphics.MeasureString(value.c_str(), -1, &font, Gdiplus::PointF(0, 0), &rect);
        return rect.Width;
    }

    void DisplayArtwork::Sprite(Gdiplus::Graphics &graphics, const unsigned id, const int x, const int y, const bool cx, const bool cy) {
        const auto &im = Get(id);
        const auto logical = LogicalSize(id);
        const int draw_w = logical.width > 0 ? logical.width : im.width;
        const int draw_h = logical.height > 0 ? logical.height : im.height;
        const int dx = x - (cx ? draw_w / 2 : 0);
        const int dy = y - (cy ? draw_h / 2 : 0);
        Gdiplus::Bitmap bitmap(im.width, im.height, im.width * 4, PixelFormat32bppARGB,
                               const_cast<BYTE *>(im.bgra.data()));
        if (im.width == draw_w && im.height == draw_h) {
            if (graphics.DrawImage(&bitmap, dx, dy) != Gdiplus::Ok)
                throw std::runtime_error("Cannot draw original display sprite");
        } else {
            const auto prev_mode = graphics.GetInterpolationMode();
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            const auto status = graphics.DrawImage(&bitmap, Gdiplus::Rect(dx, dy, draw_w, draw_h),
                                                   0, 0, im.width, im.height, Gdiplus::UnitPixel);
            graphics.SetInterpolationMode(prev_mode);
            if (status != Gdiplus::Ok) throw std::runtime_error("Cannot draw original display sprite");
        }
    }

    void DisplayArtwork::PaintHudImages(Gdiplus::Graphics &g, const ReplayState &state, const HudSprite &draw_sprite,
                                        float scale_x, float scale_y) {
        const auto hud = BuildOriginalHud(state, width_);
        const float s = hud.scale;
        const auto saved = g.Save();
        try {
            g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            // 00482fe1 pushes 0x24d. 00482f30 draws that rounded plate at
            // (9,10) inside a centered (image+18,image+20) background capture.
            const Affine center{1, 0, 0, 1, hud.center[0], hud.center[1]};
            const auto sprite = [&](const unsigned id, const int x, const int y, const Affine &transform,
                                    const bool cx = false, const bool cy = false) {
                if (draw_sprite) {
                    const auto &im = Get(id);
                    const auto logical = LogicalSize(id);
                    const int lw = logical.width > 0 ? logical.width : im.width;
                    const int lh = logical.height > 0 ? logical.height : im.height;
                    draw_sprite(id, im, x - (cx ? lw / 2 : 0),
                                y - (cy ? lh / 2 : 0), transform);
                } else {
                    Transform(g, transform, scale_x, scale_y);
                    Sprite(g, id, x, y, cx, cy);
                }
            };
            if (panel_enabled_) sprite(589, hud.panel[0], hud.panel[1], {1, 0, 0, 1, 0, 0});
            for (const auto &item: hud.sprites)
                sprite(item.id, item.x, item.y, item.seat < 0 ? center : item.id == 466 ?
                       hud.sticks[item.seat] : hud.seats[item.seat], item.center_x, item.center_y);
            Transform(g, center, scale_x, scale_y);
            if (!state.variant) {
            const auto sticks = std::to_wstring(state.sticks), honba = std::to_wstring(state.honba);
            const float sw = MeasureText(std::to_string(state.sticks), 12 * s), hw = MeasureText(
                std::to_string(state.honba), 12 * s);
            float cursor = -(sw + 7 + hw + 4 * s + 7) * .5f;
            sprite(401, static_cast<int>(cursor), static_cast<int>(6 * s), center, false, true);
            cursor += 7;
            Caption(g, sticks, cursor, 6 * s, 12 * s, hud);
            cursor += sw + 4 * s;
            sprite(402, static_cast<int>(cursor), static_cast<int>(6 * s), center, false, true);
            Caption(g, honba, cursor + 7, 6 * s, 12 * s, hud);
            }
            for (int i = 0; i < 4; ++i) {
                if (!SeatPresent(state.seat_mask, i)) continue;
                Transform(g, hud.seats[i], scale_x, scale_y);
                const auto &player = state.players[i];
                const int value = state.relative_scores && i
                                      ? player.score - state.players[0].score
                                      : player.score;
                const auto score = (state.relative_scores && i && value > 0 ? L"+" : L"") + std::to_wstring(value);
                Caption(g, score, -26 * s, 28 * s, 16 * s, hud);
                if (!state.relative_scores) {
                    // 00484a98–00484c27: rank and line are relative to measured score width.
                    const float base = MeasureText(std::to_string(value), 16 * s) - 53 * s;
                    Caption(g, Wide(SeatRankLabel(player.rank)), base + 25 * s, 31 * s, (28.0f / 3) * s, hud);
                    const auto accent = !player.connected || player.gender == 'F' ? 0xccaa0000u :
                                        player.gender == 'M' ? 0xcc0000ffu : 0xcc00aa00u;
                    Gdiplus::Pen underline(Gdiplus::Color(accent), s);
                    const int y = static_cast<int>(42 * s);
                    g.DrawLine(&underline, static_cast<int>(base + 27 * s), y, static_cast<int>(base + 47 * s), y);
                    auto name_hud = hud;
                    if (!player.connected) name_hud.text_color = 0xffcc0000;
                    Caption(g, Wide(player.name), -25 * s, 46 * s, (32.0f / 3) * s, name_hud, 70 * s);
                }
            }
            g.Restore(saved);
        } catch (...) {
            g.Restore(saved);
            throw;
        }
    }

    void DisplayArtwork::PaintChromeBackground(Gdiplus::Graphics &g, const int hovered) const
    {
        const auto saved = g.Save();
        try {
            // Caption styles/opacity are appended separately at final device size.
            for (int i = 0; i < 4; ++i) {
                const RECT r = OriginalAutoButtonRect(width_, i);
                Gdiplus::LinearGradientBrush shade(Gdiplus::Point(r.left, r.top), Gdiplus::Point(r.left, r.bottom),
                                                   Gdiplus::Color(0, 0, 0, 0), Gdiplus::Color(0x22, 0, 0, 0));
                g.FillRectangle(&shade, r.left, r.top, r.right - r.left, r.bottom - r.top);
                Gdiplus::Pen border(Gdiplus::Color(hovered == i ? 0xffff9933u : 0x11000000u), 1);
                g.DrawRectangle(&border, r.left, r.top, r.right - r.left - 1, r.bottom - r.top - 1);
            }
            g.Restore(saved);
        } catch (...) {
            g.Restore(saved);
            throw;
        }
    }

    std::array<float, 2> DisplayArtwork::MeasureTextExtent(const std::string &text, const float em) {
        Gdiplus::Bitmap bitmap(1, 1, PixelFormat32bppARGB);
        Gdiplus::Graphics g(&bitmap);
        Gdiplus::FontFamily family(L"System");
        Gdiplus::Font font(family.IsAvailable() ? &family : Gdiplus::FontFamily::GenericSansSerif(), em,
                           Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        const auto value = Wide(text);
        Gdiplus::RectF bounds;
        if (g.MeasureString(value.c_str(), -1, &font, Gdiplus::PointF(0, 0), &bounds) != Gdiplus::Ok)
            throw std::runtime_error("Cannot measure north-count text");
        return {bounds.Width, bounds.Height};
    }

    NukiCountLayout OriginalNukiCountLayout(const RECT a, const int seat, const int text_width, const int text_height) {
        const int w = (seat & 1) ? text_height : text_width, h = (seat & 1) ? text_width : text_height;
        RECT r{};
        switch (seat) {
            case 0: r = {a.right - 2, a.bottom - h, a.right - 2 + w, a.bottom};
                break;
            case 1: r = {a.right - w - 3, a.top - 3 - h, a.right - 3, a.top - 3};
                break;
            case 2: r = {a.left - w, a.top + w - h, a.left, a.top + w};
                break;
            case 3: r = {a.left, a.bottom, a.left + w, a.bottom + h};
                break;
            default: throw std::runtime_error("Invalid north-count direction");
        }
        --r.left;
        --r.top;
        ++r.right;
        ++r.bottom;
        POINT p{1, 1};
        if (seat == 1) p.x = 1 - (r.bottom - r.top);
        else if (seat == 2) {
            p.x = 1 - (r.right - r.left);
            p.y = 1 - (r.bottom - r.top);
        } else if (seat == 3) p.y = 1 - (r.right - r.left);
        return {r, p};
    }

    void DisplayArtwork::PaintNukiCounts(Gdiplus::Graphics &g, const ReplayState &state, TableArtwork &tiles,
                                         const float scale_x, const float scale_y) const
    {
        if (std::all_of(state.players.begin(), state.players.end(),
                [](const ReplayPlayer &p) { return p.nuki.empty() && p.flowers.empty(); })) return;
        // 0049c200 / 004aa250: one n/0 sprite plus a rotated xN count,
        // System bold 12*1.4, white fill and four one-pixel black offsets.
        const auto saved = g.Save();
        try {
            Gdiplus::FontFamily family(L"System");
            Gdiplus::Font font(family.IsAvailable() ? &family : Gdiplus::FontFamily::GenericSansSerif(),
                               12 * (width_ == 1440 ? 1.4f : 1.f), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
            Gdiplus::SolidBrush black(Gdiplus::Color(0xff000000)), white(Gdiplus::Color(0xffffffff));
            for (int seat = 0; seat < 4; ++seat) {
                const auto &player = state.players[seat];
                const bool flower = !player.flowers.empty();
                if (!SeatPresent(state.seat_mask, seat) || (!flower && (!(state.type & 16) || player.nuki.empty()))) continue;
                // 00480930 maps the original entity sentinel 254 to variant 37.
                const auto &tile = tiles.Get({'n', 0, flower ? player.flowers.front() : -1, seat});
                const double to_ui = tiles.width() > 0 && tiles.width() != width_
                                         ? static_cast<double>(width_) / static_cast<double>(tiles.width())
                                         : 1.0;
                const RECT anchor{
                    static_cast<LONG>(std::lround(tile.x * to_ui)),
                    static_cast<LONG>(std::lround(tile.y * to_ui)),
                    static_cast<LONG>(std::lround((tile.x + tile.image.width) * to_ui)),
                    static_cast<LONG>(std::lround((tile.y + tile.image.height) * to_ui))
                };
                const auto text = (flower ? L"\n×" : L"\nx") + std::to_wstring(flower ? player.flowers.size() : player.nuki.size());
                Gdiplus::RectF measure;
                // WM_CREATE measures the fixed two-line PE string, not the
                // current count. The standard mode leaves the first line blank.
                g.MeasureString(L"99+\nx99", -1, &font, Gdiplus::PointF(0, 0), &measure);
                const auto layout = OriginalNukiCountLayout(
                    anchor, seat, static_cast<int>(measure.Width), static_cast<int>(measure.Height));
                g.ResetTransform();
                if (scale_x != 1.0f || scale_y != 1.0f) g.ScaleTransform(scale_x, scale_y);
                g.TranslateTransform(static_cast<float>(layout.bounds.left), static_cast<float>(layout.bounds.top));
                g.RotateTransform(static_cast<float>(seat * 270));
                const float x = static_cast<float>(layout.text_origin.x), y = static_cast<float>(layout.text_origin.y);
                for (const auto &p: std::array<Gdiplus::PointF, 4>{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}})
                    g.DrawString(text.c_str(), -1, &font, Gdiplus::PointF(x + p.X, y + p.Y), &black);
                g.DrawString(text.c_str(), -1, &font, Gdiplus::PointF(x, y), &white);
            }
            g.Restore(saved);
        } catch (...) {
            g.Restore(saved);
            throw;
        }
    }

    TextRenderer &DisplayArtwork::Text() {
        if (!text_renderer_) {
            auto font = SharedAssets().GetTextFont(directory_);
            auto renderer = std::make_unique<TextRenderer>(*font);
            text_font_ = std::move(font);
            text_renderer_ = std::move(renderer);
        }
        return *text_renderer_;
    }

    SceneTextRun DisplayArtwork::TextRun(const TextKey key, const float x, const float y, const bool cx, const bool cy) {
        const float hd = width_ == 1024 ? 1.4f : 1.f;
        auto layout = Text().Layout(key);
        const auto &ink = layout.metrics.ink;
        SceneTextRun run;
        run.layout = layout;
        run.anchor = {x * hd, y * hd};
        run.transform[4] = -ink.X - (cx ? ink.Width / 2 : 0);
        run.transform[5] = -ink.Y - (cy ? ink.Height / 2 : 0);
        return run;
    }

    void DisplayArtwork::AppendHud(FinalScene &scene, const ReplayState &state) {
        const auto hud = BuildOriginalHud(state, width_);
        const float hd = width_ == 1024 ? 1.4f : 1.f;
        std::array<Gdiplus::PointF, 2> round_centers{};
        for (std::size_t i = 0; i < 3; i += 2) {
            const auto &item = hud.text[i];
            const auto run = TextRun(item.key, static_cast<float>(item.x), static_cast<float>(item.y));
            const auto &ink = run.layout.metrics.ink;
            round_centers[i / 2] = {run.anchor.X + ink.Width / 2, run.anchor.Y + ink.Height / 2};
        }
        for (std::size_t i = 0; i < hud.text.size(); ++i) {
            const auto &item = hud.text[i];
            auto run = TextRun(item.key, static_cast<float>(item.x), static_cast<float>(item.y), item.center_x, item.center_y);
            if (i == 1) {
                const auto &ink = run.layout.metrics.ink;
                run.anchor = {(round_centers[0].X + round_centers[1].X) / 2,
                              (round_centers[0].Y + round_centers[1].Y) / 2};
                run.transform[4] = -ink.X - ink.Width / 2;
                run.transform[5] = -ink.Y - ink.Height / 2;
            }
            const Affine base{1, 0, 0, 1, hud.center[0], hud.center[1]};
            const auto &t = item.seat < 0 ? base : hud.seats[item.seat];
            const float x = static_cast<float>(run.anchor.X + run.transform[4]);
            const float y = static_cast<float>(run.anchor.Y + run.transform[5]);
            run.anchor = {t[4] * hd, t[5] * hd};
            run.transform = {t[0], t[1], t[2], t[3], x * t[0] + y * t[2], x * t[1] + y * t[3]};
            scene.Text(std::move(run));
        }
    }

    std::vector<TextKey> OriginalInfoKeys(const unsigned type, const bool variant) {
        constexpr TextKey levels[]{TextKey::info_general, TextKey::info_advanced, TextKey::info_expert, TextKey::info_phoenix};
        constexpr TextKey parlor_levels[]{TextKey::info_wakaba, TextKey::info_silver, TextKey::info_amber, TextKey::info_peacock};
        constexpr TextKey parlor_bonuses[]{TextKey::info_no_bonus, TextKey::info_bonus_2000, TextKey::info_bonus_5000};
        constexpr TextKey rules[]{TextKey::info_east, TextKey::info_fast_east, TextKey::info_east_south, TextKey::info_fast_east_south};
        const unsigned tier = (((type >> 3) & 0x10u) | (type & 0x20u)) >> 4;
        if (variant) return {(type & 0x800u) ? TextKey::info_toutenkou : levels[tier]};
        if (!(type & 0x800u) && (type & 0x600u)) {
            const unsigned bonus = (type & 0x200u) ? (((type & 0x400u) != 0u) + 1u) : 0u;
            return {parlor_levels[tier], (type & 8u) ? TextKey::info_east_south : TextKey::info_fast_east,
                    parlor_bonuses[bonus]};
        }
        return {(type & 0x800u) ? TextKey::info_toutenkou : levels[tier],
                rules[((type >> 6) & 1u) + ((type >> 2) & 2u)],
                (type >> 2 & 1u) ? TextKey::info_no_open_tanyao : TextKey::info_open_tanyao,
                (type >> 1 & 1u) ? TextKey::info_no_red : TextKey::info_red};
    }

    void DisplayArtwork::AppendChrome(FinalScene &scene, const unsigned type, const std::array<bool, 4> &automatic, const bool variant) {
        {
            float y = 0;
            for (auto key : OriginalInfoKeys(type, variant)) {
                auto layout = Text().Layout(key);
                const float hd = width_ == 1024 ? 1.4f : 1.f;
                SceneTextRun run; run.layout = layout;
                run.anchor = {width_ * hd - static_cast<float>(layout.metrics.advance), y};
                scene.Text(run);
                y += layout.metrics.effects.GetBottom();
            }
        }
        constexpr TextKey checked[]{TextKey::option_checked_auto_sort, TextKey::option_checked_no_call,
            TextKey::option_checked_tsumogiri, TextKey::option_checked_auto_win};
        constexpr TextKey unchecked[]{TextKey::option_unchecked_auto_sort, TextKey::option_unchecked_no_call,
            TextKey::option_unchecked_tsumogiri, TextKey::option_unchecked_auto_win};
        for (int i = 0; i < 4; ++i) {
            const auto r = OriginalAutoButtonRect(width_, i);
            auto run = TextRun(automatic[i] ? checked[i] : unchecked[i], (r.left + r.right) / 2.f, (r.top + r.bottom) / 2.f, true, true);
            run.opacity = automatic[i] ? 1 : .7;
            scene.Text(run);
        }
    }

    void DisplayArtwork::AppendStatus(FinalScene &scene, const ReplayState &state) {
        const float hd = width_ == 1024 ? 1.4f : 1.f;
        float y = 0;
        for (auto key : OriginalInfoKeys(state.type, state.variant))
            y += Text().Layout(key).metrics.effects.GetBottom();
        if (state.variant) {
            const auto key = state.variant_rule.rfind("sichuan", 0) == 0 ? TextKey::info_sichuan : TextKey::info_guobiao;
            auto layout = Text().Layout(key);
            SceneTextRun run; run.layout = layout;
            run.anchor = {width_ * hd - static_cast<float>(layout.metrics.advance), y};
            scene.Text(run);
            y += layout.metrics.effects.GetBottom();
        }
        const int last = state.total_hands > 0 ? state.total_hands - 1 : (state.variant_rule.rfind("sichuan", 0) == 0 ? 7 : 15);
        const int riichi_last = static_cast<int>(((state.type & 8 | 6) >> 1) - (state.type >> 4 & 1));
        if (state.initialized && state.round >= (state.variant ? last : riichi_last)) {
            auto layout = Text().Layout(TextKey::info_all_last);
            SceneTextRun run; run.layout = layout;
            run.anchor = {width_ * hd - static_cast<float>(layout.metrics.advance), y};
            scene.Text(run);
        }
        if (state.initialized && !state.ended && state.furiten)
            scene.Text(TextRun(TextKey::status_furiten, 300 * (width_ == 1440 ? 1.4f : 1.f),
                               static_cast<float>(OriginalAutoButtonRect(width_, 0).top)));
    }
}
