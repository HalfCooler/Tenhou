#pragma once
#include <filesystem>
#include <string>
#include <stdexcept>

namespace authority {
    struct StorageFailure : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    std::string secret();

    std::string read_state(const std::filesystem::path &path);

    void write_state(const std::filesystem::path &path, const std::string &text);
}
