#pragma once
#include "replay_audio.hpp"
#include "resource_manager.hpp"
#include "client_preferences.hpp"

namespace tenhou {
    inline const std::array<const wchar_t *, 9> &MusicKeys() {
        static constexpr std::array<const wchar_t *, 9> keys{
            L"BGM_LOBBY0", L"BGM_LOBBY1", L"BGM_GAME0", L"BGM_GAME1", L"BGM_GAME2", L"BGM_GAME3", L"BGM_GAME4",
            L"BGM_GAME5", L"BGM_GAME9"
        };
        return keys;
    }

    inline std::wstring MusicOption(const std::wstring &key) { return key + L"OPT"; }

    inline std::filesystem::path MusicPath(const std::filesystem::path &resources, const std::filesystem::path &config,
                                           const std::wstring &key) {
        auto source = PreferenceString(config, L"RESOURCE", key.c_str(), L"default");
        if (source.empty() || _wcsicmp(source.c_str(), L"default") == 0) {
            if (key == L"BGM_LOBBY0") return ResourceManager(resources).Path(AssetId::LobbyMusic);
            if (key == L"BGM_LOBBY1") return ResourceManager(resources).Path(AssetId::LobbyMusicAlternate);
            if (key != L"BGM_GAME0") return MusicPath(resources, config, L"BGM_GAME0");
            return {};
        }
        if (_wcsicmp(source.c_str(), L"none") == 0) return {};
        std::filesystem::path path(source);
        if (path.is_relative()) path = config.parent_path() / path;
        // Older settings saved the absolute flat path when selecting a built-in BGM.
        // Redirect only missing built-in paths within this resource root.
        if (!std::filesystem::exists(path)) {
            for (const auto id: {AssetId::LobbyMusic, AssetId::LobbyMusicAlternate}) {
                const auto legacy = std::filesystem::absolute(resources / ResourceManager::Info(id).legacy_filename).lexically_normal();
                if (_wcsicmp(std::filesystem::absolute(path).lexically_normal().c_str(), legacy.c_str()) == 0)
                    return ResourceManager(resources).Path(id);
            }
        }
        return path;
    }

    // Settings previews retain their explicit track. Default live lobby playback
    // samples both built-ins, while room-specific overrides and game BGM keep
    // their configured meaning.
    inline std::filesystem::path MusicPlaybackPath(const std::filesystem::path &resources,
                                                   const std::filesystem::path &config,
                                                   const std::wstring &key, unsigned lobby_track) {
        if (key == L"BGM_LOBBY0" || key == L"BGM_LOBBY1") {
            const auto source = PreferenceString(config, L"RESOURCE", key.c_str(), L"default");
            if (source.empty() || _wcsicmp(source.c_str(), L"default") == 0)
                return ResourceManager(resources).Path(lobby_track % 2 ? AssetId::LobbyMusicAlternate : AssetId::LobbyMusic);
        }
        return MusicPath(resources, config, key);
    }
}
