#include "settlement_sequencer.hpp"
#include <algorithm>

namespace tenhou {
    void SettlementEffects::Merge(const SettlementEffects &other) {
        sounds.insert(sounds.end(), other.sounds.begin(), other.sounds.end());
        stop_sound = stop_sound || other.stop_sound;
        if (other.presentation_timer_ms.has_value()) presentation_timer_ms = other.presentation_timer_ms;
        if (other.confirm_timer_ms.has_value()) confirm_timer_ms = other.confirm_timer_ms;
        notify_bgm_changed = notify_bgm_changed || other.notify_bgm_changed;
        notify_presentation_done = notify_presentation_done || other.notify_presentation_done;
        confirm_finished = confirm_finished || other.confirm_finished;
        dirty = dirty || other.dirty;
    }

    bool SettlementSequencer::SetState(const SettlementState &s, const std::array<std::string, 4> &names,
                                       bool red, std::uint64_t now_ms, SettlementEffects &out) {
        if (SettlementJson(state_) == SettlementJson(s) && state_.red_tiles == s.red_tiles &&
            names_ == names && red_ == red)
            return false;
        const bool was_final = state_.has_final && selected_ == state_.receipts.size();
        const bool new_round = s.receipts.size() < state_.receipts.size() ||
                               (!s.receipts.empty() && !state_.receipts.empty() &&
                                (s.receipts[0].tag != state_.receipts[0].tag ||
                                 s.receipts[0].base != state_.receipts[0].base ||
                                 s.receipts[0].hand != state_.receipts[0].hand));
        state_ = s;
        names_ = names;
        red_ = red;
        if (new_round) selected_ = 0;
        if (was_final && s.has_final) selected_ = s.receipts.size();
        const auto count = ChoiceCount();
        selected_ = count ? std::min(selected_, count - 1) : 0;
        out.Merge(SelectReceipt(selected_, now_ms));
        return true;
    }

    void SettlementSequencer::SetContext(int dealer, int honba, int sticks, bool &changed) {
        if (dealer_ == dealer && honba_ == honba && sticks_ == sticks) {
            changed = false;
            return;
        }
        dealer_ = dealer;
        honba_ = honba;
        sticks_ = sticks;
        changed = true;
    }

    SettlementEffects SettlementSequencer::SetVisible(bool visible, std::uint64_t now_ms, bool window_visible) {
        SettlementEffects effects;
        const bool opening = visible && !visible_;
        const bool changed = visible != visible_;
        visible_ = visible;
        if (!visible) {
            effects.Merge(CancelPresentation(now_ms));
        } else if (opening) {
            effects.Merge(Present(now_ms, window_visible));
        }
        if (changed) {
            effects.Merge(SyncCountdown(now_ms));
            effects.dirty = true;
        }
        return effects;
    }

    SettlementEffects SettlementSequencer::SetSoundEnabled(bool enabled, std::uint64_t now_ms) {
        SettlementEffects effects;
        sound_enabled_ = enabled;
        if (!enabled && !animation_enabled_) effects.Merge(CancelPresentation(now_ms));
        return effects;
    }

    SettlementEffects SettlementSequencer::SetAnimationEnabled(bool enabled, std::uint64_t now_ms) {
        SettlementEffects effects;
        animation_enabled_ = enabled;
        if (!enabled && !sound_enabled_) effects.Merge(CancelPresentation(now_ms));
        effects.Merge(SyncCountdown(now_ms));
        effects.dirty = true;
        return effects;
    }

    SettlementEffects SettlementSequencer::SetConfirmEnabled(bool enabled, std::uint64_t now_ms) {
        SettlementEffects effects;
        if (confirm_enabled_ != enabled) {
            confirm_enabled_ = enabled;
            effects.Merge(SyncCountdown(now_ms));
            effects.dirty = true;
        }
        return effects;
    }

    SettlementEffects SettlementSequencer::SetContinuation(bool available, std::uint64_t now_ms) {
        SettlementEffects effects;
        if (continuation_ != available) {
            continuation_ = available;
            effects.Merge(SyncCountdown(now_ms));
            effects.dirty = true;
        }
        return effects;
    }

