#pragma once
#include <chrono>
#include <cstdint>

namespace tenhou {
    inline std::uint64_t MonotonicMs() {
        using namespace std::chrono;
        return static_cast<std::uint64_t>(
            duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
    }
} // namespace tenhou
