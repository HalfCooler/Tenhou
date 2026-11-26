#include "settings_schema.hpp"
#include "music_preferences.hpp"
#include "window_layout.hpp"
#include <algorithm>
#include <stdexcept>

namespace tenhou {
    namespace {
        constexpr SettingsFieldDescriptor kTileFields[]{
            {"tile_red", "牌背 R", SettingsFieldKind::IntegerRange, "REBUILD", "TileRed", 0x403, 0, 256, 0, 0, false},
            {"tile_green", "牌背 G", SettingsFieldKind::IntegerRange, "REBUILD", "TileGreen", 0x404, 0, 256, 0, 0, false},
            {"tile_blue", "牌背 B", SettingsFieldKind::IntegerRange, "REBUILD", "TileBlue", 0x405, 0, 256, 38, 0, false},
            {"tile_shadow", "牌影强度", SettingsFieldKind::IntegerRange, "RESOURCE", "HAI_SDW", 0x3ee, 0, 10, 6, 0, false},
        };

        constexpr SettingsFieldDescriptor kTableFields[]{
            {"bg_source", "桌布来源", SettingsFieldKind::FilePath, "RESOURCE", "BG_SRC", 0x408, 0, 0, 0, 0, false},
            {"bg_lhsv", "光照与分层 HSV", SettingsFieldKind::Choice, "RESOURCE", "BG_LHSV", 0x3f4, 0, 10, 2, 0, false},
            {"hide_center_panel", "隐藏中央底板", SettingsFieldKind::BitFlag, "STATUS", "FLAGS0", 0x420, 0, 1, 0, 0x2000u, false},
        };

        constexpr SettingsFieldDescriptor kSoundFields[]{
            {"se_mode", "语音与音效模式", SettingsFieldKind::Choice, "RESOURCE", "SE_NOVOICE", 0x426, 0, 2, 0, 0, false},
            {"se_volume", "音效音量", SettingsFieldKind::IntegerRange, "RESOURCE", "SE_VOL", 0x3fa, 0, 100, 100, 0, false},
        };

        constexpr SettingsFieldDescriptor kMusicFields[]{
            {"bgm_volume", "主音乐音量", SettingsFieldKind::IntegerRange, "RESOURCE", "BGM_VOL", 0x3fa, 0, 200, 100, 0, false},
            {"bgm_track_volume", "场景音乐音量", SettingsFieldKind::IntegerRange, "RESOURCE", "BGM_TRACK_OPT", 0x3f9, 0, 200, 60, 0, false},
        };

        constexpr SettingsFieldDescriptor kReplayFields[]{
            {"replay_save_enabled", "自动保存牌谱", SettingsFieldKind::Toggle, "LOG", "PATH", 0x420, 0, 1, 0, 0, false},
            {"replay_path", "牌谱保存目录", SettingsFieldKind::DirectoryPath, "LOG", "PATH", 0x419, 0, 0, 0, 0, false},
        };

        constexpr SettingsFieldDescriptor kScreenFields[]{
            {"screen_size", "画面尺寸", SettingsFieldKind::Choice, "STATUS", "SCREENSIZE", 0x426, 0, 4, 0, 0, false},
            {"fullscreen_size", "全屏分辨率", SettingsFieldKind::Choice, "STATUS", "FULLSCREENSIZE", 0x429, 0, 0, 0, 0, false},
        };

        constexpr SettingsFieldDescriptor kOtherFields[]{
            {"hidden_id", "隐藏登录 ID", SettingsFieldKind::Toggle, "STATUS", "HIDDENID", 0x420, 0, 1, 0, 0, false},
            {"flag_423", "对局操作选项 0x423", SettingsFieldKind::BitFlag, "STATUS", "FLAGS0", 0x423, 0, 1, 0, 0x20u, false},
            {"flag_425", "对局操作选项 0x425", SettingsFieldKind::BitFlag, "STATUS", "FLAGS0", 0x425, 0, 1, 1, 0x200u, true},
            {"flag_424", "对局操作选项 0x424", SettingsFieldKind::BitFlag, "STATUS", "FLAGS0", 0x424, 0, 1, 1, 0x400u, true},
            {"flag_426", "对局操作选项 0x426", SettingsFieldKind::BitFlag, "STATUS", "FLAGS0", 0x426, 0, 1, 1, 0x800u, true},
            {"flag_427", "对局操作选项 0x427", SettingsFieldKind::BitFlag, "STATUS", "FLAGS0", 0x427, 0, 1, 0, 0x8000u, false},
            {"flag_428", "结算长按透视牌桌", SettingsFieldKind::BitFlag, "STATUS", "FLAGS0", 0x428, 0, 1, 1, 0x10000u, true},
        };
    } // namespace