    SettlementEffects SettlementSequencer::SetRoundFinished(bool finished, std::uint64_t now_ms) {
        SettlementEffects effects;
        if (finished && !round_finished_ && !state_.receipts.empty() && state_.receipts.front().generic) {
            effects.Merge(SelectReceipt(0, now_ms));
        }
        round_finished_ = finished;
        effects.Merge(SyncCountdown(now_ms));
        effects.dirty = true;
        return effects;
    }

    void SettlementSequencer::SetPaused(bool paused, std::uint64_t now_ms) {
        if (paused && !paused_at_) paused_at_ = now_ms;
        else if (!paused && paused_at_) {
            if (confirm_deadline_) confirm_deadline_ += now_ms - paused_at_;
            paused_at_ = 0;
        }
    }

    SettlementEffects SettlementSequencer::SelectReceipt(std::size_t index, std::uint64_t now_ms) {
        SettlementEffects effects;
        const auto count = ChoiceCount();
        selected_ = count ? std::min(index, count - 1) : 0;
        pattern_page_ = 0;
        effects.Merge(CancelPresentation(now_ms));
        effects.Merge(SyncCountdown(now_ms));
        effects.notify_bgm_changed = true;
        effects.dirty = true;
        return effects;
    }

    SettlementEffects SettlementSequencer::ChangePatternPage(int step, std::uint64_t now_ms) {
        SettlementEffects effects;
        if (Animating() || selected_ >= state_.receipts.size()) return effects;
        const int pages = static_cast<int>(SettlementPatternPages(state_.receipts[selected_]));
        const int page = std::clamp(static_cast<int>(pattern_page_) + step, 0, pages - 1);
        if (page == static_cast<int>(pattern_page_)) return effects;
        pattern_page_ = static_cast<std::size_t>(page);
        confirm_deadline_ = 0;
        confirm_seconds_ = -1;
        effects.confirm_timer_ms = 0u;
        effects.Merge(SyncCountdown(now_ms));
        effects.dirty = true;
        return effects;
    }

    std::size_t SettlementSequencer::CurrentPatternPages() const {
        return selected_ < state_.receipts.size() ? SettlementPatternPages(state_.receipts[selected_]) : 1;
    }

    std::size_t SettlementSequencer::YakuCount() const {
        if (selected_ >= state_.receipts.size()) return 0;
        const auto &r = state_.receipts[selected_];
        if (r.generic) return r.entries.size();
        return r.yakuman.empty() ? SettlementDisplayYaku(r).size() : r.yakuman.size();
    }

    bool SettlementSequencer::UraPending() const {
        return !ura_revealed_ && selected_ < state_.receipts.size() && !state_.receipts[selected_].ura.empty();
    }

    bool SettlementSequencer::Complete() const {
        return totals_revealed_ && shown_yaku_ >= YakuCount() && shown_tiles_ >= PreviewTiles() * 2 && !UraPending();
    }

    void SettlementSequencer::ArmPresentationTimer(SettlementEffects &effects) {
        effects.presentation_timer_ms = 0u;
        if (Complete() || selected_ >= state_.receipts.size()) return;
        const auto &r = state_.receipts[selected_];
        effects.presentation_timer_ms = r.who < 0 ? 80u : 10u;
    }

    SettlementEffects SettlementSequencer::CancelPresentation(std::uint64_t now_ms, bool keep_countdown) {
        SettlementEffects effects;
        effects.presentation_timer_ms = 0u;
        if (!keep_countdown) {
            effects.confirm_timer_ms = 0u;
            confirm_deadline_ = 0;
            confirm_seconds_ = -1;
            confirm_ready_ = false;
        }
        presentation_frames_ = 0;
        yaku_entrance_frame_ = 10;
        totals_revealed_ = true;
        ura_pause_done_ = false;
        if (shown_yaku_ != kAllShown || shown_tiles_ != kAllShown)
            effects.stop_sound = true;
        shown_yaku_ = shown_tiles_ = kAllShown;
        ura_revealed_ = true;
        effects.Merge(SyncCountdown(now_ms));
        effects.dirty = true;
        return effects;
    }

