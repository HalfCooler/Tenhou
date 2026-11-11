#pragma once
#include <windows.h>
#include <filesystem>
#include <string>
#include <cstdint>
#include <array>

namespace tenhou {
// Original STATUS/FLAGS0, FUN_0041d830. A checkbox is not always a set bit.
struct PreferenceFlag { int control; std::uint32_t mask; bool inverted; };
inline constexpr PreferenceFlag kPreferenceFlags[]{
    {0x423,0x20,false}, {0x425,0x200,true}, {0x424,0x400,true},
    {0x426,0x800,true}, {0x427,0x8000,false}, {0x428,0x10000,true}};
struct ClientPreferences {
    std::uint32_t flags = 0x402d;
    bool hidden_id = false;
    int se_volume = 100;
    int tile_shadow = 6;
    std::array<int,3> tile_rgb{0,0,38};
    std::array<std::wstring,10> background_lhsv{L"2",L"0",L"255",L"255",L"",L"",L"",L"",L"",L""};
    std::wstring background_source;
    std::wstring BackgroundLhsv() const;
    std::array<float,3> TileColor() const;
    bool Checked(int control) const;
    void SetChecked(int control, bool checked);
    bool ResultPeek() const { return Checked(0x428); }
    static ClientPreferences Load(const std::filesystem::path& config);
};
std::wstring PreferenceString(const std::filesystem::path& config, const wchar_t* section,
                              const wchar_t* key, const wchar_t* fallback = L"");
void WritePreference(const std::filesystem::path& config, const wchar_t* section,
                     const wchar_t* key, const wchar_t* value);
void WritePreferenceInt(const std::filesystem::path& config, const wchar_t* section,
                        const wchar_t* key, int value);
} // namespace tenhou
