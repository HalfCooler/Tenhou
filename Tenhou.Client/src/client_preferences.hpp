#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace tenhou {
    // Original STATUS/FLAGS0, FUN_0041d830. A checkbox is not always a set bit.
    struct PreferenceFlag {
        int control;
        std::uint32_t mask;
        bool inverted;
    };

    inline constexpr PreferenceFlag kPreferenceFlags[]{
        {0x423, 0x20, false}, {0x425, 0x200, true}, {0x424, 0x400, true},
        {0x426, 0x800, true}, {0x427, 0x8000, false}, {0x428, 0x10000, true}
    };

    struct ClientPreferences {
        std::uint32_t flags = 0x402d;
        bool hidden_id = false;
        int se_volume = 100;
        int tile_shadow = 6;
        std::array<int, 3> tile_rgb{0, 0, 38};
        std::array<std::string, 10> background_lhsv{"2", "0", "255", "255", "", "", "", "", "", ""};
        std::string background_source;
        bool hd_artwork = false;

        std::string BackgroundLhsv() const;

        std::array<float, 3> TileColor() const;

        bool Checked(int control) const;

        void SetChecked(int control, bool checked);

        bool ResultPeek() const { return Checked(0x428); }

        static ClientPreferences Load(const std::filesystem::path &config);
    };

    std::string PreferenceString(const std::filesystem::path &config, std::string_view section,
                                 std::string_view key, std::string_view fallback = {});

    int PreferenceInt(const std::filesystem::path &config, std::string_view section,
                      std::string_view key, int fallback = 0);

    void WritePreference(const std::filesystem::path &config, std::string_view section,
                         std::string_view key, const char *value);

    inline void WritePreference(const std::filesystem::path &config, std::string_view section,
                                std::string_view key, const std::string &value) {
        WritePreference(config, section, key, value.c_str());
    }

    void WritePreferenceInt(const std::filesystem::path &config, std::string_view section,
                            std::string_view key, int value);
} // namespace tenhou
