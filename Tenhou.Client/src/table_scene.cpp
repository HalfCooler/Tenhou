#include "table_scene.hpp"
#include "resource_manager.hpp"
#include "shared_assets.hpp"
#include "window_layout.hpp"
#include <algorithm>
#include <stdexcept>
#include <objidl.h>
#include <gdiplus.h>

namespace tenhou {
    void TableArtwork::Load(const std::filesystem::path &directory, const int width) {
        if (!IsSupportedTableWidth(width)) throw std::runtime_error("Unsupported table canvas");
        const AssetId even_id = width == 1024 ? AssetId::TablePackSd :
                                width == 1440 ? AssetId::TablePack :
                                width == 1920 ? AssetId::TablePack1920 : AssetId::TablePack2640;
        const AssetId odd_id = width == 1024 ? AssetId::TableCompanionPackSd :
                               width == 1440 ? AssetId::TableCompanionPack :
                               width == 1920 ? AssetId::TableCompanionPack1920 : AssetId::TableCompanionPack2640;
        const std::string expected_even = width == 1440 ? "obj2" : "obj0";
        const std::string expected_odd = width == 1440 ? "obj3" : "obj1";
        ResourceManager manager(directory);
        auto even = SharedAssets().GetObjectPack(manager.Path(even_id));
        auto odd = SharedAssets().GetObjectPack(manager.Path(odd_id));
        if (even->signature != expected_even || odd->signature != expected_odd)
            throw std::runtime_error("Wrong table object pack resolution");
        even_ = std::move(even);
        odd_ = std::move(odd);
        geometry_.LoadPacks(even_, odd_, width);
        variants_.Configure(directory);
        cache_.clear();
        lru_.clear();
        flight_cache_.clear();
        cached_bytes_ = 0;
        if (!custom_budget_) {
            cache_budget_ = width >= 2640 ? 128u * 1024 * 1024 :
                            width >= 1920 ? 64u * 1024 * 1024 : 32u * 1024 * 1024;
        }
        frame_ = 1;
        evictions_ = 0;
    }

    void TableArtwork::BeginFrame() {
        if (flight_cache_.size() > 192) flight_cache_.clear();
        while (cached_bytes_ > cache_budget_ && !lru_.empty()) {
            const auto &victim = lru_.back();
            if (victim.used == frame_) break; // Pin every resource used by the visible frame.
            cached_bytes_ -= victim.image.image.bgra.size();
            cache_.erase(victim.key);
            lru_.pop_back();
            ++evictions_;
        }
        ++frame_;
    }

    const ObjectImage &TableArtwork::Get(const TableTile &tile, const bool red) {
        const auto key = ImageKey(tile, red);
        const unsigned variant = (key >> 16) & 0xff;
        const bool flower = variant >= 40;
        const auto found = cache_.find(key);
        if (found != cache_.end()) {
            found->second->used = frame_;
            lru_.splice(lru_.begin(), lru_, found->second);
            return found->second->image;
        }
        const bool direct = flower || variant == 34 || variant == 37;
        const auto &pack = direct ? *odd_ : *even_;
        const auto entry = pack.key_index.find(flower ? (key & 0xff00ffffu) | (34u << 16) : key);
        if (entry == pack.key_index.end())
            throw std::runtime_error("Missing table resource key " + std::to_string(key));

        auto decoded = DecodeObject(pack, entry->second, direct ? even_.get() : odd_.get(), true, &rgb_);
        if (flower) {
            if (tile.family != 'n' || tile.slot != 0) throw std::runtime_error("Unsupported flower placeholder");
            const auto *pattern = variants_.Get(FlowerArtwork(tile.tile));
            if (!pattern) throw std::runtime_error("Missing flower pattern");
            Gdiplus::Bitmap surface(decoded.image.width, decoded.image.height, decoded.image.width * 4,
                                    PixelFormat32bppARGB, decoded.image.bgra.data());
            Gdiplus::Bitmap source(pattern->width, pattern->height, pattern->width * 4,
                                   PixelFormat32bppARGB, const_cast<BYTE *>(pattern->bgra.data()));
            const Gdiplus::RotateFlipType rotations[]{
                Gdiplus::RotateNoneFlipNone, Gdiplus::Rotate270FlipNone,
                Gdiplus::Rotate180FlipNone, Gdiplus::Rotate90FlipNone
            };
            source.RotateFlip(rotations[tile.direction]);
            constexpr float planes[4][6]{
                {.22f, .09f, .76f, .09f, .34f, .62f},
                {.17f, .09f, .83f, .09f, .25f, .45f},
                {.23f, .08f, .85f, .08f, .12f, .51f},
                {.20f, .08f, .82f, .08f, .10f, .56f}
            };
            Gdiplus::PointF points[3];
            for (int i = 0; i < 3; ++i)
                points[i] = {
                    planes[tile.direction][i * 2] * decoded.image.width,
                    planes[tile.direction][i * 2 + 1] * decoded.image.height
                };
            Gdiplus::Graphics graphics(&surface);
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            if (graphics.DrawImage(&source, points, 3) != Gdiplus::Ok)
                throw std::runtime_error("Cannot compose flower face");
        }
        cached_bytes_ += decoded.image.bgra.size();
        lru_.push_front(Cached{key, std::move(decoded), frame_});
        cache_[key] = lru_.begin();
        return lru_.front().image;
    }

    const ObjectImage &TableArtwork::GetFlight(const TileFlightImage &flight, const bool red) {
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
        ObjectImage combined{left, top, {width, height, Bytes(static_cast<std::size_t>(width) * height * 4)}};
        Gdiplus::Bitmap surface(width, height, width * 4, PixelFormat32bppARGB, combined.image.bgra.data());
        Gdiplus::Bitmap sprite(source.width, source.height, source.width * 4, PixelFormat32bppARGB,
                               const_cast<BYTE *>(source.bgra.data()));
        Gdiplus::Graphics graphics(&surface);
        Gdiplus::ColorMatrix matrix{{{1, 0, 0, 0, 0}, {0, 1, 0, 0, 0}, {0, 0, 1, 0, 0},
                                     {0, 0, 0, flight.opacity, 0}, {0, 0, 0, 0, 1}}};
        Gdiplus::ImageAttributes attributes;
        attributes.SetColorMatrix(&matrix);
        for (unsigned i = 0; i < flight.count; ++i) {
            const auto &offset = flight.offsets[i];
            const Gdiplus::Rect target(offset.x - left, offset.y - top, source.width, source.height);
            if (graphics.DrawImage(&sprite, target, 0, 0, source.width, source.height, Gdiplus::UnitPixel,
                                   &attributes) != Gdiplus::Ok)
                throw std::runtime_error("Cannot compose tile flight");
        }
        graphics.Flush(Gdiplus::FlushIntentionSync);
        return flight_cache_.emplace(key, std::move(combined)).first->second;
    }
} // namespace tenhou
