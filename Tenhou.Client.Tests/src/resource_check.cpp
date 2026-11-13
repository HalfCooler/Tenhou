#include "resource_manager.hpp"
#include "object_pack.hpp"
#include "background.hpp"
#include "music_preferences.hpp"
#include "replay_effects.hpp"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
    if (argc != 3 && !(argc == 4 && std::string(argv[3]) == "--play-audio")) {
        std::cerr << "Usage: tenhou_resource_check RUNTIME_RESOURCES SOURCE_ASSETS [--play-audio]\n";
        return 1;
    }
    ULONG_PTR token = 0;
    try {
        Gdiplus::GdiplusStartupInput startup;
        if (Gdiplus::GdiplusStartup(&token, &startup, nullptr) != Gdiplus::Ok)
            throw std::runtime_error("Cannot initialize GDI+");
        tenhou::ResourceManager resources(argv[1]);
        for (const auto &asset : tenhou::kClientAssets) {
            const auto original = std::filesystem::path(argv[2]) / asset.path;
            if (!asset.required && !std::filesystem::exists(original)) continue;
            if (resources.Read(asset.id) != tenhou::ReadFile(original))
                throw std::runtime_error(std::string("Asset bytes differ: ") + asset.name);
        }
        for (const auto id : {tenhou::AssetId::Background, tenhou::AssetId::BackgroundAlternate}) {
            const auto image = tenhou::BuildBackground(resources.Path(id), 640, 480);
            if (image.width != 640 || image.height != 480 || image.bgra.size() != 640u * 480u * 4u)
                throw std::runtime_error("Background decode failed");
        }
        const auto display = tenhou::LoadResourcePack(resources.Path(tenhou::AssetId::DisplayPack));
        if (display.entries.empty()) throw std::runtime_error("Empty display resource pack");
        const auto table = tenhou::LoadObjectPack(resources.Path(tenhou::AssetId::TablePack));
        const auto companion = tenhou::LoadObjectPack(resources.Path(tenhou::AssetId::TableCompanionPack));
        if (table.entries.empty() || companion.entries.empty()) throw std::runtime_error("Empty table resource pack");
        // Exercise the compatibility path without copying the large packs.
        tenhou::ResourceManager legacy(std::filesystem::path(argv[2]) / "packs");
        if (legacy.Path(tenhou::AssetId::DisplayPack) != resources.Path(tenhou::AssetId::DisplayPack)
            && tenhou::ReadFile(legacy.Path(tenhou::AssetId::DisplayPack)) != resources.Read(tenhou::AssetId::DisplayPack))
            throw std::runtime_error("Legacy flat resource lookup failed");
        // Load through the same bank as live play; merely inspecting pack headers
        // misses broken sound IDs, aliases, PCM decoding and directory resolution.
        wchar_t temporary[MAX_PATH]{};
        if (!GetTempPathW(MAX_PATH, temporary)) throw std::runtime_error("Cannot locate temporary directory");
        const auto config = std::filesystem::path(temporary) /
            (L"tenhou-audio-check-" + std::to_wstring(GetCurrentProcessId()) + L".ini");
        const auto custom_sound = config.parent_path() / (config.stem().wstring() + L".wav");
        struct Cleanup {
            std::filesystem::path config, sound;
            ~Cleanup() {
                std::error_code error;
                std::filesystem::remove(config, error);
                std::filesystem::remove(sound, error);
            }
        } cleanup{config, custom_sound};
        tenhou::WritePreference(config, L"REBUILD", L"NetworkSound", L"0");
        if (tenhou::ClientPreferences::Load(config).se_volume != 100)
            throw std::runtime_error("Legacy NetworkSound must not mute original sound preferences");
        tenhou::ReplaySoundBank sounds, flat_sounds;
        sounds.Load(argv[1], 1440, config);
        flat_sounds.Load(std::filesystem::path(argv[2]) / "packs", 1440, config);
        for (const auto &key: tenhou::SoundKeys()) {
            const auto &clip = sounds.Get(key.first);
            if (key.first >= 115 && key.first <= 117) {
                if (clip || flat_sounds.Get(key.first)) throw std::runtime_error("Unexpected optional built-in sound");
                continue;
            }
            if (!clip || clip->pcm.empty() || clip->sha256 != flat_sounds.Get(key.first)->sha256)
                throw std::runtime_error("Sound decode/legacy lookup failed: " + key.second);
        }
        for (const auto id: {tenhou::AssetId::LobbyMusic, tenhou::AssetId::LobbyMusicAlternate}) {
            if (tenhou::DecodeMusic(resources.Read(id)).pcm.empty()) throw std::runtime_error("Empty lobby BGM");
        }
        for (const auto key: {L"BGM_LOBBY0", L"BGM_LOBBY1"}) {
            if (tenhou::MusicPlaybackPath(argv[1], config, key, 0) != resources.Path(tenhou::AssetId::LobbyMusic) ||
                tenhou::MusicPlaybackPath(argv[1], config, key, 1) != resources.Path(tenhou::AssetId::LobbyMusicAlternate))
                throw std::runtime_error("Default live lobby must allow both tracks");
            tenhou::WritePreference(config, L"RESOURCE", key, L"custom.wav");
            for (unsigned track: {0u, 1u})
                if (tenhou::MusicPlaybackPath(argv[1], config, key, track) != config.parent_path() / "custom.wav")
                    throw std::runtime_error("Random selection replaced custom lobby music");
            tenhou::WritePreference(config, L"RESOURCE", key, L"none");
            for (unsigned track: {0u, 1u})
                if (!tenhou::MusicPlaybackPath(argv[1], config, key, track).empty())
                    throw std::runtime_error("Random selection replaced lobby mute");
            tenhou::WritePreference(config, L"RESOURCE", key, nullptr);
        }
        if (!tenhou::MusicPlaybackPath(argv[1], config, L"BGM_GAME0", 1).empty())
            throw std::runtime_error("Lobby random selection leaked into game music");
        tenhou::WritePreference(config, L"RESOURCE", L"SE_DA", L"none");
        tenhou::ReplaySoundBank muted;
        muted.Load(argv[1], 1440, config);
        if (muted.Get(103) || !muted.Get(105)) throw std::runtime_error("Per-sound mute affected another sound");
        const auto sample = std::find_if(display.entries.begin(), display.entries.end(),
            [](const tenhou::Resource &entry) { return entry.id == 103; });
        if (sample == display.entries.end()) throw std::runtime_error("Missing custom sound fixture");
        tenhou::WriteFile(custom_sound, sample->payload);
        tenhou::WritePreference(config, L"RESOURCE", L"SE_LOGIN", custom_sound.filename().c_str());
        tenhou::ReplaySoundBank overridden;
        overridden.Load(argv[1], 1440, config);
        if (!overridden.Get(116) || overridden.Get(116)->sha256 != sounds.Get(103)->sha256)
            throw std::runtime_error("Optional sound relative override failed");
        if (argc == 4) {
            tenhou::ReplayAudio audio([](const std::string &line) { std::cout << line << '\n'; });
            for (const unsigned id: {104u, 105u, 103u, 112u, 137u, 155u}) {
                audio.Schedule(id, sounds.Get(id), 0);
                const auto deadline = GetTickCount64() + 10000;
                while (audio.Active() && GetTickCount64() < deadline) { audio.Poll(); Sleep(10); }
                if (audio.Active()) throw std::runtime_error("Audio playback did not finish");
            }
        }
        Gdiplus::GdiplusShutdown(token);
        std::cout << "PASS client asset bytes, embedded resources, background/pack decoding, legacy paths, "
                     "sound aliases/PCM, both BGM tracks, playback selection and overrides\n";
        return 0;
    } catch (const std::exception &error) {
        if (token) Gdiplus::GdiplusShutdown(token);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
