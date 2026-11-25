#include "settlement_scene.hpp"
#include "network_protocol.hpp"
#include <algorithm>
#include <cmath>
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

        void CatalogText(SettlementScene &scene, std::string role, RECT rect,
                         std::vector<SettlementTextSpan> spans, int align = 0) {
            SettlementTextItem item;
            item.role = std::move(role);
            item.rect = rect;
            item.align = align;
            for (const auto &span: spans) {
                if (span.key == TextKey::Count) item.text += span.text;
                else item.text += TextUtf8(span.key);
            }
            item.spans = std::move(spans);
            scene.text.push_back(std::move(item));
        }

        std::string WithUnit(long long value, TextKey unit) {
            return std::to_string(value) + " " + std::string(TextUtf8(unit));
        }

        int WinYakuCount(const SettlementReceipt &r) {
            const auto count = r.yakuman.empty() ? SettlementDisplayYaku(r).size() : r.yakuman.size();
            return static_cast<int>(std::min(count, kSettlementPatternsPerPage));
        }

        int WinRows(const SettlementReceipt &r) {
            const int count = WinYakuCount(r);
            return count > 10 ? (count + 1) / 2 : count;
        }

        int WinHeight(const SettlementReceipt &r, double scale) {
            // The native AGARI window adds one resource-height per yaku row and
            // reserves separate room for the visible and hidden indicators.
            return static_cast<int>((370 + (r.dora.empty() ? 0 : 70) + (r.ura.empty() ? 0 : 80)) * scale) +
                   (scale == 1 ? 24 : 34) * WinRows(r);
        }

        SettlementScene BuildRoundSettlement(const SettlementReceipt &r, const std::array<std::string, 4> &names,
                                             std::size_t shown_yaku, bool ura, bool totals,
                                             const SettlementLayout &layout) {
            SettlementScene scene;
            scene.width = layout.width;
            scene.height = layout.width * 3 / 4;
            const double scale = layout.width == 1440 ? 1.4 : 1.0;
            const auto px = [&](double n) { return static_cast<int>(n * scale); };
            const bool riichi_win = r.who >= 0 && !r.generic;
            const int height = riichi_win ? WinHeight(r, scale) : px(640);
            const int payment_shift = height - px(640);
            const int left = (scene.width - px(660)) / 2, top = (scene.height - height) / 2;
            scene.panel = {left, top, left + px(660), top + height};
            const auto catalog = [&](std::string role, int x, int y, int w, int h,
                                     std::vector<SettlementTextSpan> spans, int align = 0) {
                CatalogText(scene, std::move(role), {left + px(x), top + px(y), left + px(x + w), top + px(y + h)},
                            std::move(spans), align);
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
            if (r.generic) {
                if (r.who >= 0) text("variant_name", names[r.who], 24, 12, 580, 16);
            }
            if (r.who < 0) {
                auto title = TextKey::title_draw;
                if (r.reason == "yao9") title = TextKey::draw_nine_terminals;
                else if (r.reason == "reach4") title = TextKey::draw_four_riichi;
                else if (r.reason == "ron3") title = TextKey::draw_triple_ron;
                else if (r.reason == "kan4") title = TextKey::draw_four_kans;
                else if (r.reason == "kaze4") title = TextKey::draw_four_winds;
                else if (r.reason == "nm") title = TextKey::draw_nagashi_mangan;
                catalog("reason", 80, 27, 500, 45, {{{}, title}}, 1);
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
                for (const auto &public_meld: r.public_melds) {
                    ReplayMeld meld;
                    const auto &kind = public_meld.kind;
                    meld.kind = kind == "chi" ? MeldKind::Chi : kind == "pon" ? MeldKind::Pon : kind == "open_kan" ? MeldKind::OpenKan : kind == "closed_kan" ? MeldKind::ClosedKan : MeldKind::AddedKan;
                    meld.tiles = public_meld.tiles; meld.called_tile = public_meld.called;
                    meld.added_tile = public_meld.added; meld.from = (public_meld.source - r.who + 4) % 4;
                    meld_tiles += kind == "closed_kan" || kind == "open_kan" ? 4 : 3;
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
                    if (riichi_win) {
                        const int width = std::min(scene.width, std::max(px(540),
                            static_cast<int>(union_rect.right - union_rect.left) + px(60)));
                        scene.panel.left = (scene.width - width) / 2;
                        scene.panel.right = scene.panel.left + width;
                    }
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
                const int pitch = 24, rows = std::max(1, WinRows(r));
                const bool two_columns = WinYakuCount(r) > rows;
                const int yaku_top = r.dora.empty() ? 117 : 185;
                ura_y = yaku_top + WinRows(r) * pitch + 8;
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
                if (!r.generic && !r.dora.empty()) indicators(r.dora, 110, "dora", true);
                if (!r.ura.empty()) indicators(r.ura, ura_y, "ura", ura);
                for (std::size_t i = 0; !r.generic && i < std::min({count, shown_yaku, kSettlementPatternsPerPage}); ++i) {
                    const int id = r.yakuman.empty() ? yaku[i].id : r.yakuman[i];
                    const int row = static_cast<int>(i) % rows, column = static_cast<int>(i) / rows;
                    const int x = two_columns ? 34 + column * 310 : 190, y = yaku_top + row * pitch;
                    const int name_width = two_columns ? 228 : 216;
                    const int row_height = pitch - 2;
                    const double entrance = i + 1 == shown_yaku ? layout.yaku_entrance : 1;
                    if (const auto key = SettlementYakuKey(id))
                        catalog("yaku", x, y, name_width, row_height, {{{}, *key}});
                    else text("unknown_yaku", SettlementYakuName(id), x, y, name_width, 18);
                    scene.text.back().entrance = entrance;
                    if (r.yakuman.empty()) {
                        catalog("han", x + name_width + 8, y, two_columns ? 66 : 140, row_height,
                                {{WithUnit(yaku[i].han, TextKey::unit_han)}});
                        scene.text.back().entrance = entrance;
                    }
                }
                if (count > 14) text("overflow", "More yaku: F2 / Text details", 210, 170, 350, 14);
                if (!r.generic && !r.has_yaku && !r.has_yakuman) text("missing_yaku", "Yaku not supplied", 190, 194, 400, 18);
                if (r.generic) {
                    catalog("variant_win", 245, 116, 170, 58,
                            {{{}, r.who == r.from ? TextKey::caption_tsumo : TextKey::caption_ron}}, 1);
                    const auto visible = std::min(shown_yaku, r.entries.size());
                    const auto pages = SettlementPatternPages(r), page = std::min(layout.pattern_page, pages - 1);
                    const auto begin = page * kSettlementPatternsPerPage;
                    const auto end = std::min(r.entries.size(), begin + kSettlementPatternsPerPage);
                    const bool pattern_columns = end - begin > 6;
                    for (std::size_t i = begin; i < std::min(visible, end); ++i) {
                        const auto &entry = r.entries[i];
                        const auto display = ResolveSettlementEntry(entry, r.rule);
                        const int j = static_cast<int>(i - begin);
                        const int x = pattern_columns ? 35 + j / 7 * 310 : 150;
                        const int y = 194 + (pattern_columns ? j % 7 : j) * 24;
                        const int width = pattern_columns ? 225 : 300;
                        const auto multiplier = display.count > 1 ? "×" + std::to_string(display.count) : "";
                        if (display.pattern) {
                            std::vector<SettlementTextSpan> spans{{{}, display.pattern->text}};
                            if (!multiplier.empty()) spans.push_back({multiplier});
                            catalog("variant_pattern", x, y, width, 22, std::move(spans));
                        } else {
                            // Arbitrary server labels may require glyphs outside the private font.
                            const int multiplier_width = multiplier.empty() ? 0 : 64;
                            text("variant_pattern", display.name, x, y, width - multiplier_width, 16);
                            if (!multiplier.empty())
                                catalog("variant_multiplier", x + width - multiplier_width, y, multiplier_width, 22,
                                        {{multiplier}});
                        }
                        if (display.total_value > 0)
                            catalog("variant_value", x + width + 8, y, pattern_columns ? 70 : 110, 22,
                                    {{WithUnit(display.total_value, TextKey::unit_han)}});
                    }
                    if (pages > 1) text("variant_page", std::to_string(page + 1) + " / " + std::to_string(pages), 280, 365, 100, 14, 0xffffffff, 1);
                    if (!r.flowers.empty())
                        catalog("variant_flowers", 35, 130, 200, 25,
                                {{{}, TextKey::yaku_guobiao_flower}, {" ×" + std::to_string(r.flowers.size())}});
                    if (totals) {
                        const int payout = r.points > 0 ? r.points : std::max(0, r.delta[r.who]);
                        auto value = WithUnit(r.fan, TextKey::unit_han);
                        if (payout > 0) value += " " + WithUnit(payout, TextKey::unit_points);
                        catalog("variant_fan", 35, 390, 590, 34, {{std::move(value)}}, 1);
                    }
                }
            }
            if (totals) {
                if (r.who >= 0 && !r.generic) {
                    const int total_y = ura_y + (r.ura.empty() ? 0 : 80);
                    if (r.has_ten) {
                        std::vector<SettlementTextSpan> spans;
                        if (r.yakuman.empty()) {
                            long long han = 0;
                            for (const auto &y: r.yaku) han += y.han;
                            spans.push_back({WithUnit(r.ten[0], TextKey::unit_fu) + "  " +
                                             WithUnit(han, TextKey::unit_han) + "  "});
                        }
                        constexpr TextKey limits[]{TextKey::limit_mangan, TextKey::limit_haneman, TextKey::limit_baiman,
                                                   TextKey::limit_sanbaiman, TextKey::limit_yakuman};
                        const int limit = r.ten[2];
                        if (limit >= 1 && limit <= static_cast<int>(std::size(limits))) {
                            spans.push_back({{}, limits[limit - 1]});
                            spans.push_back({"  "});
                        }
                        auto points = WithUnit(r.ten[1], TextKey::unit_points);
                        if (limit > static_cast<int>(std::size(limits))) points += " (limit " + std::to_string(limit) + ")";
                        spans.push_back({std::move(points)});
                        catalog("total", 35, total_y, 590, 30, std::move(spans), 1);
                    } else text("total", "Fu / points not supplied", 35, total_y, 590, 22, 0xffffffff, 1);
                }
                // Native screen positions are relative to the viewer: 0 below, 1
                // right, 2 above, 3 left. Seat wind depends on the current dealer.
                constexpr int xs[]{235, 407, 235, 63}, ys[]{515, 480, 445, 480};
                const int card_height = r.has_chips ? 57 : 41;
                for (int seat = 0; seat < 4; ++seat) {
                    if (!SeatPresent(r.seat_mask, seat)) continue;
                    const int x = xs[seat], y = ys[seat] - (r.has_chips ? (seat == 2 ? 16 : seat == 1 || seat == 3 ? 8 : 0) : 0);
                    const int wind = SeatWind(r.seat_mask, layout.dealer, seat);
                    const auto text_start = scene.text.size();
                    scene.score_panels.push_back({left + px(x), top + px(y), left + px(x + 190), top + px(y + card_height)});
                    OffsetRect(&scene.score_panels.back(), 0, payment_shift);
                    constexpr TextKey winds[]{TextKey::seat_neutral_east, TextKey::seat_neutral_south,
                                              TextKey::seat_neutral_west, TextKey::seat_neutral_north};
                    catalog("score_wind", x + 12, y + (card_height - 31) / 2, 33, 31, {{{}, winds[wind]}}, 1);
                    text("score_name", names[seat].empty() ? "Seat " + std::to_string(seat) : names[seat], x + 55,
                         y + 2, 112, 14, 0xffffffff, 1);
                    text("base", std::to_string(r.base[seat]), x + 48, y + 17, 62, 14, 0xffffffff, 2);
                    text("delta", Number(r.delta[seat], true), x + 114, y + 17, 72, 14,
                         r.delta[seat] > 0 ? 0xff00ffff : 0xffff0000, 0, r.delta[seat] != 0);
                    if (r.has_chips) {
                        const auto chip_str = [](int value) {
                            return (value > 0 ? "+" : "") + std::to_string(value) + "枚";
                        };
                        if (r.chip_delta[seat] == 0) {
                            text("chip_base", chip_str(r.chip_base[seat]), x + 55, y + 34, 112, 14, 0xffffffff, 1);
                        } else {
                            text("chip_base", chip_str(r.chip_base[seat]), x + 48, y + 34, 62, 14, 0xffffffff, 2);
                            text("chip_delta", chip_str(r.chip_delta[seat]), x + 114, y + 34, 72, 14,
                                 r.chip_delta[seat] > 0 ? 0xff00ffff : 0xffff0000, 0);
                        }
                    }
                    // The terminal value remains exported for independent receipt
                    // checks; the original card shows base plus delta, not a third row.
                    text("score", std::to_string(r.base[seat] + r.delta[seat]), x, y, 180, 14, 0xffffffff, 0, false);
                    for (auto i = text_start; i < scene.text.size(); ++i)
                        OffsetRect(&scene.text[i].rect, 0, payment_shift);
                }
                const int honba = r.ba.empty() ? layout.honba : r.ba[0], sticks =
                        r.ba.empty() ? layout.sticks : r.ba[1];
                if (!r.generic && honba >= 0 && sticks >= 0) {
                    image(401, left + px(311), top + px(496) + payment_shift);
                    text("honba", std::to_string(honba), 317, 493, 15, 12);
                    OffsetRect(&scene.text.back().rect, 0, payment_shift);
                    image(402, left + px(334), top + px(496) + payment_shift);
                    text("sticks", std::to_string(sticks), 340, 493, 24, 12);
                    OffsetRect(&scene.text.back().rect, 0, payment_shift);
                }
            }
            if (r.who < 0) {
                // Draws only need the title, payment cards and confirmation.
                // Share this compact layout across all rules.
                const int compact_top = (scene.height - px(300)) / 2;
                const int shift = compact_top + px(95) - (top + px(445));
                scene.panel = {left + px(60), compact_top, left + px(600), compact_top + px(300)};
                for (auto &panel: scene.score_panels) OffsetRect(&panel, 0, shift);
                for (auto &item: scene.text) {
                    if (item.role == "reason") OffsetRect(&item.rect, 0, compact_top - top);
                    else OffsetRect(&item.rect, 0, shift);
                }
                for (auto &item: scene.images) item.y += shift;
            }
            return scene;
        }
    }

    RECT SettlementConfirmRect(const SettlementState &s, std::size_t selected, int width) {
        const double scale = width == 1440 ? 1.4 : 1;
        const auto px = [&](int n) { return static_cast<int>(n * scale); };
        const bool round = selected < s.receipts.size();
        const int height = !round ? px(480) : s.receipts[selected].who < 0 ? px(300) :
                           s.receipts[selected].generic ? px(640) : WinHeight(s.receipts[selected], scale);
        const int top = (width * 3 / 4 - height) / 2 + height - px(50);
        const int x = width / 2 - px(28);
        return {x, top, x + px(56), top + px(21)};
    }

    SettlementScene BuildSettlementScene(const SettlementState &s, std::size_t selected,
                                         const std::array<std::string, 4> &names,
                                         std::size_t shown_yaku, std::size_t /*shown_tiles*/, bool ura, bool totals,
                                         const SettlementLayout &layout) {
        if (selected < s.receipts.size()) {
            auto scene = BuildRoundSettlement(s.receipts[selected], names, shown_yaku, ura, totals, layout);
            scene.confirm = SettlementConfirmRect(s, selected, layout.width);
            return scene;
        }
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
            CatalogText(scene, "final_title", {left, top + px(69), left + px(450), top + px(120)},
                        {{{}, TextKey::title_game_end}}, 1);
            std::vector<int> order;
            for (int i = 0; i < 4; ++i) if (SeatPresent(s.seat_mask, i)) order.push_back(i);
            // Original compares the supplied ranking points, retaining seat order on ties.
            std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
                return s.final_points[a] > s.final_points[b];
            });
            std::array<std::string, 4> scores, points, chips, gold;
            std::array<long long, 4> gold_value{};
            int score_width = 0, point_width = 0, chip_width = 0, gold_width = 0;
            const auto measure = [&](const std::string &value) {
                return layout.text_width
                           ? static_cast<int>(layout.text_width(value, static_cast<float>(14 * scale)))
                           : px(static_cast<double>(value.size()) * 9);
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
                if (s.has_final_chips) {
                    chips[seat] = (s.final_chips[seat] > 0 ? "+" : "") + std::to_string(s.final_chips[seat]) + "枚";
                    chip_width = std::max(chip_width, measure(chips[seat]));
                }
                if (s.has_ratio) {
                    gold_value[seat] = s.has_final_chips
                        ? std::llround((s.final_points[seat] * 1000.0 +
                                        static_cast<double>(s.ratio[0]) * s.final_chips[seat]) * s.ratio[1] * 0.01)
                        : std::llround(s.final_points[seat] * s.ratio[1] * 10.0);
                    gold[seat] = (gold_value[seat] > 0 ? "+" : "") + std::to_string(gold_value[seat]) + "G";
                    gold_width = std::max(gold_width, measure(gold[seat]));
                }
            }
            const int gap = px(8);
            int total_width = score_width + gap + point_width;
            if (s.has_final_chips) total_width += gap + chip_width;
            if (s.has_ratio) total_width += gap + gold_width;
            const int start = -total_width / 2;
            const int panel_half = std::max(px(140), total_width / 2 + px(24));
            const auto label = [&](const char *role, const std::string &value, int x, int y, int width, float em,
                                   unsigned color, int align) {
                scene.text.push_back({role, value, {x, y, x + width, y + px(24)}, em, color, true, align, true});
            };
            for (int rank = 0; rank < static_cast<int>(order.size()); ++rank) {
                const int seat = order[rank], y = top + px(190 + rank * 60);
                scene.score_panels.push_back({center - panel_half, y + px(-27), center + panel_half, y + px(27)});
                label("name", name(seat), center - panel_half, y + px(-21), panel_half * 2,
                      static_cast<float>((44.0 / 3) * scale), 0xffffffff, 1);
                int cursor = center + start;
                label("final_score", scores[seat], cursor, y + px(2), score_width,
                      static_cast<float>(14 * scale), 0xffffffff, 2);
                cursor += score_width + gap;
                label("final_points", points[seat], cursor, y + px(2), point_width,
                      static_cast<float>(14 * scale),
                      s.final_points[seat] > 0 ? 0xff00ffff : s.final_points[seat] < 0 ? 0xffff0000 : 0xffffffff, 2);
                cursor += point_width + gap;
                if (s.has_final_chips) {
                    label("final_chips", chips[seat], cursor, y + px(2), chip_width,
                          static_cast<float>(14 * scale),
                          s.final_chips[seat] > 0 ? 0xff00ffff : s.final_chips[seat] < 0 ? 0xffff0000 : 0xffffffff, 2);
                    cursor += chip_width + gap;
                }
                if (s.has_ratio) {
                    label("final_gold", gold[seat], cursor, y + px(2), gold_width,
                          static_cast<float>(14 * scale),
                          gold_value[seat] > 0 ? 0xff00ffff : gold_value[seat] < 0 ? 0xffff0000 : 0xffffffff, 2);
                }
            }
            return scene;
        }
        text("missing", "No settlement received", 40, 80, 900);
        return scene;
    }

    std::vector<SceneTextRun> LayoutSettlementText(TextRenderer &renderer, const SettlementTextItem &item, bool sd) {
        std::vector<SceneTextRun> runs;
        double cursor = 0, left = 0, right = 0, height = 0;
        for (const auto &span: item.spans) {
            TextLayout layout;
            if (span.key != TextKey::Count) layout = renderer.Layout(span.key);
            else layout = renderer.Layout(span.text, span.style);
            const auto &effects = layout.metrics.effects;
            if (!effects.IsEmptyArea()) {
                left = runs.empty() ? cursor + effects.X : std::min(left, cursor + effects.X);
                right = std::max(right, cursor + effects.GetRight());
                height = std::max(height, static_cast<double>(effects.Height));
                SceneTextRun run;
                run.layout = layout;
                run.transform[4] = cursor;
                runs.push_back(std::move(run));
            }
            cursor += layout.metrics.advance;
        }
        if (runs.empty()) return runs;
        const double to_hd = sd ? 1.4 : 1;
        const double width = (item.rect.right - item.rect.left) * to_hd;
        const double row_height = (item.rect.bottom - item.rect.top) * to_hd;
        const double fit = std::min({1.0, width / (right - left), row_height / height});
        const double indent = item.align == 1 ? (width - (right - left) * fit) / 2 :
                              item.align == 2 ? width - (right - left) * fit : 0;
        for (auto &run: runs) {
            const auto &effects = run.layout.metrics.effects;
            run.anchor = {static_cast<float>(item.rect.left * to_hd + indent),
                          static_cast<float>(item.rect.top * to_hd + row_height / 2)};
            run.transform = {fit, 0, 0, fit, (run.transform[4] - left) * fit,
                             -(effects.Y + effects.Height / 2) * fit};
            if (item.entrance < 1) {
                const double zoom = 2.5 - 1.5 * item.entrance;
                const double center = (right - left) * fit / 2;
                run.transform[0] *= zoom;
                run.transform[3] *= zoom;
                run.transform[4] = center + (run.transform[4] - center) * zoom;
                run.transform[5] *= zoom;
                run.opacity = item.entrance;
            }
        }
        return runs;
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
