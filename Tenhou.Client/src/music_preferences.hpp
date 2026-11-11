#pragma once
#include "replay_audio.hpp"
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
            if (key == L"BGM_LOBBY0") return resources / L"_bgm0.wav";
            if (key == L"BGM_LOBBY1") return resources / L"_bgm1.wav";
            if (key != L"BGM_GAME0") return MusicPath(resources, config, L"BGM_GAME0");
            return {};
        }
        if (_wcsicmp(source.c_str(), L"none") == 0) return {};
        std::filesystem::path path(source);
        return path.is_absolute() ? path : config.parent_path() / path;
    }
}
