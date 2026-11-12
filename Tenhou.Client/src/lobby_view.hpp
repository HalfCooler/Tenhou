#pragma once
#include "network_client.hpp"
#include "online_table.hpp"
#include "settlement_view.hpp"
#include "replay_audio.hpp"
#include "paint_buffer.hpp"
#include "motion_timer.hpp"
#include "match_intro_view.hpp"
#include "action_selection_view.hpp"
#include "action_captions.hpp"
#include "client_preferences.hpp"
#include <windows.h>
#include <filesystem>
#include <functional>
#include <memory>
#include <deque>

namespace tenhou {
    class MatchmakingView;
    enum class PreferenceChange;

    class LobbyView {
    public:
        LobbyView(HWND parent, const std::filesystem::path &config, std::function<void(const std::string &)> log);

        ~LobbyView();

        void Login(const ConnectionOptions &options);

        bool LoggedIn() const;

        void SetConnectionObserver(std::function<void(const LobbySnapshot &)> observer) {
            connection_changed_ = std::move(observer);
        }

        void Show(bool visible);

        void Resize(int width, int height);

        void Capture(HDC dc);

        void Save();

        void Disconnect();

        void ConfigureArtwork(const std::filesystem::path &directory, int width);

        void SetOriginalDisplay(bool enabled);

        bool HandleKey(UINT key);

        bool HandleExitVoteMessage(MSG &message) {
            return exit_vote_ && IsWindowVisible(exit_vote_) &&
                   (message.hwnd == exit_vote_ || IsChild(exit_vote_, message.hwnd)) && IsDialogMessageW(
                       exit_vote_, &message);
        }

        void SetEffectsOnly(bool enabled) {
            StopAudio();
            effects_only_ = enabled;
            individual_voices_ = false;
        }

        void ApplyPreferences(const ClientPreferences &preferences, PreferenceChange change);

    private:
        static LRESULT CALLBACK Procedure(HWND, UINT, WPARAM, LPARAM);

        static LRESULT CALLBACK ExitVoteProcedure(HWND, UINT, WPARAM, LPARAM);

        void UpdateExitVote();

        void PaintExit(const DRAWITEMSTRUCT &item);

        void Command(int id);

        void UpdateActions();

        void UpdateActionAvailability();

        void Submit(std::size_t index);

        void CancelSelection();

        bool LocalTableInput() const;

        bool AutomaticInput() const;

        void CancelHandDrag();

        void PreferenceInput(bool double_click);

        void AnnounceRiichi();

        void CancelResultTimer();

        bool Compact() const;

        RECT BoardBounds() const;

        void Update(const LobbySnapshot &snapshot);

        void Arrange();

        void Paint(HDC dc, bool table_only = false);

        void PaintAction(const DRAWITEMSTRUCT &item);

        void OnGameEvent(const NetworkMessage &message, const LobbySnapshot &snapshot);

        void PresentGameEvent(const NetworkMessage &message, const LobbySnapshot &snapshot);

        const LobbySnapshot &Snapshot() const;

        void PresentationTick();

        void ClearPresentationQueue();

        void PlayEventSound(const ReplayEvent &event, const LobbySnapshot &snapshot);

        void PlaySoundId(unsigned id, unsigned delay = 0);

        void StopSound(bool preserve_resume = false);

        void StopAudio();

        bool Moving() const {
            return sort_refresh_pending_ || (animation_enabled_ && table_.Animating()) || !presentation_queue_.empty()
                   || pause_ticks_ || draw_end_ticks_;
        }

        struct QueuedEvent {
            NetworkMessage message;
            LobbySnapshot snapshot;
            bool sound_allowed = false;
        };

        std::deque<QueuedEvent> presentation_queue_;
        LobbySnapshot displayed_;
        bool displaying_ = false;
        unsigned pause_ticks_ = 0, draw_end_ticks_ = 0;
        bool pause_after_motion_ = false;
        bool event_sound_allowed_ = true;

