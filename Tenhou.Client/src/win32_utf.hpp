#pragma once
#include <filesystem>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <windows.h>

namespace tenhou {
    inline std::wstring Utf8ToWide(const std::string_view utf8) {
        if (utf8.empty()) return {};
        const int length = MultiByteToWideChar(
            CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
        if (length <= 0) return {};
        std::wstring result(static_cast<std::size_t>(length), L'\0');
        MultiByteToWideChar(
            CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), result.data(), length);
        return result;
    }

    inline std::string WideToUtf8(const std::wstring_view wide) {
        if (wide.empty()) return {};
        const int length = WideCharToMultiByte(
            CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
        if (length <= 0) return {};
        std::string result(static_cast<std::size_t>(length), '\0');
        WideCharToMultiByte(
            CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), result.data(), length, nullptr, nullptr);
        return result;
    }

    inline std::filesystem::path PathFromUtf8(const std::string_view utf8) {
        return std::filesystem::path(Utf8ToWide(utf8));
    }

    inline std::string PathToUtf8(const std::filesystem::path &path) {
        return WideToUtf8(path.wstring());
    }
} // namespace tenhou
#else
namespace tenhou {
    inline std::filesystem::path PathFromUtf8(std::string_view utf8) {
        return std::filesystem::path(utf8);
    }

    inline std::string PathToUtf8(const std::filesystem::path &path) {
        return path.string();
    }
} // namespace tenhou
#endif