    const std::vector<SettingsPageDescriptor> &SettingsSchemaPages() {
        static const std::vector<SettingsPageDescriptor> kPages{
            {SettingsPageId::Tile, "tile", "牌", kTileFields, std::size(kTileFields)},
            {SettingsPageId::Table, "table", "卓", kTableFields, std::size(kTableFields)},
            {SettingsPageId::Sound, "sound", "SE", kSoundFields, std::size(kSoundFields)},
            {SettingsPageId::Music, "music", "BGM", kMusicFields, std::size(kMusicFields)},
            {SettingsPageId::Replay, "replay", "牌谱", kReplayFields, std::size(kReplayFields)},
            {SettingsPageId::Screen, "screen", "画面", kScreenFields, std::size(kScreenFields)},
            {SettingsPageId::Other, "other", "其他", kOtherFields, std::size(kOtherFields)},
        };
        return kPages;
    }

    const SettingsPageDescriptor *FindSettingsPage(SettingsPageId id) {
        for (const auto &page: SettingsSchemaPages()) {
            if (page.id == id) return &page;
        }
        return nullptr;
    }

    const std::vector<TileColorPreset> &SettingsTilePresets() {
        static const std::vector<TileColorPreset> kPresets{
            {"DarkBlue(default)", {0, 0, 38}, 6, 0x260000u},
            {"DarkGreen", {0, 38, 0}, 6, 0x2600u},
            {"DarkRed", {38, 0, 0}, 6, 0x26u},
            {"DarkGray", {24, 24, 24}, 6, 0x181818u},
            {"Black", {0, 0, 0}, 6, 0xffffffffu},
        };
        return kPresets;
    }

    const std::vector<BackgroundPreset> &SettingsBackgroundPresets() {
        static const std::vector<BackgroundPreset> kPresets{
            {"标准", "", {"2", "0", "255", "255", "", "", "", "", "", ""}, 100},
            {"单色", "#FF0000", {"2", "240", "16", "128", "", "", "", "", "", ""}, 200},
            {"BG1", "./BG1", {"2", "0", "255", "255", "0", "255", "255", "0", "255", "255"}, 300},
        };
        return kPresets;
    }

    const std::vector<ScreenSizePreset> &SettingsScreenSizePresets() {
        static const std::vector<ScreenSizePreset> kPresets{
            {"default", "自动", 0},
            {"1024x768", "1024 x 768", 1024},
            {"1440x1080", "1440 x 1080", 1440},
            {"1920x1440", "1920 x 1440", 1920},
            {"2640x1980", "2640 x 1980", 2640},
        };
        return kPresets;
    }

    const std::vector<SettingsChoiceOption> &SettingsSoundModes() {
        static const std::vector<SettingsChoiceOption> kModes{
            {"0", "标准（音效 + 语音）"},
            {"1", "仅音效（无语音）"},
            {"2", "全部静音"},
        };
        return kModes;
    }

    int SettingsStore::ClampRange(int value, int min_v, int max_v) {
        return std::clamp(value, min_v, max_v);
    }

    ClientPreferences SettingsStore::LoadPreferences() const {
        return ClientPreferences::Load(config_);
    }

    void SettingsStore::SaveTileAppearance(std::array<int, 3> rgb, int shadow) const {
        const char *keys[]{"TileRed", "TileGreen", "TileBlue"};
        for (unsigned i = 0; i < 3; ++i)
            WritePreferenceInt(config_, "REBUILD", keys[i], ClampRange(rgb[i], 0, 256));
        WritePreferenceInt(config_, "RESOURCE", "HAI_SDW", ClampRange(shadow, 0, 10));
    }

    void SettingsStore::ApplyTilePreset(std::size_t preset_index) const {
        const auto &presets = SettingsTilePresets();
        if (preset_index >= presets.size()) throw std::out_of_range("Invalid tile preset index");
        SaveTileAppearance(presets[preset_index].rgb, presets[preset_index].shadow);
    }

    void SettingsStore::SetCenterPanelHidden(bool hidden) const {
        auto prefs = ClientPreferences::Load(config_);
        prefs.flags = (prefs.flags & ~0x2000u) | (hidden ? 0x2000u : 0u);
        WritePreferenceInt(config_, "STATUS", "FLAGS0", static_cast<int>(prefs.flags));
    }

