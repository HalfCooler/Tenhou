#include "table_scene.hpp"
#include "resource_manager.hpp"
#include <algorithm>
#include <stdexcept>
#include <objidl.h>
#include <gdiplus.h>

namespace tenhou {
    void TableArtwork::Load(const std::filesystem::path &directory, int width) {
        if (width != 1024 && width != 1440) throw std::runtime_error("Unsupported table canvas");
        const int number = width == 1024 ? 0 : 2;
        auto even = std::make_shared<ObjectPack>(
            LoadObjectPack(ResourceManager(directory).Path(width == 1024 ? AssetId::TablePackSd : AssetId::TablePack)));
        auto odd = std::make_shared<ObjectPack>(
            LoadObjectPack(ResourceManager(directory).Path(width == 1024 ? AssetId::TableCompanionPackSd : AssetId::TableCompanionPack)));
        if (even->signature != "obj" + std::to_string(number) || odd->signature != "obj" + std::to_string(number + 1))
            throw std::runtime_error("Wrong table object pack resolution");
        even_ = std::move(even);
        odd_ = std::move(odd);
        geometry_.LoadPacks(even_, odd_, width);
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
