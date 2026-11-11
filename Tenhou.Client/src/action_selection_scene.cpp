#include "action_selection_scene.hpp"
#include "table_scene.hpp"
#include <algorithm>

namespace tenhou {
std::string ActionGroup(const std::string& kind) { return kind.find("kan") != std::string::npos ? "kan" : kind; }
std::vector<CandidateTile> BuildCandidateTiles(const OnlineAction& choice, const OnlineState& game) {
    std::vector<CandidateTile> result; for (int tile : choice.tiles) result.push_back({tile,false});
    if ((choice.kind == "chi" || choice.kind == "pon" || choice.kind == "open_kan") && game.last_discard_seat > 0) {
        const auto& river = game.players[game.last_discard_seat].discards;
        if (!river.empty()) result.push_back({river.back().tile,true});
    }
    if (choice.kind == "added_kan" && !choice.tiles.empty()) for (const auto& meld : game.players[0].melds)
        if (meld.kind == MeldKind::Pon && meld.tiles.front()/4 == choice.tiles.front()/4)
            for (int tile : meld.tiles) result.push_back({tile,true});
    std::stable_sort(result.begin(),result.end(),[](const CandidateTile& a,const CandidateTile& b) { return a.tile < b.tile; });
    return result;
}
std::vector<ActionPlacement> BuildActionPlacements(const ActionSnapshot& actions, int width, POINT anchor) {
    const char* kinds[]{"discard", "chi", "pon", "kan", "riichi", "ron", "tsumo", "draw", "skip", "next", "nuki"};
    std::vector<int> buttons;
    if (!actions.enabled) return {};
    for (int i : {1,2,3,4,10,6,5,7,8,9})
        if (std::any_of(actions.choices.begin(), actions.choices.end(), [&](const OnlineAction& a) { return ActionGroup(a.kind) == kinds[i]; }))
            buttons.push_back(i==10 ? 5151 : 5120+i);
    const double scale = width == 1440 ? 1.4 : 1.0;
    int x = std::max(0, static_cast<int>(anchor.x + 65.0 - (static_cast<int>(buttons.size()) - 1) * 110 * scale));
    const int y = static_cast<int>(anchor.y - 45 * scale);
    std::vector<ActionPlacement> result;
    for (int button : buttons) {
        result.push_back({button, {x, y, x + static_cast<int>(100 * scale), y + static_cast<int>(36 * scale)}});
        x = static_cast<int>(x + 110 * scale);
    }
    return result;
}
void ActionSelection::Clear() { group.clear(); indices.clear(); selected.clear(); }
void ActionSelection::Sync(const ActionSnapshot& actions) {
    if (!actions.enabled || actions.revision != revision || !actions.pending.empty()) Clear();
    revision = actions.revision;
}
void ActionSelection::Open(const ActionSnapshot& actions, const std::string& wanted) {
    Clear(); revision = actions.revision;
    if (!actions.enabled || !actions.pending.empty()) return;
    for (std::size_t i = 0; i < actions.choices.size(); ++i)
        if (ActionGroup(actions.choices[i].kind) == wanted) indices.push_back(i);
    if (!indices.empty()) group = wanted;
}
std::size_t ActionSelection::UniqueCall(const ActionSnapshot& actions, bool red) const {
    const auto none = static_cast<std::size_t>(-1);
    if (!HandSelection() || !actions.enabled || revision != actions.revision ||
        !actions.pending.empty() || indices.empty()) return none;
    const auto signature = [red](const OnlineAction& choice) {
        // Ordinary copies are interchangeable; red fives remain distinct.
        // Keep the original physical IDs in the action submitted to the server.
        std::vector<int> result;
        for (int tile : choice.tiles) result.push_back(TileVariant(tile,red));
        std::sort(result.begin(),result.end());
        return result;
    };
    if (indices.front() >= actions.choices.size()) return none;
    const auto first = signature(actions.choices[indices.front()]);
    for (auto index : indices)
        if (index >= actions.choices.size() || signature(actions.choices[index]) != first) return none;
    return indices.front();
}
std::vector<int> ActionSelection::Selectable(const ActionSnapshot& actions) const {
    std::vector<int> result;
    if (!HandSelection() || revision != actions.revision || !actions.pending.empty()) return result;
    for (auto index : indices) {
        if (index >= actions.choices.size()) continue;
        const auto& tiles = actions.choices[index].tiles;
        if (!std::all_of(selected.begin(), selected.end(), [&](int tile) {
            return std::find(tiles.begin(), tiles.end(), tile) != tiles.end();
        })) continue;
        for (int tile : tiles) if (std::find(result.begin(), result.end(), tile) == result.end()) result.push_back(tile);
    }
    return result;
}
std::size_t ActionSelection::Pick(const ActionSnapshot& actions, int tile) {
    const auto none = static_cast<std::size_t>(-1);
    const auto allowed = Selectable(actions);
    if (std::find(allowed.begin(), allowed.end(), tile) == allowed.end()) return none;
    const auto old = std::find(selected.begin(), selected.end(), tile);
    if (old != selected.end()) { selected.erase(old); return none; }
    selected.push_back(tile);
    for (auto index : indices) {
        const auto& tiles = actions.choices[index].tiles;
        if (tiles.size() == selected.size() && std::all_of(selected.begin(), selected.end(), [&](int value) {
            return std::find(tiles.begin(), tiles.end(), value) != tiles.end();
        })) return index;
    }
    return none;
}
} // namespace tenhou
