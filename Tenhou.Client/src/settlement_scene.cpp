#include "settlement_scene.hpp"
#include "network_protocol.hpp"
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace tenhou {
    namespace {
        std::string Number(double n, bool sign = false) {
            std::ostringstream s;
            if (sign) s << std::showpos;
            s << std::setprecision(12) << n;
            return s.str();
        }

        SettlementScene BuildRoundSettlement(const SettlementReceipt &r, const std::array<std::string, 4> &names,
                                             std::size_t shown_yaku, bool ura, bool totals,
                                             const SettlementLayout &layout) {
            SettlementScene scene;
            scene.width = layout.width;
            scene.height = layout.width * 3 / 4;
            const double scale = layout.width == 1440 ? 1.4 : 1.0;
            const auto px = [&](double n) { return static_cast<int>(n * scale); };
            const int left = (scene.width - px(660)) / 2, top = (scene.height - px(640)) / 2;
            scene.panel = {left, top, left + px(660), top + px(640)};
            scene.confirm = {left + px(307), top + px(591), left + px(363), top + px(612)};
            const auto size = [&](unsigned id) -> SIZE {
                if (layout.image_size) return layout.image_size(id);
                return {px(id >= 552 ? 22 : id >= 481 ? 180 : 66), px(id >= 552 ? 24 : id >= 481 ? 24 : 28)};
            };
            const auto text = [&](const std::string &role, const std::string &value, int x, int y, int w,
                                  int em = 14, unsigned color = 0xffffffff, int align = 0, bool visible = true) {
                scene.text.push_back({
                    role, value, {left + px(x), top + px(y), left + px(x + w), top + px(y + em + 6)},
                    static_cast<float>(px(em)), color, visible, align
                });
            };
            const auto image = [&](unsigned id, int x, int y, double fit = 1) {
                scene.images.push_back({id, x, y, fit});
            };
            const auto bounds = [&](const TableTile &tile) -> RECT {
                const auto offset = TableTileOffset(tile, layout.width);
                if (layout.tile_bounds) {
                    auto rect = layout.tile_bounds(tile);
                    OffsetRect(&rect, offset.x, offset.y);
                    return rect;
                }
                const int x = px((17 - tile.slot % 20) * 42), y = tile.slot >= 40 && tile.slot < 60 ? -px(45) : 0;
                const bool side = tile.slot >= 20;
                return {
                    x + offset.x, y + offset.y, x + offset.x + px(side ? 64 : 45), y + offset.y + px(side ? 45 : 64)
                };
            };
            int ura_y = 322;
            if (r.who < 0) {
                // FUN_004af580 chooses these exact reason image IDs from type.
                unsigned title = 461;
                if (r.reason == "yao9") title = 467;
                else if (r.reason == "reach4") title = 468;
                else if (r.reason == "ron3") title = 469;
                else if (r.reason == "kan4") title = 470;
                else if (r.reason == "kaze4") title = 471;
                else if (r.reason == "nm") title = 472;
                image(title, scene.width / 2 - size(title).cx / 2, top + px(30));
                text("reason", r.reason.empty() ? "流局" : r.reason, 80, 30, 500, 24, 0xffffffff, 1, false);
            }
            if (r.who >= 0) {
                // Original tehai capture: t/i -> n/(17-i), translated -6; machi -4.
                // Use each perspective slot's actual rectangle, then center the union.
                std::vector<SettlementTileItem> hand;
                if (r.has_hand) {
                    auto tiles = r.hand;
                    const auto win = std::find(tiles.begin(), tiles.end(), r.winning_tile);
                    if (win != tiles.end()) tiles.erase(win);
                    std::sort(tiles.begin(), tiles.end());
                    tiles.push_back(r.winning_tile);
                    for (std::size_t i = 0; i < tiles.size(); ++i) {
                        const bool winning = i + 1 == tiles.size();
                        TableTile command{'n', 17 - static_cast<int>(i), tiles[i], 0};
                        auto rect = bounds(command);
                        OffsetRect(&rect, winning ? -4 : -6, 0);
                        hand.push_back({winning ? "winning_tile" : "hand", command, rect, false});
                    }
                } else text("missing_hand", "Revealed hand not supplied", 40, 45, 580, 20);
                // Reuse the table's original side/stack sprites, rather than rotating an
                // upright tile. Native n slots run right-to-left, including +60 for kamicha.
                ReplayState meld_state;
                meld_state.initialized = true;
                meld_state.seat_mask = r.seat_mask;
                meld_state.type = r.seat_mask == 15 ? 0 : 16;
                int meld_tiles = 0;
                for (int code: r.melds) {
                    auto meld = DecodeReplayMeld(static_cast<unsigned>(code), 0);
                    if (meld.kind == MeldKind::Nuki) {
                        meld_state.players[0].nuki.push_back(meld.tiles.front());
                        continue;
                    }
                    if (meld.kind == MeldKind::AddedKan) {
                        auto pon = meld.tiles;
                        pon.erase(std::find(pon.begin(), pon.end(), meld.called_tile));
                        meld.called_tile = pon.at((static_cast<unsigned>(code) >> 9) % 3);
                    }
                    meld_tiles += meld.kind == MeldKind::ClosedKan || meld.kind == MeldKind::OpenKan ? 4 : 3;
                    meld_state.players[0].melds.push_back(std::move(meld));
                }
                // C82135 starts the tehai capture at slot 1 in sanma even when no
                // north was extracted. Its m list is consumed backward, including n/0.
                if (r.seat_mask != 15) ++meld_tiles;
                std::reverse(meld_state.players[0].nuki.begin(), meld_state.players[0].nuki.end());
                // Original tehai capture packs the melds behind the concealed hand:
                // C 82230..82257 shifts n slots by 18 - meld slots - hai count.
                const int shift = std::max(0, 18 - meld_tiles - static_cast<int>(r.hand.size()));
                std::reverse(meld_state.players[0].melds.begin(), meld_state.players[0].melds.end());
                unsigned meld_sideways = 0;
                for (auto command: BuildTableScene(meld_state)) {
                    command.slot += shift;
                    command.meld_sideways <<= shift;
                    meld_sideways |= command.meld_sideways;
                    hand.push_back({command.role, command, bounds(command), command.slot >= 20, true});
                }
                // The original expands all later n placeholders, including concealed tiles
                // in the settlement capture. Keep the hand clear of the leftmost meld too.
                for (auto &item: hand)
                    if (!item.native_orientation) {
                        item.tile.meld_sideways = meld_sideways;
                        const auto offset = TableTileOffset(item.tile, layout.width);
                        OffsetRect(&item.rect, offset.x, offset.y);
                    }
                if (!hand.empty()) {
                    RECT union_rect = hand.front().rect;
                    for (const auto &item: hand) UnionRect(&union_rect, &union_rect, &item.rect);
                    const auto north_count = meld_state.players[0].nuki.size();
                    const auto count_extent = north_count
                                                  ? (layout.text_extent
                                                         ? layout.text_extent(
                                                             "99+\nx99", static_cast<float>(12 * scale))
                                                         : std::array < float, 2 >
                    {
                        static_cast<float>(px(30)), static_cast<float>(px(29))
                    }) : std::array < float, 2 >
                    {
                    };
                    // 0047eb19..0047eb2f reserves the fixed two-line label width in
                    // the tehai capture. 0047ed86..0047ede4 draws it at bottom right.
                    if (north_count) union_rect.right += static_cast<int>(count_extent[0]);
                    // Keep normal meld rows in native pixels, including the wider side
                    // sprite. Only exceptional long rows fit the available inset.
                    const double fit = std::min({
                        1.0, double(px(650)) / (union_rect.right - union_rect.left),
                        double(px(96)) / (union_rect.bottom - union_rect.top)
                    });
                    const int origin = left + (px(660) - static_cast<int>((union_rect.right - union_rect.left) * fit)) /
                                       2;
                    // 004b20cb..004b20e7: 100*resourceScale - composite image height.
                    // Added-kan stacks therefore grow upward without pushing into dora.
                    const int origin_y = top + px(100) - static_cast<int>((union_rect.bottom - union_rect.top) * fit);
                    for (auto &item: hand) {
                        item.rect = {
                            origin + static_cast<int>((item.rect.left - union_rect.left) * fit),
                            origin_y + static_cast<int>((item.rect.top - union_rect.top) * fit),
                            origin + static_cast<int>((item.rect.right - union_rect.left) * fit),
                            origin_y + static_cast<int>((item.rect.bottom - union_rect.top) * fit)
                        };
                        scene.tiles.push_back(item);
                    }
                    if (north_count) {
                        const int right = origin + static_cast<int>((union_rect.right - union_rect.left) * fit), bottom
                                = top + px(100);
                        const int x = right - static_cast<int>(count_extent[0] * fit), y =
                                bottom - static_cast<int>(count_extent[1] * fit);
                        scene.text.push_back({
                            "nuki_count", "\nx" + std::to_string(north_count), {x, y, right, bottom},
                            static_cast<float>(12 * scale * fit), 0xffffffff, true, 0, true
                        });
                    }
                }
                const auto yaku = SettlementDisplayYaku(r);
                const std::size_t count = r.yakuman.empty() ? yaku.size() : r.yakuman.size();
                const int columns = count > 7 ? 2 : 1;
                const int rows = static_cast<int>((std::min<std::size_t>(count, 14) + columns - 1) / columns);
                const int pitch = rows > 6 ? 20 : 24;
                ura_y = 194 + std::max(5, rows) * pitch + 8;
                const auto indicators = [&](const std::vector<int> &ids, int y, const char *role, bool reveal) {
                    // Original dora capture consists of n/14,n/13 (backs), followed by
                    // n/(12-i), i=0..4; not just the received indicator count.
                    std::vector<SettlementTileItem> strip;
                    RECT extent{};
                    for (int i = 0; i < 7; ++i) {
                        const int id = i >= 2 && static_cast<std::size_t>(i - 2) < ids.size() && reveal
                                           ? ids[i - 2]
                                           : -1;
                        TableTile command{'n', 14 - i, id, 0};
                        const auto rect = bounds(command);
                        strip.push_back({
                            i >= 2 && static_cast<std::size_t>(i - 2) < ids.size() ? role : "indicator_back", command,
                            rect, false
                        });
                        if (!i) extent = rect;
                        else UnionRect(&extent, &extent, &rect);
                    }
                    const int x = left + (px(660) - (extent.right - extent.left)) / 2;
                    for (auto &item: strip) {
                        OffsetRect(&item.rect, x - extent.left, top + px(y) - extent.top);
                        scene.tiles.push_back(item);
                    }
                };
                indicators(r.dora, 110, "dora", true);
                if (!r.ura.empty()) indicators(r.ura, ura_y, "ura", ura);
                for (std::size_t i = 0; i < std::min<std::size_t>({count, shown_yaku, 14}); ++i) {
                    const int column = static_cast<int>(i) / std::max(1, rows), row =
                            static_cast<int>(i) % std::max(1, rows);
                    const int id = r.yakuman.empty() ? yaku[i].id : r.yakuman[i];
                    const int x = columns == 1 ? 190 : 34 + column * 310, y = 194 + row * pitch;
                    const double glyph_scale = rows > 6 ? .8 : 1;
                    if (id >= 0 && id < 55) image(481 + id, left + px(x), top + px(y), glyph_scale);
                    else text("unknown_yaku", SettlementYakuName(id), x, y, columns == 1 ? 210 : 225, 18);
                    const int han = r.yakuman.empty() ? yaku[i].han : 0;
                    text("han", r.yakuman.empty() ? std::to_string(han) + " 飜" : "役満", columns == 1 ? 414 : x + 238, y,
                         60, 20, 0xffffffff, 0, false);
                    if (r.yakuman.empty() && han > 99) text("large_han", std::to_string(han) + " 飜",
                                                            columns == 1 ? 414 : x + 238, y, 60, 14);
                    else if (r.yakuman.empty()) {
                        const auto digits = std::to_string(han);
                        int cursor = left + px(columns == 1 ? 414 : x + 238);
                        for (char digit: digits) {
                            const unsigned key = 552 + digit - '0';
                            image(key, cursor, top + px(y), glyph_scale);
                            cursor += static_cast<int>(size(key).cx * glyph_scale);
                        }
                        image(562, cursor, top + px(y), glyph_scale);
                    }
                }
                if (count > 14) text("overflow", "More yaku: F2 / Text details", 210, 170, 350, 14);
                if (!r.has_yaku && !r.has_yakuman) text("missing_yaku", "Yaku not supplied", 190, 194, 400, 18);
            }
            if (totals) {
                if (r.who >= 0) {
                    int han = 0;
                    for (const auto &y: r.yaku) han += y.han;
                    const unsigned limit = r.has_ten && r.ten[2] >= 1 && r.ten[2] <= 5 ? 434 + r.ten[2] : 0;
                    const char *labels[]{"", "満貫", "跳満", "倍満", "三倍満", "役満"};
                    std::string value = r.has_ten
                                            ? (r.yakuman.empty()
                                                   ? std::to_string(r.ten[0]) + " 符  " + std::to_string(han) + " 飜  "
                                                   : "") +
                                              (limit ? std::string(labels[r.ten[2]]) + "  " : "") + std::to_string(
                                                  r.ten[1]) + " 点"
                                            : "Fu / points not supplied";
                    if (r.has_ten && r.ten[2] > 5) value += " (limit " + std::to_string(r.ten[2]) + ")";
                    const int total_y = std::min(414, ura_y + 80);
                    text("total", value, 35, total_y, 590, 22, 0xffffffff, 1, !r.has_ten || r.ten[2] > 5);
                    if (r.has_ten && r.ten[2] <= 5) {
                        std::vector<unsigned> glyphs;
                        const auto number = [&](int n) {
                            for (char c: std::to_string(n)) glyphs.push_back(552 + c - '0');
                        };
                        if (r.yakuman.empty()) {
                            number(r.ten[0]);
                            glyphs.push_back(563);
                            number(han);
                            glyphs.push_back(562);
                            glyphs.push_back(0);
                        }
                        if (limit) {
                            glyphs.push_back(limit);
                            glyphs.push_back(0);
                        }
                        number(r.ten[1]);
                        glyphs.push_back(564);
                        int width = 0;
                        for (unsigned id: glyphs) width += id ? size(id).cx : px(18);
                        int cursor = left + (px(660) - width) / 2;
                        for (unsigned id: glyphs) {
                            if (id)image(id, cursor, top + px(total_y));
                            cursor += id ? size(id).cx : px(18);
                        }
                    }
                }
                // Native screen positions are relative to the viewer: 0 below, 1
                // right, 2 above, 3 left. Seat wind depends on the current dealer.
                constexpr int xs[]{235, 407, 235, 63}, ys[]{515, 480, 445, 480};
                for (int seat = 0; seat < 4; ++seat) {
                    if (!SeatPresent(r.seat_mask, seat)) continue;
                    const int x = xs[seat], y = ys[seat], wind = SeatWind(r.seat_mask, layout.dealer, seat);
                    scene.score_panels.push_back({left + px(x), top + px(y), left + px(x + 190), top + px(y + 41)});
                    image(412 + wind, left + px(x + 15), top + px(y + 7));
                    text("score_name", names[seat].empty() ? "Seat " + std::to_string(seat) : names[seat], x + 55,
                         y + 2, 112, 14, 0xffffffff, 1);
                    text("base", std::to_string(r.base[seat]), x + 48, y + 17, 62, 14, 0xffffffff, 2);
                    text("delta", Number(r.delta[seat], true), x + 114, y + 17, 72, 14,
                         r.delta[seat] > 0 ? 0xff00ffff : 0xffff0000, 0, r.delta[seat] != 0);
                    // The terminal value remains exported for independent receipt
                    // checks; the original card shows base plus delta, not a third row.
                    text("score", std::to_string(r.base[seat] + r.delta[seat]), x, y, 180, 14, 0xffffffff, 0, false);
                }
                const int honba = r.ba.empty() ? layout.honba : r.ba[0], sticks =
                        r.ba.empty() ? layout.sticks : r.ba[1];
                if (honba >= 0 && sticks >= 0) {
                    image(401, left + px(311), top + px(496));
                    text("honba", std::to_string(honba), 317, 493, 15, 12);
                    image(402, left + px(334), top + px(496));
                    text("sticks", std::to_string(sticks), 340, 493, 24, 12);
                }
            }
            return scene;
        }
    }

    SettlementScene BuildSettlementScene(const SettlementState &s, std::size_t selected,
                                         const std::array<std::string, 4> &names,
                                         std::size_t shown_yaku, std::size_t /*shown_tiles*/, bool ura, bool totals,
                                         const SettlementLayout &layout) {
        if (selected < s.receipts.size())
            return BuildRoundSettlement(s.receipts[selected], names, shown_yaku, ura, totals, layout);
        SettlementScene scene;
        scene.width = layout.width;
        scene.height = layout.width * 3 / 4;
        const auto text = [&](std::string role, std::string value, int x, int y, int w, int size = 24,
                              unsigned color = 0xfff5f3db) {
            scene.text.push_back({
                std::move(role), std::move(value), {x, y, x + w, y + size + 10}, static_cast<float>(size), color
            });
        };
        const auto name = [&](int seat) { return names[seat].empty() ? "Seat " + std::to_string(seat) : names[seat]; };
        if (selected == s.receipts.size() && s.has_final) {
            // FUN_004aad30 OWARI @004ac129..004ac168: 450 x 480, centered.
            const double scale = layout.width == 1440 ? 1.4 : 1.0;
            const auto px = [&](double v) { return static_cast<int>(v * scale); };
            const int left = (scene.width - px(450)) / 2, top = (scene.height - px(480)) / 2, center = scene.width / 2;
            scene.panel = {left, top, left + px(450), top + px(480)};
            scene.confirm = {center - px(28), top + px(430), center + px(28), top + px(451)};
            const auto title = layout.image_size ? layout.image_size(462) : SIZE{px(100), px(30)};
            scene.images.push_back({462, center - title.cx / 2, top + px(75)});
            std::vector<int> order;
            for (int i = 0; i < 4; ++i) if (SeatPresent(s.seat_mask, i)) order.push_back(i);
            // Original compares the supplied ranking points, retaining seat order on ties.
            std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
                return s.final_points[a] > s.final_points[b];
            });
            std::array<std::string, 4> scores, points;
            int score_width = 0, point_width = 0;
            const auto measure = [&](const std::string &value) {
                return layout.text_width
                           ? static_cast<int>(layout.text_width(value, static_cast<float>(14 * scale)))
                           : px(value.size() * 9);
            };
            for (int seat = 0; seat < 4; ++seat) {
                if (!SeatPresent(s.seat_mask, seat)) continue;
                scores[seat] = Number(s.final_scores[seat]);
                std::ostringstream p;
                if (s.final_points[seat] > 0) p << '+';
                p << std::fixed << std::setprecision(1) << s.final_points[seat];
                points[seat] = p.str();
                score_width = std::max(score_width, measure(scores[seat]));
                point_width = std::max(point_width, measure(points[seat]));
            }
            const int start = -(score_width + point_width + 24) / 2;
            const auto label = [&](const char *role, const std::string &value, int x, int y, int width, float em,
                                   unsigned color, int align) {
                scene.text.push_back({role, value, {x, y, x + width, y + px(24)}, em, color, true, align, true});
            };
            for (int rank = 0; rank < static_cast<int>(order.size()); ++rank) {
                const int seat = order[rank], y = top + px(190 + rank * 60);
                scene.score_panels.push_back({center + px(-140), y + px(-27), center + px(140), y + px(27)});
                label("name", name(seat), center - px(140), y + px(-21), px(280),
                      static_cast<float>((44.0 / 3) * scale), 0xffffffff, 1);
                label("final_score", scores[seat], center + start, y + px(2), score_width,
                      static_cast<float>(14 * scale), 0xffffffff, 2);
                label("final_points", points[seat], center + start + score_width + 8, y + px(2), point_width,
                      static_cast<float>(14 * scale),
                      s.final_points[seat] > 0 ? 0xff00ffff : s.final_points[seat] < 0 ? 0xffff0000 : 0xffffffff, 2);
            }
            return scene;
        }
        text("missing", "No settlement received", 40, 80, 900);
        return scene;
    }

    std::vector<std::size_t> SettlementPaintOrder(const SettlementScene &scene) {
        std::vector<std::size_t> order;
        for (std::size_t i = 0; i < scene.tiles.size(); ++i) order.push_back(i);
        std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
            const auto key = [&](std::size_t index) {
                const auto &rect = scene.tiles[index].rect;
                return std::pair<LONG, LONG>{rect.bottom, -std::abs(rect.left + rect.right - scene.width)};
            };
            return key(a) < key(b);
        });
        return order;
    }

    std::string SettlementSceneJson(const SettlementScene &s) {
        std::ostringstream out;
        out << "{\"width\":" << s.width << ",\"height\":" << s.height << ",\"panel\":[" << s.panel.left << ',' << s.
                panel.top << ',' << s.panel.right << ',' << s.panel.bottom << "],\"confirm\":[" << s.confirm.left << ','
                << s.confirm.top << ',' << s.confirm.right << ',' << s.confirm.bottom << "],\"text\":[";
        for (std::size_t i = 0; i < s.text.size(); ++i) {
            if (i)out << ',';
            const auto &t = s.text[i];
            out << "{\"role\":" << NetworkJson(t.role) << ",\"text\":" << NetworkJson(t.text) << ",\"x\":" << t.rect.
                    left << ",\"y\":" << t.rect.top << ",\"right\":" << t.rect.right << ",\"bottom\":" << t.rect.bottom
                    << ",\"color\":" << t.color << ",\"visible\":" << (t.visible ? "true" : "false") << '}';
        }
        out << "],\"tiles\":[";
        for (std::size_t i = 0; i < s.tiles.size(); ++i) {
            if (i)out << ',';
            const auto &t = s.tiles[i];
            out << "{\"role\":" << NetworkJson(t.role) << ",\"tile\":" << t.tile.tile << ",\"x\":" << t.rect.left <<
                    ",\"y\":" << t.rect.top << ",\"right\":" << t.rect.right << ",\"bottom\":" << t.rect.bottom <<
                    ",\"family\":" << NetworkJson(std::string(1, t.tile.family)) << ",\"slot\":" << t.tile.slot <<
                    ",\"sideways\":" << (t.sideways ? "true" : "false") << '}';
        }
        out << "],\"paint_order\":[";
        const auto order = SettlementPaintOrder(s);
        for (std::size_t i = 0; i < order.size(); ++i) {
            if (i)out << ',';
            out << order[i];
        }
        out << "],\"images\":[";
        for (std::size_t i = 0; i < s.images.size(); ++i) {
            if (i)out << ',';
            const auto &im = s.images[i];
            out << "{\"id\":" << im.id << ",\"x\":" << im.x << ",\"y\":" << im.y << ",\"scale\":" << im.scale << '}';
        }
        out << "]}";
        return out.str();
    }
} // namespace tenhou
