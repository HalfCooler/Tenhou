#pragma once
#include <cstdint>

namespace tenhou {
    inline constexpr int kHdFaceCellWidth = 384;
    inline constexpr int kHdFaceCellHeight = 512;
    inline constexpr int kHdFaceAtlasCols = 8;
    inline constexpr int kHdFaceAtlasRows = 6;
    inline constexpr int kHdFaceSliceCount = 44;

    // Maps TileGeometryCatalog variant (bits 16..23 of ImageKey) to Texture2DArray slice [0..43].
    // Returns -1 for blank white dragon (34), tile back (37), masks (38/39), or invalid variants.
    inline constexpr int HdTileFaceSlice(unsigned variant) {
        if (variant <= 33) return static_cast<int>(variant);
        if (variant == 35) return 34; // Green dragon (6z)
        if (variant == 36) return 35; // Red dragon (7z)
        if (variant >= 40 && variant <= 47) return static_cast<int>(variant - 40 + 36); // Flowers 136..143
        return -1;
    }

    inline constexpr const char *kHdFaceSvgNames[kHdFaceSliceCount]{
        "0m.svg", "1m.svg", "2m.svg", "3m.svg", "4m.svg", "5m.svg", "6m.svg", "7m.svg", "8m.svg", "9m.svg",
        "0p.svg", "1p.svg", "2p.svg", "3p.svg", "4p.svg", "5p.svg", "6p.svg", "7p.svg", "8p.svg", "9p.svg",
        "0s.svg", "1s.svg", "2s.svg", "3s.svg", "4s.svg", "5s.svg", "6s.svg", "7s.svg", "8s.svg", "9s.svg",
        "1z.svg", "2z.svg", "3z.svg", "4z.svg", "6z.svg", "7z.svg",
        "mei.svg", "lan.svg", "zu.svg", "ju.svg", "chun.svg", "xia.svg", "qiu.svg", "dong.svg"
    };
} // namespace tenhou
