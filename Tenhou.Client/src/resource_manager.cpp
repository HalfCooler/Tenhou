#include "resource_manager.hpp"
#include "embedded_assets.hpp"
#include <stdexcept>

namespace tenhou {
    const AssetInfo &ResourceManager::Info(AssetId id) {
        const auto index = static_cast<std::size_t>(id);
        if (index >= static_cast<std::size_t>(AssetId::Count))
            throw std::runtime_error("Invalid client asset ID");
        return kClientAssets[index];
    }

    std::filesystem::path ResourceManager::Path(AssetId id) const {
        const auto &asset = Info(id);
        if (asset.embedded_id) throw std::runtime_error(std::string("Embedded asset has no file path: ") + asset.name);
        const auto path = directory_ / std::filesystem::u8path(asset.path);
        if (std::filesystem::is_regular_file(path)) return path;
        const auto legacy = directory_ / std::filesystem::u8path(asset.legacy_filename);
        if (std::filesystem::is_regular_file(legacy)) return legacy;
        return path; // Let the format loader report the missing canonical path.
    }

    EmbeddedAsset ResourceManager::Embedded(AssetId id) {
        const auto &asset = Info(id);
        if (!asset.embedded_id) throw std::runtime_error(std::string("Asset is not embedded: ") + asset.name);
        const auto view = FindEmbeddedAsset(id);
        if (!view.data || !view.size) throw std::runtime_error(std::string("Missing embedded asset: ") + asset.name);
        return view;
    }

    Bytes ResourceManager::Read(AssetId id) const {
        if (!Info(id).embedded_id) return ReadFile(Path(id));
        const auto view = Embedded(id);
        return Bytes(view.data, view.data + view.size);
    }
}
