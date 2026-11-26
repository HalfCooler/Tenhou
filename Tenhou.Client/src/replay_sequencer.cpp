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

} // namespace tenhou
