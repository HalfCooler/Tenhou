#include "tile_geometry.hpp"
#include "resource_manager.hpp"
#include "window_layout.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string_view>

namespace tenhou {
    CanvasRect FitNativeCanvasBounds(const CanvasRect bounds, const int width) {
        const int aw = std::max(1, bounds.right - bounds.left);
        const int ah = std::max(1, bounds.bottom - bounds.top);
        const double scale = std::min(static_cast<double>(aw) / width, static_cast<double>(ah) / (width * 3 / 4));
        const int w = std::max(1, static_cast<int>(width * scale));
        const int h = std::max(1, static_cast<int>(width * 3 / 4 * scale));
        const int x = bounds.left + (aw - w) / 2;
        const int y = bounds.top + (ah - h) / 2;
        return {x, y, x + w, y + h};
    }

    TileOffset TableTileOffset(const TableTile &tile, const int width) {
        const auto scale_hd = [&](const int v) -> int {
            if (width <= 1440) return v;
            return static_cast<int>(std::lround(v * (static_cast<double>(width) / 1440.0)));
        };
        // FUN_004902f0 stores literal native-pixel wall offsets from the PE table.
        if (tile.family == 'y') return {scale_hd(tile.dx), scale_hd(tile.dy)};
        TileOffset offset{static_cast<int>(tile.dx * width / 1024.0), static_cast<int>(tile.dy * width / 1024.0)};
        if (tile.native_draw_offset) {
            constexpr int x[]{4, -1, -3, -1}, y[]{0, -3, 0, 3}; // PE 004e6098 / 004e60a8.
            offset.x += scale_hd(x[tile.direction]);
            offset.y += scale_hd(y[tile.direction]);
        }
        if (tile.river_waiting) {
            // FUN_00491fe0 discard: original signed-short placeholder offsets,
            // PE 004e61b0 (x) and 004e61a0 (y), literal pixels at 1024/1440, scaled above 1440.
            static constexpr int x[]{15, 5, -15, -5}, y[]{5, -15, -5, 15};
            offset.x += scale_hd(x[tile.direction]);
            offset.y += scale_hd(y[tile.direction]);
        }
        const bool hd_table = width != 1024;
        if (tile.family == 'k' && tile.river_reach_slot >= 0) {
            // FUN_00478be0: literal PE 004fd518 / 004fd724, indexed by
            // declaration slot (not following slot); only the rest of that row.
            const int slot = tile.river_reach_slot, seat = tile.direction;
            if (slot < 24 && slot != 5 && slot != 11) {
                static constexpr int x[2][4]{{13, -1, -11, -1}, {18, -2, -16, -2}};
                offset.x += scale_hd(x[hd_table][seat]);
                if (seat == 1) offset.y += scale_hd(hd_table ? (slot >= 18 ? -11 : -12) : -9);
                if (seat == 3) offset.y += scale_hd(hd_table ? (slot >= 18 ? 13 : 12) : (slot >= 18 ? 10 : 9));
            }
        }
        if (tile.family != 'n' || !tile.meld_sideways) return offset;
        // FUN_00478e30 / OffsetAdd_NakiYoko_NoRedraw. Original PE strings at
        // 004fd97c (1024) and 004fdacc (1440), indexed by seat, axis, called slot.
        // Apply each expansion only AFTER its horizontal tile, including kan stacks.
        static constexpr int shifts[2][4][2][16]{
            {
                {{-14, -14, -14, -14, -14, -14, -14, -14, -14, -14, -14, -14, -14, -14, -14, -14}, {}},
                {
                    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
                    {8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 11}
                },
                {{11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11}, {}},
                {
                    {3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
                    {-12, -12, -12, -11, -11, -11, -10, -10, -10, -9, -9, -9, -9, -9, -8, -8}
                }
            },
            {
                {{-19, -19, -19, -19, -19, -19, -19, -19, -19, -19, -19, -19, -19, -19, -19, -19}, {}},
                {
                    {2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3},
                    {11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14}
                },
                {{15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15}, {}},
                {
                    {3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2},
                    {-16, -16, -15, -15, -15, -14, -14, -14, -13, -13, -13, -12, -12, -12, -11, -11}
                }
            }
        };
        int sum_x = 0, sum_y = 0;
        for (int slot = 0; slot < std::min(16, tile.slot % 20); ++slot)
            if (tile.meld_sideways & (1u << slot)) {
                sum_x += shifts[hd_table][tile.direction][0][slot];
                sum_y += shifts[hd_table][tile.direction][1][slot];
            }
        offset.x += scale_hd(sum_x);
        offset.y += scale_hd(sum_y);
        return offset;
    }

    int TileVariant(const int tile, const bool red) {
        if (tile < 0 || tile > 135) return 37;
        if (red && (tile == 16 || tile == 52 || tile == 88)) return tile == 16 ? 0 : tile == 52 ? 10 : 20;
        const int kind = tile / 4;
        return kind < 27 ? kind / 9 * 10 + kind % 9 + 1 : 30 + kind % 9;
    }

    void UpdateOriginSuitOrder(std::array<int, 3> &suit_order, const std::vector<int> &tiles) {
        std::array<int, 3> present{0, 0, 0};
        for (int tile: tiles)
            if (tile >= 0 && tile < 108) ++present[static_cast<std::size_t>(tile / 36)];
        if (present[0] + present[1] + present[2] == 0) return;
        if (present[0] != 0 && present[1] != 0 && present[2] != 0) return;
        for (int &count: present) count = count > 0 ? 1 : 0;
        std::array<int, 3> by_rank{0, 1, 2};
        for (int suit = 0; suit < 3; ++suit) {
            const int rank = suit_order[static_cast<std::size_t>(suit)];
            if (rank >= 0 && rank < 3) by_rank[static_cast<std::size_t>(rank)] = suit;
        }
        int r0 = by_rank[0], r1 = by_rank[1], r2 = by_rank[2];
        if (present[static_cast<std::size_t>(r0)] < present[static_cast<std::size_t>(r1)]) std::swap(r0, r1);
        if (present[static_cast<std::size_t>(r0)] < present[static_cast<std::size_t>(r2)]) std::swap(r0, r2);
        if (present[static_cast<std::size_t>(r1)] < present[static_cast<std::size_t>(r2)]) std::swap(r1, r2);
        suit_order[static_cast<std::size_t>(r0)] = 0;
        suit_order[static_cast<std::size_t>(r1)] = 1;
        suit_order[static_cast<std::size_t>(r2)] = 2;
    }

    void SortOriginHand(std::vector<int> &tiles, const std::array<int, 3> &suit_order) {
        const auto key = [&](const int tile) {
            return tile >= 0 && tile < 108
                       ? (tile % 36) + suit_order[static_cast<std::size_t>(tile / 36)] * 36
                       : tile;
        };
        std::sort(tiles.begin(), tiles.end(), [&](const int a, const int b) {
            const int ka = key(a), kb = key(b);
            return ka != kb ? ka < kb : a < b;
        });
    }

    std::vector<TableTile> BuildTableScene(const ReplayState &state, bool sort_self, bool sort_opponents) {
        std::vector<TableTile> result;
        if (!state.initialized) return result;
        for (int seat = 0; seat < 4; ++seat) {
            if (!SeatPresent(state.seat_mask, seat)) continue;
            const auto seat_begin = result.size();
            const auto &player = state.players[seat];
            auto hand = player.hand;
            if (player.drawn_tile >= 0) hand.erase(std::remove(hand.begin(), hand.end(), player.drawn_tile), hand.end());
            if (seat ? sort_opponents : sort_self) std::sort(hand.begin(), hand.end());
            if (player.drawn_tile >= 0) hand.push_back(player.drawn_tile);
            if (player.retired) {
                std::fill(hand.begin(), hand.end(), -1);
                if (player.winning_tile >= 0) hand.push_back(player.winning_tile);
            }
            if (hand.size() > 14) throw std::runtime_error("Table hand exceeds fourteen tiles");
            for (std::size_t i = 0; i < hand.size(); ++i) {
                const bool winning = player.retired && i + 1 == hand.size() && player.winning_tile >= 0;
                const bool drawn = winning || (hand[i] >= 0 && hand[i] == player.drawn_tile) ||
                                   (player.hidden_draw && i + 1 == hand.size() && hand[i] < 0);
                // FUN_00486e80: revealed opponent t/i/d becomes n/(17-i)/d.
                TableTile command{
                    seat ? 'n' : 't', seat ? 17 - static_cast<int>(i) : static_cast<int>(i), hand[i], seat
                };
                // Unknown online opponents use original standing t/slot/back resources.
                if (hand[i] < 0) {
                    command.family = 't';
                    command.slot = static_cast<int>(i);
                }
                if (player.retired) {
                    command.family = 'n';
                    command.slot = 17 - static_cast<int>(i);
                    command.role = winning ? "winning" : "retired";
                }
                if (state.ended && !player.retired && state.result.rfind("RYUUKYOKU", 0) == 0 &&
                    i < state.draw_end_slots && !state.settlement.receipts.empty()) {
                    const auto &receipt = state.settlement.receipts.back();
                    // FUN_0048de30 event 0x84 / FUN_00486e80: absent haiN ->
                    // 0xfd -> n/(17-slot)/37; only supplied hands are exposed.
                    command.family = 'n';
                    command.slot = 17 - static_cast<int>(i);
                    command.tile = receipt.has_revealed[seat] && i < receipt.revealed[seat].size()
                                       ? receipt.revealed[seat][i]
                                       : -1;
                    command.drawn = false;
                    result.push_back(command);
                    continue;
                }
                command.drawn = drawn;
                command.native_draw_offset = drawn && !winning && !state.variant;
                if (drawn && !command.native_draw_offset) {
                    constexpr int x[]{12, 0, -12, 0}, y[]{0, 10, 0, -10};
                    command.dx = x[seat];
                    command.dy = y[seat];
                }
                result.push_back(command);
            }
            int index = 0, row = -1, reach_slot = -1;
            if (state.variant && player.missing_suit >= 0) {
                TableTile marker{'k', 0, 16 + player.missing_suit * 36, seat};
                marker.role = "missing_suit";
                result.push_back(marker);
                ++index;
            }
            for (const auto &discard: player.discards) {
                if (discard.claimed) continue; // Claimed tiles live in the meld only.
                int slot = index < 25 ? index : 12 + (index - 25) % 13;
                const int current_row = index < 6 ? 0 : index < 12 ? 1 : index < 25 ? 2 : 3 + (index - 25) / 13;
                if (current_row != row) {
                    row = current_row;
                    reach_slot = -1;
                }
                TableTile command{'k', slot + (discard.riichi ? 40 : 0), discard.tile, seat};
                command.riichi = discard.riichi;
                command.role = "river";
                command.river_reach_slot = reach_slot;
                command.river_waiting = !state.ended && state.last_discard_seat == seat &&
                                        &discard == &player.discards.back();
                if (index >= 25) {
                    const int offset = 42 * (1 + (index - 25) / 13);
                    command.dy += seat == 0 ? offset : seat == 2 ? -offset : 0;
                    command.dx += seat == 1 ? offset : seat == 3 ? -offset : 0;
                }
                if (discard.riichi) reach_slot = slot;
                result.push_back(command);
                ++index;
            }
            // FUN_0049a570: n/0 is the last extraction, with a separate count.
            if (!player.nuki.empty()) {
                TableTile extracted{'n', 0, player.nuki.back(), seat};
                extracted.role = "nuki";
                result.push_back(extracted);
            }
            if (!player.flowers.empty()) {
                TableTile flower{'n', 0, player.flowers.front(), seat};
                flower.role = "flower";
                result.push_back(flower);
            }
            int slot = (state.type & 16) || !player.flowers.empty() ? 1 : 0;
            unsigned meld_sideways = 0;
            for (const auto &meld: player.melds) {
                auto tiles = meld.tiles;
                const bool closed = meld.kind == MeldKind::ClosedKan;
                const bool added = meld.kind == MeldKind::AddedKan;
                const int source = (meld.from - seat + 4) % 4;
                const int count = closed || meld.kind == MeldKind::OpenKan ? 4 : 3;
                const int sideways = closed ? -1 : source == 1 ? 0 : source == 2 ? count - 2 : count - 1;
                if (sideways >= 0) meld_sideways |= 1u << (slot + sideways);
                int added_tile = -1;
                if (added) {
                    added_tile = meld.added_tile >= 0 ? meld.added_tile : DecodeReplayMeld(meld.encoded, seat).called_tile;
                    tiles.erase(std::remove(tiles.begin(), tiles.end(), added_tile), tiles.end());
                }
                if (!closed) tiles.erase(std::remove(tiles.begin(), tiles.end(), meld.called_tile), tiles.end());
                std::sort(tiles.rbegin(), tiles.rend());
                if (closed) {
                    const auto red = std::find_if(tiles.begin(), tiles.end(), [](const int tile) {
                        return tile == 16 || tile == 52 || tile == 88;
                    });
                    if (red != tiles.end()) std::iter_swap(tiles.begin() + 1, red);
                }
                std::size_t next = 0;
                for (int i = 0; i < count; ++i) {
                    const int tile = i == sideways ? meld.called_tile : tiles.at(next++);
                    TableTile command{
                        'n', slot + i + (i == sideways ? (source == 3 ? 60 : 20) : 0),
                        closed && (i == 0 || i == 3) ? -1 : tile, seat
                    };
                    command.role = "meld";
                    command.meld_sideways = meld_sideways;
                    result.push_back(command);
                }
                if (added) {
                    TableTile command{'n', slot + sideways + 40, added_tile, seat};
                    command.role = "added_kan";
                    command.meld_sideways = meld_sideways;
                    result.push_back(command);
                }
                slot += count;
            }
            // Face-up opponent hands also use n placeholders beyond the melds.
            for (std::size_t i = seat_begin; i < result.size(); ++i)
                if (result[i].family == 'n') result[i].meld_sideways = meld_sideways;
        }
        return result;
    }

    void TileGeometryCatalog::Load(const std::filesystem::path &directory, const int width) {
        if (!IsSupportedTableWidth(width)) throw std::runtime_error("Unsupported table canvas");
        const AssetId even_id = width == 1024 ? AssetId::TablePackSd :
                                width == 1440 ? AssetId::TablePack :
                                width == 1920 ? AssetId::TablePack1920 : AssetId::TablePack2640;
        const AssetId odd_id = width == 1024 ? AssetId::TableCompanionPackSd :
                               width == 1440 ? AssetId::TableCompanionPack :
                               width == 1920 ? AssetId::TableCompanionPack1920 : AssetId::TableCompanionPack2640;
        const std::string expected_even = width == 1440 ? "obj2" : "obj0";
        const std::string expected_odd = width == 1440 ? "obj3" : "obj1";
        auto even = std::make_shared<ObjectPack>(LoadObjectPack(ResourceManager(directory).Path(even_id)));
        auto odd = std::make_shared<ObjectPack>(LoadObjectPack(ResourceManager(directory).Path(odd_id)));
        if (even->signature != expected_even || odd->signature != expected_odd)
            throw std::runtime_error("Wrong table object pack resolution");
        LoadPacks(std::move(even), std::move(odd), width);
    }

    void TileGeometryCatalog::LoadPacks(std::shared_ptr<const ObjectPack> even, std::shared_ptr<const ObjectPack> odd,
                                        const int width) {
        if (!IsSupportedTableWidth(width)) throw std::runtime_error("Unsupported table canvas");
        if (!even || !odd) throw std::runtime_error("Missing table object packs");
        even_ = std::move(even);
        odd_ = std::move(odd);
        width_ = width;
        alpha_cache_.clear();
    }

    std::uint32_t TileGeometryCatalog::ImageKey(const TableTile &tile, const bool red) const {
        if (!loaded()) throw std::runtime_error("Table artwork is not loaded");
        if (tile.slot < 0 || tile.slot > 255 || tile.direction < 0 || tile.direction > 3)
            throw std::runtime_error("Invalid table resource command");
        const bool aka = std::string_view(tile.role) == "missing_suit" ||
                         (red && std::find(red_tiles_.begin(), red_tiles_.end(), tile.tile) != red_tiles_.end());
        const bool flower = tile.tile >= 136 && tile.tile < 144;
        const unsigned variant = static_cast<unsigned>(
            flower ? 40 + tile.tile - 136 : aka ? tile.tile / 36 * 10 : TileVariant(tile.tile, false));
        return static_cast<unsigned char>(tile.family) | (static_cast<unsigned>(tile.slot) << 8) |
               (variant << 16) | (static_cast<unsigned>(tile.direction) << 24);
    }

    TileSpriteBounds TileGeometryCatalog::Bounds(const TableTile &tile, const bool red) const {
        const auto key = ImageKey(tile, red);
        const unsigned variant = (key >> 16) & 0xff;
        const bool flower = variant >= 40;
        if (flower && (tile.family != 'n' || tile.slot != 0))
            throw std::runtime_error("Unsupported flower placeholder");
        const bool direct = flower || variant == 34 || variant == 37;
        const auto &pack = direct ? *odd_ : *even_;
        const auto lookup_key = flower ? (key & 0xff00ffffu) | (34u << 16) : key;
        const auto entry = pack.key_index.find(lookup_key);
        if (entry == pack.key_index.end())
            throw std::runtime_error("Missing table resource key " + std::to_string(key));
        const auto &record = pack.entries[entry->second];
        if (direct || variant == 38 || variant == 39)
            return {record.x, record.y, record.width, record.height};
        const std::uint32_t base_key = record.key[0] | (static_cast<std::uint32_t>(record.key[1]) << 8) |
                                       (34u << 16) | (static_cast<std::uint32_t>(record.key[3]) << 24);
        const auto base_entry = odd_->key_index.find(base_key);
        if (base_entry == odd_->key_index.end())
            throw std::runtime_error("Object companion base is missing");
        const auto &base = odd_->entries[base_entry->second];
        return {base.x, base.y, base.width, base.height};
    }

    TileOffset TileGeometryCatalog::Offset(const TableTile &tile) const {
        auto offset = TableTileOffset(tile, width_);
        if (tile.family == 'y' && (tile.wall_extra || tile.wall_stacks == 18)) {
            TableTile last{'y', 16, -1, tile.direction}, previous = last;
            --previous.slot;
            const auto face = Bounds(last, false), before = Bounds(previous, false);
            const double steps = tile.wall_extra - (tile.wall_stacks == 18 ? .5 : 0);
            offset.x += static_cast<int>((face.x - before.x) * steps);
            offset.y += static_cast<int>((face.y - before.y) * steps);
            if (tile.wall_stacks == 18) {
                const int inset = std::max(1, 8 * width_ / 1440);
                constexpr int x[]{0, -1, 0, 1}, y[]{-1, 0, 1, 0};
                offset.x += x[tile.direction] * inset;
                offset.y += y[tile.direction] * inset;
            }
        }
        if (tile.drawn && tile.family == 'n' && (tile.direction & 1) &&
            (std::string_view(tile.role) == "hand" || std::string_view(tile.role) == "winning")) {
            auto neighbour = tile;
            ++neighbour.slot;
            neighbour.drawn = false;
            neighbour.dx = neighbour.dy = 0;
            const auto face = Bounds(tile, true);
            const auto previous = Bounds(neighbour, true);
            const auto adjacent = TableTileOffset(neighbour, width_);
            const int gap = std::max(1, 10 * width_ / 1024);
            offset.y = tile.direction == 1
                           ? previous.y + adjacent.y - gap - face.y - face.height
                           : previous.y + adjacent.y + previous.height + gap - face.y;
        }
        return offset;
    }

    std::pair<int, int> TileGeometryCatalog::Origin(const TableTile &tile, const bool red) const {
        const auto bounds = Bounds(tile, red);
        const auto offset = Offset(tile);
        return {bounds.x + offset.x, bounds.y + offset.y};
    }

    bool TileGeometryCatalog::OpaqueAt(const TableTile &tile, const bool red, const int local_x, const int local_y) {
        const auto key = ImageKey(tile, red);
        const unsigned variant = (key >> 16) & 0xff;
        const bool flower = variant >= 40;
        const auto mask_key = flower ? (key & 0xff00ffffu) | (34u << 16) : key;
        auto it = alpha_cache_.find(mask_key);
        if (it == alpha_cache_.end()) {
            if (flower && (tile.family != 'n' || tile.slot != 0))
                throw std::runtime_error("Unsupported flower placeholder");
            const bool direct = flower || variant == 34 || variant == 37;
            const auto &pack = direct ? *odd_ : *even_;
            const auto entry = pack.key_index.find(mask_key);
            if (entry == pack.key_index.end())
                throw std::runtime_error("Missing table resource key " + std::to_string(key));
            const auto decoded = DecodeObject(pack, entry->second, direct ? even_.get() : odd_.get(), false, nullptr);
            AlphaMask mask;
            mask.width = decoded.image.width;
            mask.height = decoded.image.height;
            const std::size_t pixels = static_cast<std::size_t>(mask.width) * mask.height;
            mask.bits.assign((pixels + 7) / 8, 0);
            for (std::size_t i = 0; i < pixels; ++i) {
                if (decoded.image.bgra[i * 4 + 3] != 0)
                    mask.bits[i >> 3] |= static_cast<std::uint8_t>(1u << (i & 7));
            }
            it = alpha_cache_.emplace(mask_key, std::move(mask)).first;
        }
        const auto &mask = it->second;
        if (local_x < 0 || local_y < 0 || local_x >= mask.width || local_y >= mask.height) return false;
        const std::size_t index = static_cast<std::size_t>(local_y) * mask.width + local_x;
        return (mask.bits[index >> 3] & static_cast<std::uint8_t>(1u << (index & 7))) != 0;
    }

    void SortTableScene(std::vector<PositionedTile> &scene, const TileGeometryCatalog &geometry, const bool red) {
        struct DepthTile {
            PositionedTile placed;
            std::pair<int, int> extent;
        };
        std::vector<DepthTile> ordered;
        ordered.reserve(scene.size());
        for (const auto &placed: scene) {
            auto tile = placed.tile;
            if (tile.family == 'y') tile.tile = -1;
            if (tile.family == 'n' && (tile.slot / 20 == 1 || tile.slot / 20 == 3)) tile.slot %= 20;
            const auto image = geometry.Bounds(tile, red);
            const auto face = geometry.Bounds(placed.tile, red);
            const int placed_x = placed.flight ? placed.flight->landing_x : placed.x;
            const int placed_y = placed.flight ? placed.flight->landing_y : placed.y;
            ordered.push_back({
                placed, {
                    placed_y + image.y - face.y + image.height,
                    -std::abs(2 * (placed_x + image.x - face.x) + image.width - geometry.width())
                }
            });
        }
        std::stable_sort(ordered.begin(), ordered.end(), [](const DepthTile &a, const DepthTile &b) {
            return a.extent < b.extent;
        });
        for (std::size_t i = 0; i < scene.size(); ++i) scene[i] = ordered[i].placed;
        static constexpr int river_order[4][25]{
            {0, 5, 1, 4, 2, 3, 6, 11, 7, 10, 8, 9, 24, 23, 22, 21, 20, 19, 18, 12, 17, 13, 16, 14, 15},
            {24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
            {24, 23, 22, 21, 20, 19, 18, 12, 17, 13, 16, 14, 15, 6, 11, 7, 10, 8, 9, 0, 5, 1, 4, 2, 3},
            {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5}
        };
        for (int seat = 0; seat < 4; ++seat) {
            std::vector<std::size_t> positions;
            std::vector<PositionedTile> river;
            for (std::size_t i = 0; i < scene.size(); ++i)
                if (scene[i].tile.family == 'k' && scene[i].tile.direction == seat) {
                    positions.push_back(i);
                    river.push_back(scene[i]);
                }
            int rank[25]{};
            for (int i = 0; i < 25; ++i) rank[river_order[seat][i]] = i;
            std::stable_sort(river.begin(), river.end(), [&](const PositionedTile &a, const PositionedTile &b) {
                return rank[a.tile.slot % 40] < rank[b.tile.slot % 40];
            });
            for (std::size_t i = 0; i < positions.size(); ++i) scene[positions[i]] = river[i];
        }
    }
} // namespace tenhou
