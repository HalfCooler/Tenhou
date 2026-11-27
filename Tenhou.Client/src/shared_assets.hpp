#pragma once
#include "client_preferences.hpp"
#include "object_pack.hpp"
#include "resource_pack.hpp"
#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace tenhou {
    class TextFontService;
    struct SoundClip;

    struct SharedAssetStats {
        std::size_t object_pack_loads = 0, object_pack_hits = 0;
        std::size_t display_pack_loads = 0, display_pack_hits = 0;
        std::size_t font_loads = 0, font_hits = 0;
        std::size_t background_builds = 0, background_hits = 0;
        std::size_t sound_bank_loads = 0, sound_bank_hits = 0;
    };

    using SharedSoundBank = std::map<unsigned, std::shared_ptr<const SoundClip>>;

    // Process-level cache sharing read-only decoded packs, fonts, backgrounds and sound banks
    // across WelcomeView, TableView, LobbyView, SettlementView, MatchIntroView and OfflineLobbyView.
    class SharedAssetStore {
    public:
        SharedAssetStore();
        ~SharedAssetStore();

        SharedAssetStore(const SharedAssetStore &) = delete;
        SharedAssetStore &operator=(const SharedAssetStore &) = delete;

        std::shared_ptr<const ObjectPack> GetObjectPack(const std::filesystem::path &path) const;
        std::shared_ptr<const ResourcePack> GetDisplayPack(const std::filesystem::path &path, int width) const;
        std::shared_ptr<const TextFontService> GetTextFont(const std::filesystem::path &resources_dir) const;
        std::shared_ptr<const Image> GetBackground(const std::filesystem::path &directory,
                                                   const std::filesystem::path &config,
                                                   int width, int height,
                                                   const ClientPreferences &preferences,
                                                   const std::function<Image()> &builder) const;
        std::shared_ptr<const SharedSoundBank> GetSoundBank(const std::string &fingerprint,
                                                            const std::function<SharedSoundBank()> &loader) const;

        static std::string FileFingerprint(const std::filesystem::path &path);

        SharedAssetStats Stats() const;
        void ResetStats() const;
        void Clear() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

    SharedAssetStore &SharedAssets();
} // namespace tenhou
