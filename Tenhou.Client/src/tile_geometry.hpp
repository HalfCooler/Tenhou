#pragma once
#include "object_pack.hpp"
#include "replay.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace tenhou {
    // Native resource commands; pure scene descriptors independent of GDI/GDI+.
    struct TableTile {
        char family = 't';
        int slot = 0, tile = 0, direction = 0;
        int dx = 0, dy = 0; // Translation in the selected native canvas.
        bool drawn = false, riichi = false;
        const char *role = "hand";
        unsigned meld_sideways = 0; // Native n slots that widen the following slots.
        int river_reach_slot = -1; // Declaration preceding this tile in the same row.
        bool river_waiting = false;
        int wall_extra = 0; // Additional stacks translated from measured native resource anchors.
        int wall_stack = -1, wall_stacks = 0; // Logical identity, independent of the reused sprite slot.
        bool native_draw_offset = false;
    };

    struct TileOffset {
        int x = 0, y = 0;
        bool operator==(const TileOffset &other) const { return x == other.x && y == other.y; }
    };

    struct TileFlightImage {
        TableTile sprite;
        std::array<TileOffset, 4> offsets{};
        unsigned count = 0;
        float opacity = 1.f;
        int landing_x = 0, landing_y = 0;
        bool cover_landing = false;

        bool operator==(const TileFlightImage &other) const {
            return sprite.family == other.sprite.family && sprite.slot == other.sprite.slot &&
                   sprite.tile == other.sprite.tile && sprite.direction == other.sprite.direction &&
                   offsets == other.offsets && count == other.count && opacity == other.opacity &&
                   landing_x == other.landing_x && landing_y == other.landing_y && cover_landing == other.cover_landing;
        }
    };

    struct PositionedTile {
        TableTile tile;
        int x = 0, y = 0;
        std::optional<TileFlightImage> flight{};
    };

    struct TileSpriteBounds {
        int x = 0, y = 0, width = 0, height = 0;
    };

    struct CanvasRect {
        int left = 0, top = 0, right = 0, bottom = 0;
    };

    CanvasRect FitNativeCanvasBounds(CanvasRect bounds, int width);

    int TileVariant(int tile, bool red = true);

    // Origin tenhou.exe.c:90593-90661 (FUN_00488220 "od eat 0") & 109604-109653 (FUN_004a3500):
    // Move absent suits behind present suits when fewer than three suits are held;
    // keep suit order unchanged when all three suits are present.
    void UpdateOriginSuitOrder(std::array<int, 3> &suit_order, const std::vector<int> &tiles);
    void SortOriginHand(std::vector<int> &tiles, const std::array<int, 3> &suit_order);

    TileOffset TableTileOffset(const TableTile &tile, int width);

    std::vector<TableTile> BuildTableScene(const ReplayState &state, bool sort_self = true, bool sort_opponents = true);

    // Pure C++ tile geometry and 1-bit hit-mask catalog (zero HWND, HDC, or GDI+ dependencies).
    class TileGeometryCatalog {
    public:
        void SetRedTiles(const std::vector<int> &tiles) { red_tiles_ = tiles; }
        const std::vector<int> &red_tiles() const { return red_tiles_; }

        void Load(const std::filesystem::path &directory, int width);
        void LoadPacks(std::shared_ptr<const ObjectPack> even, std::shared_ptr<const ObjectPack> odd, int width);

        int width() const { return width_; }
        bool loaded() const { return width_ != 0 && even_ && odd_; }

        std::uint32_t ImageKey(const TableTile &tile, bool red = true) const;
        TileSpriteBounds Bounds(const TableTile &tile, bool red = true) const;
        TileOffset Offset(const TableTile &tile) const;
        std::pair<int, int> Origin(const TableTile &tile, bool red = true) const;

        // Exact alpha != 0 hit test matching decoded ObjectPack sprites.
        bool OpaqueAt(const TableTile &tile, bool red, int local_x, int local_y);

    private:
        struct AlphaMask {
            int width = 0, height = 0;
            std::vector<std::uint8_t> bits;
        };

        std::vector<int> red_tiles_{16, 52, 88};
        std::shared_ptr<const ObjectPack> even_, odd_;
        int width_ = 0;
        std::map<std::uint32_t, AlphaMask> alpha_cache_;
    };

    // Native perspective rows overlap toward the canvas centre. Equal-depth
    // sprites must be painted from the outside inward, on both sides of the row.
    void SortTableScene(std::vector<PositionedTile> &scene, const TileGeometryCatalog &geometry, bool red = true);
} // namespace tenhou
