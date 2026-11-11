#pragma once
#include "settlement.hpp"
#include "wall_state.hpp"
#include "game_seats.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace tenhou {
enum class ReplayEventKind { Metadata, Init, Draw, Discard, Meld, Reach, Dora, Win, DrawEnd };
enum class MeldKind { Chi, Pon, OpenKan, ClosedKan, AddedKan, Nuki };
struct ReplayMeld {
    MeldKind kind = MeldKind::Chi;
    std::vector<int> tiles;
    int called_tile = -1;
    int from = -1; // Absolute source seat; same seat for a closed kan.
    std::uint32_t encoded = 0;
};
struct ReplayDiscard { int tile = -1; bool claimed = false; bool riichi = false; bool tsumogiri = false; };
struct ReplayPlayer {
    std::string name;
    char gender = '?';
    bool connected = true;
    std::vector<int> hand;
    std::vector<int> deal_hand; // INIT order, retained only for the opening presentation.
    std::vector<ReplayDiscard> discards;
    std::vector<ReplayMeld> melds;
    std::vector<int> nuki; // Public extracted entities; never a three-tile meld.
    int score = 25000;
    int reach = 0; // 0 none, 1 declared, 2 accepted.
    int drawn_tile = -1;
};
struct ReplayState {
    WallState wall;
    std::array<ReplayPlayer, 4> players;
    bool initialized = false, ended = false;
    bool furiten = false;
    unsigned type = 0;
    unsigned seat_mask = 15;
    std::vector<int> red_tiles{16,52,88};
    int round = 0, honba = 0, sticks = 0, dealer = 0, turn = -1;
    int last_discard_seat = -1;
    std::vector<int> dora_indicators;
    std::string result, last_event;
    std::vector<int> winners;
    SettlementState settlement;
    bool relative_scores = false; // HUD projection only.
    unsigned draw_end_slots = 14; // Presentation projection only; not protocol state.
};
struct ReplayEvent {
    ReplayEventKind kind = ReplayEventKind::Metadata;
    std::string tag;
    std::map<std::string, std::string> attributes;
    int who = -1, tile = -1;
    std::size_t source_offset = 0;
};
struct ReplayLog {
    std::vector<ReplayEvent> events;
    std::string source;
    static ReplayLog Load(const std::filesystem::path& path);
    static ReplayLog Parse(const std::string& xml, const std::string& source = "memory");
};
class ReplaySession {
public:
    explicit ReplaySession(ReplayLog log);
    bool Step(); // False at EOF; failed steps preserve state and cursor.
    void Reset();
    void Seek(std::size_t applied_events); // Rebuilds deterministically; bounds checked.
    const ReplayState& state() const { return state_; }
    const ReplayLog& log() const { return log_; }
    std::size_t position() const { return position_; }
private:
    ReplayLog log_;
    ReplayState state_;
    std::size_t position_ = 0;
};
// For AddedKan, the decoder's called_tile identifies the added tile (original
// FUN_0044d880); the session keeps the existing pon's historical called tile.
ReplayMeld DecodeReplayMeld(std::uint32_t encoded, int who);
char SoundGender(const std::string& seats, int who);
std::string TileLabel(int tile);
const char* MeldKindName(MeldKind kind);
std::string ReplayStateJson(const ReplayState& state);
} // namespace tenhou
