#pragma once
#include "client_preferences.hpp"
#include "replay_audio.hpp"
#include "resource_manager.hpp"
#include "win32_utf.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>

namespace tenhou {
    inline bool AsciiCaseEqual(const std::string_view a, const std::string_view b) {
        if (a.size() != b.size()) return false;
        for (std::size_t i = 0; i < a.size(); ++i) {
            const auto ca = static_cast<unsigned char>(a[i]);
            const auto cb = static_cast<unsigned char>(b[i]);
            if (std::tolower(ca) != std::tolower(cb)) return false;
        }
        return true;
    }

    inline const std::array<const char *, 9> &MusicKeys() {
        static constexpr std::array<const char *, 9> keys{
            "BGM_LOBBY0", "BGM_LOBBY1", "BGM_GAME0", "BGM_GAME1", "BGM_GAME2", "BGM_GAME3", "BGM_GAME4",
            "BGM_GAME5", "BGM_GAME9"
        };
        return keys;
    }

    inline std::string MusicOption(const std::string_view key) { return std::string(key) + "OPT"; }

    inline std::filesystem::path MusicPath(const std::filesystem::path &resources, const std::filesystem::path &config,
                                           const std::string_view key, const bool third_party_lobby = false) {
        const auto source = PreferenceString(config, "RESOURCE", key, "default");
        if (AsciiCaseEqual(source, "none")) return {};
        if (third_party_lobby && key == "BGM_LOBBY0") return ResourceManager(resources).Path(AssetId::LobbyMusic);
        if (source.empty() || AsciiCaseEqual(source, "default")) {
            if (key == "BGM_LOBBY0") return ResourceManager(resources).Path(AssetId::LobbyMusic);
            if (key == "BGM_LOBBY1") return ResourceManager(resources).Path(AssetId::LobbyMusicAlternate);
            if (key != "BGM_GAME0") return MusicPath(resources, config, "BGM_GAME0");
            return {};
        }
        std::filesystem::path path = PathFromUtf8(source);
        if (path.is_relative()) path = config.parent_path() / path;
        // Older settings saved the absolute flat path when selecting a built-in BGM.
        // Redirect only missing built-in paths within this resource root.
        if (!std::filesystem::exists(path)) {
            for (const auto id: {AssetId::LobbyMusic, AssetId::LobbyMusicAlternate}) {
                const auto legacy = std::filesystem::absolute(
                    resources / PathFromUtf8(ResourceManager::Info(id).legacy_filename)).lexically_normal();
                if (AsciiCaseEqual(PathToUtf8(std::filesystem::absolute(path).lexically_normal()),
                                   PathToUtf8(legacy)))
                    return ResourceManager(resources).Path(id);
            }
        }
        return path;
    }
} // namespace tenhou
