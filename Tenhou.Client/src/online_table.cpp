#include "online_table.hpp"
#include "action_captions.hpp"
#include "tile_shadow.hpp"
#include "wall_scene.hpp"
#include "network_protocol.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <stdexcept>

namespace tenhou {
    void OnlineTable::SetPreferences(const ClientPreferences &preferences, const std::filesystem::path &config,
                                     bool rebuild) {
        Image background;
        if (rebuild) background = BuildPreferredBackground(directory_, config, width_, width_ * 3 / 4, preferences);
        preferences_ = preferences;
        config_ = config;
        if (rebuild) background_ = std::move(background);
        display_artwork_.SetPanelEnabled((preferences.flags & 0x2000) == 0);
        artwork_.SetColor(preferences.TileColor());
        native_buffer_.Invalidate();
        hud_cache_.Invalidate();
    }

    POINT OnlineTable::ActionAnchor(const OnlineState &online) {
        artwork_.SetRedTiles(online.red_tiles);
        if (!artwork_.width()) artwork_.Load(directory_, width_);
        const auto anchor = hand_.ActionAnchor(online, artwork_.geometry());
        return {static_cast<LONG>(anchor.first), static_cast<LONG>(anchor.second)};
    }

    void OnlineTable::Configure(const std::filesystem::path &directory, int width) {
        CancelAnimation();
        display_artwork_.Configure(directory, width);
        directory_ = directory;
        width_ = width;
        artwork_ = {};
        hand_.ClearRaised();
        background_ = {};
        native_buffer_.Invalidate();
        hud_cache_.Invalidate();
        artwork_.SetColor(preferences_.TileColor());
    }

    void OnlineTable::Prepare() {
        if (artwork_.width() && !background_.bgra.empty()) return;
        try {
            if (!artwork_.width()) artwork_.Load(directory_, width_);
            if (background_.bgra.empty()) background_ = BuildPreferredBackground(
                                              directory_, config_, width_, width_ * 3 / 4, preferences_);
        } catch (const std::exception &) {
            // Paint retries lazily and reports the resource error to the user.
        }
    }

    void OnlineTable::Accept(const OnlineState &online, const ReplayEvent &event, bool animate, double now) {
        artwork_.SetRedTiles(online.red_tiles);

        auto after = Project(online);
        auto before = previous_;
        if (event.kind == ReplayEventKind::Init || after.round != before.round || after.settlement.receipts.empty())
            settlement_presented_ = false;
        previous_ = after; // Retain only the public projection, even if artwork fails.
        if (event.kind == ReplayEventKind::Metadata && event.tag != "VARIANT") return; // Identity refresh keeps a running move.
        CancelAnimation();
        if (!animate || !before.initialized || event.kind == ReplayEventKind::Init) return;
        if (!artwork_.width()) artwork_.Load(directory_, width_);
        const bool red = (online.type & 2) == 0;
        presentation_.Begin(before, after, event, 240, width_,
                            [&](const TableTile &t) { return artwork_.geometry().Origin(t, red); },
                            now, false, &artwork_, red);
    }

    bool OnlineTable::MoveHandTile(int tile, int target, const OnlineState &online) {
        ReplayState before, after;
        if (!hand_.MoveHandTile(tile, target, online, &before, &after)) return false;
        previous_ = after;
        if (artwork_.width()) {
            const bool red = (online.type & 2) == 0;
            presentation_.BeginManual(before, previous_, width_,
                                      [&](const TableTile &t) { return artwork_.geometry().Origin(t, red); },
                                      AnimationNow());
        }
        return true;
    }

    void OnlineTable::SetAutoSort(bool enabled, const OnlineState &online) {
        ReplayState before, after;
        if (!hand_.SetAutoSort(enabled, online, &before, &after)) return;
        previous_ = after;
        CancelAnimation();
        if (!enabled || !artwork_.width()) return;
        ReplayEvent event;
        event.tag = "SORT";
        const bool red = (online.type & 2) == 0;
        presentation_.Begin(before, after, event, 240, width_,
                            [&](const TableTile &tile) { return artwork_.geometry().Origin(tile, red); },
                            AnimationNow(), false);
    }

