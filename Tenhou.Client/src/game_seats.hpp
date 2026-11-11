#pragma once
#include <array>
#include <stdexcept>

namespace tenhou {
    // Original keeps four physical display directions, including the absent seat.
    inline bool SeatPresent(unsigned mask, int seat) { return seat >= 0 && seat < 4 && (mask & (1u << seat)) != 0; }

    inline int SeatCount(unsigned mask) {
        int count = 0;
        for (int seat = 0; seat < 4; ++seat) if (SeatPresent(mask, seat)) ++count;
        return count;
    }

    inline int NextSeat(unsigned mask, int seat) {
        for (int step = 1; step <= 4; ++step) if (SeatPresent(mask, (seat + step) % 4)) return (seat + step) % 4;
        throw std::runtime_error("No active player seats");
    }

    inline int SeatWind(unsigned mask, int dealer, int seat) {
        if (!SeatPresent(mask, dealer) || !SeatPresent(mask, seat)) return 4;
        int wind = 0;
        for (int current = dealer; current != seat; current = NextSeat(mask, current)) ++wind;
        return wind;
    }

    inline unsigned NamedSeats(const std::array<bool, 4> &named) {
        unsigned mask = 0;
        for (int i = 0; i < 4; ++i) if (named[i]) mask |= 1u << i;
        return mask;
    }
} // namespace tenhou
