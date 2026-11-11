#pragma once
#include <utility>

namespace tenhou {
// Presentation metadata only. No wall tile identities or future draws are stored.
struct WallState {
    bool known = false, pending_kan = false;
    int offset = 0, front = 52, replacements = 0;
    int initial_removed = 0, replacement_limit = 4, indicator_index = 130;
    const char* unavailable = "snapshot lacks draw history";
    void Init(int dealer, int die0, int die1, unsigned type = 0) {
        *this = {};
        // seed dice are zero based. The original special (6,0) mode has a
        // different per-seat wall and is deliberately outside this four-wall model.
        if (die0 < 0 || die0 > 5 || die1 < 0 || die1 > 5) {
            unavailable = "unsupported wall seed"; return;
        }
        offset = (650 - dealer * 34 - (die0 + die1) * 32) % 136;
        // 004938b4 / 0048c...: remove 136-108 before dealing three hands.
        // 0049cd70: sanma reserves eight replacement slots before dora.
        if (type & 16) {
            initial_removed = 28; front = initial_removed + 39;
            replacement_limit = 8; indicator_index = 126;
        }
        known = true; unavailable = "";
    }
    void Kan() { if (known) pending_kan = true; }
    void Draw() {
        if (!known) return;
        if (pending_kan) { ++replacements; pending_kan = false; }
        else ++front;
        if (replacements > replacement_limit || front + replacements > 136-replacement_limit-10) {
            known = false; unavailable = "wall history exceeds supported bounds";
        }
    }
    // Original FUN_0048f140: return direction and individual (top/bottom) slot.
    std::pair<int, int> Position(int index) const {
        const int p = offset + index;
        // ASM signed reciprocal division returns -trunc(p/34), then masks 3.
        return {(4 - (p / 34) % 4) % 4, (33 - p % 34) ^ 1};
    }
};
} // namespace tenhou
