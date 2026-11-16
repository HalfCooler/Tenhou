#pragma once
#include "resource_manager.hpp"
#include <map>
#include <optional>

namespace tenhou {
    struct PatternArtwork {
        AssetId asset;
        int value = 0;
    };

    // Only catalogued names can select a file. Server text never becomes a path.
    std::optional<PatternArtwork> FindPatternArtwork(const std::string &rule, const std::string &name);
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
