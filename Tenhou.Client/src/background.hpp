#pragma once
#include "resource_pack.hpp"
#include "client_preferences.hpp"

namespace tenhou {
    std::uint32_t AdjustOriginalHsv(std::uint32_t colorref, float hue, float saturation, float value);

    Image BuildPreferredBackground(const std::filesystem::path &directory, const std::filesystem::path &config,
                                   int width, int height, const ClientPreferences &preferences);

    // Restored offline background pipeline, FUN_00468130 / FUN_00408000.
    // GDI+ must already be initialized by the calling application.
    Image BuildBackground(const std::filesystem::path &source, int width, int height,
                          float light_strength = 8.0f, float light_y = 600.0f,
                          const std::array<std::array < float, 3>, 3>* hsv = nullptr);

    void ApplyBackgroundLighting(Image &image, float strength = 8.0f, float light_y = 600.0f);
} // namespace tenhou