    void OnlineTable::Paint(HDC dc, RECT bounds, const OnlineState &online, bool interactive,
                            const OpeningPresentation *opening, const TableInteraction *interaction, double now, const ActionCaptions *captions, unsigned dpi) {
        if (paused_at_) now = paused_at_;
        if (!artwork_.width() || background_.bgra.empty()) {
            TableArtwork art;
            art.Load(directory_, width_);
            art.SetColor(preferences_.TileColor());
            auto bg = BuildPreferredBackground(directory_, config_, width_, width_ * 3 / 4, preferences_);
            artwork_ = std::move(art);
            background_ = std::move(bg);
        }
        auto state = opening && opening->Active() ? opening->Frame() : Project(online);
        for (int seat = 0; seat < 4; ++seat)
            state.players[seat].connected = online.players[seat].connected;
        if (!settlement_presented_) {
            std::array<int, 4> scores{};
            for (int seat = 0; seat < 4; ++seat) scores[seat] = state.players[seat].score;
            scores = ScoresBeforeSettlement(scores, online.settlement);
            for (int seat = 0; seat < 4; ++seat) state.players[seat].score = scores[seat];
        }
        artwork_.SetRedTiles(online.red_tiles);
        state.relative_scores = relative_scores_;
        const bool red = (online.type & 2) == 0;
        const int w = width_, h = w * 3 / 4;
        const double unit = double(w) / 1024;
        hand_.SetRaised(interaction ? interaction->selected : std::vector<int>{});
        const auto &raised = hand_.raised();
        artwork_.BeginFrame();
        const unsigned placed = opening && opening->Active() ? opening->placed() : static_cast<unsigned>(state.wall.total);
        std::vector<PositionedTile> tiles;
        if (opening && opening->Active()) tiles = opening->Scene(artwork_, red);
        else if (presentation_.Active()) tiles = presentation_.Frame(now);
        else
            for (const auto &tile: BuildTableScene(state, false)) {
                const auto origin = artwork_.geometry().Origin(tile, red);
                tiles.push_back({tile, origin.first, origin.second});
            }
        for (auto &tile: tiles)
            if (tile.tile.direction == 0 && std::string(tile.tile.role) == "hand" &&
                std::find(raised.begin(), raised.end(), tile.tile.tile) != raised.end()) tile.y -= static_cast<int>(8 * unit);
        SortTableScene(tiles, artwork_, red);
        if (original_display_) InsertNearWall(tiles, artwork_, state, red, placed, true);
        if (opening && opening->Active()) opening->SlideWall(tiles);
        const auto shadows = BuildTileShadows(tiles, artwork_, ShadowPack::Embedded());
        const auto shadow_damage = TileShadowDamage(tiles, shadows, artwork_, red);
        std::ostringstream stamp;
        stamp << TableMetadata(state) << ':' << original_display_ << ':' << red << ':' << online.type << ':' << online.
                complete << ':' << interactive;
        if (interaction) {
            stamp << ':' << interaction->hovered_button;
            for (auto c: interaction->hint) stamp << ':' << int(c);
            for (auto c: interaction->countdown) stamp << ':' << int(c);
            for (auto value: interaction->automatic) stamp << ':' << value;
        }
        const auto hand_color = [&](const TableTile &tile) {
            if (!interaction) return HandColor::Normal;
            return HandProjection::ClassifyTile(tile, interactive, state.ended, state.variant, interaction->hovered,
                                                interaction->playable, interaction->selected,
                                                interaction->selecting, interaction->restrict_discards);
        };
        std::vector<unsigned> appearances;
        for (const auto &tile: tiles) appearances.push_back(static_cast<unsigned>(hand_color(tile.tile)));
        std::vector<RECT> metadata_regions, atomic_layers;
        if (original_display_) {
            const auto hud = BuildOriginalHud(state, w);
            const int radius = static_cast<int>(128 * hud.scale);
            const int cx = static_cast<int>(hud.center[0]), cy = static_cast<int>(hud.center[1]);
            metadata_regions.push_back({cx - radius, cy - radius, cx + radius, cy + radius});
            atomic_layers.push_back(metadata_regions.front());
            metadata_regions.push_back({w - static_cast<int>(220 * unit), 0, w, static_cast<int>(200 * unit)});
            metadata_regions.push_back({
                static_cast<LONG>(300 * (w == 1440 ? 1.4 : 1.0)), OriginalAutoButtonRect(w, 0).top,
                static_cast<LONG>(300 * (w == 1440 ? 1.4 : 1.0) + 100), OriginalAutoButtonRect(w, 0).top + 35
            });
            for (int i = 0; i < 4; ++i) metadata_regions.push_back(OriginalAutoButtonRect(w, i));
        }
        RECT clock_overlay{};
        if (interaction && !interaction->countdown.empty())
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
        std::optional<FinalScene> final;
        if (original_display_) {
            if (!dpi) {
                const auto window = WindowFromDC(dc);
                dpi = window ? GetDpiForWindow(window) : 96;
            }
            final.emplace(display_artwork_.Text(), w == 1024, dpi);
            display_artwork_.AppendHud(*final, state);
            display_artwork_.AppendChrome(*final, online.type,
                interaction ? interaction->automatic : std::array<bool, 4>{true, false, false, false}, state.variant);
            display_artwork_.AppendStatus(*final, state);
            if (captions) captions->Append(*final, w, display_artwork_.Text());
        }
        native_buffer_.Render(dc, w, FitNativeCanvas(bounds, w), tiles, artwork_, red, {}, stamp.str(), [&](HDC memory) {
                                  Gdiplus::Graphics graphics(memory);
                                  Gdiplus::Bitmap bg(w, h, w * 4, PixelFormat32bppARGB, background_.bgra.data());
                                  if (graphics.DrawImage(&bg, 0, 0) != Gdiplus::Ok) throw std::runtime_error(
                                      "Cannot draw online background");
                              }, [&](HDC memory, RECT region) {
                                  {
                                      Gdiplus::Graphics graphics(memory);
                                      ClipGraphics(graphics, region);
                                      shadow_layer_.Paint(graphics, shadows, preferences_.tile_shadow);
                                      for (const auto &tile: tiles) {
                                          if (tile.flight) continue;
                                          const bool missing = std::string(tile.tile.role) == "missing_suit";
                                          const auto &item = artwork_.Get(tile.tile, missing || red);
                                          if (!Overlaps(region, TileBounds(tile, item.image.width, item.image.height)))
                                              continue;
                                          Image shaded;
                                          const auto *pixels = item.image.bgra.data();
                                          if (const auto *shadow = FindTileShadow(shadows, tile);
                                              shadow && shadow->received) {
                                              shaded = ShadeTile(item.image, tile.x - shadow->dx, tile.y - shadow->dy,
                                                                 *shadow->received, preferences_.tile_shadow);
                                              pixels = shaded.bgra.data();
                                          }
                                          Gdiplus::Bitmap image(item.image.width, item.image.height,
                                                                item.image.width * 4,
                                                                PixelFormat32bppARGB, const_cast<BYTE *>(pixels));
                                          Gdiplus::ImageAttributes attributes;
                                          const auto color = hand_color(tile.tile);
                                          // FUN_00481ab0 selects matrices at 0x4fde78 by hand flags.
                                          const auto matrix = [](float r, float g, float b, float dr, float dg, float db) {
                                              return Gdiplus::ColorMatrix{{{r,0,0,0,0}, {0,g,0,0,0}, {0,0,b,0,0},
                                                                            {0,0,0,1,0}, {dr,dg,db,0,1}}};
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
                                          if (color != HandColor::Normal)
                                              attributes.SetColorMatrix(&colors[static_cast<unsigned>(color)]);
                                          if (graphics.DrawImage(
                                                  &image, Gdiplus::Rect(tile.x, tile.y, item.image.width,
                                                                        item.image.height),
                                                  0, 0, item.image.width, item.image.height, Gdiplus::UnitPixel,
                                                  color != HandColor::Normal ? &attributes : nullptr) != Gdiplus::Ok)
                                              throw std::runtime_error("Cannot draw online tile");
                                          if (missing) {
                                              Gdiplus::SolidBrush overlay(Gdiplus::Color(145, 90, 90, 90));
                                              graphics.FillRectangle(&overlay, tile.x, tile.y, item.image.width, item.image.height);
                                          }
                                      }
                                      PaintTableFlights(graphics, region, tiles, artwork_, red);
                                      for (std::size_t i = 0; i < state.dora_indicators.size(); ++i) {
                                          if (original_display_ &&state.wall.known) break;
                                          const auto &item = artwork_.Get({'k', 0, state.dora_indicators[i], 0}, red);
                                          Gdiplus::Bitmap image(item.image.width, item.image.height,
                                                                item.image.width * 4,
                                                                PixelFormat32bppARGB,
                                                                const_cast<BYTE *>(item.image.bgra.data()));
                                          if (graphics.DrawImage(
                                                  &image,
                                                  static_cast<int>(
                                                      (original_display_ ? 28 + i * 40 : 410 + i * 40) * unit),
                                                  static_cast<int>((original_display_ ? 28 : 345) * unit)) !=
                                              Gdiplus::Ok)
                                              throw std::runtime_error("Cannot draw online indicator");
                                      }
                                  }
                                  const HBRUSH gold = CreateSolidBrush(RGB(235, 185, 74));
                                  for (const auto &tile: tiles)
                                      if (!tile.flight && !original_display_ && (tile.tile.drawn || tile.tile.riichi)) {
                                          const auto &image = artwork_.Get(tile.tile, red).image;
                                          RECT r{tile.x, tile.y, tile.x + image.width, tile.y + image.height};
                                          FrameRect(memory, &r, gold);
                                      }
                                  DeleteObject(gold);
                                  if (interaction && !interaction->hint.empty()) {
                                      const auto font = CreateFontW(-static_cast<int>(14 * unit), 0, 0, 0, FW_NORMAL,
                                          FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                          CLEARTYPE_QUALITY, DEFAULT_PITCH, L"KaiTi");
                                      const auto old = SelectObject(memory, font);
                                      SetBkMode(memory, TRANSPARENT); SetTextColor(memory, RGB(255, 242, 190));
                                      RECT hint{w / 8, h - static_cast<int>(52 * unit), w * 7 / 8, h - static_cast<int>(28 * unit)};
                                      DrawTextW(memory, interaction->hint.c_str(), -1, &hint, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
                                      SelectObject(memory, old); DeleteObject(font);
                                  }
                                  if (interaction && !interaction->countdown.empty()) {
                                      int right = 0, top = 0;
                                      for (const auto &tile: tiles)
                                          if (tile.tile.direction == 0 && std::string(tile.tile.role) == "hand") {
                                              const auto &image = artwork_.Get(tile.tile, red).image;
                                              if (tile.x + image.width > right) {
                                                  right = tile.x + image.width;
                                                  top = tile.y;
                                              }
                                          }
                                      const auto clock_font = CreateFontW(
                                          -std::max(8, static_cast<int>(9 * unit)), 0, 0, 0, FW_NORMAL, FALSE, FALSE,
                                          FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                          NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"Arial");
                                      const auto old = SelectObject(
                                          memory, clock_font ? clock_font : GetStockObject(DEFAULT_GUI_FONT));
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
                                  if (original_display_) {
                                      if (Overlaps(region, atomic_layers.front())) {
                                          const auto draw_hud = [&](HDC target) {
                                              Gdiplus::Graphics g(target);
                                              display_artwork_.PaintHudImages(g, state);
                                          };
                                          if (reference_rendering_) draw_hud(memory);
                                          else hud_cache_.Paint(memory, w, atomic_layers.front(), TableMetadata(state),
                                                                draw_hud);
                                      }
                                      Gdiplus::Graphics graphics(memory);
                                      ClipGraphics(graphics, region);
                                      display_artwork_.PaintChromeBackground(graphics,
                                          interaction ? interaction->hovered_button : -1);
                                      display_artwork_.PaintNukiCounts(graphics, state, artwork_);
                                  } else {
                                      const auto font = CreateFontW(-static_cast<int>(22 * unit), 0, 0, 0, FW_NORMAL,
                                                                    FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                                                    CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
                                      const auto old_font = SelectObject(
                                          memory, font ? font : GetStockObject(DEFAULT_GUI_FONT));
                                      SetBkMode(memory, TRANSPARENT);
                                      SetTextColor(memory, RGB(250, 246, 225));
                                      const auto caption = [&](const std::string &text, int x, int y, int width) {
                                          RECT r{
                                              static_cast<LONG>(x * unit), static_cast<LONG>(y * unit),
                                              static_cast<LONG>((x + width) * unit), static_cast<LONG>((y + 28) * unit)
                                          };
                                          const auto wide = NetworkWide(text);
                                          DrawTextW(memory, wide.c_str(), -1, &r,
                                                    DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                                      };
                                      constexpr int xs[]{220, 776, 280, 12}, ys[]{740, 580, 84, 580};
                                      const char *winds[]{"E", "S", "W", "N"};
                                      for (int i = 0; i < 4; ++i) {
                                          if (!SeatPresent(state.seat_mask, i)) continue;
                                          const auto &p = online.players[i];
                                          std::string label = std::string(
                                                                  winds[SeatWind(state.seat_mask, state.dealer, i)]) +
                                                              " " + std::to_string(p.score);
                                          if (state.turn == i) label += " >";
                                          if (p.reach) label += " RIICHI";
                                          if (std::find(state.winners.begin(), state.winners.end(), i) != state.winners.
                                              end()) label += " WIN";
                                          caption(label, xs[i], ys[i], i % 2 ? 235 : 500);
                                          caption(p.name.empty()
                                                      ? (i == 0 ? "You (relative seat 0)" : "Seat " + std::to_string(i))
                                                      : p.name,
                                                  xs[i], ys[i] - (i == 2 ? -28 : 26), i % 2 ? 235 : 500);
                                      }
                                      caption(std::string(winds[(state.round / 4) % 4]) + " " + std::to_string(
                                                  state.round % 4 + 1) +
                                              " | Honba " + std::to_string(state.honba), 350, 258, 330);
                                      caption("Sticks " + std::to_string(state.sticks) + " | Dora indicators", 350, 288,
                                              330);
                                      caption(online.complete
                                                  ? "MATCH COMPLETE"
                                                  : state.ended
                                                        ? state.result
                                                        : interactive
                                                              ? "LOCAL GAME ACTIONS"
                                                              : "READ ONLY", 350, 407, 330);
                                      SelectObject(memory, old_font);
                                      if (font) DeleteObject(font);
                                  }
                              }, clock_overlay, appearances, metadata_regions, atomic_layers, shadow_damage, final ? &*final : nullptr);
    }

    int OnlineTable::HitTile(RECT bounds, const OnlineState &online, int x, int y) {
        if (!artwork_.width() || !online.initialized) return -1;
        artwork_.SetRedTiles(online.red_tiles);
        std::vector<PositionedTile> animated;
        const std::vector<PositionedTile> *active = nullptr;
        if (presentation_.Active()) {
            animated = presentation_.Frame(AnimationNow());
            active = &animated;
        }
        return hand_.HitTile({bounds.left, bounds.top, bounds.right, bounds.bottom}, online,
                             artwork_.geometry(), x, y, active);
    }
} // namespace tenhou
