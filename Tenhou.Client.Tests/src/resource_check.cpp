#include "resource_manager.hpp"
#include "object_pack.hpp"
#include "background.hpp"
#include "ini_store.hpp"
#include "music_preferences.hpp"
#include "original_display.hpp"
#include "replay_effects.hpp"
#include "shared_assets.hpp"
#include "sprite_assets.hpp"
#include "table_scene.hpp"
#include "win32_utf.hpp"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
    void CheckDisplayImages(const std::filesystem::path &directory, const tenhou::ResourcePack &pack) {
        using namespace tenhou;
        const ResourceManager resources(directory);
        DisplayArtwork bundled;
        bundled.Configure(directory, 1440);
        for (unsigned id = 401; id <= 705; ++id) {
            if (FindTextImageByOrigin(id) || id == 593) continue;
            auto path = directory / "images" / "sprites" / (std::to_string(id) + ".png");
            if (std::filesystem::is_regular_file(path))
                throw std::runtime_error("Retired sprite still staged: " + path.u8string());
        }

        const auto fixture = std::filesystem::temp_directory_path() /
            (L"tenhou-display-images-" + std::to_wstring(GetCurrentProcessId()));
        if (!std::filesystem::create_directory(fixture)) throw std::runtime_error("Display fixture directory already exists");
        struct Cleanup {
            std::filesystem::path path;
            ~Cleanup() { std::error_code error; std::filesystem::remove_all(path, error); }
        } cleanup{fixture};
        const auto png = resources.Read(AssetId::FlowerMei);
        VariantArtwork overrides; overrides.Configure(fixture);
        DisplayArtwork hd, sd, tier1920, tier2640;
        hd.Configure(fixture, 1440); sd.Configure(fixture, 1024);
        tier1920.Configure(directory, 1920); tier2640.Configure(directory, 2640);
        DisplayArtwork bundled_sd; bundled_sd.Configure(directory, 1024);
        for (const auto &sprite : kSpriteAssets) {
            const auto entry = std::find_if(pack.entries.begin(), pack.entries.end(),
                [&](const Resource &resource) { return resource.id == sprite.id; });
            if (entry == pack.entries.end()) throw std::runtime_error("Missing bundled display image " + std::to_string(sprite.id));
            const auto original = DecodeImage(*entry);
            if ((sprite.id == 591 && (original.width != 728 || original.height != 330)) ||
                (sprite.id == 592 && (original.width != 590 || original.height != 430)))
                throw std::runtime_error("Bundled panel dimensions changed for image " + std::to_string(sprite.id));
            const bool has_override = std::filesystem::is_regular_file(resources.Path(sprite.asset));
            if (!has_override && bundled.Get(sprite.id).bgra != original.bgra)
                throw std::runtime_error("DAT display fallback differs for image " + std::to_string(sprite.id));
            if (has_override) {
                const auto hd_sz = OriginalDisplaySpriteSize(sprite.id, 1440);
                const auto sd_sz = OriginalDisplaySpriteSize(sprite.id, 1024);
                const auto &img1440 = bundled.Get(sprite.id);
                const auto &img1024 = bundled_sd.Get(sprite.id);
                const auto &img1920 = tier1920.Get(sprite.id);
                const auto &img2640 = tier2640.Get(sprite.id);
                if (img1440.width != hd_sz.width || img1440.height != hd_sz.height ||
                    img1024.width != sd_sz.width || img1024.height != sd_sz.height ||
                    img1920.width <= hd_sz.width || img2640.width <= img1920.width)
                    throw std::runtime_error("Multi-tier Area downsampling failed for HD sprite " + std::to_string(sprite.id));
            }

            const auto path = fixture / PathFromUtf8(ResourceManager::Info(sprite.asset).path);
            std::filesystem::create_directories(path.parent_path());
            WriteFile(path, png);
            const auto *custom = overrides.GetSprite(sprite.id);
            const auto &full = hd.Get(sprite.id), &scaled = sd.Get(sprite.id);
            const auto hd_sz = OriginalDisplaySpriteSize(sprite.id, 1440);
            const auto sd_sz = OriginalDisplaySpriteSize(sprite.id, 1024);
            if (!custom ||
                full.width != std::min(custom->width, hd_sz.width) ||
                full.height != std::min(custom->height, hd_sz.height) ||
                scaled.width != std::min(custom->width, sd_sz.width) ||
                scaled.height != std::min(custom->height, sd_sz.height))
                throw std::runtime_error("HD override or SD scaling failed for image " + std::to_string(sprite.id));
        }
    }
}

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
            const auto original = std::filesystem::path(argv[2]) / tenhou::PathFromUtf8(asset.path);
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
        for (const auto &entry : display.entries) {
            if (entry.metadata && !tenhou::FindTextImageByOrigin(entry.id))
                throw std::runtime_error("Unexpected image in bundled display pack: " + std::to_string(entry.id));
        }
        CheckDisplayImages(argv[1], display);
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
        tenhou::WritePreference(config, "REBUILD", "NetworkSound", "0");
        if (tenhou::ClientPreferences::Load(config).se_volume != 100)
            throw std::runtime_error("Legacy NetworkSound must not mute original sound preferences");
        tenhou::SharedAssets().ResetStats();
        tenhou::ReplaySoundBank sounds, flat_sounds;
        sounds.Load(argv[1], 1440, config);
        {
            tenhou::ReplaySoundBank cached_sounds;
            cached_sounds.Load(argv[1], 1440, config);
            if (tenhou::SharedAssets().Stats().sound_bank_hits == 0)
                throw std::runtime_error("SharedAssetStore did not cache ReplaySoundBank");
        }
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
            const auto bgm = tenhou::DecodeMusic(resources.Read(id));
            if (bgm.pcm.empty() || bgm.format.sample_rate != 44100 || bgm.format.channels != 2 || bgm.format.bits_per_sample != 16)
                throw std::runtime_error("Invalid decoded lobby BGM");
        }
        const auto lobby_music = resources.Path(tenhou::AssetId::LobbyMusic);
        if (tenhou::MusicPath(argv[1], config, "BGM_LOBBY0") != lobby_music ||
            tenhou::MusicPath(argv[1], config, "BGM_LOBBY1") !=
                resources.Path(tenhou::AssetId::LobbyMusicAlternate))
            throw std::runtime_error("Default lobby tracks must follow the original keys");
        for (const auto *key: {"BGM_LOBBY0", "BGM_LOBBY1"}) {
            tenhou::WritePreference(config, "RESOURCE", key, "custom.wav");
            if (tenhou::MusicPath(argv[1], config, key) != config.parent_path() / "custom.wav")
                throw std::runtime_error("Custom lobby BGM path was lost");
            if (std::string_view(key) == "BGM_LOBBY0" &&
                tenhou::MusicPath(argv[1], config, key, true) != lobby_music)
                throw std::runtime_error("Third-party lobby must use the built-in BGM");
            tenhou::WritePreference(config, "RESOURCE", key, "none");
            if (!tenhou::MusicPath(argv[1], config, key).empty() ||
                !tenhou::MusicPath(argv[1], config, key, true).empty())
                throw std::runtime_error("Lobby mute was lost");
            tenhou::WritePreference(config, "RESOURCE", key, nullptr);
        }
        if (!tenhou::MusicPath(argv[1], config, "BGM_GAME0").empty())
            throw std::runtime_error("Default game BGM must be silent");
        tenhou::WritePreference(config, "RESOURCE", "SE_DA", "none");
        tenhou::ReplaySoundBank muted;
        muted.Load(argv[1], 1440, config);
        if (muted.Get(103) || !muted.Get(105)) throw std::runtime_error("Per-sound mute affected another sound");
        const auto sample = std::find_if(display.entries.begin(), display.entries.end(),
            [](const tenhou::Resource &entry) { return entry.id == 103; });
        if (sample == display.entries.end()) throw std::runtime_error("Missing custom sound fixture");
        tenhou::WriteFile(custom_sound, sample->payload);
        tenhou::WritePreference(config, "RESOURCE", "SE_LOGIN", tenhou::PathToUtf8(custom_sound.filename()));
        tenhou::ReplaySoundBank overridden;
        overridden.Load(argv[1], 1440, config);
        if (!overridden.Get(116) || overridden.Get(116)->sha256 != sounds.Get(103)->sha256)
            throw std::runtime_error("Optional sound relative override failed");

        // Verify SharedAssetStore deduplicates ObjectPack, DisplayPack, TextFontService, and Background.
        {
            tenhou::SharedAssets().ResetStats();
            tenhou::TableArtwork a1, a2;
            a1.Load(argv[1], 1440);
            a2.Load(argv[1], 1440);
            tenhou::DisplayArtwork d1, d2;
            d1.Configure(argv[1], 1440);
            d2.Configure(argv[1], 1440);
            (void) d1.Get(440);
            (void) d2.Get(440);
            (void) d1.Text();
            (void) d2.Text();
            const auto prefs = tenhou::ClientPreferences::Load(config);
            (void) tenhou::BuildPreferredBackground(argv[1], config, 640, 480, prefs);
            (void) tenhou::BuildPreferredBackground(argv[1], config, 640, 480, prefs);
            const auto st = tenhou::SharedAssets().Stats();
            if (st.object_pack_hits < 2 || st.display_pack_hits < 1 || st.font_hits < 1 || st.background_hits < 1)
                throw std::runtime_error("SharedAssetStore failed to deduplicate shared packs/fonts/background");
        }

        // Verify ini_store legacy UTF-16 LE BOM reading, comment preservation, and UTF-8 roundtrip.
        {
            const auto utf16_ini = config.parent_path() / (config.stem().wstring() + L"-utf16.ini");
            struct IniCleanup {
                std::filesystem::path p;
                ~IniCleanup() { std::error_code ec; std::filesystem::remove(p, ec); }
            } ini_cleanup{utf16_ini};
            const std::wstring content = L"; legacy comment\r\n[REBUILD]\r\nGreeting=你好世界\r\nNum=42\r\n";
            tenhou::Bytes raw{0xFF, 0xFE};
            for (wchar_t wc : content) {
                raw.push_back(static_cast<std::uint8_t>(wc & 0xFF));
                raw.push_back(static_cast<std::uint8_t>((wc >> 8) & 0xFF));
            }
            tenhou::WriteFile(utf16_ini, raw);
            if (tenhou::ReadIniString(utf16_ini, "rebuild", "greeting") != "你好世界" ||
                tenhou::ReadIniInt(utf16_ini, "REBUILD", "NUM", 0) != 42)
                throw std::runtime_error("ini_store failed to read legacy UTF-16 LE BOM file");
            tenhou::WriteIniString(utf16_ini, "REBUILD", "Num", "99");
            if (tenhou::ReadIniString(utf16_ini, "REBUILD", "Greeting") != "你好世界" ||
                tenhou::ReadIniInt(utf16_ini, "REBUILD", "Num", 0) != 99)
                throw std::runtime_error("ini_store failed to preserve UTF-8 values after update");
        }
        tenhou::SharedAssets().Clear();
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
        std::cout << "PASS client asset bytes, embedded resources, display images and PNG overrides, legacy paths, "
                     "sound aliases/PCM, both BGM tracks, playback selection and overrides\n";
        return 0;
    } catch (const std::exception &error) {
        if (token) Gdiplus::GdiplusShutdown(token);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
