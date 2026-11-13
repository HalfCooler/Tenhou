#pragma once
#include "object_pack.hpp"
#include "replay.hpp"
#include <map>

namespace tenhou {
    // Native resource commands; the toolbar and score captions are a rebuild UI.
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
    };

    struct PositionedTile {
        TableTile tile;
        int x = 0, y = 0;
    };

    int TileVariant(int tile, bool red = true);

    struct TileOffset {
        int x = 0, y = 0;
    };

    TileOffset TableTileOffset(const TableTile &tile, int width);

    std::vector<TableTile> BuildTableScene(const ReplayState &state, bool sort_self = true, bool sort_opponents = true);

    class TableArtwork {
    public:
        void SetRedTiles(const std::vector<int> &tiles) { red_tiles_ = tiles; }

        void SetColor(const std::array<float, 3> &rgb) {
            if (rgb_ != rgb) {
                rgb_ = rgb;
                cache_.clear();
                cached_bytes_ = 0;
            }
        }

        void Load(const std::filesystem::path &directory, int width);

        const ObjectImage &Get(const TableTile &tile, bool red = true);

        TileOffset Offset(const TableTile &tile);

        // Only evict at the next frame boundary, never while Get references live.
        void BeginFrame();

        std::size_t cached_bytes() const { return cached_bytes_; }
        void SetCacheBudget(std::size_t bytes) { cache_budget_ = bytes; }
        std::size_t evictions() const { return evictions_; }
        int width() const { return width_; }

    private:
        std::vector<int> red_tiles_{16, 52, 88};
        std::array<float, 3> rgb_{0, 0, 38.0f / 255.0f};
        ObjectPack even_, odd_;
        int width_ = 0;

        struct Cached {
            ObjectImage image;
            std::uint64_t used = 0;
        };

        std::map<std::uint32_t, Cached> cache_;
        std::uint64_t frame_ = 1;
        std::size_t cached_bytes_ = 0;
        std::size_t cache_budget_ = 32 * 1024 * 1024, evictions_ = 0;
    };

    // Native perspective rows overlap toward the canvas centre. Equal-depth
    // sprites must be painted from the outside inward, on both sides of the row.
    void SortTableScene(std::vector<PositionedTile> &scene, TableArtwork &artwork, bool red = true);
} // namespace tenhou
