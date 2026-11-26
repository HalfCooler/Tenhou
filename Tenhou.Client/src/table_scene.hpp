#pragma once
#include "tile_geometry.hpp"
#include "variant_artwork.hpp"
#include <list>
#include <map>
#include <memory>
#include <tuple>
#include <unordered_map>

namespace tenhou {
    class TableArtwork {
    public:
        void SetRedTiles(const std::vector<int> &tiles) { geometry_.SetRedTiles(tiles); }

        void SetColor(const std::array<float, 3> &rgb) {
            if (rgb_ != rgb) {
                rgb_ = rgb;
                cache_.clear();
                lru_.clear();
                flight_cache_.clear();
                cached_bytes_ = 0;
            }
        }

        void Load(const std::filesystem::path &directory, int width);

        const ObjectImage &Get(const TableTile &tile, bool red = true);
        const ObjectImage &GetFlight(const TileFlightImage &flight, bool red);

        TileOffset Offset(const TableTile &tile) { return geometry_.Offset(tile); }

        TileGeometryCatalog &geometry() { return geometry_; }
        const TileGeometryCatalog &geometry() const { return geometry_; }

        // Only evict at the next frame boundary, never while Get references live.
        void BeginFrame();

        std::size_t cached_bytes() const { return cached_bytes_; }
        void SetCacheBudget(std::size_t bytes) { cache_budget_ = bytes; custom_budget_ = true; }
        std::size_t evictions() const { return evictions_; }
        int width() const { return geometry_.width(); }

    private:
        std::uint32_t ImageKey(const TableTile &tile, bool red) const { return geometry_.ImageKey(tile, red); }
        std::array<float, 3> rgb_{0, 0, 38.0f / 255.0f};
        std::shared_ptr<const ObjectPack> even_, odd_;
        TileGeometryCatalog geometry_;
        VariantArtwork variants_;

        struct Cached {
            std::uint32_t key = 0;
            ObjectImage image;
            std::uint64_t used = 0;
        };

        std::list<Cached> lru_;
        std::unordered_map<std::uint32_t, std::list<Cached>::iterator> cache_;
        using FlightKey = std::tuple<std::uint32_t, unsigned, float, std::array<int, 8>>;
        std::map<FlightKey, ObjectImage> flight_cache_;
        std::uint64_t frame_ = 1;
        std::size_t cached_bytes_ = 0;
        std::size_t cache_budget_ = 32 * 1024 * 1024, evictions_ = 0;
        bool custom_budget_ = false;
    };

    inline void SortTableScene(std::vector<PositionedTile> &scene, TableArtwork &artwork, bool red = true) {
        SortTableScene(scene, artwork.geometry(), red);
    }
} // namespace tenhou
