#include "variant_artwork.hpp"
#include "sprite_assets.hpp"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <cstring>
#include <stdexcept>

namespace tenhou {
    const Image *VariantArtwork::GetSprite(unsigned id) {
        for (const auto &sprite: kSpriteAssets) if (sprite.id == id) return Get(sprite.asset);
        return nullptr;
    }

    AssetId FlowerArtwork(int entity) {
        static constexpr AssetId ids[]{AssetId::FlowerMei, AssetId::FlowerLan, AssetId::FlowerZhu, AssetId::FlowerJu,
            AssetId::FlowerChun, AssetId::FlowerXia, AssetId::FlowerQiu, AssetId::FlowerDong};
        if (entity < 136 || entity > 143) throw std::runtime_error("Invalid flower entity");
        return ids[entity - 136];
    }

    const Image *VariantArtwork::Get(AssetId id) {
        if (const auto found = images_.find(id); found != images_.end())
            return found->second ? &*found->second : nullptr;
        const auto path = ResourceManager(directory_).Path(id);
        if (!std::filesystem::is_regular_file(path)) {
            if (ResourceManager::Info(id).required) throw std::runtime_error("Missing required variant artwork");
            images_.emplace(id, std::nullopt); return nullptr;
        }
        Gdiplus::Bitmap bitmap(path.c_str());
        const auto w = bitmap.GetWidth(), h = bitmap.GetHeight();
        if (bitmap.GetLastStatus() != Gdiplus::Ok || !w || !h || w > 4096 || h > 4096)
            throw std::runtime_error("Invalid variant PNG: " + path.u8string());
        Image image; image.width = static_cast<int>(w); image.height = static_cast<int>(h);
        image.bgra.resize(std::size_t(w) * h * 4);
        Gdiplus::Rect rect(0, 0, image.width, image.height);
        Gdiplus::BitmapData data{};
        if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) != Gdiplus::Ok)
            throw std::runtime_error("Cannot decode variant PNG");
        for (unsigned y = 0; y < h; ++y)
            std::memcpy(image.bgra.data() + std::size_t(y) * w * 4,
                static_cast<const BYTE *>(data.Scan0) + std::ptrdiff_t(y) * data.Stride, std::size_t(w) * 4);
        bitmap.UnlockBits(&data);
        return &*images_.emplace(id, std::move(image)).first->second;
    }
}
