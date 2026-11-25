#include "table_sequencer.hpp"
#include <algorithm>
#include <charconv>

namespace tenhou {
    // =========================================================================
    // ReplayTableSequencer
    // =========================================================================
    void ReplaySequencerEffects::Merge(const ReplaySequencerEffects &other) {
        play_sounds.insert(play_sounds.end(), other.play_sounds.begin(), other.play_sounds.end());
        stop_audio = stop_audio || other.stop_audio;
        cancel_settlement = cancel_settlement || other.cancel_settlement;
        cancel_intro = cancel_intro || other.cancel_intro;
        if (other.begin_match_intro) {
            begin_match_intro = true;
            intro_type = other.intro_type;
        }
        if (other.sync_results_checkbox) {
            sync_results_checkbox = true;
            results_checked = other.results_checked;
        }
        playback_ended = playback_ended || other.playback_ended;
        update_ui = update_ui || other.update_ui;
        dirty_table = dirty_table || other.dirty_table;
        dirty_all = dirty_all || other.dirty_all;
        if (!other.artwork_error.empty()) artwork_error = other.artwork_error;
        logs.insert(logs.end(), other.logs.begin(), other.logs.end());
    }

    void ReplayTableSequencer::SetSession(std::unique_ptr<ReplaySession> session,
                                          std::vector<std::size_t> round_positions) {
        session_ = std::move(session);
        round_positions_ = std::move(round_positions);
    }

    ReplaySequencerEffects ReplayTableSequencer::CancelEffects() {
        ReplaySequencerEffects fx;
        captions_.Clear();
        start_sound_pending_ = false;
        fx.cancel_intro = true;
        opening_.Cancel();
        draw_end_ticks_ = 0;
        result_pending_ = false;
        fx.cancel_settlement = true;
        if (presentation_.Active()) fx.logs.emplace_back("ANIMATION settled");
        presentation_.Cancel();
        fx.stop_audio = true;
        fx.dirty_all = true;
        return fx;
    }

    ReplaySequencerEffects ReplayTableSequencer::Stop() {
        ReplaySequencerEffects fx;
        if (playing_) {
            fx.logs.emplace_back("REPLAY PLAYBACK paused");
            fx.playback_ended = true;
        }
        playing_ = false;
        fx.Merge(CancelEffects());
        return fx;
    }

    bool ReplayTableSequencer::Start(unsigned interval_ms, std::uint64_t now_tick_ms, bool viewport_visible,
                                     ReplaySequencerEffects &out) {
        if (!session_ || session_->position() >= session_->log().events.size() || !viewport_visible) return false;
        playing_ = true;
        next_event_at_ = now_tick_ms + interval_ms;
        out.logs.push_back("REPLAY PLAYBACK interval_ms=" + std::to_string(interval_ms));
        return true;
    }

    unsigned ReplayTableSequencer::MatchIntroType() const {
        unsigned type = red_tiles_ ? 1u : 3u;
        if (!session_) return type;
        for (std::size_t i = 0; i < session_->position(); ++i) {
            const auto &e = session_->log().events[i];
            if (e.tag == "GO" && e.attributes.count("type"))
                type = static_cast<unsigned>(std::stoul(e.attributes.at("type")));
        }
        return type;
    }

    ReplaySequencerEffects ReplayTableSequencer::Advance(unsigned speed_interval_ms, int canvas_width,
                                                         bool artwork_healthy, std::uint64_t now_tick_ms,
                                                         double now_anim_ms, const GeometryLoader &load_geometry) {
        ReplaySequencerEffects fx;
        if (!session_ || session_->position() >= session_->log().events.size()) return fx;
        fx.cancel_settlement = true;
        if (opening_.Active()) {
            opening_.Cancel();
            fx.stop_audio = true;
        }
        presentation_.Cancel();
        const auto before = session_->state();
        const auto event = session_->log().events[session_->position()];
        session_->Step();
        captions_.Accept(event);

        result_pending_ = event.kind == ReplayEventKind::Win || event.kind == ReplayEventKind::DrawEnd;
        draw_end_ticks_ = event.kind == ReplayEventKind::DrawEnd ? 28 : 0;
        if (animation_enabled_ && event.kind != ReplayEventKind::Init && artwork_healthy) {
            try {
                auto *geometry = load_geometry ? load_geometry() : nullptr;
                if (geometry) {
                    const unsigned limit = playing_ && speed_interval_ms > 0 ? speed_interval_ms * 3 / 4 : 0;
                    geometry->SetRedTiles(session_->state().red_tiles);
                    presentation_.Begin(before, session_->state(), event, 240, canvas_width,
                                        [&](const TableTile &tile) { return geometry->Origin(tile, red_tiles_); },
                                        now_anim_ms, true, geometry, red_tiles_, limit);
                    if (presentation_.Active())
                        fx.logs.push_back("ANIMATION BEGIN cursor=" + std::to_string(session_->position()));
                }
            } catch (const std::exception &e) {
                fx.artwork_error = e.what();
                fx.logs.push_back("REPLAY ARTWORK ERROR " + fx.artwork_error);
                presentation_.Cancel();
            }
        }
        const int who = event.kind == ReplayEventKind::Win ? std::stoi(event.attributes.at("who")) : event.who;
        const char gender = who >= 0 && who < 4 ? session_->state().players[who].gender : '?';
        const bool intro = event.kind == ReplayEventKind::Init && !before.initialized && playing_ &&
                           animation_enabled_ && original_display_;
        if (intro) {
            start_sound_pending_ = !ReplaySounds(event, gender, effects_only_, individual_voices_).empty();
        } else {
            for (auto id: ReplaySounds(event, gender, effects_only_, individual_voices_))
                fx.play_sounds.push_back(id);
        }
        if (event.kind == ReplayEventKind::Init && (animation_enabled_ || EffectsEnabled())) {
            presentation_.Cancel();
            auto *geometry = load_geometry ? load_geometry() : nullptr;
            if (geometry) geometry->SetRedTiles(session_->state().red_tiles);
            opening_.Begin(session_->state(), original_display_, geometry, red_tiles_);
            opening_.ResumeClock(now_anim_ms);
            fx.logs.push_back("OPENING BEGIN wall=" + std::to_string(opening_.wall_tiles()) +
                              " tick=" + std::to_string(now_tick_ms));
        }
        if (intro) {
            show_results_ = false;
            fx.sync_results_checkbox = true;
            fx.results_checked = false;
            fx.begin_match_intro = true;
            fx.intro_type = MatchIntroType();
            fx.logs.emplace_back("MATCH INTRO BEGIN");
        }
        return fx;
    }

    ReplaySequencerEffects ReplayTableSequencer::PlaybackTick(unsigned speed_interval_ms, int canvas_width,
                                                              bool artwork_healthy, std::uint64_t now_tick_ms,
                                                              double now_anim_ms, bool intro_active,
                                                              bool settlement_animating, bool audio_active,
                                                              const GeometryLoader &load_geometry) {
        ReplaySequencerEffects fx;
        if (intro_active || opening_.Active() || draw_end_ticks_ ||
            (show_results_ && (settlement_animating || audio_active)))
            return fx;
        if (!playing_ || !session_ || now_tick_ms < next_event_at_) return fx;
        next_event_at_ = now_tick_ms + speed_interval_ms;
        fx.Merge(Advance(speed_interval_ms, canvas_width, artwork_healthy, now_tick_ms, now_anim_ms, load_geometry));
        fx.update_ui = true;
        return fx;
    }

    ReplaySequencerEffects ReplayTableSequencer::EffectsTick() {
        ReplaySequencerEffects fx;
        if (draw_end_ticks_ && session_) {
            const unsigned before = 14 - draw_end_ticks_ / 2;
            --draw_end_ticks_;
            const unsigned slots = 14 - draw_end_ticks_ / 2;
            if (slots != before) {
                const unsigned seats = DrawEndChangedSeats(session_->state(), slots - 1);
                if (seats) fx.play_sounds.push_back(104);
                fx.logs.push_back("REPLAY DRAW SLOT index=" + std::to_string(slots - 1) +
                                  " seats=" + std::to_string(seats));
            }
            if (!draw_end_ticks_) {
                fx.logs.emplace_back("REPLAY DRAW TABLE DONE");
                show_results_ = true;
                result_pending_ = true;
                fx.sync_results_checkbox = true;
                fx.results_checked = true;
                fx.update_ui = true;
            }
            fx.dirty_all = true;
        }
        return fx;
    }

