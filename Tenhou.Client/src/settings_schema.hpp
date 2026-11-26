#pragma once
#include "client_preferences.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tenhou {
    // Declarative cross-platform schema for all 7 settings pages, abstracting away the
    // binary Win32 RT_DIALOG templates (assets/dialogs/104.bin..114.bin) while preserving
    // 100% compatibility with Tenhou.Client.Origin INI keys, bit flags, and presets.
    enum class SettingsPageId : int {
        Tile = 104,
        Table = 105,
        Sound = 106,
        Music = 109,
        Replay = 110,
        Screen = 114,
        Other = 107,
    };

    enum class SettingsFieldKind {
        IntegerRange,
        Toggle,
        BitFlag,
        Choice,
        FilePath,
        DirectoryPath,
    };

    struct SettingsChoiceOption {
        const char *value;
        const char *label;
    };

    struct SettingsFieldDescriptor {
        const char *id;
        const char *label;
        SettingsFieldKind kind;
        const char *ini_section;
        const char *ini_key;
        int legacy_control_id; // Original dialog control ID (0 if none)
        int min_value;
        int max_value;
        int default_value;
        std::uint32_t flag_mask;
        bool flag_inverted;
    };

    struct SettingsPageDescriptor {
        SettingsPageId id;
        const char *slug;
        const char *title;
        const SettingsFieldDescriptor *fields;
        std::size_t field_count;
    };

    struct TileColorPreset {
        const char *name;
        std::array<int, 3> rgb;
        int shadow; // Original 004175b0 resets HAI_SDW to 6 on every preset.
        std::uint32_t menu_command;
    };

    struct BackgroundPreset {
        const char *name;
        const char *source;
        std::array<const char *, 10> lhsv;
        int menu_command;
    };

    struct ScreenSizePreset {
        const char *value;
        const char *label;
        int width; // 0 for "default" (auto)
    };

    // Ordered identically to Tenhou.Client.Origin settings property sheet (104,105,106,109,110,114,107).
    const std::vector<SettingsPageDescriptor> &SettingsSchemaPages();
    const SettingsPageDescriptor *FindSettingsPage(SettingsPageId id);

    const std::vector<TileColorPreset> &SettingsTilePresets();
    const std::vector<BackgroundPreset> &SettingsBackgroundPresets();
    const std::vector<ScreenSizePreset> &SettingsScreenSizePresets();
    const std::vector<SettingsChoiceOption> &SettingsSoundModes();

    // Pure C++ headless settings validation and INI persistence helper.
    class SettingsStore {
    public:
        explicit SettingsStore(std::filesystem::path config_path) : config_(std::move(config_path)) {}

        ClientPreferences LoadPreferences() const;

        void SaveTileAppearance(std::array<int, 3> rgb, int shadow) const;
        void ApplyTilePreset(std::size_t preset_index) const;

        void SetCenterPanelHidden(bool hidden) const;
        void SaveBackground(const std::string &source, const std::array<std::string, 10> &lhsv) const;
        void ApplyBackgroundPreset(std::size_t preset_index) const;

        int LoadSoundMode() const;
        void SaveSoundMode(int mode) const;
        void SaveSoundVolume(int volume) const;

        int LoadMusicMasterVolume() const;
        void SaveMusicMasterVolume(int volume) const;
        int LoadMusicTrackVolume(std::string_view key) const;
        void SaveMusicTrackVolume(std::string_view key, int volume) const;

        std::string LoadReplayDirectory() const;
        void SaveReplayDirectory(std::optional<std::string_view> directory) const;

        std::string LoadScreenSize() const;
        bool SaveScreenSize(std::string_view value) const; // Returns true if changed.
        std::string LoadFullscreenSize() const;
        void SaveFullscreenSize(std::optional<std::pair<int, int>> dims) const;

        void SaveHiddenId(bool hidden) const;
        void SaveOtherFlag(int legacy_control_id, bool checked) const;

        static int ClampRange(int value, int min_v, int max_v);

    private:
        std::filesystem::path config_;
    };
} // namespace tenhou