        void ResetActionClock();

        void ReceiveActionClock(const NetworkMessage &message, const LobbySnapshot &snapshot);

        void ActionClockTick();

        TableInteraction Interaction(const LobbySnapshot &snapshot);

        int HitAutomaticButton(int x, int y) const;

        void AutomaticAction();

        bool SuppressCallPrompt() const;

        void UpdateLobbyMusic();

        void StopLobbyMusic();

        void UpdateAutomaticTimer();

        void ApplyAutomaticSort();

        HWND parent_ = nullptr, window_ = nullptr;
        HFONT font_ = nullptr;
        HFONT exit_font_ = nullptr;
        HWND exit_vote_ = nullptr, exit_vote_text_ = nullptr, exit_yes_ = nullptr, exit_no_ = nullptr;
        ULONGLONG exit_vote_deadline_ = 0, paused_at_ = 0;
        PaintBuffer paint_buffer_;
        MotionTimer motion_timer_;
        std::wstring painted_countdown_;
        int painted_hover_ = -1, painted_auto_hover_ = -1;
        std::map<int, HWND> controls_;
        std::filesystem::path config_;
        std::function<void(const std::string &)> log_;
        ReplayAudio audio_;
        ReplayAudio lobby_music_;
        std::string lobby_music_key_;
        bool music_preview_ = false;
        ReplaySoundBank sounds_;
        bool sound_enabled_ = false, sound_loaded_ = false;
        bool animation_enabled_ = true;
        bool effects_only_ = false;
        bool individual_voices_ = false;
        ClientPreferences preferences_;
        OpeningPresentation opening_;
        ULONGLONG opening_start_ = 0;
        std::vector<unsigned> deferred_sounds_;
        ULONGLONG result_deadline_ = 0;
        bool result_pending_ = false;
        std::uint64_t riichi_sound_revision_ = 0;
        bool suppress_self_reach_ = false;
        std::string audio_error_;
        std::uint64_t sound_action_revision_ = 0;
        std::unique_ptr<NetworkClient> client_;
        ConnectionOptions login_options_;
        std::function<void(const LobbySnapshot &)> connection_changed_;
        OnlineTable table_;
        std::unique_ptr<MatchmakingView> matching_;
        std::unique_ptr<SettlementView> settlement_view_;
        std::unique_ptr<MatchIntroView> match_intro_;
        std::unique_ptr<ActionSelectionView> action_view_;
        ActionSelection selection_;
        std::string match_dan_;
        bool match_intro_pending_ = true;
        bool resume_preparation_pending_ = false;
        bool start_sound_pending_ = false, start_barrier_ = false;
        POINT hover_point_{-1, -1};
        int pressed_tile_ = -1, pressed_auto_ = -1;
        bool hand_dragged_ = false;
        HudInteraction hud_interaction_;
        std::uint64_t pressed_revision_ = 0;
        ActionCaptions captions_;
        bool clock_active_ = false;
        ULONGLONG clock_deadline_ = 0;
        int clock_base_ms_ = -1;
        ULONGLONG clock_base_deadline_ = 0;
        int clock_warning_second_ = -1;
        std::uint64_t clock_revision_ = 0;
        std::array<bool, 4> automatic_{true, false, false, false};
        bool automatic_timer_ = false;
        ULONGLONG automatic_discard_at_ = 0;
        bool sort_refresh_pending_ = false;
        bool show_results_ = false;
        std::string action_filter_ = "discard";
        std::uint64_t action_revision_ = 0;
        bool text_details_ = false;
        bool artwork_painted_ = false;
        std::string artwork_error_;
        bool original_display_ = true;
        std::filesystem::path resource_directory_;
        int canvas_width_ = 1440;
        Image lobby_background_;
        DisplayArtwork action_artwork_;
        std::string action_artwork_error_;
    };
} // namespace tenhou