    ReplaySequencerEffects ReplayTableSequencer::OpeningTick(double now_anim_ms, bool intro_active) {
        ReplaySequencerEffects fx;
        if (intro_active) {
            opening_.ResumeClock(now_anim_ms);
            return fx;
        }
        if (opening_.Advance(now_anim_ms)) {
            if (opening_.sound()) fx.play_sounds.push_back(opening_.sound());
            if (!opening_.Active()) {
                if (animation_enabled_) fx.logs.emplace_back("ANIMATION END opening");
                fx.logs.emplace_back("OPENING DONE");
            }
            fx.dirty_all = true;
        }
        return fx;
    }

    ReplaySequencerEffects ReplayTableSequencer::MotionTick(double now_anim_ms) {
        ReplaySequencerEffects fx;
        const bool active = presentation_.Active();
        if (presentation_.Tick(now_anim_ms)) fx.logs.emplace_back("ANIMATION END");
        if (active) fx.dirty_table = true;
        return fx;
    }

    bool ReplayTableSequencer::CaptionTick() {
        if (!captions_.Active()) return false;
        captions_.Tick();
        return true;
    }

    ReplaySequencerEffects ReplayTableSequencer::OnMatchIntroFinished(double now_anim_ms) {
        ReplaySequencerEffects fx;
        opening_.ResumeClock(now_anim_ms);
        if (start_sound_pending_) {
            start_sound_pending_ = false;
            fx.play_sounds.push_back(112);
        }
        fx.update_ui = true;
        return fx;
    }

    // =========================================================================
    // OnlineTableSequencer
    // =========================================================================
    void OnlineSequencerEffects::Merge(const OnlineSequencerEffects &other) {
        play_sounds.insert(play_sounds.end(), other.play_sounds.begin(), other.play_sounds.end());
        audio_queue_overflow = audio_queue_overflow || other.audio_queue_overflow;
        if (other.stop_all_sound) {
            stop_all_sound = true;
            preserve_intro_resume = other.preserve_intro_resume;
        }
        stop_audio_only = stop_audio_only || other.stop_audio_only;
        cancel_settlement = cancel_settlement || other.cancel_settlement;
        cancel_intro = cancel_intro || other.cancel_intro;
        if (other.intro.kind != OnlineIntroRequest::Kind::None) intro = other.intro;
        reload_config_preferences = reload_config_preferences || other.reload_config_preferences;
        confirm_match_start = confirm_match_start || other.confirm_match_start;
        if (other.submit_action.has_value()) submit_action = other.submit_action;
        if (other.result_timer_ms.has_value()) result_timer_ms = other.result_timer_ms;
        if (other.auto_discard_timer_ms.has_value()) auto_discard_timer_ms = other.auto_discard_timer_ms;
        start_deferred_sound_timer = start_deferred_sound_timer || other.start_deferred_sound_timer;
        release_capture = release_capture || other.release_capture;
        focus_action_view = focus_action_view || other.focus_action_view;
        focus_window = focus_window || other.focus_window;
        arrange = arrange || other.arrange;
        dirty_all = dirty_all || other.dirty_all;
        dirty_board = dirty_board || other.dirty_board;
        update_lobby_music = update_lobby_music || other.update_lobby_music;
        if (other.settlement_sync.valid) settlement_sync = other.settlement_sync;
        if (other.notified_snapshot.has_value()) notified_snapshot = other.notified_snapshot;
        logs.insert(logs.end(), other.logs.begin(), other.logs.end());
    }

    bool OnlineTableSequencer::AutoTimerNeeded(const LobbySnapshot &live) const {
        const auto &snapshot = Snapshot(live);
        return snapshot.actions.enabled && !snapshot.actions.choices.empty() && !snapshot.game.ended &&
               (automatic_[1] || automatic_[2] || automatic_[3] || snapshot.game.players[0].reach >= 2);
    }

    bool OnlineTableSequencer::LocalTableInput(const LobbySnapshot &live, bool intro_active) const {
        if (show_results_ || intro_active || opening_.Active() || start_barrier_) return false;
        const auto &s = Snapshot(live);
        return s.phase == LobbyPhase::Game && s.game.initialized && s.actions.enabled && s.actions.pending.empty() &&
               !s.game.ended && !s.game.players[0].retired && !s.recovering && !live.table_paused && !live.exit_pending;
    }

    bool OnlineTableSequencer::AutomaticInput(const LobbySnapshot &live) const {
        if (show_results_) return false;
        const auto &s = Snapshot(live);
        return s.phase == LobbyPhase::Game && s.game.initialized && !s.recovering &&
               !live.table_paused && !live.exit_pending;
    }

    bool OnlineTableSequencer::SuppressCallPrompt(const LobbySnapshot &live) const {
        const auto &a = Snapshot(live).actions;
        return (automatic_[1] || automatic_[2]) &&
               std::any_of(a.choices.begin(), a.choices.end(),
                           [](const OnlineAction &c) { return c.kind == "skip"; }) &&
               std::none_of(a.choices.begin(), a.choices.end(),
                            [](const OnlineAction &c) { return c.kind == "ron"; });
    }

    bool OnlineTableSequencer::ActionReady(const LobbySnapshot &live, bool intro_active, bool table_animating) const {
        const auto &s = Snapshot(live);
        const auto &a = s.actions;
        return !SuppressCallPrompt(live) && !intro_active && !opening_.Active() && !Moving(table_animating) &&
               !show_results_ && !s.table_paused && !s.recovering && a.enabled && a.pending.empty();
    }

    bool OnlineTableSequencer::IsActionButtonEnabled(int control_id, const LobbySnapshot &live,
                                                     bool intro_active, bool table_animating) const {
        const auto &s = Snapshot(live);
        const auto &a = s.actions;
        if (control_id >= 5121 && control_id <= 5128) {
            if (!ActionReady(live, intro_active, table_animating)) return false;
            const char *kinds[]{"discard", "chi", "pon", "kan", "riichi", "ron", "tsumo", "draw", "skip", "next"};
            const auto group = [](const std::string &kind) {
                return kind.find("kan") != std::string::npos ? std::string("kan") : kind;
            };
            const std::string target = kinds[control_id - 5120];
            return std::any_of(a.choices.begin(), a.choices.end(),
                               [&](const OnlineAction &c) { return group(c.kind) == target; });
        }
        if (control_id == 5151) {
            return ActionReady(live, intro_active, table_animating) && !start_barrier_ &&
                   std::any_of(a.choices.begin(), a.choices.end(),
                               [](const OnlineAction &c) { return c.kind == "nuki"; });
        }
        if (control_id >= 5170 && control_id <= 5172) {
            return LocalTableInput(live, intro_active) && a.pending.empty();
        }
        if (control_id == 5173) {
            return LocalTableInput(live, intro_active) && selection_.Confirm(a) != static_cast<std::size_t>(-1);
        }
        if (control_id == 5129) {
            return a.pending.empty() && !s.recovering && !s.table_paused;
        }
        if (control_id == 0xbc6) {
            return !live.exit_pending && !live.exit_voting && !live.recovering;
        }
        return true;
    }

    std::wstring OnlineTableSequencer::FormatCountdown(const LobbySnapshot &snapshot,
                                                       std::uint64_t now_tick_ms) const {
        if (!clock_active_ || clock_revision_ != snapshot.actions.revision || !snapshot.actions.pending.empty())
            return {};
        if (!clock_deadline_) return L"\u221e+0";
        const auto remaining = clock_deadline_ > now_tick_ms ? clock_deadline_ - now_tick_ms : 0;
        const auto base = clock_base_ms_ < 0
                              ? remaining
                              : std::min(remaining, clock_base_deadline_ > now_tick_ms
                                                        ? clock_base_deadline_ - now_tick_ms
                                                        : 0);
        return std::to_wstring((base + 999) / 1000) + L"+" + std::to_wstring((remaining - base + 999) / 1000);
    }

