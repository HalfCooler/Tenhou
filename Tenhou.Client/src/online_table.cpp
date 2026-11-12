#include "online_table.hpp"
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
        POINT anchor{};
        for (const auto &t: BuildTableScene(Project(online), auto_sort_))
            if (t.direction == 0 && std::string(t.role) == "hand") {
                const auto &im = artwork_.Get(t, (online.type & 2) == 0);
                anchor.x = std::max(anchor.x, static_cast<LONG>(im.x));
                anchor.y = std::max(anchor.y, static_cast<LONG>(im.y));
            }
        return anchor;
    }

    void OnlineTable::Configure(const std::filesystem::path &directory, int width) {
        CancelAnimation();
        display_artwork_.Configure(directory, width);
        directory_ = directory;
        width_ = width;
        artwork_ = {};
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
        previous_ = after; // Retain only the public projection, even if artwork fails.
        if (event.kind == ReplayEventKind::Metadata) return; // Identity/BYE refresh must not replace a running move.
        CancelAnimation();
        if (!animate || !before.initialized || event.kind == ReplayEventKind::Init) return;
        if (!artwork_.width()) artwork_.Load(directory_, width_);
        const bool red = (online.type & 2) == 0;
        presentation_.Begin(before, after, event, event.kind == ReplayEventKind::Meld ? 360 : 240, width_,
                            [&](const TableTile &t) {
                                const auto &im = artwork_.Get(t, red);
                                const auto offset = artwork_.Offset(t);
                                return std::make_pair(im.x + offset.x, im.y + offset.y);
                            }, now, auto_sort_);
    }

    ReplayState OnlineTable::Project(const OnlineState &online) {
        auto state = OnlineTableState(online);
        const auto &hand = online.players[0].known_hand;
        if (auto_sort_ || online.events < hand_events_ || (online.events != hand_events_ &&
                                                           (online.last_event == "INIT" || online.last_event ==
                                                            "REINIT"))) {
            hand_order_ = hand;
            reordered_draw_ = -1;
        } else {
            hand_order_.erase(std::remove_if(hand_order_.begin(), hand_order_.end(), [&](int t) {
                return std::find(hand.begin(), hand.end(), t) == hand.end();
            }), hand_order_.end());
            for (int t: hand) if (std::find(hand_order_.begin(), hand_order_.end(), t) == hand_order_.end()) hand_order_
                    .push_back(t);
        }
        // Retain the actual visible order, including the separated draw at the end.
        // Otherwise disabling sort would move a retained low draw back into sorted order.
        const int drawn = online.players[0].drawn_tile;
        const auto it = std::find(hand_order_.begin(), hand_order_.end(), drawn);
        if (drawn != reordered_draw_) reordered_draw_ = -1;
        if (it != hand_order_.end() && reordered_draw_ < 0) {
            hand_order_.erase(it);
            hand_order_.push_back(drawn);
        }
        hand_events_ = online.events;
        state.players[0].hand = hand_order_;
        // FUN_00488650 reassigns the gap to the final slot after manual insertion.
        // This is a display projection; the authoritative drawn entity is unchanged.
        if (drawn >= 0 && reordered_draw_ == drawn && !hand_order_.empty())
            state.players[0].drawn_tile = hand_order_.back();
        state.draw_end_slots = draw_end_slots_;
        return state;
    }

    bool OnlineTable::MoveHandTile(int tile, int target, const OnlineState &online) {
        const auto before = Project(online);
        const auto from = std::find(hand_order_.begin(), hand_order_.end(), tile);
        const auto to = std::find(hand_order_.begin(), hand_order_.end(), target);
        if (from == hand_order_.end() || to == hand_order_.end() || from == to) return false;
        const auto index = to - hand_order_.begin();
        hand_order_.erase(from);
        hand_order_.insert(hand_order_.begin() + index, tile);
        auto_sort_ = false;
        reordered_draw_ = online.players[0].drawn_tile;
        previous_ = Project(online);
        if (artwork_.width())
            presentation_.BeginManual(before, previous_, width_, [&](const TableTile &t) {
                const auto &im = artwork_.Get(t, (online.type & 2) == 0);
                const auto offset = artwork_.Offset(t);
                return std::make_pair(im.x + offset.x, im.y + offset.y);
            }, AnimationNow());
        return true;
    }

    void OnlineTable::SetAutoSort(bool enabled, const OnlineState &online) {
        if (auto_sort_ == enabled) return;
        const auto before = Project(online);
        auto_sort_ = enabled;
        const auto after = Project(online);
        previous_ = after;
        CancelAnimation();
        if (!enabled || !artwork_.width()) return;
        ReplayEvent event;
        event.tag = "SORT";
        presentation_.Begin(before, after, event, 240, width_, [&](const TableTile &tile) {
            const auto &image = artwork_.Get(tile, (online.type & 2) == 0);
            const auto offset = artwork_.Offset(tile);
            return std::make_pair(image.x + offset.x, image.y + offset.y);
        }, AnimationNow());
    }

    void OnlineTable::Paint(HDC dc, RECT bounds, const OnlineState &online, bool interactive,
                            const OpeningPresentation *opening, const TableInteraction *interaction, double now) {
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
        artwork_.SetRedTiles(online.red_tiles);
        state.relative_scores = relative_scores_;
        const bool red = (online.type & 2) == 0;
        const int w = width_, h = w * 3 / 4;
        const double unit = double(w) / 1024;
        const auto destination = FitNativeCanvas(bounds, w);
        // Native window chrome is an overlay, not a tile damage layer. Publishing
        // the retained table can otherwise erase its alpha glyphs after a silent
        // restore. Use the same original sprite painter once on the complete
        // native target; scaled analysis canvases keep their existing path.
        const bool native_chrome = original_display_ && destination.right - destination.left == w && destination.bottom
                                   - destination.top == h;
        artwork_.BeginFrame();
        const unsigned placed = opening && opening->Active() ? opening->placed() : 136;
        std::vector<PositionedTile> tiles;
        if (opening && opening->Active()) tiles = opening->Scene(artwork_, red);
        else if (presentation_.Active()) tiles = presentation_.Frame(now);
        else
            for (const auto &tile: BuildTableScene(state, auto_sort_)) {
                const auto &item = artwork_.Get(tile, red);
                const auto offset = artwork_.Offset(tile);
                tiles.push_back({tile, item.x + offset.x, item.y + offset.y});
            }
        SortTableScene(tiles, artwork_, red);
        if (original_display_) InsertNearWall(tiles, artwork_, state, red, placed, true);
        const auto shadows = BuildTileShadows(tiles, artwork_, ShadowPack::Embedded());
        const auto shadow_damage = TileShadowDamage(tiles, shadows, artwork_, red);
        std::ostringstream stamp;
        stamp << TableMetadata(state) << ':' << original_display_ << ':' << red << ':' << online.type << ':' << online.
                complete << ':' << interactive << ':' << native_chrome;
        if (interaction) {
            stamp << ':' << interaction->hovered_button;
            for (auto c: interaction->countdown) stamp << ':' << int(c);
            for (auto value: interaction->automatic) stamp << ':' << value;
        }
        std::vector<unsigned> appearances;
        for (const auto &t: tiles) {
            const bool hand = interaction && interactive && !state.ended && t.tile.tile >= 0 && t.tile.direction == 0 &&
                              std::string(t.tile.role) == "hand";
            const bool hover = hand && t.tile.tile == interaction->hovered;
            const bool playable = hand && std::find(interaction->playable.begin(), interaction->playable.end(),
                                                    t.tile.tile) != interaction->playable.end();
            const bool selected = hand && std::find(interaction->selected.begin(), interaction->selected.end(),
                                                    t.tile.tile) != interaction->selected.end();
            appearances.push_back(selected
                                      ? 3u
                                      : interaction && interaction->selecting && playable
                                            ? 1u
                                            : hover
                                                  ? (playable ? 1u : 2u)
                                                  : 0u);
        }
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
        native_buffer_.Render(dc, w, FitNativeCanvas(bounds, w), tiles, artwork_, red,
                              WallMetadata(state, placed, original_display_, red), stamp.str(), [&](HDC memory) {
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
                                          const auto &item = artwork_.Get(tile.tile, red);
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
                                          const bool own_hand =
                                                  interaction && interactive && !state.ended && tile.tile.tile >= 0 &&
                                                  tile.tile.direction == 0 && std::string(tile.tile.role) == "hand";
                                          const bool selectable = own_hand && std::find(interaction->playable.begin(),
                                                                      interaction->playable.end(),
                                                                      tile.tile.tile) != interaction->playable.end();
                                          const bool selected = own_hand && std::find(interaction->selected.begin(),
                                                                    interaction->selected.end(),
                                                                    tile.tile.tile) != interaction->selected.end();
                                          const bool hover =
                                                  own_hand && (
                                                      tile.tile.tile == interaction->hovered || selected || (
                                                          interaction->selecting && selectable));
                                          if (hover) {
                                              const bool playable = std::find(interaction->playable.begin(),
                                                                              interaction->playable.end(),
                                                                              tile.tile.tile) != interaction->playable.
                                                                    end();
                                              // Tint the selected/hovered sprite while preserving its alpha.
                                              Gdiplus::ColorMatrix matrix{
                                                  {
                                                      {.95f, 0, 0, 0, 0}, {0, .5f, 0, 0, 0}, {0, 0, .75f, 0, 0},
                                                      {0, 0, 0, 1, 0}, {0, .5f, .25f, 0, 1}
                                                  }
                                              };
                                              if (!playable) matrix = {
                                                                 {
                                                                     {.5f, 0, 0, 0, 0}, {0, 1, 0, 0, 0},
                                                                     {0, 0, 1, 0, 0}, {0, 0, 0, 1, 0}, {.5f, 0, 0, 0, 1}
                                                                 }
                                                             };
                                              if (selected) matrix = {
                                                                {
                                                                    {1, 0, 0, 0, 0}, {0, .8f, 0, 0, 0},
                                                                    {0, 0, .35f, 0, 0}, {0, 0, 0, 1, 0},
                                                                    {0, .2f, 0, 0, 1}
                                                                }
                                                            };
                                              attributes.SetColorMatrix(&matrix);
                                          }
                                          if (graphics.DrawImage(
                                                  &image, Gdiplus::Rect(tile.x, tile.y, item.image.width,
                                                                        item.image.height),
                                                  0, 0, item.image.width, item.image.height, Gdiplus::UnitPixel,
                                                  hover ? &attributes : nullptr) != Gdiplus::Ok)
                                              throw std::runtime_error("Cannot draw online tile");
                                      }
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
                                      if (!original_display_ && (tile.tile.drawn || tile.tile.riichi)) {
                                          const auto &image = artwork_.Get(tile.tile, red).image;
                                          RECT r{tile.x, tile.y, tile.x + image.width, tile.y + image.height};
                                          FrameRect(memory, &r, gold);
                                      }
                                  DeleteObject(gold);
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
                                              display_artwork_.PaintHud(g, state);
                                          };
                                          if (reference_rendering_) draw_hud(memory);
                                          else hud_cache_.Paint(memory, w, atomic_layers.front(), TableMetadata(state),
                                                                draw_hud);
                                      }
                                      Gdiplus::Graphics graphics(memory);
                                      ClipGraphics(graphics, region);
                                      if (!native_chrome) display_artwork_.PaintChrome(
                                          graphics, online.type,
                                          interaction ? interaction->automatic : std::array < bool, 4 >
                                      {
                                          true, false, false, false
                                      },
                                      interaction ? interaction->hovered_button : -1);
                                      display_artwork_.PaintStatus(graphics, state);
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
                              }, clock_overlay, appearances, metadata_regions, atomic_layers, shadow_damage);
        if (native_chrome) {
            const int saved = SaveDC(dc);
            OffsetViewportOrgEx(dc, destination.left, destination.top, nullptr);
            {
                Gdiplus::Graphics graphics(dc);
                display_artwork_.PaintChrome(graphics, online.type,
                                             interaction ? interaction->automatic : std::array < bool, 4 >
                {
                    true, false, false, false
                },
                interaction ? interaction->hovered_button : -1);
            }
            RestoreDC(dc, saved);
        }
    }

    int OnlineTable::HitTile(RECT bounds, const OnlineState &online, int x, int y) {
        if (!artwork_.width() || !online.initialized) return -1;
        const int w = width_, h = w * 3 / 4;
        const auto target = FitNativeCanvas(bounds, w);
        if (x < target.left || x >= target.right || y < target.top || y >= target.bottom) return -1;
        const int px = (x - target.left) * w / (target.right - target.left);
        const int py = (y - target.top) * h / (target.bottom - target.top);
        int found = -1;
        const bool red = (online.type & 2) == 0;
        std::vector<PositionedTile> scene;
        if (presentation_.Active()) scene = presentation_.Frame(AnimationNow());
        else
            for (const auto &t: BuildTableScene(Project(online), auto_sort_))
                if (t.direction == 0 && std::string(t.role) == "hand") {
                    const auto &item = artwork_.Get(t, red);
                    const auto offset = artwork_.Offset(t);
                    const int tx = item.x + offset.x, ty = item.y + offset.y;
                    scene.push_back({t, tx, ty});
                }
        SortTableScene(scene, artwork_, red);
        for (const auto &placed: scene) {
            if (placed.tile.direction != 0 || std::string(placed.tile.role) != "hand") continue;
            const auto &image = artwork_.Get(placed.tile, red).image;
            const int x0 = px - placed.x, y0 = py - placed.y;
            if (x0 >= 0 && y0 >= 0 && x0 < image.width && y0 < image.height &&
                image.bgra[(static_cast<std::size_t>(y0) * image.width + x0) * 4 + 3])
                found = placed.tile.tile;
        }
        return found;
    }
} // namespace tenhou
