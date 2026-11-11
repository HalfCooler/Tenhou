#include "client_preferences.hpp"
#include <algorithm>
#include <stdexcept>

namespace tenhou {
    bool ClientPreferences::Checked(int control) const {
        for (const auto &item: kPreferenceFlags)
            if (item.control == control)
                return bool(flags & item.mask) != item.inverted;
        throw std::runtime_error("Unknown original preference control");
    }

    void ClientPreferences::SetChecked(int control, bool checked) {
        for (const auto &item: kPreferenceFlags)
            if (item.control == control) {
                flags = (flags & ~item.mask) | (checked != item.inverted ? item.mask : 0);
                return;
            }
        throw std::runtime_error("Unknown original preference control");
    }

    ClientPreferences ClientPreferences::Load(const std::filesystem::path &config) {
        ClientPreferences value;
        value.flags = GetPrivateProfileIntW(L"STATUS", L"FLAGS0", 0x402d, config.c_str());
        value.hidden_id = GetPrivateProfileIntW(L"STATUS", L"HIDDENID", 0, config.c_str()) != 0;
        value.tile_shadow = std::clamp(
            static_cast<int>(GetPrivateProfileIntW(L"RESOURCE", L"HAI_SDW", 6, config.c_str())), 0, 10);
        // The Win32 trackbar limits the effective original UI range to 0..100.
        value.se_volume = std::clamp(static_cast<int>(GetPrivateProfileIntW(
                                         L"RESOURCE", L"SE_VOL", 100, config.c_str())), 0, 100);
        const wchar_t *keys[]{L"TileRed", L"TileGreen", L"TileBlue"};
        for (unsigned i = 0; i < 3; ++i)
            value.tile_rgb[i] = std::clamp(static_cast<int>(GetPrivateProfileIntW(
                                               L"REBUILD", keys[i], value.tile_rgb[i], config.c_str())), 0, 256);
        const auto lhsv = PreferenceString(config, L"RESOURCE", L"BG_LHSV", L"2,0,255,255,,,,,,");
        std::size_t start = 0;
        for (unsigned i = 0; i < 10 && start <= lhsv.size(); ++i) {
            const auto end = lhsv.find(L',', start);
            const auto text = lhsv.substr(start, end == std::wstring::npos ? end : end - start);
            // Empty fields intentionally disable the corresponding original edit.
            value.background_lhsv[i] = text;
            if (end == std::wstring::npos) break;
            start = end + 1;
        }
        value.background_source = PreferenceString(config, L"RESOURCE", L"BG_SRC");
        return value;
    }

    std::wstring ClientPreferences::BackgroundLhsv() const {
        std::wstring value;
        for (unsigned i = 0; i < background_lhsv.size(); ++i) {
            if (i) value += L',';
            value += background_lhsv[i];
        }
        return value;
    }

    std::array<float, 3> ClientPreferences::TileColor() const {
        // ASM 00417959/83/ab: float(1/255), stored after integer multiplication.
        return {
            float(tile_rgb[0] * double(1.0f / 255.0f)), float(tile_rgb[1] * double(1.0f / 255.0f)),
            float(tile_rgb[2] * double(1.0f / 255.0f))
        };
    }

    std::wstring PreferenceString(const std::filesystem::path &config, const wchar_t *section,
                                  const wchar_t *key, const wchar_t *fallback) {
        std::wstring result(32768, L'\0');
        const auto length = GetPrivateProfileStringW(section, key, fallback, result.data(),
                                                     static_cast<DWORD>(result.size()), config.c_str());
        if (length >= result.size() - 2) throw std::runtime_error("Preference value too long");
        result.resize(length);
        return result;
    }

    void WritePreference(const std::filesystem::path &config, const wchar_t *section,
                         const wchar_t *key, const wchar_t *value) {
        if (!WritePrivateProfileStringW(section, key, value, config.c_str()))
            throw std::runtime_error("Cannot write client preferences (Win32 " + std::to_string(GetLastError()) + ")");
    }

    void WritePreferenceInt(const std::filesystem::path &config, const wchar_t *section,
                            const wchar_t *key, int value) {
        const auto text = std::to_wstring(value);
        WritePreference(config, section, key, text.c_str());
    }
} // namespace tenhou
