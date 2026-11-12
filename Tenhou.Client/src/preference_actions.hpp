#pragma once
#include "client_preferences.hpp"
#include "online_actions.hpp"

namespace tenhou {
// FUN_0047a... WM_LBUTTONDBLCLK / WM_RBUTTONUP: pass takes precedence
// when its original button is present; otherwise use the currently drawn tile.
inline std::size_t PreferenceShortcut(const ClientPreferences& preferences, bool double_click,
                                     const OnlineState& state, const ActionSnapshot& actions) {
    constexpr auto none=static_cast<std::size_t>(-1);
    if (!actions.enabled || !actions.pending.empty() || !state.initialized || state.ended || state.awaiting_context) return none;
    bool pass=false;
    for (const auto& action:actions.choices) if (action.kind=="skip") pass=true;
    const int control=pass?(double_click?0x426:0x425):(double_click?0x424:0x423);
    if (!preferences.Checked(control)) return none;
    if (!pass && (state.turn!=0 || !state.players[0].draw_pending || state.players[0].drawn_tile<0)) return none;
    for (std::size_t i=0;i<actions.choices.size();++i) {
        const auto& a=actions.choices[i];
        if (pass?a.kind=="skip":(a.kind=="discard" && a.tiles==std::vector<int>{state.players[0].drawn_tile})) return i;
    }
    return none;
}
} // namespace tenhou
