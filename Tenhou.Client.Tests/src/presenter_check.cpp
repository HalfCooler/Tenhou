#include "hand_projection.hpp"
#include "settlement_sequencer.hpp"
#include "table_render_frame.hpp"
#include "table_sequencer.hpp"
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
            // tick 30 -> reveals yaku[0] (1), next is yaku[1] (52) -> presentation_frames = 55.
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

    void CheckReplayAndOnlineSequencers(const std::filesystem::path &resources) {
        using namespace tenhou;
        TileGeometryCatalog geometry;
        geometry.Load(resources, 1440);

        // 1. Headless ReplayTableSequencer: INIT -> match intro -> opening -> draw flight -> 28-tick RYUUKYOKU flip.
        {
            ReplayTableSequencer seq;
            seq.set_sound_enabled(true);
            seq.set_animation_enabled(true);
            const char *xml =
                "<mjloggm ver=\"2.3\">"
                "<GO type=\"9\" />"
                "<INIT seed=\"0,0,0,1,1,52\" ten=\"250,250,250,250\" oya=\"0\" "
                "hai0=\"0,4,8,12,16,20,24,28,32,36,40,44,48\" "
                "hai1=\"1,5,9,13,17,21,25,29,33,37,41,45,49\" "
                "hai2=\"2,6,10,14,18,22,26,30,34,38,42,46,50\" "
                "hai3=\"3,7,11,15,19,23,27,31,35,39,43,47,51\" />"
                "<T56 />"
                "<D56 />"
                "<RYUUKYOKU sc=\"250,15,250,-5,250,-5,250,-5\" hai0=\"0,4,8,12,16,20,24,28,32,36,40,44,48\" />"
                "</mjloggm>";
            seq.SetSession(std::make_unique<ReplaySession>(ReplayLog::Parse(xml)), {1});
            ReplaySequencerEffects start_fx;
            Check(seq.Start(500, 1000, true, start_fx) && seq.playing(), "ReplayTableSequencer failed to start");

            // Step GO (0) then INIT (1) via PlaybackTick -> triggers match intro + opening!
            auto go_fx = seq.PlaybackTick(500, 1440, true, 1500, 1500.0, false, false, false, [&] { return &geometry; });
            Check(go_fx.update_ui && !go_fx.begin_match_intro, "GO should not start match intro");
            auto init_fx = seq.PlaybackTick(500, 1440, true, 2000, 2000.0, false, false, false, [&] { return &geometry; });
            Check(init_fx.begin_match_intro && init_fx.intro_type == 9 && seq.opening().Active() &&
                      init_fx.play_sounds.empty(),
                  "Replay INIT while playing should defer 112 until match intro finishes");

            auto intro_done = seq.OnMatchIntroFinished(2500.0);
            Check(intro_done.play_sounds == std::vector<unsigned>{112}, "Match intro completion did not play 112");
            for (int tick = 1; tick <= 400 && seq.opening().Active(); ++tick)
                seq.OpeningTick(2500.0 + tick * kTableTickMs, false);
            Check(!seq.opening().Active(), "Headless replay opening did not complete");

            // Advance T56 -> starts draw flight!
            auto draw_fx = seq.Advance(500, 1440, true, 3000, 3000.0, [&] { return &geometry; });
            Check(seq.presentation().Active() && draw_fx.dirty_table == false,
                  "Replay draw transition did not start");

            // Advance D56 then RYUUKYOKU -> 28-tick draw-end slot reveal!
            seq.Advance(500, 1440, true, 3250, 3250.0, [&] { return &geometry; });
            auto draw_end_fx = seq.Advance(500, 1440, true, 3500, 3500.0, [&] { return &geometry; });
            Check(seq.draw_end_ticks() == 28 && !seq.show_results(), "RYUUKYOKU did not arm 28-tick tenpai flip");
            ReplaySequencerEffects flip_acc;
            for (int i = 0; i < 28; ++i) flip_acc.Merge(seq.EffectsTick());
            Check(seq.draw_end_ticks() == 0 && seq.show_results() && seq.consume_result_pending() &&
                      !flip_acc.play_sounds.empty() && flip_acc.play_sounds.front() == 104,
                  "28-tick RYUUKYOKU flip did not play 104 or reveal results at completion");
        }

        // 2. Headless OnlineTableSequencer: packet burst queueing, 41-tick opponent draw wait,
        //    1000ms auto-tsumogiri, 2s clock warning (108), and 134-tick + 2000ms AGARI delay.
        {
            OnlineTableSequencer seq;
            seq.set_sound_enabled(true);
            seq.set_animation_enabled(false);
            bool table_animating = false;
            bool settlement_scores_presented = false;
            unsigned draw_end_slots = 14;
            OnlineMotionCallbacks motion;
            motion.animating = [&] { return table_animating; };
            motion.set_draw_end_slots = [&](unsigned slots) { draw_end_slots = slots; };
            motion.present_settlement = [&] { settlement_scores_presented = true; };

            LobbySession session;
            session.official = session.interactive_games = true;
            session.Connected("ID00000000-00000000", 'M');
            OnlineSequencerContext ctx;
            ctx.tick_ms = 1000;
            ctx.anim_ms = 1000.0;
            ctx.window_visible = true;

            auto feed = [&](const char *wire) {
                const auto msg = ParseNetworkMessage(wire, true);
                (void) session.Receive(msg);
                auto fx = seq.OnGameEvent(msg, session.snapshot, session.snapshot, ctx, motion);
                if (fx.intro.kind != OnlineIntroRequest::Kind::None) ctx.intro_active = true;
                fx.Merge(seq.SyncSnapshot(session.snapshot, session.snapshot, ctx, motion));
                return fx;
            };

            feed("<HELO uname=\"Player\" PF4=\"9,0,1500,0,0,0,0,0,0,0,0,0,0,0\" />");
            feed("<LN n=\"A1A1A1A\" j=\"112A\" g=\"112A\" />");
            feed("<GO type=\"9\" lobby=\"0\" />");
            feed("<UN n0=\"A\" n1=\"B\" n2=\"C\" n3=\"D\" />");
            auto taikyoku = feed("<TAIKYOKU oya=\"0\" />");
            Check(ctx.intro_active && seq.start_barrier(), "TAIKYOKU did not enter intro barrier");
            auto intro_done = seq.OnMatchIntroFinished(session.snapshot, ctx, motion);
            ctx.intro_active = false;
            Check(!seq.start_barrier() && intro_done.confirm_match_start,
                  "Finishing official match intro did not clear barrier or confirm start");

            feed("<INIT seed=\"0,0,0,1,1,52\" ten=\"250,250,250,250\" oya=\"0\" "
                 "hai=\"0,4,8,12,16,20,24,28,32,36,40,44,48\" />");
            seq.opening().Cancel();

            // Self draw T56 + automatic tsumogiri enabled -> waits 1000ms before auto-submitting!
            seq.set_automatic(2, true); // tsumogiri
            auto self_draw = feed("<T56 />");
            Check(self_draw.auto_discard_timer_ms == std::optional<unsigned>{1000u},
                  "Self draw did not arm 1000ms automatic discard timer");
            ctx.tick_ms = 1999;
            Check(!seq.AutomaticAction(session.snapshot, ctx, false).submit_action.has_value(),
                  "Automatic tsumogiri fired before 1000ms");
            ctx.tick_ms = 2000;
            auto auto_sub = seq.AutomaticAction(session.snapshot, ctx, false);
            Check(auto_sub.submit_action.has_value(), "Automatic tsumogiri did not fire at 1000ms");
            session.SubmitAction(auto_sub.submit_action->first, auto_sub.submit_action->second);

            // Action clock warning at <= 2s plays sound 108 once per second!
            seq.set_automatic(2, false);
            feed("<D56 />");
            while (seq.QueueTimerNeeded()) seq.PresentationTick(session.snapshot, ctx, motion);

            // Queue burst: opponent draw <U /> immediately followed by opponent discard <e60 />!
            const auto u_msg = ParseNetworkMessage("<U />", true);
            (void) session.Receive(u_msg);
            const auto u_snap = session.snapshot;
            const auto e_msg = ParseNetworkMessage("<e60 />", true);
            (void) session.Receive(e_msg);
            const auto e_snap = session.snapshot;

            seq.OnGameEvent(u_msg, u_snap, e_snap, ctx, motion);
            seq.OnGameEvent(e_msg, e_snap, e_snap, ctx, motion);
            Check(seq.displaying() && seq.displayed().game.events == u_snap.game.events && seq.queue().size() == 1,
                  "Opponent draw/discard burst was not queued behind visible draw");
            for (int i = 0; i < 41; ++i) seq.PresentationTick(e_snap, ctx, motion);
            Check(seq.displayed().game.events == u_snap.game.events, "Opponent draw did not hold 41 think ticks");
            seq.PresentationTick(e_snap, ctx, motion);
            Check(seq.displayed().game.events == e_snap.game.events && seq.queue().empty(),
                  "Opponent discard did not pop after 41 think ticks");
            while (seq.QueueTimerNeeded()) seq.PresentationTick(e_snap, ctx, motion);
            Check(!seq.QueueTimerNeeded(), "Empty presentation queue did not disarm queue timer");

            // Feed V, f64, W, g68, then T72 + CLOCK ms=2500 -> check 2s warning sound 108!
            for (const char *step: {"<V />", "<f64 />", "<W />", "<g68 />"}) {
                feed(step);
                while (seq.QueueTimerNeeded()) seq.PresentationTick(session.snapshot, ctx, motion);
            }
            ctx.tick_ms = 5000;
            feed("<T72 />");
            while (seq.QueueTimerNeeded()) seq.PresentationTick(session.snapshot, ctx, motion);
            NetworkMessage clock_msg{"CLOCK", {{"ms", "2500"}, {"base_ms", "2500"}}};
            seq.OnGameEvent(clock_msg, session.snapshot, session.snapshot, ctx, motion);
            Check(seq.ClockTimerNeeded() && seq.FormatCountdown(session.snapshot, 5000) == "3+0",
                  "CLOCK 2500ms did not format as 3+0");
            ctx.tick_ms = 5600; // remaining 1900ms -> second == 2 -> plays 108!
            auto warn2 = seq.ActionClockTick(session.snapshot, ctx);
            Check(warn2.play_sounds == std::vector<unsigned>{108}, "Action clock at 2s did not play 108");
            Check(seq.ActionClockTick(session.snapshot, ctx).play_sounds.empty(),
                  "Action clock repeated 108 within the same second");

            // AGARI -> 2000ms result_timer_ms (FUN_004aad30) + pause shift!
            ctx.tick_ms = 6000;
            auto agari = feed("<AGARI who=\"0\" fromWho=\"0\" hai=\"0,4,8,12,16,20,24,28,32,36,40,44,48,72\" "
                              "machi=\"72\" ten=\"30,4000,1\" yaku=\"0,1\" doraHai=\"52\" "
                              "sc=\"250,40,250,-20,250,-10,250,-10\" />");
            Check(seq.pause_ticks() == 0 && agari.result_timer_ms == std::optional<unsigned>{2000u} &&
                      seq.result_deadline() == 8000 && !seq.show_results(),
                  "AGARI did not set 2000ms result timer");

            // Table pause for 1500ms shifts result_deadline from 8000 to 9500!
            auto paused_snap = session.snapshot;
            paused_snap.table_paused = true;
            ctx.tick_ms = 6500;
            seq.SyncSnapshot(paused_snap, paused_snap, ctx, motion);
            ctx.tick_ms = 8000;
            paused_snap.table_paused = false;
            auto unpaused = seq.SyncSnapshot(paused_snap, paused_snap, ctx, motion);
            Check(seq.result_deadline() == 9500 && unpaused.result_timer_ms == std::optional<unsigned>{1500u},
                  "Table pause did not shift AGARI settlement deadline");
            ctx.tick_ms = 9500;
            auto expired = seq.ResultTimerExpired(session.snapshot, ctx, motion);
            Check(seq.show_results() && expired.settlement_sync.valid && expired.settlement_sync.show &&
                      settlement_scores_presented,
                  "AGARI result timer expiration did not show settlement and update table scores");

            // 3. Headless TableRenderFrame construction for online & replay scenes.
            HandProjection proj;
            TablePresentation pres;
            TableInteraction inter;
            inter.selected = {0, 4};
            inter.hovered = 8;
            inter.playable = {0, 4, 8};
            const auto online_frame = BuildOnlineTableRenderFrame(
                1440, {0, 0, 1200, 900}, session.snapshot.game, proj, geometry, pres,
                nullptr, &inter, &seq.captions(), false, true, true, true, 0.0, 96);
            Check(!online_frame.tiles.empty() && online_frame.tiles.size() == online_frame.tile_colors.size() &&
                      online_frame.state.relative_scores,
                  "BuildOnlineTableRenderFrame produced mismatched tiles/colors or lost relative_scores");
            (void) draw_end_slots;
        }
    }
} // namespace

int main(int argc, char **argv) {
    try {
        if (argc != 2) throw std::runtime_error("Usage: tenhou_presenter_check RESOURCES");
        CheckTileGeometryAndHandProjection(argv[1]);
        CheckSettlementSequencer();
        CheckReplayAndOnlineSequencers(argv[1]);
        std::cout << "PASS headless presenter: TileGeometryCatalog, HandProjection, SettlementSequencer, ReplayTableSequencer, OnlineTableSequencer\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
