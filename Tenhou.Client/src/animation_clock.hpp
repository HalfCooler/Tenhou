#pragma once
#include <chrono>
#include <cstdint>

namespace tenhou {
    // Milliseconds as a real number backed by steady_clock (QPC on Windows), zero <windows.h>.
    inline double AnimationNow() {
        using namespace std::chrono;
        return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
    }
} // namespace tenhou
