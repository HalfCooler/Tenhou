#pragma once
#include <windows.h>
#include <cstdint>

namespace tenhou {
    inline std::int64_t QpcTicks() {
        LARGE_INTEGER n{};
        QueryPerformanceCounter(&n);
        return n.QuadPart;
    }

    inline std::int64_t QpcFrequency() {
        static const auto frequency = [] {
            LARGE_INTEGER n{};
            QueryPerformanceFrequency(&n);
            return n.QuadPart;
        }();
        return frequency;
    }

    // Milliseconds as a real number: no GetTickCount64 or integer-ms quantization.
    inline double AnimationNow() { return double(QpcTicks()) * 1000.0 / double(QpcFrequency()); }
} // namespace tenhou
