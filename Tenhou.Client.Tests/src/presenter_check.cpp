#include "hand_projection.hpp"
#include "settlement_sequencer.hpp"
#include "tile_geometry.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
    void Check(bool ok, const char *message) {
        if (!ok) throw std::runtime_error(message);
    }

    void CheckTileGeometryAndHandProjection(const std::filesystem::path &resources) {
        using namespace tenhou;
        TileGeometryCatalog geometry;
        geometry.Load(resources, 1440);
        Check(geometry.loaded() && geometry.width() == 1440, "TileGeometryCatalog failed to load HD packs");

        // Flower, standing hand, sideways meld, and 18-stack wall geometry without GDI+.
        const auto hand0 = geometry.Bounds({'t', 0, 16, 0}, true);
        const auto flower = geometry.Bounds({'n', 0, 136, 0, 0, 0, false, false, "flower"}, true);
        Check(hand0.width > 40 && hand0.height > 60 && flower.width > 30 && flower.height > 30,
              "Unexpected tile sprite bounds");
        Check(geometry.OpaqueAt({'t', 0, 16, 0}, true, hand0.width / 2, hand0.height / 2),
              "Center of standing hand tile must be opaque");

        // Suit ordering (FUN_00488220) + manual insertion gap remapping (FUN_00488650).
        OnlineState online;
        online.started = online.initialized = true;
        online.events = 1;
        online.last_event = "INIT";
        // Hold only man (0, 4) and sou (72, 76) -> absent pin (suit 1) moves to back of suit_order.
        online.players[0].known_hand = {72, 0, 76, 4};
        online.players[0].drawn_tile = -1;

        HandProjection projection;
        auto state = projection.Project(online);
        Check(state.players[0].hand == std::vector<int>({0, 4, 72, 76}),
              "Initial two-suit hand order mismatch");

        // Draw a pin tile (36): suit_order now has all 3 suits so pin stays at the rightmost suit group after sort.
        online.events = 2;
        online.last_event = "T36";
        online.players[0].known_hand.push_back(36);
        online.players[0].drawn_tile = 36;
        state = projection.Project(online);
        Check(state.players[0].hand == std::vector<int>({0, 4, 72, 76, 36}) &&
                  state.players[0].drawn_tile == 36,
              "Separated drawn tile must stay at the end");

        // Manual drag: move drawn tile 36 before 72 -> disables auto_sort and remaps drawn gap to new back (76).
        ReplayState before, after;
        Check(projection.MoveHandTile(36, 72, online, &before, &after) && !projection.auto_sort(),
              "Manual tile move did not disable auto_sort");
        Check(after.players[0].hand == std::vector<int>({0, 4, 36, 72, 76}) &&
                  after.players[0].drawn_tile == 76,
              "Manual insertion did not remap display drawn slot to back");

        // Re-enabling auto_sort restores sorted order (man, sou, pin because pin was previously absent) + separated draw.
        Check(projection.SetAutoSort(true, online, &before, &after) && projection.auto_sort(),
              "Re-enabling auto_sort failed");
        Check(after.players[0].hand == std::vector<int>({0, 4, 72, 76, 36}) &&
                  after.players[0].drawn_tile == 36,
              "Suit order after absent-suit promotion did not keep pin behind sou");

        // Hit-testing in non-integer fitted viewport, including raised selection offset.
        const CanvasRect viewport{13, 9, 13 + 1197, 9 + 897};
        const auto fitted = FitNativeCanvasBounds(viewport, 1440);
        const auto origin0 = geometry.Origin({'t', 0, 0, 0}, true);
        const int center_x = fitted.left + (origin0.first + hand0.width / 2) * (fitted.right - fitted.left) / 1440;
        const int center_y = fitted.top + (origin0.second + hand0.height / 2) * (fitted.bottom - fitted.top) / 1080;
        Check(projection.HitTile(viewport, online, geometry, center_x, center_y) == 0,
              "Headless HitTile missed first hand tile");
        Check(projection.HitTile(viewport, online, geometry, 2, 2) == -1,
              "Headless HitTile matched outside fitted canvas");

        // HandColor classification checks for Riichi and Variant exchange.
        const TableTile first_tile{'t', 0, 0, 0};
        Check(HandProjection::ClassifyTile(first_tile, true, false, false, 0, {4}, {}, false, true) ==
                  HandColor::UnavailableHover,
              "Restricted unplayable hovered tile should be UnavailableHover");
        Check(HandProjection::ClassifyTile(first_tile, true, false, true, 0, {0, 4}, {0}, true, false) ==
                  HandColor::Exchange,
              "Selected variant exchange tile should use Exchange highlight");
    }

    void CheckSettlementSequencer() {
        using namespace tenhou;

        // 1. Riichi win with ura dora (id 53 -> 1200ms pause before ura reveal) + mid-flight cancel stop_sound.
        {
            SettlementSequencer seq;
            seq.SetSoundEnabled(true, 1000);
            seq.SetAnimationEnabled(true, 1000);
            SettlementState state;
            SettlementReceipt win;
            win.who = 0;
            win.has_yaku = true;
            win.dora = {16};
            win.ura = {20};
            win.yaku = {{1, 1}, {52, 1}, {53, 1}};
            state.receipts = {win};
            state.has_final = true;

            SettlementEffects init_fx;
            Check(seq.SetState(state, {"A", "B", "C", "D"}, true, 1000, init_fx), "SetState rejected new state");
            auto open_fx = seq.SetVisible(true, 1000);
            Check(seq.Animating() && open_fx.sounds == std::vector<unsigned>{101} &&
                      open_fx.presentation_timer_ms == std::optional<unsigned>{10u},
                  "Riichi settlement did not emit opening sound 101 and 10ms timer");

            // 30 frames to reveal yaku[0] (id 1), then 54 frames to hit ura pause before id 53!
            // Wait: display yaku has {1, 52, 53} (3 items):
            // Tick 30 -> reveals yaku[0] (1), next is yaku[1] (52) -> presentation_frames = 55.
            for (int i = 0; i < 29; ++i) Check(seq.PresentationTick(1010).sounds.empty(), "Early yaku 0 sound");
            auto y0 = seq.PresentationTick(1300);
            Check(seq.shown_yaku() == 1 && y0.sounds == std::vector<unsigned>{109}, "First yaku sound mismatch");

            // Next 55 ticks -> reveals yaku[1] (52), next is yaku[2] (53) -> presentation_frames = 54!
            for (int i = 0; i < 54; ++i) seq.PresentationTick(1400);
            auto y1 = seq.PresentationTick(1850);
            Check(seq.shown_yaku() == 2 && y1.sounds == std::vector<unsigned>{109} && seq.UraPending(),
                  "Dora row revealed ura prematurely");

            // Next 54 ticks -> hits `next_ura && !ura_pause_done_` and requests 1200ms timer!
            for (int i = 0; i < 53; ++i) seq.PresentationTick(2000);
            auto ura_wait = seq.PresentationTick(2400);
            Check(ura_wait.presentation_timer_ms == std::optional<unsigned>{1200u} &&
                      ura_wait.sounds.empty() && seq.UraPending(),
                  "Missing 1200ms native pause before ura dora row");

            // Next tick after 1200ms reveals ura row (id 53) and flips ura!
            auto y2 = seq.PresentationTick(3600);
            Check(seq.shown_yaku() == 3 && !seq.UraPending() && y2.sounds == std::vector<unsigned>{109},
                  "Ura row did not reveal after 1200ms pause");

            // Final 12 frames settle totals and emit notify_presentation_done.
            for (int i = 0; i < 11; ++i) seq.PresentationTick(3700);
            auto done = seq.PresentationTick(3800);
            Check(!seq.Animating() && seq.Complete() && done.notify_presentation_done &&
                      done.presentation_timer_ms == std::optional<unsigned>{0u} && seq.EnterFinalLabel(),
                  "Riichi receipt did not complete cleanly or set END label");

            // Confirm advances to FinalPage -> plays SE_GAMEEND (115) and notifies BGM_GAME9.
            auto to_final = seq.Confirm(4000);
            Check(seq.FinalPage() && to_final.sounds == std::vector<unsigned>{115} && to_final.notify_bgm_changed,
                  "Entering final page did not play 115 or notify BGM");
        }

        // 2. Yakuman sound (111 + 109) & mid-animation cancel stop_sound.
        {
            SettlementSequencer seq;
            seq.SetSoundEnabled(true, 0);
            SettlementState state;
            SettlementReceipt yakuman;
            yakuman.who = 0;
            yakuman.yakuman = {47};
            state.receipts = {yakuman};
            SettlementEffects fx;
            seq.SetState(state, {}, true, 0, fx);
            auto open_fx = seq.SetVisible(true, 0);
            Check(open_fx.sounds.empty(), "Yakuman must not play normal opening sound 101");
            for (int i = 0; i < 29; ++i) seq.PresentationTick(10);
            auto reveal = seq.PresentationTick(300);
            Check(reveal.sounds == std::vector<unsigned>({111, 109}), "Yakuman did not play 111 + 109");
            auto cancel = seq.SetVisible(false, 310);
            Check(cancel.stop_sound, "Hiding active settlement presentation must stop active audio");
        }

        // 3. Audio-only reveal when animation=false & sound=true, then 5s countdown pause shift & 102 ticks.
        {
            SettlementSequencer seq;
            seq.SetAnimationEnabled(false, 0);
            seq.SetSoundEnabled(true, 0);
            seq.SetContinuation(true, 0);
            SettlementState state;
            SettlementReceipt win;
            win.generic = true;
            win.who = 0;
            win.entries = {{"清一色"}};
            state.receipts = {win};
            SettlementEffects fx;
            seq.SetState(state, {}, false, 1000, fx);
            auto open_fx = seq.SetVisible(true, 1000);
            Check(seq.Animating() && open_fx.sounds == std::vector<unsigned>{101} && seq.confirm_deadline() == 0,
                  "Audio-enabled win receipt must finish sound reveal before starting 5s countdown");
            for (int i = 0; i < 29; ++i) seq.PresentationTick(1010);
            Check(seq.PresentationTick(1300).sounds == std::vector<unsigned>{109}, "Generic pattern sound 109 missing");
            for (int i = 0; i < 11; ++i) seq.PresentationTick(1400);
            auto ready_fx = seq.PresentationTick(1500);
            Check(!seq.Animating() && seq.confirm_seconds() == 5 && seq.confirm_deadline() == 6500 &&
                      ready_fx.sounds == std::vector<unsigned>{102},
                  "Initial 5s countdown or sound 102 missing after audio reveal");

            // Pause at t=2000 for 3500ms -> deadline shifts from 6500 to 10000, ticks while paused do nothing.
            seq.SetPaused(true, 2000);
            Check(seq.ConfirmTick(7000).sounds.empty() && !seq.ConfirmTick(7000).confirm_finished,
                  "Paused countdown fired early");
            seq.SetPaused(false, 5500);
            Check(seq.confirm_deadline() == 10000, "Pause duration did not shift confirm deadline");

            auto tick4 = seq.ConfirmTick(6001);
            Check(seq.confirm_seconds() == 4 && tick4.sounds == std::vector<unsigned>{102},
                  "Countdown decrement to 4s did not play 102");
            auto finish = seq.ConfirmTick(10000);
            Check(finish.confirm_finished && seq.confirm_deadline() == 0,
                  "Expired countdown did not finish confirmation");
        }
    }
} // namespace

int main(int argc, char **argv) {
    try {
        if (argc != 2) throw std::runtime_error("Usage: tenhou_presenter_check RESOURCES");
        CheckTileGeometryAndHandProjection(argv[1]);
        CheckSettlementSequencer();
        std::cout << "PASS headless presenter: TileGeometryCatalog, HandProjection, SettlementSequencer\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
