#include "client_preferences.hpp"
#include "ini_store.hpp"
#include <algorithm>
#include <stdexcept>

namespace tenhou {
    bool ClientPreferences::Checked(const int control) const {
        for (const auto &item: kPreferenceFlags)
            if (item.control == control)
                return static_cast<bool>(flags & item.mask) != item.inverted;
        throw std::runtime_error("Unknown original preference control");
    }

    void ClientPreferences::SetChecked(const int control, const bool checked) {
        for (const auto &item: kPreferenceFlags)
            if (item.control == control) {
                flags = (flags & ~item.mask) | (checked != item.inverted ? item.mask : 0);
                return;
            }
        throw std::runtime_error("Unknown original preference control");
    }

    ClientPreferences ClientPreferences::Load(const std::filesystem::path &config) {
        ClientPreferences value;
        value.flags = static_cast<std::uint32_t>(ReadIniInt(config, "STATUS", "FLAGS0", 0x402d));
        value.hidden_id = ReadIniInt(config, "STATUS", "HIDDENID", 0) != 0;
        value.tile_shadow = std::clamp(ReadIniInt(config, "RESOURCE", "HAI_SDW", 6), 0, 10);
        // The Win32 trackbar limits the effective original UI range to 0..100.
        value.se_volume = std::clamp(ReadIniInt(config, "RESOURCE", "SE_VOL", 100), 0, 100);
        const char *keys[]{"TileRed", "TileGreen", "TileBlue"};
        for (unsigned i = 0; i < 3; ++i)
            value.tile_rgb[i] = std::clamp(ReadIniInt(config, "REBUILD", keys[i], value.tile_rgb[i]), 0, 256);
        const auto lhsv = ReadIniString(config, "RESOURCE", "BG_LHSV", "2,0,255,255,,,,,,");
        std::size_t start = 0;
        for (unsigned i = 0; i < 10 && start <= lhsv.size(); ++i) {
            const auto end = lhsv.find(',', start);
            const auto text = lhsv.substr(start, end == std::string::npos ? end : end - start);
            // Empty fields intentionally disable the corresponding original edit.
            value.background_lhsv[i] = text;
            if (end == std::string::npos) break;
            start = end + 1;
        }
        value.background_source = ReadIniString(config, "RESOURCE", "BG_SRC");
        return value;
    }

    std::string ClientPreferences::BackgroundLhsv() const {
        std::string value;
        for (unsigned i = 0; i < background_lhsv.size(); ++i) {
            if (i) value += ',';
            value += background_lhsv[i];
        }
        return value;
    }

    std::array<float, 3> ClientPreferences::TileColor() const {
        // ASM 00417959/83/ab: float(1/255), stored after integer multiplication.
        return {
            static_cast<float>(tile_rgb[0] * static_cast<double>(1.0f / 255.0f)), static_cast<float>(tile_rgb[1] * static_cast<double>(1.0f / 255.0f)),
            static_cast<float>(tile_rgb[2] * static_cast<double>(1.0f / 255.0f))
        };
    }

    std::string PreferenceString(const std::filesystem::path &config, const std::string_view section,
                                 const std::string_view key, const std::string_view fallback) {
        auto result = ReadIniString(config, section, key, fallback);
        if (result.size() >= 32766) throw std::runtime_error("Preference value too long");
        return result;
    }

    int PreferenceInt(const std::filesystem::path &config, const std::string_view section,
                      const std::string_view key, const int fallback) {
        return ReadIniInt(config, section, key, fallback);
    }

    void WritePreference(const std::filesystem::path &config, const std::string_view section,
                         const std::string_view key, const char *value) {
        WriteIniString(config, section, key,
                       value ? std::optional<std::string_view>(value) : std::nullopt);
    }

    void WritePreferenceInt(const std::filesystem::path &config, const std::string_view section,
                            const std::string_view key, const int value) {
        WriteIniInt(config, section, key, value);
    }
} // namespace tenhou