    TableInteraction OnlineTableSequencer::BuildInteraction(const LobbySnapshot &snapshot,
                                                            const LobbySnapshot &live,
                                                            bool intro_active, bool table_animating,
                                                            int hovered_button,
                                                            const std::function<int()> &hit_tile,
                                                            std::uint64_t now_tick_ms) const {
        TableInteraction result;
        result.automatic = automatic_;
        if (snapshot.variant_active && snapshot.variant.Preparing()) {
            if (snapshot.variant.phase == VariantPhase::Exchange) {
                const auto &direction = snapshot.variant.direction;
                result.hint = (direction == "clockwise" ? L"顺时针"
                               : direction == "counterclockwise" ? L"逆时针" : L"对家") +
                              std::wstring(L"换三张  ") + std::to_wstring(selection_.selected.size()) + L" / 3";
            } else {
                result.hint = L"请选择定缺花色";
            }
            if ((!snapshot.actions.pending.empty() || snapshot.actions.choices.empty()) && !snapshot.recovering)
                result.hint = L"选择完毕，等待其他玩家";
        }
        result.hovered_button = hovered_button;
        if (!LocalTableInput(live, intro_active)) return result;
        if (hit_tile) result.hovered = hit_tile();
        // Local hover/manual order remain available during opponent queue waits.
        // Playable colours and requests still require the presented live revision.
        if (Moving(table_animating) || snapshot.actions.revision != live.actions.revision) return result;
        const std::string kind = action_filter_ == "riichi" ? "riichi" : "discard";
        for (const auto &choice: snapshot.actions.choices)
            if (choice.kind == kind && choice.tiles.size() == 1)
                result.playable.push_back(choice.tiles[0]);
        if (selection_.HandSelection()) result.playable = selection_.Selectable(snapshot.actions);
        result.selected = selection_.selected;
        result.selecting = selection_.HandSelection();
        result.restrict_discards = !snapshot.variant_active &&
                                   (!result.playable.empty() || action_filter_ == "riichi") && !result.selecting;
        result.countdown = FormatCountdown(snapshot, now_tick_ms);
        return result;
    }

    OnlineSequencerEffects OnlineTableSequencer::CancelHandDrag() {
        OnlineSequencerEffects fx;
        pressed_tile_ = pressed_auto_ = -1;
        hand_dragged_ = false;
        fx.release_capture = true;
        return fx;
    }

    OnlineSequencerEffects OnlineTableSequencer::CancelSelection() {
        OnlineSequencerEffects fx;
        if (selection_.group == "riichi") riichi_sound_revision_ = 0;
        selection_.Clear();
        action_filter_ = "discard";
        fx.arrange = true;
        fx.dirty_all = true;
        return fx;
    }

    OnlineSequencerEffects OnlineTableSequencer::CancelResultTimer() {
        OnlineSequencerEffects fx;
        fx.result_timer_ms = 0u;
        result_deadline_ = 0;
        return fx;
    }

    OnlineSequencerEffects OnlineTableSequencer::ResetActionClock() {
        OnlineSequencerEffects fx;
        clock_active_ = false;
        clock_deadline_ = 0;
        clock_warning_second_ = -1;
        return fx;
    }

    void OnlineTableSequencer::ResetForStopSound() {
        opening_.Cancel();
        opening_flowers_ = {};
        deferred_sounds_.clear();
        result_pending_ = false;
    }

    OnlineSequencerEffects OnlineTableSequencer::ClearPresentationQueue(const OnlineMotionCallbacks &motion) {
        OnlineSequencerEffects fx = CancelHandDrag();
        captions_.Clear();
        presentation_queue_.clear();
        queue_timer_active_ = false;
        pause_ticks_ = draw_end_ticks_ = 0;
        displaying_ = false;
        automatic_discard_at_ = 0;
        fx.auto_discard_timer_ms = 0u;
        if (motion.set_draw_end_slots) motion.set_draw_end_slots(14);
        return fx;
    }

    void OnlineTableSequencer::QueueOrEmitSound(unsigned id, OnlineSequencerEffects &fx,
                                                bool window_visible, bool audio_healthy) {
        if (!EffectsEnabled() || !audio_healthy || !window_visible) return;
        if (opening_.Active() && id != 104 && id != 105 && id != 102 && id != 112) {
            if (deferred_sounds_.size() < 256) deferred_sounds_.push_back(id);
            else fx.audio_queue_overflow = true;
            return;
        }
        fx.play_sounds.push_back(id);
    }

    std::optional<unsigned> OnlineTableSequencer::PopDeferredSound(bool intro_active) {
        if (opening_.Active() || intro_active || start_barrier_ || deferred_sounds_.empty())
            return std::nullopt;
        const auto id = deferred_sounds_.front();
        deferred_sounds_.erase(deferred_sounds_.begin());
        return id;
    }

    OnlineSequencerEffects OnlineTableSequencer::OnMatchIntroFinished(const LobbySnapshot &live,
                                                                      const OnlineSequencerContext &ctx,
                                                                      const OnlineMotionCallbacks &motion) {
        OnlineSequencerEffects fx;
        opening_.ResumeClock(ctx.anim_ms);
        if (start_barrier_) {
            if (Snapshot(live).official) start_barrier_ = false;
            fx.confirm_match_start = true;
        } else if (start_sound_pending_) {
            start_sound_pending_ = false;
            QueueOrEmitSound(112, fx, ctx.window_visible, ctx.audio_healthy);
        }
        fx.logs.emplace_back("ONLINE MATCH INTRO DONE");
        auto updated_ctx = ctx;
        updated_ctx.intro_active = false;
        updated_ctx.intro_resuming = false;
        fx.Merge(SyncSnapshot(Snapshot(live), live, updated_ctx, motion));
        return fx;
    }