    void SettingsStore::SaveBackground(const std::string &source, const std::array<std::string, 10> &lhsv) const {
        ClientPreferences tmp;
        tmp.background_lhsv = lhsv;
        WritePreference(config_, "RESOURCE", "BG_LHSV", tmp.BackgroundLhsv());
        WritePreference(config_, "RESOURCE", "BG_SRC", source.empty() ? nullptr : source.c_str());
    }

    void SettingsStore::ApplyBackgroundPreset(std::size_t preset_index) const {
        const auto &presets = SettingsBackgroundPresets();
        if (preset_index >= presets.size()) throw std::out_of_range("Invalid background preset index");
        std::array<std::string, 10> lhsv;
        for (std::size_t i = 0; i < 10; ++i) lhsv[i] = presets[preset_index].lhsv[i];
        SaveBackground(presets[preset_index].source, lhsv);
    }

    int SettingsStore::LoadSoundMode() const {
        const int previous = PreferenceInt(config_, "REBUILD", "SoundEffectsOnly", 0) ? 1 : 0;
        return ClampRange(PreferenceInt(config_, "RESOURCE", "SE_NOVOICE", previous), 0, 2);
    }

    void SettingsStore::SaveSoundMode(int mode) const {
        WritePreferenceInt(config_, "RESOURCE", "SE_NOVOICE", ClampRange(mode, 0, 2));
    }

    void SettingsStore::SaveSoundVolume(int volume) const {
        WritePreferenceInt(config_, "RESOURCE", "SE_VOL", ClampRange(volume, 0, 100));
    }

    int SettingsStore::LoadMusicMasterVolume() const {
        return ClampRange(PreferenceInt(config_, "RESOURCE", "BGM_VOL", 100), 0, 200);
    }

    void SettingsStore::SaveMusicMasterVolume(int volume) const {
        WritePreferenceInt(config_, "RESOURCE", "BGM_VOL", ClampRange(volume, 0, 200));
    }

    int SettingsStore::LoadMusicTrackVolume(std::string_view key) const {
        return ClampRange(PreferenceInt(config_, "RESOURCE", MusicOption(std::string(key)), 60), 0, 200);
    }

    void SettingsStore::SaveMusicTrackVolume(std::string_view key, int volume) const {
        WritePreferenceInt(config_, "RESOURCE", MusicOption(std::string(key)), ClampRange(volume, 0, 200));
    }

    std::string SettingsStore::LoadReplayDirectory() const {
        return PreferenceString(config_, "LOG", "PATH");
    }

    void SettingsStore::SaveReplayDirectory(std::optional<std::string_view> directory) const {
        if (!directory || directory->empty()) {
            WritePreference(config_, "LOG", "PATH", nullptr);
            return;
        }
        const std::string copy(*directory);
        WritePreference(config_, "LOG", "PATH", copy.c_str());
    }

    std::string SettingsStore::LoadScreenSize() const {
        const auto raw = PreferenceString(config_, "STATUS", "SCREENSIZE");
        const int parsed = ParseScreenSizeSetting(raw);
        for (const auto &preset: SettingsScreenSizePresets()) {
            if (preset.width == parsed) return preset.value;
        }
        return "default";
    }

    bool SettingsStore::SaveScreenSize(std::string_view value) const {
        const char *canonical = "default";
        for (const auto &preset: SettingsScreenSizePresets()) {
            if (value == preset.value) {
                canonical = preset.value;
                break;
            }
        }
        if (PreferenceString(config_, "STATUS", "SCREENSIZE") == canonical) return false;
        WritePreference(config_, "STATUS", "SCREENSIZE", canonical);
        return true;
    }

    std::string SettingsStore::LoadFullscreenSize() const {
        return PreferenceString(config_, "STATUS", "FULLSCREENSIZE");
    }

    void SettingsStore::SaveFullscreenSize(std::optional<std::pair<int, int>> dims) const {
        if (!dims || dims->first <= 0 || dims->second <= 0) {
            WritePreference(config_, "STATUS", "FULLSCREENSIZE", nullptr);
            return;
        }
        const std::string text = std::to_string(dims->first) + "," + std::to_string(dims->second);
        WritePreference(config_, "STATUS", "FULLSCREENSIZE", text.c_str());
    }

    void SettingsStore::SaveHiddenId(bool hidden) const {
        WritePreference(config_, "STATUS", "HIDDENID", hidden ? "1" : nullptr);
    }

    void SettingsStore::SaveOtherFlag(int legacy_control_id, bool checked) const {
        auto prefs = ClientPreferences::Load(config_);
        prefs.SetChecked(legacy_control_id, checked);
        WritePreferenceInt(config_, "STATUS", "FLAGS0", static_cast<int>(prefs.flags));
    }
} // namespace tenhou
