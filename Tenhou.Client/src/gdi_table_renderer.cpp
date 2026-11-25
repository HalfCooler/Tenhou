#include "gdi_table_renderer.hpp"
#include "action_captions.hpp"
#include "background.hpp"
#include "network_protocol.hpp"
#include "original_button.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace tenhou {
    GdiTableRenderer::GdiTableRenderer() {
        ui_font_ = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        exit_font_ = CreateOriginalUiFont();
    }

    GdiTableRenderer::~GdiTableRenderer() {
        if (exit_font_) DeleteObject(exit_font_);
        if (ui_font_) DeleteObject(ui_font_);
    }

    void GdiTableRenderer::Configure(const std::filesystem::path &directory, int canvas_width) {
        display_artwork_.Configure(directory, canvas_width);
        directory_ = directory;
        width_ = canvas_width;
        artwork_ = {};
        background_ = {};
        native_buffer_.Invalidate();
        hud_cache_.Invalidate();
        artwork_.SetColor(preferences_.TileColor());
    }

    void GdiTableRenderer::SetPreferences(const ClientPreferences &preferences, const std::filesystem::path &config,
                                          bool rebuild_background) {
        Image background;
        if (rebuild_background && !directory_.empty())
            background = BuildPreferredBackground(directory_, config, width_, width_ * 3 / 4, preferences);
        preferences_ = preferences;
        config_ = config;
        if (rebuild_background && !directory_.empty()) background_ = std::move(background);
        display_artwork_.SetPanelEnabled((preferences.flags & 0x2000) == 0);
        artwork_.SetColor(preferences.TileColor());
        native_buffer_.Invalidate();
        hud_cache_.Invalidate();
    }

    void GdiTableRenderer::Prepare() {
        if (artwork_.width() && !background_.bgra.empty()) return;
        try {
            if (!artwork_.width()) artwork_.Load(directory_, width_);
            if (background_.bgra.empty())
                background_ = BuildPreferredBackground(directory_, config_, width_, width_ * 3 / 4, preferences_);
        } catch (const std::exception &) {
        }
    }

    void GdiTableRenderer::Invalidate() {
        native_buffer_.Invalidate();
        hud_cache_.Invalidate();
    }

    void GdiTableRenderer::SetReferenceRendering(bool enabled) {
        native_buffer_.SetReferenceRendering(enabled);
        reference_rendering_ = enabled;
    }

    TileGeometryCatalog &GdiTableRenderer::EnsureGeometry(const std::vector<int> &red_tiles) {
        artwork_.SetRedTiles(red_tiles);
        if (!artwork_.width()) {
            artwork_.Load(directory_, width_);
            artwork_.SetColor(preferences_.TileColor());
        }
        return artwork_.geometry();
    }

    void PaintTableInteraction(HDC memory, const TableRenderFrame &frame, TableArtwork &artwork,
                               bool grayscale) {
        if (!frame.interaction) return;
        const auto *interaction = &*frame.interaction;
        const int w = frame.canvas_width, h = w * 3 / 4;
        const double unit = double(w) / 1024.0;
        const auto &tiles = frame.tiles;
        const bool red = frame.red_tiles;
        if (!interaction->hint.empty()) {
            const auto font = CreateFontW(-static_cast<int>(14 * unit), 0, 0, 0, FW_NORMAL,
                                          FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                          CLIP_DEFAULT_PRECIS, grayscale ? ANTIALIASED_QUALITY : CLEARTYPE_QUALITY,
                                          DEFAULT_PITCH, L"KaiTi");
            const auto old = SelectObject(memory, font);
            SetBkMode(memory, TRANSPARENT);
            SetTextColor(memory, RGB(255, 242, 190));
            RECT hint{w / 8, h - static_cast<int>(52 * unit), w * 7 / 8, h - static_cast<int>(28 * unit)};
            DrawTextW(memory, interaction->hint.c_str(), -1, &hint, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            SelectObject(memory, old);
            DeleteObject(font);
        }
        if (!interaction->countdown.empty()) {
            int right = 0, top = 0;
            for (const auto &tile: tiles)
                if (tile.tile.direction == 0 && std::string(tile.tile.role) == "hand") {
                    const auto &image = artwork.Get(tile.tile, red).image;
                    if (tile.x + image.width > right) {
                        right = tile.x + image.width;
                        top = tile.y;
                    }
                }
            const auto clock_font = CreateFontW(
                -std::max(8, static_cast<int>(9 * unit)), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
                DEFAULT_PITCH, L"Arial");
            const auto old = SelectObject(memory, clock_font ? clock_font : GetStockObject(DEFAULT_GUI_FONT));
            SIZE size{};
            const auto &text = interaction->countdown;
            const int length = static_cast<int>(text.size());
            GetTextExtentPoint32W(memory, text.c_str(), length, &size);
            const int x = std::min(right + std::max(2, static_cast<int>(2 * unit)),
                                   w - static_cast<int>(size.cx) - 3);
            SetBkMode(memory, TRANSPARENT);
            SetTextColor(memory, RGB(0, 0, 0));
            TextOutW(memory, x - 1, top, text.c_str(), length);
            TextOutW(memory, x + 1, top, text.c_str(), length);
            TextOutW(memory, x, top - 1, text.c_str(), length);
            TextOutW(memory, x, top + 1, text.c_str(), length);
            SetTextColor(memory, RGB(255, 255, 255));
            TextOutW(memory, x, top, text.c_str(), length);
            SelectObject(memory, old);
            if (clock_font) DeleteObject(clock_font);
        }
    }

    void GdiTableRenderer::RenderToDc(HDC dc, const TableRenderFrame &frame) {
        if (!artwork_.width() || background_.bgra.empty() || width_ != frame.canvas_width) {
            if (width_ != frame.canvas_width) {
                width_ = frame.canvas_width;
                display_artwork_.Configure(directory_, width_);
            }
            TableArtwork art;
            art.Load(directory_, width_);
            art.SetColor(preferences_.TileColor());
            auto bg = BuildPreferredBackground(directory_, config_, width_, width_ * 3 / 4, preferences_);
            artwork_ = std::move(art);
            background_ = std::move(bg);
            native_buffer_.Invalidate();
            hud_cache_.Invalidate();
        }

        const int w = width_, h = w * 3 / 4;
        const double unit = double(w) / 1024.0;
        const bool red = frame.red_tiles;
        const auto &state = frame.state;
        const auto &tiles = frame.tiles;
        const auto *interaction = frame.interaction ? &*frame.interaction : nullptr;

        artwork_.SetRedTiles(state.red_tiles);
        artwork_.BeginFrame();

        const auto shadows = BuildTileShadows(tiles, artwork_, ShadowPack::Embedded());
        const auto shadow_damage = TileShadowDamage(tiles, shadows, artwork_, red);

        std::ostringstream stamp;
        stamp << TableMetadata(state) << ':' << frame.original_display << ':' << red;
        if (frame.online_mode) {
            stamp << ':' << frame.game_type << ':' << frame.match_complete << ':' << frame.interactive;
            if (interaction) {
                stamp << ':' << interaction->hovered_button;
                for (auto c: interaction->hint) stamp << ':' << int(c);
                for (auto c: interaction->countdown) stamp << ':' << int(c);
                for (auto value: interaction->automatic) stamp << ':' << value;
            }
        }

        std::vector<unsigned> appearances;
        if (frame.online_mode) {
            appearances.reserve(frame.tile_colors.size());
            for (const auto color: frame.tile_colors) appearances.push_back(static_cast<unsigned>(color));
        }

        std::vector<RECT> metadata_regions, atomic_layers;
        if (frame.original_display) {
            const auto hud = BuildOriginalHud(state, w);
            const int radius = static_cast<int>(128 * hud.scale);
            const int cx = static_cast<int>(hud.center[0]), cy = static_cast<int>(hud.center[1]);
            atomic_layers.push_back({cx - radius, cy - radius, cx + radius, cy + radius});
            if (frame.online_mode) {
                metadata_regions.push_back(atomic_layers.front());
                metadata_regions.push_back({w - static_cast<int>(220 * unit), 0, w, static_cast<int>(200 * unit)});
                metadata_regions.push_back({
                    static_cast<LONG>(300 * (w == 1440 ? 1.4 : 1.0)), OriginalAutoButtonRect(w, 0).top,
                    static_cast<LONG>(300 * (w == 1440 ? 1.4 : 1.0) + 100), OriginalAutoButtonRect(w, 0).top + 35
                });
                for (int i = 0; i < 4; ++i) metadata_regions.push_back(OriginalAutoButtonRect(w, i));
            }
        }

        RECT clock_overlay{};
        if (interaction && !interaction->countdown.empty()) {
            for (const auto &t: tiles)
                if (t.tile.direction == 0 && std::string(t.tile.role) == "hand") {
                    RECT r{0, t.y - 3, w, t.y + 64};
                    if (IsRectEmpty(&clock_overlay)) clock_overlay = r;
                    else {
                        RECT united{};
                        UnionRect(&united, &clock_overlay, &r);
                        clock_overlay = united;
                    }
                }
        }

        std::optional<FinalScene> final;
        if (frame.original_display) {
            unsigned dpi = frame.dpi;
            if (!dpi) {
                const auto window = WindowFromDC(dc);
                dpi = window ? GetDpiForWindow(window) : 96;
            }
            final.emplace(display_artwork_.Text(), w == 1024, dpi);
            display_artwork_.AppendHud(*final, state);
            display_artwork_.AppendChrome(
                *final, frame.game_type,
                interaction ? interaction->automatic : std::array<bool, 4>{true, false, false, false},
                state.variant);
            display_artwork_.AppendStatus(*final, state);
            if (frame.captions) AppendActionCaptions(*final, *frame.captions, w, display_artwork_.Text());
        }

        const RECT dest{frame.destination.left, frame.destination.top,
                        frame.destination.right, frame.destination.bottom};

        native_buffer_.Render(
            dc, w, dest, tiles, artwork_, red, {}, stamp.str(),
            [&](HDC memory) {
                Gdiplus::Graphics graphics(memory);
                Gdiplus::Bitmap bg(w, h, w * 4, PixelFormat32bppARGB, background_.bgra.data());
                if (graphics.DrawImage(&bg, 0, 0) != Gdiplus::Ok)
                    throw std::runtime_error("Cannot draw table background");
            },
            [&](HDC memory, RECT region) {
                {
                    Gdiplus::Graphics graphics(memory);
                    ClipGraphics(graphics, region);
                    shadow_layer_.Paint(graphics, shadows, preferences_.tile_shadow);
                    const auto matrix = [](float r, float g, float b, float dr, float dg, float db) {
                        return Gdiplus::ColorMatrix{{{r, 0, 0, 0, 0},
                                                     {0, g, 0, 0, 0},
                                                     {0, 0, b, 0, 0},
                                                     {0, 0, 0, 1, 0},
                                                     {dr, dg, db, 0, 1}}};
                    };
                    const Gdiplus::ColorMatrix colors[]{
                        matrix(1, 1, 1, 0, 0, 0),
                        matrix(.95f, .5f, .75f, 0, .5f, .25f),
                        matrix(.5f, 1, 1, .5f, 0, 0),
                        matrix(1, 1, 1, 0, 0, 1),
                        matrix(.8f, .8f, .8f, 0, 0, 0),
                        matrix(1, .5f, 0, 0, .5f, 1),
                        matrix(1, .8f, .35f, 0, .2f, 0)
                    };
                    for (std::size_t idx = 0; idx < tiles.size(); ++idx) {
                        const auto &tile = tiles[idx];
                        if (tile.flight) continue;
                        const bool missing = frame.online_mode && std::string(tile.tile.role) == "missing_suit";
                        const auto &item = artwork_.Get(tile.tile, missing || red);
                        if (!Overlaps(region, TileBounds(tile, item.image.width, item.image.height))) continue;
                        Image shaded;
                        const auto *pixels = item.image.bgra.data();
                        if (const auto *shadow = FindTileShadow(shadows, tile); shadow && shadow->received) {
                            shaded = ShadeTile(item.image, tile.x - shadow->dx, tile.y - shadow->dy,
                                               *shadow->received, preferences_.tile_shadow);
                            pixels = shaded.bgra.data();
                        }
                        Gdiplus::Bitmap image(item.image.width, item.image.height, item.image.width * 4,
                                              PixelFormat32bppARGB, const_cast<BYTE *>(pixels));
                        const auto color = idx < frame.tile_colors.size() ? frame.tile_colors[idx] : HandColor::Normal;
                        if (color != HandColor::Normal) {
                            Gdiplus::ImageAttributes attributes;
                            attributes.SetColorMatrix(&colors[static_cast<unsigned>(color)]);
                            if (graphics.DrawImage(
                                    &image, Gdiplus::Rect(tile.x, tile.y, item.image.width, item.image.height),
                                    0, 0, item.image.width, item.image.height, Gdiplus::UnitPixel, &attributes) !=
                                Gdiplus::Ok)
                                throw std::runtime_error("Cannot draw table tile");
                        } else {
                            if (graphics.DrawImage(
                                    &image, Gdiplus::Rect(tile.x, tile.y, item.image.width, item.image.height),
                                    0, 0, item.image.width, item.image.height, Gdiplus::UnitPixel, nullptr) !=
                                Gdiplus::Ok)
                                throw std::runtime_error("Cannot draw table tile");
                        }
                        if (missing) {
                            Gdiplus::SolidBrush overlay(Gdiplus::Color(145, 90, 90, 90));
                            graphics.FillRectangle(&overlay, tile.x, tile.y, item.image.width, item.image.height);
                        }
                    }
                    PaintTableFlights(graphics, region, tiles, artwork_, red);
                    for (std::size_t i = 0; i < state.dora_indicators.size(); ++i) {
                        if (frame.original_display && state.wall.known) break;
                        const auto &item = artwork_.Get({'k', 0, state.dora_indicators[i], 0}, red);
                        Gdiplus::Bitmap image(item.image.width, item.image.height, item.image.width * 4,
                                              PixelFormat32bppARGB, const_cast<BYTE *>(item.image.bgra.data()));
                        const int x = static_cast<int>((frame.original_display ? 28 + i * 40 : 410 + i * 40) * unit);
                        const int y = static_cast<int>((frame.original_display ? 28 : 345) * unit);
                        if (graphics.DrawImage(&image, x, y) != Gdiplus::Ok)
                            throw std::runtime_error("Cannot draw dora indicator");
                    }
                }
                if (!frame.original_display) {
                    const HBRUSH gold = CreateSolidBrush(RGB(235, 185, 74));
                    for (const auto &tile: tiles)
                        if (!tile.flight && (tile.tile.drawn || tile.tile.riichi)) {
                            const auto &image = artwork_.Get(tile.tile, red).image;
                            RECT r{tile.x, tile.y, tile.x + image.width, tile.y + image.height};
                            FrameRect(memory, &r, gold);
                        }
                    DeleteObject(gold);
                }
                PaintTableInteraction(memory, frame, artwork_);
                if (frame.original_display) {
                    if (Overlaps(region, atomic_layers.front())) {
                        const auto draw_hud = [&](HDC target) {
                            Gdiplus::Graphics g(target);
                            display_artwork_.PaintHudImages(g, state);
                        };
                        if (reference_rendering_) draw_hud(memory);
                        else hud_cache_.Paint(memory, w, atomic_layers.front(), TableMetadata(state), draw_hud);
                    }
                    Gdiplus::Graphics graphics(memory);
                    ClipGraphics(graphics, region);
                    display_artwork_.PaintChromeBackground(graphics, interaction ? interaction->hovered_button : -1);
                    display_artwork_.PaintNukiCounts(graphics, state, artwork_);
                } else if (frame.online_mode) {
                    const auto font = CreateFontW(-static_cast<int>(22 * unit), 0, 0, 0, FW_NORMAL, FALSE, FALSE,
                                                  FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                                  CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
                    const auto old_font = SelectObject(memory, font ? font : GetStockObject(DEFAULT_GUI_FONT));
                    SetBkMode(memory, TRANSPARENT);
                    SetTextColor(memory, RGB(250, 246, 225));
                    const auto caption = [&](const std::string &text, int x, int y, int width) {
                        RECT r{static_cast<LONG>(x * unit), static_cast<LONG>(y * unit),
                               static_cast<LONG>((x + width) * unit), static_cast<LONG>((y + 28) * unit)};
                        const auto wide = NetworkWide(text);
                        DrawTextW(memory, wide.c_str(), -1, &r, DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                    };
                    constexpr int xs[]{220, 776, 280, 12}, ys[]{740, 580, 84, 580};
                    const char *winds[]{"E", "S", "W", "N"};
                    for (int i = 0; i < 4; ++i) {
                        if (!SeatPresent(state.seat_mask, i)) continue;
                        const auto &p = state.players[i];
                        std::string label = std::string(winds[SeatWind(state.seat_mask, state.dealer, i)]) +
                                            " " + std::to_string(frame.raw_scores[i]);
                        if (state.turn == i) label += " >";
                        if (p.reach) label += " RIICHI";
                        if (std::find(state.winners.begin(), state.winners.end(), i) != state.winners.end())
                            label += " WIN";
                        caption(label, xs[i], ys[i], i % 2 ? 235 : 500);
                        caption(p.name.empty() ? (i == 0 ? "You (relative seat 0)" : "Seat " + std::to_string(i))
                                               : p.name,
                                xs[i], ys[i] - (i == 2 ? -28 : 26), i % 2 ? 235 : 500);
                    }
                    caption(std::string(winds[(state.round / 4) % 4]) + " " + std::to_string(state.round % 4 + 1) +
                                " | Honba " + std::to_string(state.honba),
                            350, 258, 330);
                    caption("Sticks " + std::to_string(state.sticks) + " | Dora indicators", 350, 288, 330);
                    caption(frame.match_complete ? "MATCH COMPLETE"
                                                 : state.ended ? state.result
                                                               : frame.interactive ? "LOCAL GAME ACTIONS"
                                                                                   : "READ ONLY",
                            350, 407, 330);
                    SelectObject(memory, old_font);
                    if (font) DeleteObject(font);
                }
            },
            clock_overlay, appearances, metadata_regions, atomic_layers, shadow_damage, final ? &*final : nullptr);

        const unsigned button_dpi = frame.dpi ? frame.dpi : 96;
        for (const auto &button: frame.buttons) {
            RenderButtonToDc(dc, button, button_dpi);
        }
    }

    void GdiTableRenderer::RenderButtonToDc(HDC dc, const TableActionButton &button, unsigned dpi) {
        RECT r{button.viewport_rect.left, button.viewport_rect.top,
               button.viewport_rect.right, button.viewport_rect.bottom};
        const int bw = r.right - r.left, bh = r.bottom - r.top;
        if (bw <= 0 || bh <= 0) return;

        if (button.style == TableActionButtonStyle::OriginalExit) {
            for (int y = 0; y < bh; ++y) {
                const int value = button.enabled
                                      ? (y < bh / 2 ? 255 : 221 + 34 * (y - bh / 2) / std::max(1, bh - bh / 2 - 1))
                                      : 204;
                RECT line{r.left, r.top + y, r.right, r.top + y + 1};
                const auto brush = CreateSolidBrush(RGB(value, value, value));
                FillRect(dc, &line, brush);
                DeleteObject(brush);
            }
            RECT text = r;
            if (button.pressed) OffsetRect(&text, 1, 1);
            const auto old = SelectObject(dc, exit_font_ ? exit_font_ : GetStockObject(DEFAULT_GUI_FONT));
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, button.enabled ? RGB(34, 34, 34) : RGB(102, 102, 102));
            const wchar_t *label = button.label.empty() ? L"EXIT" : button.label.c_str();
            DrawTextW(dc, label, -1, &text, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dc, old);
            const auto border = CreateSolidBrush(
                button.enabled && button.hot ? RGB(51, 153, 255) : RGB(136, 136, 136));
            FrameRect(dc, &r, border);
            DeleteObject(border);
            if (button.focused) {
                InflateRect(&text, -2, -2);
                DrawFocusRect(dc, &text);
            }
            return;
        }

        bool rendered = false;
        if (button.style == TableActionButtonStyle::Sprite440 && ActionButtonText(button.id, false)) {
            const auto key = ActionButtonText(button.id, button.enabled && button.hot);
            Gdiplus::Graphics graphics(dc);
            const auto saved = graphics.Save();
            graphics.SetClip(Gdiplus::Rect(r.left, r.top, bw, bh));
            FillRect(dc, &r, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            const auto &im = display_artwork_.Get(440);
            Gdiplus::Bitmap bitmap(im.width, im.height, im.width * 4, PixelFormat32bppARGB,
                                   const_cast<BYTE *>(im.bgra.data()));
            const int offset = button.pressed ? 1 : 0;
            if (graphics.DrawImage(&bitmap, Gdiplus::Rect(r.left + offset, r.top + offset, bw, bh),
                                   0, 0, im.width, im.height, Gdiplus::UnitPixel, nullptr) != Gdiplus::Ok)
                throw std::runtime_error("Cannot draw native action button");
            if (!key) throw std::runtime_error("Unmapped action button text");
            auto &renderer = display_artwork_.Text();
            auto layout = renderer.Layout(*key);
            const auto &ink = layout.metrics.ink;
            const double sd = width_ == 1024 ? 1.0 / 1.4 : 1.0;
            const double fit = double(bw) / (width_ == 1440 ? 140 : 100);
            TextRenderContext context;
            context.native_layout_scale = sd;
            context.viewport_scale_x = context.viewport_scale_y = fit;
            context.dpi = dpi ? dpi : 96;
            context.origin_x = (r.left + r.right) / 2.0 + offset;
            context.origin_y = (r.top + r.bottom) / 2.0 + offset;
            context.device_clip = Gdiplus::RectF(
                static_cast<float>(r.left), static_cast<float>(r.top),
                static_cast<float>(bw), static_cast<float>(bh));
            SceneTextRun run;
            run.layout = layout;
            run.transform[4] = -ink.X - ink.Width / 2;
            run.transform[5] = -ink.Y - ink.Height / 2;
            renderer.Draw(graphics, context, run);
            if (!button.enabled) {
                Gdiplus::SolidBrush dim(Gdiplus::Color(150, 0, 0, 0));
                graphics.FillRectangle(&dim, r.left, r.top, bw, bh);
            }
            graphics.Restore(saved);
            rendered = true;
        }

        if (!rendered) {
            RECT bounds = r;
            DrawFrameControl(dc, &bounds, DFC_BUTTON, DFCS_BUTTONPUSH | (button.pressed ? DFCS_PUSHED : 0));
            const auto font = SelectObject(dc, ui_font_ ? ui_font_ : GetStockObject(DEFAULT_GUI_FONT));
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, GetSysColor(!button.enabled ? COLOR_GRAYTEXT : COLOR_BTNTEXT));
            DrawTextW(dc, button.label.c_str(), -1, &bounds, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dc, font);
        }

        if (button.focused) {
            RECT focus = r;
            InflateRect(&focus, -3, -3);
            DrawFocusRect(dc, &focus);
        }
    }
} // namespace tenhou
