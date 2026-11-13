#pragma once
#include "asset_catalog.hpp"
#include "resource_pack.hpp"
#include <utility>

namespace tenhou {
    // A view into the executable's RCDATA; valid for the process lifetime.
    struct EmbeddedAsset {
        const std::uint8_t *data;
        std::size_t size;
    };

    class ResourceManager {
    public:
        explicit ResourceManager(std::filesystem::path directory) : directory_(std::move(directory)) {}

        // Structured catalog path first, then the old flat filename.
        // Embedded assets have no runtime filesystem path.
        std::filesystem::path Path(AssetId id) const;
        Bytes Read(AssetId id) const;

        static const AssetInfo &Info(AssetId id);
        static EmbeddedAsset Embedded(AssetId id);

    private:
        std::filesystem::path directory_;
    };
}