    void OnlineTableSequencer::ReceiveActionClock(const NetworkMessage &message, const LobbySnapshot &snapshot,
                                                  std::uint64_t now_tick_ms) {
        if (message.tag != "CLOCK") return;
        ResetActionClock();
        if (snapshot.phase != LobbyPhase::Game || !snapshot.actions.enabled || snapshot.actions.choices.empty() ||
            snapshot.game.ended || snapshot.recovering)
            return;
        const auto it = message.attributes.find("ms");
        if (it == message.attributes.end()) return;
        int ms = 0;
        const auto &text = it->second;
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), ms);
        if (parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() || ms < -1 || ms > 86400000) return;
        clock_active_ = true;
        clock_revision_ = snapshot.actions.revision;
        clock_base_ms_ = -1;
        if (const auto base = message.attributes.find("base_ms"); base != message.attributes.end()) {
            int value = 0;
            const auto read = std::from_chars(base->second.data(), base->second.data() + base->second.size(), value);
            if (read.ec == std::errc() && read.ptr == base->second.data() + base->second.size() &&
                value >= 0 && value <= 300000)
                clock_base_ms_ = value;
        }
        clock_deadline_ = ms < 0 ? 0 : now_tick_ms + static_cast<std::uint64_t>(ms);
        clock_base_deadline_ = now_tick_ms + static_cast<std::uint64_t>(std::max(0, clock_base_ms_));
    }

    OnlineSequencerEffects OnlineTableSequencer::ActionClockTick(const LobbySnapshot &live,
                                                                 const OnlineSequencerContext &ctx) {
        OnlineSequencerEffects fx;
        if (!clock_active_ || !clock_deadline_) return fx;
        if (live.actions.revision != clock_revision_ || !live.actions.pending.empty() || live.game.ended ||
            live.recovering || live.actions.choices.empty()) {
            ResetActionClock();
            return fx;
        }
        const auto remaining = ctx.tick_ms >= clock_deadline_ ? 0 : clock_deadline_ - ctx.tick_ms;
        const int second = static_cast<int>((remaining + 999) / 1000);
        if (second <= 2 && second != clock_warning_second_) {
            clock_warning_second_ = second;
            QueueOrEmitSound(108, fx, ctx.window_visible, ctx.audio_healthy);
        }
        if (!remaining) clock_deadline_ = 0;
        return fx;
    }

    OnlineSequencerEffects OnlineTableSequencer::UpdateActions(const LobbySnapshot &live, bool compact) {
        OnlineSequencerEffects fx;
        const auto &s = Snapshot(live);
        const auto &actions = s.actions;
        const bool selecting = selection_.Active();
        selection_.Sync(actions);
        if (s.variant_active && s.variant.phase == VariantPhase::Exchange && !selection_.Active())
            selection_.Open(actions, "exchange");
        if (compact && s.game.initialized && !s.variant_active && !selection_.Active() && !SuppressCallPrompt(live))
            selection_.Open(actions, "call");
        action_revision_ = actions.revision;
        if (selecting && !selection_.Active()) action_filter_ = "discard";
        fx.arrange = true;
        return fx;
    }

    void OnlineTableSequencer::AnnounceRiichi(const LobbySnapshot &live, const OnlineSequencerContext &ctx,
                                              OnlineSequencerEffects &fx) {
        const auto &s = Snapshot(live);
        if (riichi_sound_revision_ == s.actions.revision) return;
        riichi_sound_revision_ = s.actions.revision;
        ReplayEvent event;
        event.kind = ReplayEventKind::Reach;
        event.attributes["step"] = "1";
        event.who = 0;
        if (captions_.Accept(event)) fx.dirty_all = true;
        for (auto id: ReplaySounds(event, s.game.players[0].gender, effects_only_, individual_voices_))
            QueueOrEmitSound(id, fx, ctx.window_visible, ctx.audio_healthy);
        fx.logs.push_back("ONLINE RIICHI ANNOUNCE revision=" + std::to_string(riichi_sound_revision_));
    }

    OnlineSequencerEffects OnlineTableSequencer::Submit(std::size_t index, const LobbySnapshot &live,
                                                        const OnlineSequencerContext &ctx, bool table_animating) {
        OnlineSequencerEffects fx;
        if (live.table_paused || live.exit_pending) return fx;
        const auto &s = Snapshot(live);
        if (ctx.intro_active || opening_.Active() || Moving(table_animating) ||
            s.actions.revision != live.actions.revision)
            return fx;
        const auto &a = s.actions;
        if (!a.enabled || !a.pending.empty() || index >= a.choices.size()) return fx;
        if (s.recovering) return fx;
        if (show_results_ && (a.choices[index].kind != "next" || ctx.settlement_animating)) return fx;
        if (a.choices[index].kind == "riichi") {
            AnnounceRiichi(live, ctx, fx);
            suppress_self_reach_ = true;
        }
        fx.submit_action = std::make_pair(action_revision_, index);
        return fx;
    }

    OnlineSequencerEffects OnlineTableSequencer::AutomaticAction(const LobbySnapshot &live,
                                                                 const OnlineSequencerContext &ctx,
                                                                 bool table_animating) {
        OnlineSequencerEffects fx;
        const auto &s = Snapshot(live);
        if (s.variant_active && s.variant.Preparing()) return fx;
        if (!ctx.window_visible || ctx.intro_active || opening_.Active() || Moving(table_animating) ||
            s.phase != LobbyPhase::Game || !s.actions.enabled || !s.actions.pending.empty() ||
            s.game.ended || s.recovering || live.table_paused)
            return fx;
        const auto &choices = s.actions.choices;
        for (std::size_t i = 0; i < choices.size(); ++i)
            if (automatic_[3] && (choices[i].kind == "ron" || choices[i].kind == "tsumo")) {
                fx.Merge(Submit(i, live, ctx, table_animating));
                return fx;
            }
        const bool ron = std::any_of(choices.begin(), choices.end(),
                                     [](const OnlineAction &c) { return c.kind == "ron"; });
        for (std::size_t i = 0; i < choices.size(); ++i) {
            const auto &c = choices[i];
            if ((automatic_[1] || automatic_[2]) && !ron && c.kind == "skip") {
                fx.Merge(Submit(i, live, ctx, table_animating));
                return fx;
            }
            if ((automatic_[2] || s.game.players[0].reach >= 2) && c.kind == "discard" &&
                c.tiles == std::vector<int>{s.game.players[0].drawn_tile} &&
                std::all_of(choices.begin(), choices.end(),
                            [](const OnlineAction &a) { return a.kind == "discard"; })) {
                if (!automatic_discard_at_ || ctx.tick_ms < automatic_discard_at_) return fx;
                fx.logs.push_back("ONLINE AUTO DISCARD tile=" + std::to_string(s.game.players[0].drawn_tile) +
                                  " tick=" + std::to_string(ctx.tick_ms));
                fx.Merge(Submit(i, live, ctx, table_animating));
                return fx;
            }
        }
        return fx;
    }

    OnlineSequencerEffects OnlineTableSequencer::ApplyAutomaticSort(const LobbySnapshot &live,
                                                                    const OnlineMotionCallbacks &motion) {
        OnlineSequencerEffects fx;
        if (!sort_refresh_pending_) return fx;
        sort_refresh_pending_ = false;
        if (motion.set_auto_sort) motion.set_auto_sort(automatic_[0], Snapshot(live).game);
        if (!animation_enabled_ && motion.cancel_animation) motion.cancel_animation();
        fx.arrange = true;
        fx.dirty_board = true;
        return fx;
    }

    OnlineSequencerEffects OnlineTableSequencer::HandleCommand(int id, const LobbySnapshot &live, bool compact,
                                                               const OnlineSequencerContext &ctx,
                                                               const OnlineMotionCallbacks &motion) {
        OnlineSequencerEffects fx;
        const auto &s = Snapshot(live);
        if (pressed_action_ == id) {
            pressed_action_ = -1;
            if (button_revision_ != s.actions.revision) return fx;
        }
        const bool table_anim = motion.animating ? motion.animating() : false;
        if (id == 5174) {
            show_results_ = true;
            fx.Merge(SyncSnapshot(s, live, ctx, motion));
            return fx;
        }
        if (id >= 5170 && id <= 5173) {
            if (!LocalTableInput(live, ctx.intro_active)) return fx;
            if (id == 5173) {
                const auto index = selection_.Confirm(s.actions);
                if (index != static_cast<std::size_t>(-1)) fx.Merge(Submit(index, live, ctx, table_anim));
            } else {
                const auto &choices = s.actions.choices;
                for (std::size_t i = 0; i < choices.size(); ++i)
                    if (choices[i].kind == "dingque" && choices[i].suit == id - 5170) {
                        fx.Merge(Submit(i, live, ctx, table_anim));
                        break;
                    }
            }
            return fx;
        }
        if (live.table_paused) return fx;
        if (id == 5150) {
            fx.Merge(CancelSelection());
            fx.Merge(UpdateActions(live, compact));
            fx.arrange = true;
            return fx;
        }
        if (SuppressCallPrompt(live) && id >= 5121 && id <= 5128) return fx;
        if ((id >= 5120 && id < 5130) || id == 5151) {
            if (ctx.intro_active || opening_.Active() || Moving(table_anim) ||
                (show_results_ && (id != 5129 || ctx.settlement_animating)))
                return fx;
            const char *kinds[]{"discard", "chi", "pon", "kan", "riichi", "ron", "tsumo", "draw", "skip", "next"};
            const auto &choices = s.actions.choices;
            const std::string wanted = id == 5151 ? "nuki" : kinds[id - 5120];
            if (std::none_of(choices.begin(), choices.end(), [&](const OnlineAction &c) {
                    return c.kind == wanted || (wanted == "kan" && c.kind.find("kan") != std::string::npos);
                }))
                return fx;
            if (id == 5124) AnnounceRiichi(live, ctx, fx);
            action_filter_ = wanted;
            fx.Merge(UpdateActions(live, compact));
            if (compact && id == 5123) {
                selection_.Open(s.actions, "kan");
                const auto unique = selection_.UniqueCall(s.actions, (s.game.type & 2) == 0);
                if (unique != static_cast<std::size_t>(-1)) fx.Merge(Submit(unique, live, ctx, table_anim));
                else {
                    fx.arrange = true;
                    fx.focus_action_view = true;
                    fx.dirty_all = true;
                }
                return fx;
            }
            if (id == 5121 || id == 5122) {
                selection_.Open(s.actions, wanted);
                const auto unique = selection_.UniqueCall(s.actions, (s.game.type & 2) == 0);
                if (unique != static_cast<std::size_t>(-1)) {
                    fx.Merge(Submit(unique, live, ctx, table_anim));
                    return fx;
                }
            }
            if (compact && (id == 5121 || id == 5122 || id == 5124)) {
                fx.logs.push_back("ACTION SELECT group=" + wanted + " revision=" + std::to_string(action_revision_));
                if (id == 5124) selection_.Open(s.actions, wanted);
                fx.arrange = true;
                fx.focus_window = true;
                fx.dirty_all = true;
                return fx;
            }
            fx.Merge(CancelSelection());
            action_filter_ = wanted;
            if (id >= 5125) {
                for (std::size_t i = 0; i < choices.size(); ++i)
                    if (choices[i].kind == wanted) {
                        fx.Merge(Submit(i, live, ctx, table_anim));
                        break;
                    }
            }
        }
        return fx;
    }

    void OnlineTableSequencer::PlayEventSound(const ReplayEvent &event, const LobbySnapshot &snapshot,
                                              const OnlineSequencerContext &ctx, OnlineSequencerEffects &fx) {
        const char gender = event.who >= 0 && event.who < 4 ? snapshot.game.players[event.who].gender : '?';
        const bool own_reach = event.kind == ReplayEventKind::Reach && event.who == 0 &&
                               event.attributes.at("step") == "1";
        if (!(own_reach && suppress_self_reach_))
            for (auto id: ReplaySounds(event, gender, effects_only_, individual_voices_))
                QueueOrEmitSound(id, fx, ctx.window_visible, ctx.audio_healthy);
        if (own_reach) suppress_self_reach_ = false;
    }

    void OnlineTableSequencer::PresentVariantState(const LobbySnapshot &snapshot, const LobbySnapshot &live,
                                                   const OnlineSequencerContext &ctx,
                                                   const OnlineMotionCallbacks &motion, OnlineSequencerEffects &fx) {
        (void) live;
        const auto &state = snapshot.variant;
        const bool new_hand = state.hand_number != variant_hand_;
        const auto cues = variant_events_.Consume(state, snapshot.recovering);
        const bool active_live = !snapshot.recovering && ctx.window_visible;
        if (new_hand) {
            variant_hand_ = state.hand_number;
            fx.Merge(CancelResultTimer());
            show_results_ = false;
            automatic_ = {true, false, false, false};
            if (motion.set_auto_sort) motion.set_auto_sort(true, snapshot.game);
        }
        if (variant_events_.resynchronized || new_hand || !active_live) {
            fx.Merge(CancelResultTimer());
            fx.stop_all_sound = true;
            fx.preserve_intro_resume = false;
            ResetForStopSound();
            if (motion.cancel_animation) motion.cancel_animation();
            captions_.Clear();
            if (motion.set_draw_end_slots) motion.set_draw_end_slots(14);
            if (motion.accept) motion.accept(snapshot.game, ReplayEvent{}, false, ctx.anim_ms);
            show_results_ = snapshot.game.ended;
            bool opening_intro_started = false;
            if (new_hand && active_live && !snapshot.game.ended && (animation_enabled_ || EffectsEnabled())) {
                if (motion.begin_opening) motion.begin_opening(opening_, snapshot.game, original_display_);
                else opening_.Begin(OnlineTableState(snapshot.game), original_display_);
                if (!state.Sichuan() && !state.events.empty() && state.events.back().kind == "start")
                    for (int seat = 0; seat < 4; ++seat)
                        opening_flowers_[state.Relative(seat)] = !state.players[seat].flowers.empty();
                opening_.ResumeClock(ctx.anim_ms);
                if (state.hand_number == 1) {
                    if (start_barrier_) start_sound_pending_ = true;
                    else if (event_sound_allowed_) QueueOrEmitSound(112, fx, ctx.window_visible, ctx.audio_healthy);
                }
            }
            if (new_hand && active_live && !snapshot.game.ended && match_intro_pending_) {
                match_intro_pending_ = false;
                if (original_display_ && snapshot.actions.enabled) {
                    fx.intro = {OnlineIntroRequest::Kind::BeginMatch, OnlineTableState(snapshot.game), snapshot.game.type};
                    fx.logs.emplace_back("ONLINE MATCH INTRO BEGIN");
                    opening_intro_started = true;
                }
            }
            if (start_barrier_ && active_live && !ctx.intro_active && !opening_intro_started)
                fx.confirm_match_start = true;
            return;
        }
        ReplayEvent presentation;
        presentation.tag = "VARIANT";
        std::array<bool, 4> flower_caption{};
        for (const auto &cue: cues) {
            const auto event = VariantReplayEvent(cue, state);
            if (event.kind != ReplayEventKind::Metadata) presentation = event;
            captions_.Accept(event);
            if (cue.kind == "exchange" || cue.kind == "dingque") {
                captions_.AcceptText(TextKey::caption_complete, event.who);
                if (event_sound_allowed_) QueueOrEmitSound(106, fx, ctx.window_visible, ctx.audio_healthy);
            } else if (cue.kind == "flower") {
                if (!flower_caption[event.who]) {
                    flower_caption[event.who] = true;
                    captions_.AcceptText(TextKey::caption_kita, event.who);
                    if (event_sound_allowed_) QueueOrEmitSound(107, fx, ctx.window_visible, ctx.audio_healthy);
                }
            }
            if (event_sound_allowed_) PlayEventSound(event, snapshot, ctx, fx);
        }
        pause_ticks_ = ActionPauseTicks(presentation);
        pause_after_motion_ = presentation.kind == ReplayEventKind::Draw;
        const bool draw_end = snapshot.game.ended && !snapshot.game.settlement.receipts.empty() &&
                              snapshot.game.settlement.receipts.back().who < 0;
        if (draw_end) presentation.kind = ReplayEventKind::DrawEnd;
        try {
            if (motion.accept)
                motion.accept(snapshot.game, presentation,
                              animation_enabled_ && !draw_end && !state.Preparing(), ctx.anim_ms);
        } catch (const std::exception &error) {
            if (motion.cancel_animation) motion.cancel_animation();
            fx.logs.push_back(std::string("VARIANT ANIMATION FALLBACK ") + error.what());
        }
        if (presentation.kind == ReplayEventKind::Draw && presentation.who == 0) {
            automatic_discard_at_ = ctx.tick_ms + kAutomaticDiscardWaitMs;
            fx.auto_discard_timer_ms = kAutomaticDiscardWaitMs;
        }
        if (presentation.kind == ReplayEventKind::Win) {
            pause_ticks_ = 134;
            pause_after_motion_ = false;
            if (snapshot.game.ended) {
                result_deadline_ = ctx.tick_ms + 2000;
                fx.result_timer_ms = 2000u;
            }
        } else if (draw_end) {
            draw_end_ticks_ = 28;
            queue_timer_active_ = true;
            if (motion.set_draw_end_slots) motion.set_draw_end_slots(0);
        }
    }

    void OnlineTableSequencer::PresentGameEvent(const NetworkMessage &message, const LobbySnapshot &snapshot,
                                                const LobbySnapshot &live, const OnlineSequencerContext &ctx,
                                                const OnlineMotionCallbacks &motion, OnlineSequencerEffects &fx) {
        if (message.tag == "VSTATE") {
            PresentVariantState(snapshot, live, ctx, motion, fx);
            return;
        }
        if (message.tag == "RESTORED") {
            fx.stop_all_sound = true;
            fx.preserve_intro_resume = false;
            ResetForStopSound();
            if (motion.cancel_animation) motion.cancel_animation();
            match_intro_pending_ = false;
            sound_action_revision_ = snapshot.actions.revision;
            resume_preparation_pending_ = false;
            show_results_ = snapshot.game.ended;
            if (motion.refresh_preferences) motion.refresh_preferences();
            if (motion.accept) motion.accept(snapshot.game, ReplayEvent{}, false, ctx.anim_ms);
            return;
        }
        if (message.tag == "GO") {
            fx.cancel_intro = true;
            match_intro_pending_ = true;
            start_barrier_ = false;
            start_sound_pending_ = false;
        }
        if (snapshot.official && message.tag == "TAIKYOKU" && match_intro_pending_ && !snapshot.recovering) {
            match_intro_pending_ = false;
            if (original_display_ && ctx.window_visible) {
                auto state = OnlineTableState(snapshot.game);
                state.initialized = true;
                start_barrier_ = true;
                fx.intro = {OnlineIntroRequest::Kind::BeginMatch, std::move(state), snapshot.game.type};
                if (motion.prepare) motion.prepare();
            } else {
                fx.confirm_match_start = true;
            }
            return;
        }
        if (message.tag == "PREPARE") {
            start_barrier_ = true;
            return;
        }
        if (message.tag == "START") {
            start_barrier_ = false;
            opening_.ResumeClock(ctx.anim_ms);
            if (start_sound_pending_) {
                start_sound_pending_ = false;
                QueueOrEmitSound(112, fx, ctx.window_visible, ctx.audio_healthy);
            }
            fx.logs.emplace_back("ONLINE ALL READY START");
            fx.Merge(UpdateActions(live, Snapshot(live).game.initialized));
            fx.arrange = true;
            return;
        }
        if (message.tag == "AGARI" || message.tag == "RYUUKYOKU" || message.tag == "PROF")
            fx.cancel_intro = true;
        if (message.tag == "REINIT") {
            fx.Merge(CancelResultTimer());
            suppress_self_reach_ = false;
            match_intro_pending_ = false;
            sound_action_revision_ = snapshot.actions.revision;
            if (motion.accept) motion.accept(snapshot.game, ReplayEvent{}, false, ctx.anim_ms);
            fx.stop_all_sound = true;
            fx.preserve_intro_resume = false;
            ResetForStopSound();
            if (motion.cancel_animation) motion.cancel_animation();
            return;
        }
        if (!snapshot.game.initialized || snapshot.recovering || !ctx.window_visible) return;
        ReplayEvent event;
        event.tag = message.tag;
        event.attributes = message.attributes;
        if (event.tag == "INIT") {
            fx.Merge(CancelResultTimer());
            suppress_self_reach_ = false;
            fx.stop_all_sound = true;
            fx.preserve_intro_resume = false;
            ResetForStopSound();
            if (motion.cancel_animation) motion.cancel_animation();
            event.kind = ReplayEventKind::Init;
        } else if (event.tag == "N") event.kind = ReplayEventKind::Meld;
        else if (event.tag == "REACH") event.kind = ReplayEventKind::Reach;
        else if (event.tag == "AGARI") event.kind = ReplayEventKind::Win;
        else if (event.tag == "RYUUKYOKU") event.kind = ReplayEventKind::DrawEnd;
        else if (!event.tag.empty() && std::string("TUVWtuvwDEFGdefg").find(event.tag[0]) != std::string::npos &&
                 std::all_of(event.tag.begin() + 1, event.tag.end(), [](char c) { return c >= '0' && c <= '9'; })) {
            const auto found = std::string("TUVWtuvwDEFGdefg").find(event.tag[0]);
            event.kind = found < 8 ? ReplayEventKind::Draw : ReplayEventKind::Discard;
            event.who = static_cast<int>(found % 4);
        } else {
            return;
        }
        const bool result = event.kind == ReplayEventKind::Win || event.kind == ReplayEventKind::DrawEnd;
        if (event.attributes.count("who")) event.who = std::stoi(event.attributes.at("who"));
        if (!(event.kind == ReplayEventKind::Reach && event.who == 0 && suppress_self_reach_) &&
            captions_.Accept(event)) {
            fx.logs.push_back("ONLINE CAPTION id=" + std::to_string(ActionCaptionResource(event)) +
                              " who=" + std::to_string(event.who));
        }
        pause_ticks_ = ActionPauseTicks(event);
        pause_after_motion_ = event.kind == ReplayEventKind::Draw;
        const bool replaced = motion.animating ? motion.animating() : false;
        try {
            if (motion.accept)
                motion.accept(snapshot.game, event,
                              animation_enabled_ && !show_results_ && !opening_.Active(), ctx.anim_ms);
            if (motion.animating && motion.animating()) {
                fx.logs.push_back("ONLINE ANIMATION BEGIN tag=" + event.tag + " replaced=" + std::to_string(replaced));
            }
        } catch (const std::exception &e) {
            fx.logs.push_back(std::string("ONLINE ANIMATION FALLBACK ") + e.what());
            if (motion.cancel_animation) motion.cancel_animation();
        }
        if (event.kind == ReplayEventKind::Draw && event.who == 0) {
            automatic_discard_at_ = ctx.tick_ms + kAutomaticDiscardWaitMs;
            fx.auto_discard_timer_ms = kAutomaticDiscardWaitMs;
            fx.logs.push_back("ONLINE AUTO DRAW WAIT until=" + std::to_string(automatic_discard_at_));
        } else if (event.kind == ReplayEventKind::Init || event.kind == ReplayEventKind::DrawEnd ||
                   event.kind == ReplayEventKind::Win || (event.kind == ReplayEventKind::Discard && event.who == 0)) {
            automatic_discard_at_ = 0;
            fx.auto_discard_timer_ms = 0u;
        }
        result_pending_ = result;
        if (result) {
            if (opening_.Active() || snapshot.game.settlement.receipts.size() == 1) fx.stop_audio_only = true;
            opening_.Cancel();
            deferred_sounds_.clear();
            fx.cancel_settlement = true;
            if (event.kind == ReplayEventKind::DrawEnd && !show_results_) {
                draw_end_ticks_ = 28;
                queue_timer_active_ = true;
                if (motion.set_draw_end_slots) motion.set_draw_end_slots(0);
                fx.logs.push_back("ONLINE DRAW TABLE BEGIN tick=" + std::to_string(ctx.tick_ms));
            }
            if (event.kind == ReplayEventKind::Win && !show_results_ && !result_deadline_) {
                constexpr unsigned delay = 2000;
                result_deadline_ = ctx.tick_ms + delay;
                fx.result_timer_ms = delay;
                fx.logs.push_back("ONLINE RESULT WAIT ms=" + std::to_string(delay) +
                                  " tick=" + std::to_string(ctx.tick_ms));
            }
        }
        const bool intro = event.kind == ReplayEventKind::Init && match_intro_pending_ &&
                           original_display_ && snapshot.actions.enabled;
        if (event.kind == ReplayEventKind::Init && (intro || start_barrier_))
            start_sound_pending_ = !ReplaySounds(event).empty();
        else if (event_sound_allowed_) PlayEventSound(event, snapshot, ctx, fx);
        if (event.kind == ReplayEventKind::Init && (animation_enabled_ || EffectsEnabled())) {
            if (motion.begin_opening) motion.begin_opening(opening_, snapshot.game, original_display_);
            else opening_.Begin(OnlineTableState(snapshot.game), original_display_);
            opening_.ResumeClock(ctx.anim_ms);
            fx.logs.push_back("ONLINE OPENING BEGIN wall=" + std::to_string(opening_.wall_tiles()));
        }
        bool opening_intro_started = false;
        if (event.kind == ReplayEventKind::Init && match_intro_pending_) {
            match_intro_pending_ = false;
            if (original_display_ && snapshot.actions.enabled) {
                fx.intro = {OnlineIntroRequest::Kind::BeginMatch, OnlineTableState(snapshot.game), snapshot.game.type};
                if (motion.prepare) motion.prepare();
                fx.Merge(UpdateActions(live, Snapshot(live).game.initialized));
                fx.logs.emplace_back("ONLINE MATCH INTRO BEGIN");
                opening_intro_started = true;
            }
        }
        if (event.kind == ReplayEventKind::Init && start_barrier_ && !ctx.intro_active &&
            !opening_intro_started && !snapshot.recovering)
            fx.confirm_match_start = true;
    }

    OnlineSequencerEffects OnlineTableSequencer::OnGameEvent(const NetworkMessage &message,
                                                             const LobbySnapshot &snapshot,
                                                             const LobbySnapshot &live,
                                                             const OnlineSequencerContext &ctx,
                                                             const OnlineMotionCallbacks &motion) {
        OnlineSequencerEffects fx;
        if (snapshot.silent_recovery && snapshot.recovering) return fx;
        const bool table_anim = motion.animating ? motion.animating() : false;
        if (snapshot.variant_active) {
            if (message.tag == "VGO") {
                fx.Merge(ClearPresentationQueue(motion));
                fx.stop_all_sound = true;
                fx.preserve_intro_resume = false;
                ResetForStopSound();
                if (motion.cancel_animation) motion.cancel_animation();
                fx.Merge(CancelResultTimer());
                start_barrier_ = false;
                variant_events_ = {};
                variant_hand_ = 0;
                show_results_ = false;
                match_intro_pending_ = true;
                return fx;
            }
            if (message.tag == "VSTATE") {
                if (!snapshot.variant.Preparing() || snapshot.actions.revision != Snapshot(live).actions.revision)
                    fx.Merge(CancelHandDrag());
                if (snapshot.recovering || !ctx.window_visible) {
                    fx.Merge(ClearPresentationQueue(motion));
                    PresentVariantState(snapshot, live, ctx, motion, fx);
                    return fx;
                }
                presentation_queue_.push_back({message, snapshot, EffectsEnabled() && ctx.audio_healthy});
                queue_timer_active_ = true;
                if (!opening_.Active() && !ctx.intro_active && !pause_ticks_ && !draw_end_ticks_ &&
                    !table_anim && !start_barrier_) {
                    fx.Merge(PresentationTick(live, ctx, motion));
                }
                if (start_barrier_ && (!displaying_ || snapshot.variant.hand_number != variant_hand_)) {
                    auto item = std::move(presentation_queue_.back());
                    presentation_queue_.clear();
                    displayed_ = std::move(item.snapshot);
                    displaying_ = true;
                    event_sound_allowed_ = item.sound_allowed;
                    PresentVariantState(displayed_, live, ctx, motion, fx);
                }
                return fx;
            }
            if (message.tag == "CLOCK") {
                ReceiveActionClock(message, snapshot, ctx.tick_ms);
                return fx;
            }
        }
        if (message.tag == "RESTORE") resume_preparation_pending_ = match_intro_pending_;
        if (message.tag == "TABLEEND") {
            fx.Merge(ClearPresentationQueue(motion));
            fx.Merge(CancelSelection());
            fx.Merge(CancelResultTimer());
            show_results_ = false;
            fx.stop_all_sound = true;
            fx.preserve_intro_resume = false;
            ResetForStopSound();
            if (motion.cancel_animation) motion.cancel_animation();
            start_barrier_ = false;
            match_intro_pending_ = true;
            return fx;
        }
        if (message.tag == "EXITVOTE" || message.tag == "PAUSED") return fx;
        if (message.tag == "INIT" || message.tag == "REINIT") {
            if (message.tag == "INIT" || !snapshot.silent_recovery) {
                fx.reload_config_preferences = true;
                automatic_ = {true, false, false, false};
                sort_refresh_pending_ = false;
                if (motion.set_auto_sort) motion.set_auto_sort(true, snapshot.game);
                fx.logs.emplace_back("ONLINE AUTO RESET sort=1 no_call=0 tsumogiri=0 win=0");
            }
            if (motion.cancel_animation) motion.cancel_animation();
        }
        ReceiveActionClock(message, snapshot, ctx.tick_ms);
        const auto &tag = message.tag;
        const bool reset = tag == "GO" || tag == "INIT" || tag == "REINIT";
        if (reset || snapshot.recovering || !ctx.window_visible) fx.Merge(ClearPresentationQueue(motion));
        if (tag == "INIT" && ctx.window_visible) {
            displayed_ = snapshot;
            displaying_ = true;
        }
        const bool tile = !tag.empty() && std::string("TUVWtuvwDEFGdefg").find(tag[0]) != std::string::npos &&
                          std::all_of(tag.begin() + 1, tag.end(), [](char c) { return c >= '0' && c <= '9'; });
        const bool event = tile || tag == "N" || tag == "DORA" || tag == "REACH" || tag == "FURITEN" ||
                           tag == "AGARI" || tag == "RYUUKYOKU" || tag == "PROF";
        const bool now_anim = motion.animating ? motion.animating() : false;
        if (snapshot.game.initialized && !snapshot.recovering && ctx.window_visible &&
            (event || !presentation_queue_.empty())) {
            presentation_queue_.push_back({message, snapshot, EffectsEnabled() && ctx.audio_healthy});
            queue_timer_active_ = true;
            if (!opening_.Active() && !ctx.intro_active && !pause_ticks_ && !draw_end_ticks_ && !now_anim)
                fx.Merge(PresentationTick(live, ctx, motion));
            return fx;
        }
        if (displaying_) displayed_ = snapshot;
        event_sound_allowed_ = true;
        PresentGameEvent(message, snapshot, live, ctx, motion, fx);
        return fx;
    }

    OnlineSequencerEffects OnlineTableSequencer::PresentationTick(const LobbySnapshot &live,
                                                                  const OnlineSequencerContext &ctx,
                                                                  const OnlineMotionCallbacks &motion) {
        OnlineSequencerEffects fx;
        if (live.table_paused) return fx;
        if (opening_.Active() || ctx.intro_active || start_barrier_) return fx;
        const bool table_anim = motion.animating ? motion.animating() : false;
        if (pause_after_motion_ && table_anim) return fx;
        if (pause_ticks_) {
            --pause_ticks_;
            return fx;
        }
        if (table_anim) return fx;
        if (draw_end_ticks_) {
            const unsigned before = 14 - draw_end_ticks_ / 2;
            --draw_end_ticks_;
            const unsigned slots = 14 - draw_end_ticks_ / 2;
            if (motion.set_draw_end_slots) motion.set_draw_end_slots(slots);
            if (slots != before) {
                const unsigned seats = DrawEndChangedSeats(OnlineTableState(Snapshot(live).game), slots - 1);
                if (seats && event_sound_allowed_)
                    QueueOrEmitSound(104, fx, ctx.window_visible, ctx.audio_healthy);
                fx.logs.push_back("ONLINE DRAW SLOT index=" + std::to_string(slots - 1) +
                                  " seats=" + std::to_string(seats) + " tick=" + std::to_string(ctx.tick_ms));
            }
            fx.dirty_all = true;
            if (draw_end_ticks_) return fx;
            fx.logs.push_back("ONLINE DRAW TABLE DONE tick=" + std::to_string(ctx.tick_ms));
            result_deadline_ = ctx.tick_ms;
            fx.result_timer_ms = 1u;
            return fx;
        }
        if (presentation_queue_.empty()) {
            queue_timer_active_ = false;
            fx.Merge(SyncSnapshot(live, live, ctx, motion));
            return fx;
        }
        auto item = std::move(presentation_queue_.front());
        presentation_queue_.pop_front();
        displayed_ = std::move(item.snapshot);
        displaying_ = true;
        event_sound_allowed_ = item.sound_allowed;
        fx.logs.push_back("ONLINE PRESENT tag=" + item.message.tag + " tick=" + std::to_string(ctx.tick_ms));
        PresentGameEvent(item.message, displayed_, live, ctx, motion, fx);
        auto updated_ctx = ctx;
        if (fx.intro.kind != OnlineIntroRequest::Kind::None) updated_ctx.intro_active = true;
        fx.Merge(SyncSnapshot(displayed_, live, updated_ctx, motion));
        return fx;
    }

    OnlineSequencerEffects OnlineTableSequencer::OpeningTick(const LobbySnapshot &live,
                                                             const OnlineSequencerContext &ctx) {
        OnlineSequencerEffects fx;
        if (!live.game.initialized) {
            opening_.Cancel();
            return fx;
        }
        if (ctx.intro_active || start_barrier_ || live.table_paused) {
            opening_.ResumeClock(ctx.anim_ms);
            return fx;
        }
        if (opening_.Advance(ctx.anim_ms)) {
            if (opening_.sound()) QueueOrEmitSound(opening_.sound(), fx, ctx.window_visible, ctx.audio_healthy);
            if (!opening_.Active()) {
                fx.logs.push_back("ONLINE OPENING DONE tick=" + std::to_string(ctx.tick_ms));
                bool flowers = false;
                for (int seat = 0; seat < 4; ++seat)
                    if (opening_flowers_[seat]) {
                        captions_.AcceptText(TextKey::caption_kita, seat);
                        flowers = true;
                    }
                opening_flowers_ = {};
                if (flowers && event_sound_allowed_)
                    QueueOrEmitSound(107, fx, ctx.window_visible, ctx.audio_healthy);
                if (!deferred_sounds_.empty()) fx.start_deferred_sound_timer = true;
                fx.Merge(UpdateActions(live, Snapshot(live).game.initialized));
                fx.arrange = true;
            }
            fx.dirty_all = true;
        }
        return fx;
    }

    OnlineSequencerEffects OnlineTableSequencer::ResultTimerExpired(const LobbySnapshot &live,
                                                                    const OnlineSequencerContext &ctx,
                                                                    const OnlineMotionCallbacks &motion) {
        OnlineSequencerEffects fx;
        if (!result_deadline_ || ctx.tick_ms < result_deadline_) return fx;
        fx.Merge(CancelResultTimer());
        const auto &s = Snapshot(live);
        if (!s.game.settlement.receipts.empty() && ctx.window_visible &&
            (s.phase == LobbyPhase::Game || s.phase == LobbyPhase::MatchComplete)) {
            show_results_ = true;
            fx.logs.push_back("ONLINE RESULT SHOW tick=" + std::to_string(ctx.tick_ms));
            fx.Merge(SyncSnapshot(s, live, ctx, motion));
        }
        return fx;
    }

    OnlineSequencerEffects OnlineTableSequencer::SyncSnapshot(const LobbySnapshot &incoming,
                                                              const LobbySnapshot &live,
                                                              const OnlineSequencerContext &ctx,
                                                              const OnlineMotionCallbacks &motion) {
        OnlineSequencerEffects fx;
        fx.notified_snapshot = incoming;
        if (incoming.actions.revision != action_revision_) hover_point_ = {-1, -1};
        const bool paused = live.table_paused;
        if (paused && !paused_at_) {
            paused_at_ = ctx.tick_ms;
            opening_.ResumeClock(ctx.anim_ms);
            fx.Merge(CancelHandDrag());
            fx.Merge(CancelSelection());
        } else if (!paused && paused_at_) {
            const auto delay = ctx.tick_ms - paused_at_;
            const auto remaining = [&](std::uint64_t deadline) {
                return static_cast<unsigned>(deadline > ctx.tick_ms ? deadline - ctx.tick_ms : 1);
            };
            if (result_deadline_) result_deadline_ += delay;
            if (automatic_discard_at_) automatic_discard_at_ += delay;
            if (result_deadline_) fx.result_timer_ms = remaining(result_deadline_);
            if (automatic_discard_at_) fx.auto_discard_timer_ms = remaining(automatic_discard_at_);
            paused_at_ = 0;
            opening_.ResumeClock(ctx.anim_ms);
        }
        if (motion.set_paused) motion.set_paused(paused);

        if (!incoming.game.initialized && (displaying_ || show_results_)) {
            fx.Merge(ClearPresentationQueue(motion));
            fx.Merge(CancelSelection());
            fx.Merge(CancelResultTimer());
            show_results_ = false;
        }
        const bool table_anim = motion.animating ? motion.animating() : false;
        if (!incoming.recovering && !incoming.game.started && !incoming.game.initialized &&
            (opening_.Active() || table_anim || ctx.intro_active || !deferred_sounds_.empty() ||
             ctx.audio_active || start_barrier_ || start_sound_pending_)) {
            fx.Merge(ClearPresentationQueue(motion));
            fx.Merge(CancelSelection());
            fx.Merge(CancelResultTimer());
            show_results_ = false;
            suppress_self_reach_ = false;
            start_barrier_ = false;
            start_sound_pending_ = false;
            match_intro_pending_ = true;
            fx.stop_all_sound = true;
            fx.preserve_intro_resume = false;
            ResetForStopSound();
            if (motion.cancel_animation) motion.cancel_animation();
        }
        if (incoming.variant_active && (incoming.table_paused || incoming.recovering)) {
            hover_point_ = {-1, -1};
            fx.Merge(CancelHandDrag());
            fx.Merge(CancelSelection());
        }
        if (incoming.phase == LobbyPhase::Disconnected || incoming.phase == LobbyPhase::Error || incoming.recovering)
            fx.Merge(ClearPresentationQueue(motion));
        if (displaying_) {
            if (presentation_queue_.empty() && incoming.game.events == displayed_.game.events)
                displayed_ = incoming;
            displayed_.table_paused = live.table_paused;
            for (int seat = 0; seat < 4; ++seat)
                displayed_.game.players[seat].connected = live.game.players[seat].connected;
        }
        const auto &snapshot = displaying_ ? displayed_ : incoming;
        if ((pressed_auto_ >= 0 ? !AutomaticInput(live) : !LocalTableInput(live, ctx.intro_active)) ||
            (pressed_tile_ >= 0 &&
             std::find(snapshot.game.players[0].known_hand.begin(),
                       snapshot.game.players[0].known_hand.end(),
                       pressed_tile_) == snapshot.game.players[0].known_hand.end())) {
            fx.Merge(CancelHandDrag());
        }
        if (snapshot.actions.revision != riichi_sound_revision_) riichi_sound_revision_ = 0;
        if (snapshot.phase != LobbyPhase::Game || snapshot.recovering || snapshot.game.ended || show_results_)
            fx.Merge(CancelSelection());
        if (clock_active_ && (live.phase != LobbyPhase::Game || live.recovering || live.table_paused ||
                              live.actions.revision != clock_revision_ || !live.actions.pending.empty() ||
                              live.actions.choices.empty())) {
            ResetActionClock();
        }
        if (snapshot.phase == LobbyPhase::Disconnected || snapshot.phase == LobbyPhase::Error || snapshot.recovering) {
            fx.Merge(CancelResultTimer());
            suppress_self_reach_ = false;
            fx.stop_all_sound = true;
            fx.preserve_intro_resume = snapshot.recovering && ctx.intro_resuming;
            ResetForStopSound();
            if (motion.cancel_animation) motion.cancel_animation();
        }
        if (snapshot.phase == LobbyPhase::Disconnected) {
            match_intro_pending_ = true;
            resume_preparation_pending_ = false;
        }
        if (snapshot.actions.revision != sound_action_revision_) {
            sound_action_revision_ = snapshot.actions.revision;
            if (snapshot.phase == LobbyPhase::Game && snapshot.actions.enabled && !SuppressCallPrompt(live) &&
                std::any_of(snapshot.actions.choices.begin(), snapshot.actions.choices.end(),
                            [](const OnlineAction &c) {
                                return c.kind == "chi" || c.kind == "pon" || c.kind.find("kan") != std::string::npos ||
                                       c.kind == "riichi" || c.kind == "tsumo" || c.kind == "ron" || c.kind == "nuki";
                            }) && event_sound_allowed_) {
                QueueOrEmitSound(106, fx, ctx.window_visible, ctx.audio_healthy);
            }
        }
        const auto &settlement = snapshot.game.settlement.receipts.empty()
                                     ? snapshot.last_settlement
                                     : snapshot.game.settlement;
        const bool result_available = !settlement.receipts.empty();
        if (!result_available) {
            fx.Merge(CancelResultTimer());
            show_results_ = false;
        }
        OnlineSettlementSync sync;
        sync.valid = true;
        sync.settlement = settlement;
        for (int i = 0; i < 4; ++i)
            sync.names[i] = snapshot.game.settlement.receipts.empty()
                                ? snapshot.last_result_names[i]
                                : snapshot.game.players[i].name;
        const bool history = snapshot.game.settlement.receipts.empty();
        sync.continuation = !history && snapshot.actions.enabled && snapshot.game.ended && !snapshot.game.complete;
        sync.dealer = history ? snapshot.last_result_dealer : snapshot.game.dealer;
        sync.honba = history ? snapshot.last_result_honba : snapshot.game.honba;
        sync.sticks = history ? snapshot.last_result_sticks : snapshot.game.sticks;
        sync.red = snapshot.game.settlement.receipts.empty() ? snapshot.last_result_red : (snapshot.game.type & 2) == 0;
        sync.confirm_enabled = !paused && !snapshot.recovering && !live.exit_pending &&
                               snapshot.actions.pending.empty() &&
                               (!snapshot.game.complete || snapshot.phase == LobbyPhase::MatchComplete);
        sync.round_finished = !history && snapshot.game.variant && snapshot.game.ended;
        sync.show = show_results_ && result_available;
        if (sync.show && !history && snapshot.game.ended && motion.present_settlement)
            motion.present_settlement();
        sync.present = result_pending_ && sync.show;
        result_pending_ = false;
        fx.settlement_sync = std::move(sync);

        fx.Merge(UpdateActions(live, Snapshot(live).game.initialized));
        fx.arrange = true;
        if (incoming.recovering && !incoming.silent_recovery &&
            (!incoming.native_service || resume_preparation_pending_) &&
            incoming.game.started && original_display_ && ctx.window_visible) {
            const bool beginning = !ctx.intro_resuming;
            auto state = OnlineTableState(incoming.game);
            if (incoming.official && incoming.game.has_dealer) state.initialized = true;
            fx.intro = {OnlineIntroRequest::Kind::BeginResume, std::move(state), incoming.game.type};
            if (beginning) fx.logs.emplace_back("ONLINE RESUME PREPARATION BEGIN");
        }
        fx.dirty_all = true;
        fx.update_lobby_music = true;
        return fx;
    }
} // namespace tenhou
