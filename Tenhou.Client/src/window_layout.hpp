#pragma once
#include <algorithm>
#include <array>
#include <string_view>

namespace tenhou {
    // Four native 4:3 table artwork tiers:
    //   1024x768  (Windows origin SD)
    //   1440x1080 (Windows origin HD)
    //   1920x1440 (C-EGG CDN 2K 4:3)
    //   2640x1980 (C-EGG CDN Desktop4K 4:3)
    inline constexpr std::array<int, 4> kTableCanvasWidths{1024, 1440, 1920, 2640};
    inline constexpr int kDefaultClientWidth = 1440;
    inline constexpr int kDefaultClientHeight = 1080;
    inline constexpr int kMinClientWidth = 640;
    inline constexpr int kMinClientHeight = 480;

    inline bool IsSupportedTableWidth(const int width) {
        return width == 1024 || width == 1440 || width == 1920 || width == 2640;
    }

    // Legacy 1024 uses SD UI metrics; 1440/1920/2640 share the 1440 HD UI design space
    // scaled linearly by width / 1440.0.
    inline int UiDesignWidth(const int table_width) {
        return table_width == 1024 ? 1024 : 1440;
    }

    inline double UiDesignScale(const int table_width) {
        return table_width == 1024 ? 1.0 : 1.4 * (static_cast<double>(table_width) / 1440.0);
    }

    inline int SelectAutoTableWidth(const int target_width, const int target_height) {
        const int fit_w = std::min(target_width, target_height * 4 / 3);
        if (fit_w <= 1024) return 1024;
        if (fit_w <= 1440) return 1440;
        if (fit_w <= 1920) return 1920;
        return 2640;
    }

    // Parses original [STATUS] SCREENSIZE values ("default", "1024x768", "1440x1080", "1920x1440", "2640x1980").
    // Returns 0 for "default" / empty / unrecognized (automatic selection).
    inline int ParseScreenSizeSetting(const std::string_view value) {
        if (value == "1024x768") return 1024;
        if (value == "1440x1080") return 1440;
        if (value == "1920x1440") return 1920;
        if (value == "2640x1980") return 2640;
        return 0;
    }

    inline const char *FormatScreenSizeSetting(const int table_width) {
        switch (table_width) {
            case 1024: return "1024x768";
            case 1440: return "1440x1080";
            case 1920: return "1920x1440";
            case 2640: return "2640x1980";
            default: return "default";
        }
    }
} // namespace tenhou
