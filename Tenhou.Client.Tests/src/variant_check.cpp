#include "variant_state.hpp"
#include "network_protocol.hpp"
#include "action_selection_scene.hpp"
#include "wall_scene.hpp"
#include "settlement_scene.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <set>

using namespace tenhou;
namespace {
    void Check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
    template<class F> void Reject(F f, const char *message) {
        try { f(); } catch (const std::runtime_error &) { return; }
        throw std::runtime_error(message);
    }
    std::map<std::string, std::string> Fixture() {
        std::map<std::string, std::string> f{{"rule", "sichuan-bloodbattle"}, {"phase", "turn"}, {"seat", "2"},
            {"turn", "2"}, {"dealer", "3"}, {"hand_number", "6"}, {"total_hands", "8"}, {"wall", "55"},
            {"wall_total", "108"}, {"wall_front", "52"}, {"wall_tail", "1"}, {"scores", "50000,49000,51000,50000"},
            {"hand2", "0,1,4,8,12,36,40,44,48,72,76,80,84,88"}, {"drawn2", "88"}, {"hidden2", "14"}};
        for (int i: {0, 1, 3}) f["hidden" + std::to_string(i)] = "13";
        return f;
    }
}
int main() {
    try {
        auto fields = Fixture();
        auto state = ParseVariantState(fields);
        auto game = VariantTableState(state, {{"n2", "local"}});
        Check(game.dealer == 1 && game.round == 5 && game.turn == 0 && game.players[0].drawn_tile == 88, "Seat/dealer/draw projection failed");
        Check(game.players[1].known_hand.empty() && game.players[1].hidden_count == 13, "Hidden entities inferred");
        auto malformed = fields; malformed["hand2"] = "0,0";
        Reject([&] { ParseVariantState(malformed); }, "Duplicate physical entities accepted");
        malformed = fields; malformed["seat"] = "4";
        Reject([&] { ParseVariantState(malformed); }, "Invalid seat accepted");
        malformed = fields; malformed["wall_tail"] = "2";
        Reject([&] { ParseVariantState(malformed); }, "Invalid wall cursor accepted");
        malformed = fields; malformed["drawn2"] = "107";
        Reject([&] { ParseVariantState(malformed); }, "Unowned drawn tile accepted");
        malformed = fields; malformed["river1"] = "108";
        Reject([&] { ParseVariantState(malformed); }, "Sichuan honor accepted");
        malformed = fields; malformed["phase"] = "missing"; malformed["missing2"] = "1";
        Check(VariantTableState(ParseVariantState(malformed), {}).players[0].missing_suit == -1, "Dingque marker appeared before public reveal");
        VariantEventCursor cursor;
        VariantState cue_state; cue_state.event_seq = 1; cue_state.events = {{1, "discard", 0, 4, -1}};
        Check(cursor.Consume(cue_state, false).empty(), "Initial snapshot replayed events");
        cue_state.event_seq = 2; cue_state.events.push_back({2, "draw", 0, 8, -1});
        Check(cursor.Consume(cue_state, false).size() == 1 && cursor.Consume(cue_state, false).empty(), "Repeated event played twice");
        cue_state.event_seq = 3; cue_state.events.push_back({3, "pon", 1, 8, 0});
        Check(cursor.Consume(cue_state, true).empty() && cursor.Consume(cue_state, false).empty(), "Recovery replayed effects");
        cue_state.event_seq = 40; cue_state.events = {{40, "draw", 0, 12, -1}};
        Check(cursor.Consume(cue_state, false).empty() && cursor.resynchronized, "Event gap did not align to snapshot");
        auto offers = ParseVariantActions("discard:0;discard:1;dingque:2;exchange:0,1,4;exchange:0,4,8", "table", 7, 108);
        Check(offers[0].tiles == std::vector<int>{0} && offers[1].tiles == std::vector<int>{1} && offers[2].tiles.empty() && offers[2].suit == 2, "Action parameters lost their types");
        Check(ParseNetworkMessage(offers[1].messages[0]).attributes.at("tiles") == "1", "Clicked entity changed on wire");
        Reject([] { ParseVariantActions("exchange:0,0,4", "t", 1, 108); }, "Invalid exchange accepted");
        Reject([] { ParseVariantActions("dingque:3", "t", 1, 108); }, "Invalid suit accepted");
        ActionSnapshot actions; actions.enabled = true; actions.revision = 4; actions.choices = offers;
        ActionSelection selection; selection.Open(actions, "exchange");
        selection.Pick(actions, 0); selection.Pick(actions, 1);
        Check(selection.Selectable(actions) == std::vector<int>({0, 1, 4}), "Exchange candidates escaped legal combinations");
        Check(selection.Pick(actions, 4) == static_cast<std::size_t>(-1) && selection.Confirm(actions) == 3, "Exchange skipped explicit confirmation");
        selection.Pick(actions, 1); selection.Pick(actions, 8);
        Check(selection.Confirm(actions) == 4, "Exchange physical selection was replaced");
        ++actions.revision; selection.Sync(actions); Check(!selection.Active(), "Stale selection survived revision");
        actions.choices = {{"closed_kan", "", {0,1,2,3}, {}}, {"closed_kan", "", {4,5,6,7}, {}}, {"added_kan", "", {40}, {}}};
        selection.Open(actions, "kan");
        Check(selection.Active() && !selection.HandSelection() && selection.indices.size() == 3 && selection.UniqueCall(actions, false) == static_cast<std::size_t>(-1), "Multiple kans were chosen automatically");
        selection.Clear(); Check(actions.choices.size() == 3, "Local cancellation skipped server window");
        actions.choices.resize(1); selection.Open(actions, "kan"); Check(selection.UniqueCall(actions, true) == 0, "Single kan needs needless selection");
        actions.pending = "closed_kan"; Check(selection.UniqueCall(actions, false) == static_cast<std::size_t>(-1), "Pending kan remained clickable");
        fields["retired2"] = "1"; fields["winning2"] = "88";
        fields["hand2"] = "0,1,4,8,12,36,40,44,48,72,76,80,84"; fields["hidden2"] = "13"; fields["drawn2"] = "-1";
        fields["missing2"] = "2"; fields["river2"] = "104";
        game = VariantTableState(ParseVariantState(fields), {});
        auto scene = BuildTableScene(OnlineTableState(game));
        Check(game.turn == -1 && std::count_if(scene.begin(), scene.end(), [](const TableTile &t) { return std::string(t.role) == "retired" && t.direction == 0 && t.tile == -1; }) == 13, "Retired hand remained face up");
        Check(std::count_if(scene.begin(), scene.end(), [](const TableTile &t) { return std::string(t.role) == "winning" && t.tile == 88; }) == 1, "Winning tile missing");
        Check(game.players[0].discards.size() == 1 && std::any_of(scene.begin(), scene.end(), [](const TableTile &t) { return std::string(t.role) == "river" && t.slot == 1; }), "Missing-suit marker entered river data");
        for (int total: {108, 136, 144}) {
            ReplayState wall; wall.initialized = true;
            for (int front = 0; front <= total; ++front) {
                const int tail = std::min(3, total - front);
                wall.wall.InitVariant(total, front, tail, 0);
                const auto tiles = BuildWallScene(wall, static_cast<unsigned>(total));
                int count = 0;
                for (const auto &t: tiles) count += t.slot < 17 ? 2 : 1;
                if (count != total - front - tail) throw std::runtime_error("Wall count total=" + std::to_string(total) + " front=" + std::to_string(front) + " tail=" + std::to_string(tail) + " visible=" + std::to_string(count));
            }
            for (int offset = 0; offset < total; ++offset) {
                wall.wall.InitVariant(total, 0, 0, offset); std::set<std::pair<int,int>> positions;
                for (int i = 0; i < total; ++i) positions.insert(wall.wall.Position(i));
                Check(positions.size() == static_cast<std::size_t>(total), "Wall opening aliases slots");
            }
        }
        LobbySession session; session.interactive_games = true;
        session.snapshot.native_service = session.snapshot.variant_active = true;
        session.snapshot.phase = LobbyPhase::Game;
        session.snapshot.actions.enabled = true; session.snapshot.actions.revision = 5; session.snapshot.actions.choices = offers;
        Reject([&] { session.SubmitAction(4, 1); }, "Old revision submitted");
        const auto wire = session.SubmitAction(5, 1);
        Check(ParseNetworkMessage(wire.at(0)).attributes.at("tiles") == "1", "Entity submission changed");
        Reject([&] { session.SubmitAction(5, 1); }, "Duplicate pending submission accepted");
        session.snapshot.recovering = true; session.snapshot.actions.pending.clear(); session.snapshot.actions.choices = offers;
        Reject([&] { session.SubmitAction(5, 0); }, "Recovery accepted action");
        session.snapshot.table_paused = true; Check(session.SubmitAction(5, 0).empty(), "Paused table accepted action");
        LobbySession snapshots; snapshots.interactive_games = true; snapshots.snapshot.native_service = true;
        snapshots.snapshot.phase = LobbyPhase::Lobby;
        snapshots.Receive({"VGO", {{"rule", "sichuan-bloodbattle"}, {"table", "test"}}});
        auto full = Fixture(); full["window"] = "7"; full["actions"] = "discard:0;discard:1";
        snapshots.Receive({"VSTATE", full});
        const auto revision = snapshots.snapshot.actions.revision;
        auto invalid = full; invalid["hand2"] = "0,0";
        Reject([&] { snapshots.Receive({"VSTATE", invalid}); }, "Malformed snapshot accepted");
        Check(snapshots.snapshot.actions.revision == revision && snapshots.snapshot.game.players[0].known_hand.size() == 14, "Malformed snapshot partially committed");
        full["phase"] = "ended"; full["actions"] = "";
        snapshots.Receive({"VSTATE", full});
        Check(snapshots.snapshot.actions.choices.size() == 1 && snapshots.snapshot.actions.choices[0].kind == "next", "Ended phase lacks next-ready barrier");
        full["phase"] = "complete"; snapshots.Receive({"VSTATE", full});
        Check(snapshots.snapshot.phase == LobbyPhase::MatchComplete && snapshots.snapshot.actions.choices.empty(), "Complete phase offered next-ready");
        SettlementState settlement; SettlementReceipt receipt;
        receipt.generic = true; receipt.tag = "win"; receipt.who = 0; receipt.from = 1; receipt.has_hand = true;
        receipt.hand = {0, 4, 8, 12}; receipt.winning_tile = 12; receipt.fan = 3; receipt.entries = {{"清一色"}};
        settlement.receipts.push_back(receipt);
        const auto result = BuildSettlementScene(settlement, 0, {"local", "right", "across", "left"});
        Check(std::none_of(result.tiles.begin(), result.tiles.end(), [](const SettlementTileItem &t) { return t.role == "dora" || t.role == "indicator_back"; }), "Variant settlement contains dora");
        Check(std::any_of(result.text.begin(), result.text.end(), [](const SettlementTextItem &t) { return t.role == "variant_name" && t.text == "local"; }), "Winner name absent from settlement");
        std::cout << "PASS variant parsing, entities, exchange, multi-kan, retirement, wall cursors and revision/pending/recovery\n";
        return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
