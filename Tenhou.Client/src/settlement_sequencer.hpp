#pragma once
#include "settlement.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tenhou {
    struct SettlementSceneParameters {
        std::size_t selected = 0;
        std::size_t pattern_page = 0;
        int dealer = 0, honba = -1, sticks = -1;
        double yaku_entrance = 1.0;
        std::size_t shown_yaku = static_cast<std::size_t>(-1);
        std::size_t shown_tiles = static_cast<std::size_t>(-1);
        bool show_ura = true;
        bool show_totals = true;
    };

    struct SettlementEffects {
        std::vector<unsigned> sounds;
        bool stop_sound = false;
        // nullopt = leave unchanged; 0 = stop timer; >0 = arm timer to N ms.
        std::optional<unsigned> presentation_timer_ms;
        std::optional<unsigned> confirm_timer_ms;
        bool notify_bgm_changed = false;
        bool notify_presentation_done = false;
        bool confirm_finished = false;
        bool dirty = false;

        void Merge(const SettlementEffects &other);
    };

    // Pure C++ state machine for settlement reveal choreography, paging, and 5s OK countdown.
    // Zero HWND, HDC, Win32 timer, or GDI+ dependencies.
    class SettlementSequencer {
    public:
        static constexpr std::size_t kAllShown = static_cast<std::size_t>(-1);

        bool SetState(const SettlementState &state, const std::array<std::string, 4> &names,
                      bool red, std::uint64_t now_ms, SettlementEffects &out);

        void SetContext(int dealer, int honba, int sticks, bool &changed);

        SettlementEffects SetVisible(bool visible, std::uint64_t now_ms, bool window_visible = true);
        SettlementEffects SetSoundEnabled(bool enabled, std::uint64_t now_ms);
        SettlementEffects SetAnimationEnabled(bool enabled, std::uint64_t now_ms);
        SettlementEffects SetConfirmEnabled(bool enabled, std::uint64_t now_ms);
        SettlementEffects SetContinuation(bool available, std::uint64_t now_ms);
        SettlementEffects SetRoundFinished(bool finished, std::uint64_t now_ms);

        void SetPaused(bool paused, std::uint64_t now_ms);
        bool paused() const { return paused_at_ != 0; }

        SettlementEffects SelectReceipt(std::size_t index, std::uint64_t now_ms);
        SettlementEffects ChangePatternPage(int step, std::uint64_t now_ms);

        SettlementEffects Present(std::uint64_t now_ms, bool window_visible = true);
        SettlementEffects CancelPresentation(std::uint64_t now_ms, bool keep_countdown = false);

        SettlementEffects PresentationTick(std::uint64_t now_ms, bool window_visible = true);
        SettlementEffects ConfirmTick(std::uint64_t now_ms);
        SettlementEffects Confirm(std::uint64_t now_ms, bool window_visible = true);

        // Re-evaluate whether the 5s auto-OK countdown should start or stop.
        SettlementEffects SyncCountdown(std::uint64_t now_ms);

        bool Animating() const { return shown_yaku_ != kAllShown && !Complete(); }
        bool Complete() const;
        bool UraPending() const;
        bool FinalPage() const { return visible_ && state_.has_final && selected_ == state_.receipts.size(); }
        bool EnterFinalLabel() const { return state_.has_final && selected_ + 1 == state_.receipts.size(); }

        std::size_t ChoiceCount() const { return state_.receipts.size() + (state_.has_final ? 1 : 0); }
        std::size_t CurrentPatternPages() const;
        std::size_t YakuCount() const;
        std::size_t PreviewTiles() const { return 0; }

        const SettlementState &state() const { return state_; }
        const std::array<std::string, 4> &names() const { return names_; }
        bool red() const { return red_; }
        int dealer() const { return dealer_; }
        int honba() const { return honba_; }
        int sticks() const { return sticks_; }
        bool visible() const { return visible_; }
        bool confirm_enabled() const { return confirm_enabled_; }
        bool animation_enabled() const { return animation_enabled_; }
        bool sound_enabled() const { return sound_enabled_; }

        std::size_t selected() const { return selected_; }
        std::size_t pattern_page() const { return pattern_page_; }
        std::uint64_t confirm_deadline() const { return confirm_deadline_; }
        int confirm_seconds() const { return confirm_seconds_; }
        std::size_t shown_yaku() const { return shown_yaku_; }
        std::size_t shown_tiles() const { return shown_tiles_; }
        int yaku_entrance_frame() const { return yaku_entrance_frame_; }

        void SetConfirmDeadlineForTest(std::uint64_t deadline) { confirm_deadline_ = deadline; }

        std::string FormatDetailsText(bool resource_available) const;
        SettlementSceneParameters SceneParameters() const;

    private:
        void ArmPresentationTimer(SettlementEffects &effects);

        SettlementState state_;
        std::array<std::string, 4> names_{};
        bool red_ = true;
        int dealer_ = 0, honba_ = -1, sticks_ = -1;
        std::size_t selected_ = 0;
        std::size_t pattern_page_ = 0;
        bool visible_ = false;
        bool sound_enabled_ = false;
        bool animation_enabled_ = true;
        bool confirm_enabled_ = true;
        bool continuation_ = false;
        bool round_finished_ = false;
        bool confirm_ready_ = false;
        std::uint64_t confirm_deadline_ = 0;
        std::uint64_t paused_at_ = 0;
        int confirm_seconds_ = -1;
        bool ura_revealed_ = true;
        bool totals_revealed_ = true;
        bool ura_pause_done_ = false;
        unsigned presentation_frames_ = 0;
        std::size_t shown_yaku_ = kAllShown;
        std::size_t shown_tiles_ = kAllShown;
        int yaku_entrance_frame_ = 10;
    };
} // namespace tenhou
