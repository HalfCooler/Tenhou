#include "table_scene.hpp"
#include "animation_clock.hpp"
#include <algorithm>
#include <stdexcept>

namespace tenhou {
    TileOffset TableTileOffset(const TableTile &tile, int width) {
        // FUN_004902f0 stores literal native-pixel wall offsets from the PE table.
        if (tile.family == 'y') return {tile.dx, tile.dy};
        TileOffset offset{static_cast<int>(tile.dx * width / 1024.0), static_cast<int>(tile.dy * width / 1024.0)};
        if (tile.river_waiting) {
            // FUN_00491fe0 discard: original signed-short placeholder offsets,
            // PE 004e61b0 (x) and 004e61a0 (y), literal pixels at either canvas.
            static constexpr int x[]{15, 5, -15, -5}, y[]{5, -15, -5, 15};
            offset.x += x[tile.direction];
            offset.y += y[tile.direction];
        }
        if (tile.family == 'k' && tile.river_reach_slot >= 0) {
            // FUN_00478be0: literal PE 004fd518 / 004fd724, indexed by
            // declaration slot (not following slot); only the rest of that row.
            const int slot = tile.river_reach_slot, seat = tile.direction;
            if (slot < 24 && slot != 5 && slot != 11) {
                static constexpr int x[2][4]{{13, -1, -11, -1}, {18, -2, -16, -2}};
                offset.x += x[width == 1440][seat];
                if (seat == 1) offset.y += width == 1440 ? (slot >= 18 ? -11 : -12) : -9;
                if (seat == 3) offset.y += width == 1440 ? (slot >= 18 ? 13 : 12) : (slot >= 18 ? 10 : 9);
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
        for (int slot = 0; slot < std::min(16, tile.slot % 20); ++slot)
            if (tile.meld_sideways & (1u << slot)) {
                offset.x += shifts[width == 1440][tile.direction][0][slot];
                offset.y += shifts[width == 1440][tile.direction][1][slot];
            }
        return offset;
    }

    int TileVariant(int tile, bool red) {
        if (tile < 0 || tile > 135) return 37;
        if (red && (tile == 16 || tile == 52 || tile == 88)) return tile == 16 ? 0 : tile == 52 ? 10 : 20;
        const int kind = tile / 4;
        return kind < 27 ? kind / 9 * 10 + kind % 9 + 1 : 30 + kind % 9;
    }

    std::vector<TableTile> BuildTableScene(const ReplayState &state, bool sort_self, bool sort_opponents) {
        std::vector<TableTile> result;
        if (!state.initialized) return result;
        for (int seat = 0; seat < 4; ++seat) {
            if (!SeatPresent(state.seat_mask, seat)) continue;
            const auto seat_begin = result.size();
            const auto &player = state.players[seat];
            auto hand = player.hand;
            if (player.drawn_tile >= 0) hand.
                    erase(std::remove(hand.begin(), hand.end(), player.drawn_tile), hand.end());
            if (seat ? sort_opponents : sort_self) std::sort(hand.begin(), hand.end());
            if (player.drawn_tile >= 0) hand.push_back(player.drawn_tile);
            if (hand.size() > 14) throw std::runtime_error("Table hand exceeds fourteen tiles");
            for (std::size_t i = 0; i < hand.size(); ++i) {
                const bool drawn = hand[i] >= 0 && hand[i] == player.drawn_tile;
                // FUN_00486e80: revealed opponent t/i/d becomes n/(17-i)/d.
                TableTile command{
                    seat ? 'n' : 't', seat ? 17 - static_cast<int>(i) : static_cast<int>(i), hand[i], seat
                };
                // Unknown online opponents use original standing t/slot/back resources.
                if (hand[i] < 0) {
                    command.family = 't';
                    command.slot = static_cast<int>(i);
                }
                if (state.ended && state.result.rfind("RYUUKYOKU", 0) == 0 && i < state.draw_end_slots && !state.
                    settlement.receipts.empty()) {
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
                if (drawn) {
                    constexpr int x[]{12, 0, -12, 0}, y[]{0, 10, 0, -10};
                    command.dx = x[seat];
                    command.dy = y[seat];
                }
                result.push_back(command);
            }
            int index = 0, row = -1, reach_slot = -1;
            for (const auto &discard: player.discards) {
                if (discard.claimed) continue; // Claimed tiles live in the meld only.
                // k slots 0..24, sideways k+40. Extend the final row explicitly
                // for unusually long inspection inputs; this extension is new UI.
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
                command.river_waiting = !state.ended && state.last_discard_seat == seat && &discard == &player.discards.
                                        back();
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
            int slot = (state.type & 16) ? 1 : 0;
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
                    const auto decoded = DecodeReplayMeld(meld.encoded, seat);
                    added_tile = decoded.called_tile;
                    tiles.erase(std::remove(tiles.begin(), tiles.end(), added_tile), tiles.end());
                }
                tiles.erase(std::remove(tiles.begin(), tiles.end(), meld.called_tile), tiles.end());
                std::sort(tiles.rbegin(), tiles.rend());
                if (closed) {
                    const auto red = std::find_if(tiles.begin(), tiles.end(), [](int tile) {
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

    void TableArtwork::Load(const std::filesystem::path &directory, int width) {
        if (width != 1024 && width != 1440) throw std::runtime_error("Unsupported table canvas");
        const int number = width == 1024 ? 0 : 2;
        auto even = LoadObjectPack(directory / ("_obj" + std::to_string(number) + ".dat"));
        auto odd = LoadObjectPack(directory / ("_obj" + std::to_string(number + 1) + ".dat"));
        if (even.signature != "obj" + std::to_string(number) || odd.signature != "obj" + std::to_string(number + 1))
            throw std::runtime_error("Wrong table object pack resolution");
        even_ = std::move(even);
        odd_ = std::move(odd);
        width_ = width;
        cache_.clear();
        cached_bytes_ = 0;
        frame_ = 1;
        evictions_ = 0;
    }

    void TableArtwork::BeginFrame() {
        while (cached_bytes_ > cache_budget_) {
            auto victim = cache_.end();
            for (auto it = cache_.begin(); it != cache_.end(); ++it)
                if (it->second.used != frame_ && (victim == cache_.end() || it->second.used < victim->second.used))
                    victim = it;
            if (victim == cache_.end()) break; // Pin every resource used by the visible frame.
            cached_bytes_ -= victim->second.image.image.bgra.size();
            cache_.erase(victim);
            ++evictions_;
        }
        ++frame_;
    }

    const ObjectImage &TableArtwork::Get(const TableTile &tile, bool red) {
        if (!width_) throw std::runtime_error("Table artwork is not loaded");
        if (tile.slot < 0 || tile.slot > 255 || tile.direction < 0 || tile.direction > 3)
            throw std::runtime_error("Invalid table resource command");
        const bool aka = red && std::find(red_tiles_.begin(), red_tiles_.end(), tile.tile) != red_tiles_.end();
        const unsigned variant = static_cast<unsigned>(aka ? tile.tile / 36 * 10 : TileVariant(tile.tile, false));
        const std::uint32_t key = static_cast<unsigned char>(tile.family) | (unsigned(tile.slot) << 8) |
                                  (variant << 16) | (unsigned(tile.direction) << 24);
        const auto found = cache_.find(key);
        if (found != cache_.end()) {
            found->second.used = frame_;
            return found->second.image;
        }
        const bool direct = variant == 34 || variant == 37;
        const auto &pack = direct ? odd_ : even_;
        const auto entry = pack.key_index.find(key);
        if (entry == pack.key_index.end()) throw
                std::runtime_error("Missing table resource key " + std::to_string(key));
        // The existing decoder applies the evidence-backed startup tint and XOR.

        auto decoded = DecodeObject(pack, entry->second, direct ? &even_ : &odd_, true, &rgb_);
        cached_bytes_ += decoded.image.bgra.size();
        return cache_.emplace(key, Cached{std::move(decoded), frame_}).first->second.image;
    }

    TileOffset TableArtwork::Offset(const TableTile &tile) {
        auto offset = TableTileOffset(tile, width_);
        if (tile.drawn && tile.family == 'n' && (tile.direction & 1) && std::string(tile.role) == "hand") {
            // Face-up n rows run opposite to the standing t row. A standing-row
            // draw shift moves these faces into their neighbour. Keep the original
            // n sprite/anchor, then separate the complete native face rectangles.
            // This spacing is a local open-hand display adaptation.
            auto neighbour = tile;
            ++neighbour.slot;
            neighbour.drawn = false;
            neighbour.dx = neighbour.dy = 0;
            const auto &face = Get(tile);
            const auto &previous = Get(neighbour);
            const auto adjacent = TableTileOffset(neighbour, width_);
            const int gap = std::max(1, 10 * width_ / 1024);
            offset.y = tile.direction == 1
                           ? previous.y + adjacent.y - gap - face.y - face.image.height
                           : previous.y + adjacent.y + previous.image.height + gap - face.y;
        }
        return offset;
    }

    void SortTableScene(std::vector<PositionedTile> &scene, TableArtwork &artwork, bool red) {
        struct DepthTile {
            PositionedTile placed;
            std::pair<int, int> extent;
        };
        std::vector<DepthTile> ordered;
        ordered.reserve(scene.size());
        for (const auto &placed: scene) {
            auto tile = placed.tile;
            // A face-up wall resource contains only the upper tile. Use the
            // full back stack's footprint to order it against adjacent stacks.
            if (tile.family == 'y') tile.tile = -1;
            const auto &image = artwork.Get(tile, red);
            const int x = image.x, y = image.y, height = image.image.height, width = image.image.width;
            const auto &face = artwork.Get(placed.tile, red);
            ordered.push_back({
                placed, {
                    placed.y + y - face.y + height,
                    -std::abs(2 * (placed.x + x - face.x) + width - artwork.width())
                }
            });
        }
        std::stable_sort(ordered.begin(), ordered.end(), [](const DepthTile &a, const DepthTile &b) {
            return a.extent < b.extent;
        });
        for (std::size_t i = 0; i < scene.size(); ++i) scene[i] = ordered[i].placed;
        // FUN_00476fe0's k placeholder order is independent of the current face
        // rectangle. Sorting a sideways face by its bottom puts it over the next
        // row. Preserve other layers' positions and restore each native river order.
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
