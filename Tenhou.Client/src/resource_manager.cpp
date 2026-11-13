#include "resource_manager.hpp"
#include <windows.h>
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
        const auto path = directory_ / asset.path;
        if (std::filesystem::is_regular_file(path)) return path;
        const auto legacy = directory_ / asset.legacy_filename;
        if (std::filesystem::is_regular_file(legacy)) return legacy;
        return path; // Let the format loader report the missing canonical path.
    }

    EmbeddedAsset ResourceManager::Embedded(AssetId id) {
        const auto &asset = Info(id);
        if (!asset.embedded_id) throw std::runtime_error(std::string("Asset is not embedded: ") + asset.name);
        const auto module = GetModuleHandleW(nullptr);
        const auto resource = FindResourceW(module, MAKEINTRESOURCEW(asset.embedded_id), MAKEINTRESOURCEW(10));
        if (!resource) throw std::runtime_error(std::string("Missing embedded asset: ") + asset.name);
        const auto size = SizeofResource(module, resource);
        const auto *data = static_cast<const std::uint8_t *>(LockResource(LoadResource(module, resource)));
        if (!data || !size) throw std::runtime_error(std::string("Cannot load embedded asset: ") + asset.name);
        return {data, size};
    }

    Bytes ResourceManager::Read(AssetId id) const {
        if (!Info(id).embedded_id) return ReadFile(Path(id));
        const auto view = Embedded(id);
        return Bytes(view.data, view.data + view.size);
    }
}
