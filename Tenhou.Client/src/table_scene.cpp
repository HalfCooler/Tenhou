#include "table_scene.hpp"
#include "resource_manager.hpp"
#include "animation_clock.hpp"
#include <algorithm>
#include <stdexcept>
#include <objidl.h>
#include <gdiplus.h>

namespace tenhou {
    TileOffset TableTileOffset(const TableTile &tile, int width) {
        // FUN_004902f0 stores literal native-pixel wall offsets from the PE table.
        if (tile.family == 'y') return {tile.dx, tile.dy};
        TileOffset offset{static_cast<int>(tile.dx * width / 1024.0), static_cast<int>(tile.dy * width / 1024.0)};
        if (tile.native_draw_offset) {
            constexpr int x[]{4, -1, -3, -1}, y[]{0, -3, 0, 3}; // PE 004e6098 / 004e60a8.
            offset.x += x[tile.direction];
            offset.y += y[tile.direction];
        }
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
        const auto key = [&](int tile) {
            return tile >= 0 && tile < 108
                       ? (tile % 36) + suit_order[static_cast<std::size_t>(tile / 36)] * 36
                       : tile;
        };
        std::sort(tiles.begin(), tiles.end(), [&](int a, int b) {
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
            if (player.drawn_tile >= 0) hand.
                    erase(std::remove(hand.begin(), hand.end(), player.drawn_tile), hand.end());
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
                    command.family = 'n'; command.slot = 17 - static_cast<int>(i);
                    command.role = winning ? "winning" : "retired";
                }
                if (state.ended && !player.retired && state.result.rfind("RYUUKYOKU", 0) == 0 && i < state.draw_end_slots && !state.
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
                result.push_back(marker); ++index;
            }
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
        auto even = LoadObjectPack(ResourceManager(directory).Path(width == 1024 ? AssetId::TablePackSd : AssetId::TablePack));
        auto odd = LoadObjectPack(ResourceManager(directory).Path(width == 1024 ? AssetId::TableCompanionPackSd : AssetId::TableCompanionPack));
        if (even.signature != "obj" + std::to_string(number) || odd.signature != "obj" + std::to_string(number + 1))
            throw std::runtime_error("Wrong table object pack resolution");
        even_ = std::move(even);
        odd_ = std::move(odd);
        width_ = width;
        variants_.Configure(directory);
        cache_.clear();
        flight_cache_.clear();
        cached_bytes_ = 0;
        frame_ = 1;
        evictions_ = 0;
    }

    void TableArtwork::BeginFrame() {
        if (flight_cache_.size() > 192) flight_cache_.clear();
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

    std::uint32_t TableArtwork::ImageKey(const TableTile &tile, bool red) const {
        if (!width_) throw std::runtime_error("Table artwork is not loaded");
        if (tile.slot < 0 || tile.slot > 255 || tile.direction < 0 || tile.direction > 3)
            throw std::runtime_error("Invalid table resource command");
        const bool aka = std::string(tile.role) == "missing_suit" || (red && std::find(red_tiles_.begin(), red_tiles_.end(), tile.tile) != red_tiles_.end());
        const bool flower = tile.tile >= 136 && tile.tile < 144;
        const unsigned variant = static_cast<unsigned>(flower ? 40 + tile.tile - 136 : aka ? tile.tile / 36 * 10 : TileVariant(tile.tile, false));
        return static_cast<unsigned char>(tile.family) | (unsigned(tile.slot) << 8) |
               (variant << 16) | (unsigned(tile.direction) << 24);
    }

    const ObjectImage &TableArtwork::Get(const TableTile &tile, bool red) {
        const auto key = ImageKey(tile, red);
        const unsigned variant = (key >> 16) & 0xff;
        const bool flower = variant >= 40;
        const auto found = cache_.find(key);
        if (found != cache_.end()) {
            found->second.used = frame_;
            return found->second.image;
        }
        const bool direct = flower || variant == 34 || variant == 37;
        const auto &pack = direct ? odd_ : even_;
        const auto entry = pack.key_index.find(flower ? (key & 0xff00ffffu) | (34u << 16) : key);
        if (entry == pack.key_index.end()) throw
                std::runtime_error("Missing table resource key " + std::to_string(key));
        // The existing decoder applies the evidence-backed startup tint and XOR.

        auto decoded = DecodeObject(pack, entry->second, direct ? &even_ : &odd_, true, &rgb_);
        if (flower) {
            // Flowers are public n/0 counters, not concealed-hand entities. Use the
            // original blank face and its measured top-plane parallelogram.
            if (tile.family != 'n' || tile.slot != 0) throw std::runtime_error("Unsupported flower placeholder");
            const auto *pattern = variants_.Get(FlowerArtwork(tile.tile));
            if (!pattern) throw std::runtime_error("Missing flower pattern");
            Gdiplus::Bitmap surface(decoded.image.width, decoded.image.height, decoded.image.width * 4,
                PixelFormat32bppARGB, decoded.image.bgra.data());
            Gdiplus::Bitmap source(pattern->width, pattern->height, pattern->width * 4,
                PixelFormat32bppARGB, const_cast<BYTE *>(pattern->bgra.data()));
            const Gdiplus::RotateFlipType rotations[]{Gdiplus::RotateNoneFlipNone, Gdiplus::Rotate270FlipNone,
                Gdiplus::Rotate180FlipNone, Gdiplus::Rotate90FlipNone};
            source.RotateFlip(rotations[tile.direction]);
            // Inset corners within the native white face, measured from obj3 n/0/34.
            constexpr float planes[4][6]{{.22f,.09f,.76f,.09f,.34f,.62f}, {.17f,.09f,.83f,.09f,.25f,.45f},
                {.23f,.08f,.85f,.08f,.12f,.51f}, {.20f,.08f,.82f,.08f,.10f,.56f}};
            Gdiplus::PointF points[3];
            for (int i = 0; i < 3; ++i) points[i] = {planes[tile.direction][i * 2] * decoded.image.width,
                planes[tile.direction][i * 2 + 1] * decoded.image.height};
            Gdiplus::Graphics graphics(&surface);
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            if (graphics.DrawImage(&source, points, 3) != Gdiplus::Ok) throw std::runtime_error("Cannot compose flower face");
        }
        cached_bytes_ += decoded.image.bgra.size();
        return cache_.emplace(key, Cached{std::move(decoded), frame_}).first->second.image;
    }

    const ObjectImage &TableArtwork::GetFlight(const TileFlightImage &flight, bool red) {
        std::array<int, 8> offsets{};
        int left = 0, top = 0, right = 0, bottom = 0;
        for (unsigned i = 0; i < flight.count; ++i) {
            const auto &offset = flight.offsets[i];
            offsets[i * 2] = offset.x;
            offsets[i * 2 + 1] = offset.y;
            left = std::min(left, offset.x);
            top = std::min(top, offset.y);
            right = std::max(right, offset.x);
            bottom = std::max(bottom, offset.y);
        }
        const FlightKey key{ImageKey(flight.sprite, red), flight.count, flight.opacity, offsets};
        const auto cached = flight_cache_.find(key);
        if (cached != flight_cache_.end()) return cached->second;

        const auto &source = Get(flight.sprite, red).image;
        const int width = source.width + right - left, height = source.height + bottom - top;
        ObjectImage combined{left, top, {width, height, Bytes(std::size_t(width) * height * 4)}};
        Gdiplus::Bitmap surface(width, height, width * 4, PixelFormat32bppARGB, combined.image.bgra.data());
        Gdiplus::Bitmap sprite(source.width, source.height, source.width * 4, PixelFormat32bppARGB,
                                const_cast<BYTE *>(source.bgra.data()));
        Gdiplus::Graphics graphics(&surface);
        Gdiplus::ColorMatrix matrix{{{1,0,0,0,0}, {0,1,0,0,0}, {0,0,1,0,0},
                                     {0,0,0,flight.opacity,0}, {0,0,0,0,1}}};
        Gdiplus::ImageAttributes attributes;
        attributes.SetColorMatrix(&matrix);
        for (unsigned i = 0; i < flight.count; ++i) {
            const auto &offset = flight.offsets[i];
            const Gdiplus::Rect target(offset.x - left, offset.y - top, source.width, source.height);
            if (graphics.DrawImage(&sprite, target, 0, 0, source.width, source.height, Gdiplus::UnitPixel,
                                   &attributes) != Gdiplus::Ok) throw std::runtime_error("Cannot compose tile flight");
        }
        graphics.Flush(Gdiplus::FlushIntentionSync);
        return flight_cache_.emplace(key, std::move(combined)).first->second;
    }

    TileOffset TableArtwork::Offset(const TableTile &tile) {
        auto offset = TableTileOffset(tile, width_);
        if (tile.family == 'y' && (tile.wall_extra || tile.wall_stacks == 18)) {
            // Use one stable step for both layers and every animation origin.
            TableTile last{'y', 16, -1, tile.direction}, previous = last; --previous.slot;
            const auto &face = Get(last, false), &before = Get(previous, false);
            const double steps = tile.wall_extra - (tile.wall_stacks == 18 ? .5 : 0);
            offset.x += static_cast<int>((face.x - before.x) * steps);
            offset.y += static_cast<int>((face.y - before.y) * steps);
            if (tile.wall_stacks == 18) {
                // Keep the longer row clear of the outer meld/hand edge.
                const int inset = std::max(1, 8 * width_ / 1440);
                constexpr int x[]{0, -1, 0, 1}, y[]{-1, 0, 1, 0};
                offset.x += x[tile.direction] * inset;
                offset.y += y[tile.direction] * inset;
            }
        }
        if (tile.drawn && tile.family == 'n' && (tile.direction & 1) &&
            (std::string(tile.role) == "hand" || std::string(tile.role) == "winning")) {
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
            // Sideways meld faces have a shorter image, but occupy the same
            // tabletop depth as an upright tile in that placeholder.
            if (tile.family == 'n' && (tile.slot / 20 == 1 || tile.slot / 20 == 3)) tile.slot %= 20;
            const auto &image = artwork.Get(tile, red);
            const int x = image.x, y = image.y, height = image.image.height, width = image.image.width;
            const auto &face = artwork.Get(placed.tile, red);
            const int placed_x = placed.flight ? placed.flight->landing_x : placed.x;
            const int placed_y = placed.flight ? placed.flight->landing_y : placed.y;
            ordered.push_back({
                placed, {
                    placed_y + y - face.y + height,
                    -std::abs(2 * (placed_x + x - face.x) + width - artwork.width())
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
