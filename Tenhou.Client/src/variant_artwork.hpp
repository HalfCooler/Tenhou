#pragma once
#include "resource_manager.hpp"
#include <map>
#include <optional>

namespace tenhou {
    AssetId FlowerArtwork(int entity);

    class VariantArtwork {
    public:
        void Configure(const std::filesystem::path &directory) { directory_ = directory; images_.clear(); }
        // Missing optional artwork uses the caller's text fallback. Invalid PNGs fail visibly.
        const Image *Get(AssetId id);
        const Image *GetSprite(unsigned id);
    private:
        std::filesystem::path directory_;
        std::map<AssetId, std::optional<Image>> images_;
    };
}