    SettlementEffects SettlementSequencer::Present(std::uint64_t now_ms, bool window_visible) {
        SettlementEffects effects = CancelPresentation(now_ms);
        if (!visible_ || !window_visible) return effects;
        confirm_ready_ = true;
        if (FinalPage()) {
            // Origin tenhou.exe.c:115556-115560 plays SE_GAMEEND (0x73/115) and switches to BGM_GAME9.
            if (sound_enabled_) effects.sounds.push_back(115);
            effects.notify_bgm_changed = true;
            effects.Merge(SyncCountdown(now_ms));
            return effects;
        }
        if ((!animation_enabled_ && !sound_enabled_) || selected_ >= state_.receipts.size()) {
            effects.Merge(SyncCountdown(now_ms));
            return effects;
        }
        const auto &r = state_.receipts[selected_];
        if (r.who < 0) {
            effects.Merge(SyncCountdown(now_ms));
            return effects;
        }
        shown_yaku_ = 0;
        presentation_frames_ = 30;
        totals_revealed_ = r.who < 0;
        ura_revealed_ = r.ura.empty();
        shown_tiles_ = r.who < 0 ? 0 : kAllShown;
        if (sound_enabled_ && r.who >= 0 && r.yakuman.empty()) effects.sounds.push_back(101);
        effects.Merge(SyncCountdown(now_ms));
        effects.dirty = true;
        ArmPresentationTimer(effects);
        return effects;
    }

    SettlementEffects SettlementSequencer::PresentationTick(std::uint64_t now_ms, bool window_visible) {
        SettlementEffects effects;
        if (paused_at_) return effects;
        if (!visible_ || !window_visible || selected_ >= state_.receipts.size()) {
            return CancelPresentation(now_ms);
        }
        const auto &r = state_.receipts[selected_];
        if (yaku_entrance_frame_ < 10) {
            ++yaku_entrance_frame_;
            if (animation_enabled_) effects.dirty = true;
        }
        if (r.who >= 0 && presentation_frames_) {
            if (--presentation_frames_) return effects;
        }
        const auto yaku = SettlementDisplayYaku(r);
        const bool next_ura = r.yakuman.empty() && shown_yaku_ && shown_yaku_ < yaku.size() &&
                              yaku[shown_yaku_].id == 53;
        if (next_ura && !ura_pause_done_) {
            // Native frame 54, 004ad739: an additional 1200ms before ura frame 0.
            ura_pause_done_ = true;
            effects.presentation_timer_ms = 1200u;
            return effects;
        }
        if (shown_yaku_ < YakuCount()) {
            if (r.generic) {
                ++shown_yaku_;
                if (animation_enabled_) pattern_page_ = (shown_yaku_ - 1) / kSettlementPatternsPerPage;
                if (sound_enabled_) effects.sounds.push_back(109);
            } else {
                if (r.yakuman.empty() && yaku[shown_yaku_].id == 53) ura_revealed_ = true;
                ++shown_yaku_;
                yaku_entrance_frame_ = 0;
                if (shown_yaku_ == 1 && !r.yakuman.empty() && sound_enabled_) effects.sounds.push_back(111);
                if (sound_enabled_) effects.sounds.push_back(109);
            }
            ura_pause_done_ = false;
            if (shown_yaku_ >= YakuCount()) presentation_frames_ = 12;
            else presentation_frames_ = !r.generic && r.yakuman.empty() && yaku[shown_yaku_].id == 53 ? 54 : 55;
        } else if (UraPending()) {
            ura_revealed_ = true;
            if (sound_enabled_) effects.sounds.push_back(104);
        } else {
            totals_revealed_ = true;
        }
        if (shown_tiles_ < PreviewTiles() * 2) {
            ++shown_tiles_;
            if (sound_enabled_) effects.sounds.push_back(104);
        }
        effects.Merge(SyncCountdown(now_ms));
        effects.dirty = true;
        ArmPresentationTimer(effects);
        if (Complete()) effects.notify_presentation_done = true;
        return effects;
    }

    SettlementEffects SettlementSequencer::SyncCountdown(std::uint64_t now_ms) {
        SettlementEffects effects;
        const auto pages = CurrentPatternPages();
        const bool next = (continuation_ && !state_.has_final && pattern_page_ + 1 >= pages &&
                           selected_ + 1 == state_.receipts.size()) ||
                          (round_finished_ && selected_ < state_.receipts.size() && state_.receipts[selected_].generic);
        const bool countdown = next && visible_ && confirm_ready_ && confirm_enabled_ && !Animating();
        if (!countdown) {
            if (confirm_deadline_ || confirm_seconds_ >= 0) {
                effects.confirm_timer_ms = 0u;
                confirm_deadline_ = 0;
                confirm_seconds_ = -1;
            }
            return effects;
        }
        if (confirm_seconds_ < 0) {
            confirm_seconds_ = 5;
            confirm_deadline_ = now_ms + 5000;
            effects.confirm_timer_ms = 100u;
            if (sound_enabled_) effects.sounds.push_back(102);
            effects.dirty = true;
        }
        return effects;
    }

