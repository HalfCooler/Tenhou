#include "storage.hpp"
#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <fstream>
#include <array>
#include <vector>
#include <stdexcept>

namespace authority {
    std::string secret() {
        std::array < unsigned char, 32 > bytes{};
        if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) <
            0) throw std::runtime_error("Secure random failed");
        std::string r;
        for (auto b: bytes) {
            r += "0123456789abcdef"[b >> 4];
            r += "0123456789abcdef"[b & 15];
        }
        return r;
    }

    std::string read_state(const std::filesystem::path &path) {
        auto read = [](const std::filesystem::path &p) {
            auto size = std::filesystem::file_size(p);
            if (size > 64 * 1024 * 1024) throw std::runtime_error("State exceeds limit");
            std::ifstream file(p, std::ios::binary);
            std::vector<BYTE> bytes(static_cast<std::size_t>(size));
            if (!file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) throw
                    std::runtime_error("State read failed");
            DATA_BLOB in{static_cast<DWORD>(bytes.size()), bytes.data()}, out{};
            if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out)) throw
                    std::runtime_error("State integrity/decryption failed");
            std::string text(reinterpret_cast<char *>(out.pbData), out.cbData);
            LocalFree(out.pbData);
            return text;
        };
        if (!std::filesystem::exists(path)) {
            if (std::filesystem::exists(path.wstring() + L".bak")) throw std::runtime_error(
                "Primary state missing; explicit backup recovery required");
            return {};
        }
        // Do not silently roll back acknowledged commands. Recovery from .bak is an
        // explicit operator action described in the deployment guide.
        return read(path);
    }

    void write_state(const std::filesystem::path &path, const std::string &text) {
        try {
            if (text.size() > 48 * 1024 * 1024) throw std::runtime_error("State capacity reached");
            std::filesystem::create_directories(path.parent_path());
            DATA_BLOB in{
                static_cast<DWORD>(text.size()), reinterpret_cast<BYTE *>(const_cast<char *>(text.data()))
            }, out{};
            if (!CryptProtectData(&in, L"Tenhou authority v26", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN,
                                  &out)) throw std::runtime_error("State encryption failed");
            const auto temporary = path.wstring() + L".tmp";
            HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE) {
                LocalFree(out.pbData);
                throw std::runtime_error("State temporary open failed");
            }
            DWORD written = 0;
            bool ok = WriteFile(file, out.pbData, out.cbData, &written, nullptr) && written == out.cbData &&
                      FlushFileBuffers(file);
            CloseHandle(file);
            LocalFree(out.pbData);
            if (!ok) throw std::runtime_error("State flush failed");
            if (std::filesystem::exists(path)) {
                const auto backup = path.wstring() + L".bak";
                if (!ReplaceFileW(path.c_str(), temporary.c_str(), backup.c_str(), 0, nullptr, nullptr)) throw
                        std::runtime_error("State atomic replacement failed");
            } else if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH)) throw std::runtime_error(
                "State atomic creation failed");
        } catch (const std::exception &e) { throw StorageFailure(e.what()); }
    }
}
