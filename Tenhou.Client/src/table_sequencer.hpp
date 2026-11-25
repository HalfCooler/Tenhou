#pragma once
#include "action_caption_state.hpp"
#include "action_selection_scene.hpp"
#include "hand_projection.hpp"
#include "network_protocol.hpp"
#include "online_actions.hpp"
#include "replay_effects.hpp"
#include "table_render_frame.hpp"
#include "variant_state.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace tenhou {
    // =========================================================================
    // Offline Replay Presentation Sequencer (zero HWND, HDC, or GDI+)
    // =========================================================================
    struct ReplaySequencerEffects {
        std::vector<unsigned> play_sounds;
        bool stop_audio = false;
        bool cancel_settlement = false;
        bool cancel_intro = false;
        bool begin_match_intro = false;
        unsigned intro_type = 0;
        bool sync_results_checkbox = false;
        bool results_checked = false;
        bool playback_ended = false;
        bool update_ui = false;
        bool dirty_table = false;
        bool dirty_all = false;
        std::string artwork_error;
        std::vector<std::string> logs;

        void Merge(const ReplaySequencerEffects &other);
    };

    class ReplayTableSequencer {
    public:
        using GeometryLoader = std::function<TileGeometryCatalog *()>;

        ReplaySession *session() { return session_.get(); }
        const ReplaySession *session() const { return session_.get(); }
        void SetSession(std::unique_ptr<ReplaySession> session, std::vector<std::size_t> round_positions);
        const std::vector<std::size_t> &round_positions() const { return round_positions_; }

        OpeningPresentation &opening() { return opening_; }
        const OpeningPresentation &opening() const { return opening_; }
        TablePresentation &presentation() { return presentation_; }
        const TablePresentation &presentation() const { return presentation_; }
        ActionCaptions &captions() { return captions_; }
        const ActionCaptions &captions() const { return captions_; }

        bool playing() const { return playing_; }
        bool show_results() const { return show_results_; }
        void set_show_results(bool show) { show_results_ = show; }
        bool consume_result_pending() {
            const bool pending = result_pending_;
            result_pending_ = false;
            return pending;
        }
        unsigned draw_end_ticks() const { return draw_end_ticks_; }

        bool animation_enabled() const { return animation_enabled_; }
        void set_animation_enabled(bool enabled) { animation_enabled_ = enabled; }
        bool sound_enabled() const { return sound_enabled_; }
        void set_sound_enabled(bool enabled) { sound_enabled_ = enabled; }
        bool effects_muted() const { return effects_muted_; }
        void set_effects_muted(bool muted) { effects_muted_ = muted; }
        bool effects_only() const { return effects_only_; }
        void set_effects_only(bool only) {
            effects_only_ = only;
            individual_voices_ = false;
        }
        bool individual_voices() const { return individual_voices_; }
        void set_voice_mode(int mode) {
            effects_only_ = mode == 1;
            individual_voices_ = mode == 2;
        }
        bool EffectsEnabled() const { return sound_enabled_ && !effects_muted_; }

        bool red_tiles() const { return red_tiles_; }
        void set_red_tiles(bool red) { red_tiles_ = red; }
        bool original_display() const { return original_display_; }
        void set_original_display(bool enabled) { original_display_ = enabled; }

        ReplaySequencerEffects CancelEffects();
        ReplaySequencerEffects Stop();
        bool Start(unsigned interval_ms, std::uint64_t now_tick_ms, bool viewport_visible,
                   ReplaySequencerEffects &out);

        ReplaySequencerEffects Advance(unsigned speed_interval_ms, int canvas_width, bool artwork_healthy,
                                       std::uint64_t now_tick_ms, double now_anim_ms,
                                       const GeometryLoader &load_geometry);

        ReplaySequencerEffects PlaybackTick(unsigned speed_interval_ms, int canvas_width, bool artwork_healthy,
                                            std::uint64_t now_tick_ms, double now_anim_ms,
                                            bool intro_active, bool settlement_animating, bool audio_active,
                                            const GeometryLoader &load_geometry);

        ReplaySequencerEffects EffectsTick();
        ReplaySequencerEffects OpeningTick(double now_anim_ms, bool intro_active);
        ReplaySequencerEffects MotionTick(double now_anim_ms);
        bool CaptionTick();
        ReplaySequencerEffects OnMatchIntroFinished(double now_anim_ms);

    private:
        unsigned MatchIntroType() const;

        std::unique_ptr<ReplaySession> session_;
        std::vector<std::size_t> round_positions_;
        OpeningPresentation opening_;
        TablePresentation presentation_;
        ActionCaptions captions_;
        bool playing_ = false;
        bool animation_enabled_ = true;
        bool sound_enabled_ = false;
        bool effects_muted_ = false;
        bool effects_only_ = false;
        bool individual_voices_ = false;
        bool red_tiles_ = true;
        bool original_display_ = true;
        bool start_sound_pending_ = false;
        bool result_pending_ = false;
        bool show_results_ = false;
        unsigned draw_end_ticks_ = 0;
        std::uint64_t next_event_at_ = 0;
    };

    // =========================================================================
    // Online Gameplay Presentation & Action Sequencer (zero HWND, HDC, or GDI+)
    // =========================================================================
    struct OnlineSequencerContext {
        std::uint64_t tick_ms = 0;
        double anim_ms = 0;
        bool window_visible = true;
        bool intro_active = false;
        bool intro_resuming = false;
        bool settlement_animating = false;
        bool audio_active = false;
        bool audio_healthy = true;
        int canvas_width = 1440;
    };

    struct OnlineMotionCallbacks {
        std::function<void(const OnlineState &, const ReplayEvent &, bool, double)> accept;
        std::function<void(OpeningPresentation &, const OnlineState &, bool)> begin_opening;
        std::function<void(bool, const OnlineState &)> set_auto_sort;
        std::function<void(unsigned)> set_draw_end_slots;
        std::function<void(bool)> set_paused;
        std::function<void()> present_settlement;
        std::function<void()> cancel_animation;
        std::function<bool()> animating;
        std::function<bool()> manual_animation;
        std::function<void()> prepare;
        std::function<void()> refresh_preferences;
    };

    struct OnlineIntroRequest {
        enum class Kind { None, BeginMatch, BeginResume } kind = Kind::None;
        ReplayState state;
        unsigned type = 0;
    };

    struct OnlineSettlementSync {
        bool valid = false;
        SettlementState settlement;
        std::array<std::string, 4> names{};
        bool red = true;
        int dealer = 0, honba = -1, sticks = -1;
        bool continuation = false;
        bool confirm_enabled = false;
        bool round_finished = false;
        bool show = false;
        bool present = false;
    };

    struct OnlineSequencerEffects {
        std::vector<unsigned> play_sounds;
        bool audio_queue_overflow = false;
        bool stop_all_sound = false;
        bool preserve_intro_resume = false;
        bool stop_audio_only = false;
        bool cancel_settlement = false;
        bool cancel_intro = false;
        OnlineIntroRequest intro;
        bool reload_config_preferences = false;
        bool confirm_match_start = false;
        std::optional<std::pair<std::uint64_t, std::size_t>> submit_action;
        std::optional<unsigned> result_timer_ms;       // nullopt = unchanged, 0 = kill, >0 = SetTimer(5)
        std::optional<unsigned> auto_discard_timer_ms; // nullopt = unchanged, 0 = kill, >0 = SetTimer(10)
        bool start_deferred_sound_timer = false;
        bool release_capture = false;
        bool focus_action_view = false;
        bool focus_window = false;
        bool arrange = false;
        bool dirty_all = false;
        bool dirty_board = false;
        bool update_lobby_music = false;
        OnlineSettlementSync settlement_sync;
        std::optional<LobbySnapshot> notified_snapshot;
        std::vector<std::string> logs;

        void Merge(const OnlineSequencerEffects &other);
    };

    class OnlineTableSequencer {
    public:
        struct QueuedEvent {
            NetworkMessage message;
            LobbySnapshot snapshot;
            bool sound_allowed = false;
        };

        const LobbySnapshot &Snapshot(const LobbySnapshot &live) const { return displaying_ ? displayed_ : live; }
        const LobbySnapshot &displayed() const { return displayed_; }
        bool displaying() const { return displaying_; }
        const std::deque<QueuedEvent> &queue() const { return presentation_queue_; }

        OpeningPresentation &opening() { return opening_; }
        const OpeningPresentation &opening() const { return opening_; }
        ActionCaptions &captions() { return captions_; }
        const ActionCaptions &captions() const { return captions_; }
        ActionSelection &selection() { return selection_; }
        const ActionSelection &selection() const { return selection_; }

        int variant_hand() const { return variant_hand_; }
        bool start_barrier() const { return start_barrier_; }
        unsigned pause_ticks() const { return pause_ticks_; }
        unsigned draw_end_ticks() const { return draw_end_ticks_; }
        const std::array<bool, 4> &opening_flowers() const { return opening_flowers_; }
        std::vector<unsigned> &deferred_sounds() { return deferred_sounds_; }
        const std::vector<unsigned> &deferred_sounds() const { return deferred_sounds_; }

        bool show_results() const { return show_results_; }
        void set_show_results(bool show) { show_results_ = show; }
        std::uint64_t result_deadline() const { return result_deadline_; }

        const std::array<bool, 4> &automatic() const { return automatic_; }
        void set_automatic(int index, bool value) { automatic_[index] = value; }
        bool sort_refresh_pending() const { return sort_refresh_pending_; }
        void set_sort_refresh_pending(bool pending) { sort_refresh_pending_ = pending; }

        const std::string &action_filter() const { return action_filter_; }
        std::uint64_t action_revision() const { return action_revision_; }

        int &pressed_tile() { return pressed_tile_; }
        int pressed_tile() const { return pressed_tile_; }
        int &pressed_auto() { return pressed_auto_; }
        int pressed_auto() const { return pressed_auto_; }
        int &pressed_action() { return pressed_action_; }
        int pressed_action() const { return pressed_action_; }
        std::uint64_t &pressed_revision() { return pressed_revision_; }
        std::uint64_t pressed_revision() const { return pressed_revision_; }
        std::uint64_t &button_revision() { return button_revision_; }
        std::uint64_t button_revision() const { return button_revision_; }
        bool &hand_dragged() { return hand_dragged_; }
        bool hand_dragged() const { return hand_dragged_; }

        std::pair<int, int> hover_point() const { return hover_point_; }
        void set_hover_point(int x, int y) { hover_point_ = {x, y}; }

        bool sound_enabled() const { return sound_enabled_; }
        void set_sound_enabled(bool enabled) { sound_enabled_ = enabled; }
        bool effects_muted() const { return effects_muted_; }
        void set_effects_muted(bool muted) { effects_muted_ = muted; }
        void set_effects_only(bool only) {
            effects_only_ = only;
            individual_voices_ = false;
        }
        void set_voice_mode(int mode) {
            effects_only_ = mode == 1;
            individual_voices_ = mode == 2;
        }
        bool EffectsEnabled() const { return sound_enabled_ && !effects_muted_; }
        bool animation_enabled() const { return animation_enabled_; }
        void set_animation_enabled(bool enabled) { animation_enabled_ = enabled; }
        bool original_display() const { return original_display_; }
        void set_original_display(bool enabled) { original_display_ = enabled; }

        bool Moving(bool table_animating) const {
            return sort_refresh_pending_ || (animation_enabled_ && table_animating) ||
                   !presentation_queue_.empty() || pause_ticks_ || draw_end_ticks_;
        }

        bool QueueTimerNeeded() const { return queue_timer_active_; }
        bool ClockTimerNeeded() const { return clock_active_ && clock_deadline_ != 0; }
        bool AutoTimerNeeded(const LobbySnapshot &live) const;

        bool LocalTableInput(const LobbySnapshot &live, bool intro_active) const;
        bool AutomaticInput(const LobbySnapshot &live) const;
        bool SuppressCallPrompt(const LobbySnapshot &live) const;
        bool ActionReady(const LobbySnapshot &live, bool intro_active, bool table_animating) const;
        std::vector<TableActionButton> BuildButtons(const LobbySnapshot &live, CanvasRect bounds,
                                                     int width, int exit_width, int exit_height,
                                                     int anchor_x, int anchor_y, bool intro_active,
                                                     bool table_animating) const;
        bool IsActionButtonEnabled(int control_id, const LobbySnapshot &live,
                                   bool intro_active, bool table_animating) const;

        std::string FormatCountdown(const LobbySnapshot &snapshot, std::uint64_t now_tick_ms) const;
        TableInteraction BuildInteraction(const LobbySnapshot &snapshot, const LobbySnapshot &live,
                                          bool intro_active, bool table_animating, int hovered_button,
                                          const std::function<int()> &hit_tile,
                                          std::uint64_t now_tick_ms) const;

        OnlineSequencerEffects ClearPresentationQueue(const OnlineMotionCallbacks &motion);
        OnlineSequencerEffects CancelHandDrag();
        OnlineSequencerEffects CancelSelection();
        OnlineSequencerEffects CancelResultTimer();
        OnlineSequencerEffects ResetActionClock();
        void ResetForStopSound();

        OnlineSequencerEffects OnMatchIntroFinished(const LobbySnapshot &live, const OnlineSequencerContext &ctx,
                                                    const OnlineMotionCallbacks &motion);

        OnlineSequencerEffects SyncSnapshot(const LobbySnapshot &incoming, const LobbySnapshot &live,
                                            const OnlineSequencerContext &ctx, const OnlineMotionCallbacks &motion);

        OnlineSequencerEffects OnGameEvent(const NetworkMessage &message, const LobbySnapshot &snapshot,
                                           const LobbySnapshot &live, const OnlineSequencerContext &ctx,
                                           const OnlineMotionCallbacks &motion);

        OnlineSequencerEffects PresentationTick(const LobbySnapshot &live, const OnlineSequencerContext &ctx,
                                                const OnlineMotionCallbacks &motion);

        OnlineSequencerEffects OpeningTick(const LobbySnapshot &live, const OnlineSequencerContext &ctx);
        OnlineSequencerEffects ActionClockTick(const LobbySnapshot &live, const OnlineSequencerContext &ctx);
        OnlineSequencerEffects AutomaticAction(const LobbySnapshot &live, const OnlineSequencerContext &ctx,
                                               bool table_animating);
        OnlineSequencerEffects ResultTimerExpired(const LobbySnapshot &live, const OnlineSequencerContext &ctx,
                                                  const OnlineMotionCallbacks &motion);
        OnlineSequencerEffects ApplyAutomaticSort(const LobbySnapshot &live, const OnlineMotionCallbacks &motion);

        OnlineSequencerEffects UpdateActions(const LobbySnapshot &live, bool compact);
        OnlineSequencerEffects Submit(std::size_t index, const LobbySnapshot &live,
                                      const OnlineSequencerContext &ctx, bool table_animating);
        OnlineSequencerEffects HandleCommand(int id, const LobbySnapshot &live, bool compact,
                                             const OnlineSequencerContext &ctx, const OnlineMotionCallbacks &motion);

        void QueueOrEmitSound(unsigned id, OnlineSequencerEffects &fx, bool window_visible, bool audio_healthy);
        std::optional<unsigned> PopDeferredSound(bool intro_active);

    private:
        void ReceiveActionClock(const NetworkMessage &message, const LobbySnapshot &snapshot,
                                std::uint64_t now_tick_ms);
        void PresentGameEvent(const NetworkMessage &message, const LobbySnapshot &snapshot,
                              const LobbySnapshot &live, const OnlineSequencerContext &ctx,
                              const OnlineMotionCallbacks &motion, OnlineSequencerEffects &fx);
        void PresentVariantState(const LobbySnapshot &snapshot, const LobbySnapshot &live,
                                 const OnlineSequencerContext &ctx, const OnlineMotionCallbacks &motion,
                                 OnlineSequencerEffects &fx);
        void PlayEventSound(const ReplayEvent &event, const LobbySnapshot &snapshot,
                            const OnlineSequencerContext &ctx, OnlineSequencerEffects &fx);
        void AnnounceRiichi(const LobbySnapshot &live, const OnlineSequencerContext &ctx,
                            OnlineSequencerEffects &fx);

        std::deque<QueuedEvent> presentation_queue_;
        bool queue_timer_active_ = false;
        LobbySnapshot displayed_;
        bool displaying_ = false;
        VariantEventCursor variant_events_;
        int variant_hand_ = 0;
        OpeningPresentation opening_;
        ActionCaptions captions_;
        ActionSelection selection_;
        unsigned pause_ticks_ = 0, draw_end_ticks_ = 0;
        bool pause_after_motion_ = false;
        bool event_sound_allowed_ = true;
        bool match_intro_pending_ = true;
        bool resume_preparation_pending_ = false;
        bool start_sound_pending_ = false;
        bool start_barrier_ = false;
        std::array<bool, 4> opening_flowers_{};
        std::vector<unsigned> deferred_sounds_;
        std::uint64_t result_deadline_ = 0;
        std::uint64_t paused_at_ = 0;
        bool result_pending_ = false;
        bool show_results_ = false;
        std::uint64_t riichi_sound_revision_ = 0;
        bool suppress_self_reach_ = false;
        std::uint64_t sound_action_revision_ = 0;
        std::uint64_t action_revision_ = 0;
        std::string action_filter_ = "discard";
        bool clock_active_ = false;
        std::uint64_t clock_deadline_ = 0;
        int clock_base_ms_ = -1;
        std::uint64_t clock_base_deadline_ = 0;
        int clock_warning_second_ = -1;
        std::uint64_t clock_revision_ = 0;
        std::array<bool, 4> automatic_{true, false, false, false};
        bool sort_refresh_pending_ = false;
        std::uint64_t automatic_discard_at_ = 0;
        std::pair<int, int> hover_point_{-1, -1};
        int pressed_tile_ = -1, pressed_auto_ = -1, pressed_action_ = -1;
        std::uint64_t pressed_revision_ = 0, button_revision_ = 0;
        bool hand_dragged_ = false;
        bool sound_enabled_ = true;
        bool effects_muted_ = false;
        bool effects_only_ = false;
        bool individual_voices_ = false;
        bool animation_enabled_ = true;
        bool original_display_ = true;
    };
} // namespace tenhou
