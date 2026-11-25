#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace tenhou {
    // Cross-platform INI persistence compatible with Win32 GetPrivateProfile* files
    // (supports UTF-8, UTF-8 BOM, and legacy UTF-16 LE BOM files; case-insensitive ASCII
    // section/key matching; preserves unknown lines and comments; atomic write-through replace).
    std::string ReadIniString(const std::filesystem::path &path, std::string_view section,
                              std::string_view key, std::string_view fallback = {});

    int ReadIniInt(const std::filesystem::path &path, std::string_view section,
                   std::string_view key, int fallback = 0);

    void WriteIniString(const std::filesystem::path &path, std::string_view section,
                        std::string_view key, std::optional<std::string_view> value);

    void WriteIniInt(const std::filesystem::path &path, std::string_view section,
                     std::string_view key, int value);

    // Drop in-memory cache for a path (useful if external tools edited the file).
    void InvalidateIniCache(const std::filesystem::path &path = {});
} // namespace tenhou