    SettlementEffects SettlementSequencer::ConfirmTick(std::uint64_t now_ms) {
        SettlementEffects effects;
        if (paused_at_ || !visible_ || !confirm_deadline_) return effects;
        const int seconds = now_ms >= confirm_deadline_
                                ? 0
                                : static_cast<int>((confirm_deadline_ - now_ms + 999) / 1000);
        if (!seconds) {
            effects.confirm_timer_ms = 0u;
            confirm_deadline_ = 0;
            effects.Merge(Confirm(now_ms));
            return effects;
        }
        if (seconds != confirm_seconds_) {
            confirm_seconds_ = seconds;
            effects.dirty = true;
            if (sound_enabled_) effects.sounds.push_back(102);
        }
        return effects;
    }

    SettlementEffects SettlementSequencer::Confirm(std::uint64_t now_ms, bool window_visible) {
        SettlementEffects effects;
        if (!visible_ || Animating() || !confirm_enabled_) return effects;
        if (selected_ < state_.receipts.size() &&
            pattern_page_ + 1 < SettlementPatternPages(state_.receipts[selected_])) {
            return ChangePatternPage(1, now_ms);
        }
        effects.confirm_timer_ms = 0u;
        confirm_deadline_ = 0;
        effects.dirty = true;
        const auto count = ChoiceCount();
        if (selected_ + 1 < count) {
            effects.Merge(SelectReceipt(selected_ + 1, now_ms));
            effects.Merge(Present(now_ms, window_visible));
        } else {
            effects.confirm_finished = true;
        }
        return effects;
    }

    std::string SettlementSequencer::FormatDetailsText(bool resource_available) const {
        auto displayed = state_;
        for (auto &receipt: displayed.receipts) receipt.yaku = SettlementDisplayYaku(receipt);
        const bool animate = animation_enabled_ && resource_available;
        if (animate && selected_ < displayed.receipts.size()) {
            auto &receipt = displayed.receipts[selected_];
            if (shown_yaku_ < receipt.yaku.size()) receipt.yaku.resize(shown_yaku_);
            if (receipt.generic && shown_yaku_ < receipt.entries.size()) receipt.entries.resize(shown_yaku_);
            if (shown_yaku_ < receipt.yakuman.size()) receipt.yakuman.resize(shown_yaku_);
            if (UraPending()) receipt.ura.clear();
            if (receipt.who < 0 && shown_tiles_ != kAllShown) {
                std::size_t visible = shown_tiles_ > PreviewTiles() ? shown_tiles_ - PreviewTiles() : 0;
                for (int i = 0; i < 4; ++i)
                    if (receipt.has_revealed[i]) {
                        const auto count = std::min(visible, receipt.revealed[i].size());
                        visible -= count;
                        receipt.revealed[i].resize(count);
                        if (!count) receipt.has_revealed[i] = false;
                    }
            }
        }
        auto text = SettlementText(displayed, selected_, names_, red_, !animate || Complete());
        if (animate && UraPending()) {
            const auto at = text.find("Ura indicators: not supplied");
            if (at != std::string::npos) text.replace(at, 28, "Ura indicators: awaiting reveal");
        }
        if (!resource_available) text = "Tile artwork unavailable; text remains complete.\r\n" + text;
        return text;
    }

    SettlementSceneParameters SettlementSequencer::SceneParameters() const {
        SettlementSceneParameters params;
        params.selected = selected_;
        params.pattern_page = pattern_page_;
        params.dealer = dealer_;
        params.honba = honba_;
        params.sticks = sticks_;
        params.yaku_entrance = yaku_entrance_frame_ / 10.0;
        params.shown_yaku = animation_enabled_ ? shown_yaku_ : kAllShown;
        params.shown_tiles = animation_enabled_ ? shown_tiles_ : kAllShown;
        params.show_ura = !animation_enabled_ || !UraPending();
        params.show_totals = !animation_enabled_ || Complete();
        return params;
    }
} // namespace tenhou
