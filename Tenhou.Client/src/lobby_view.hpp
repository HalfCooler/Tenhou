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
#include "table_sequencer.hpp"
#include <windows.h>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>

namespace tenhou {
    class MatchmakingView;
    class OfflineLobbyView;
    enum class PreferenceChange;

    class LobbyView {
    public:
        LobbyView(HWND parent, const std::filesystem::path &config, std::function<void(const std::string &)> log);

        ~LobbyView();

        void Login(const ConnectionOptions &options);
        void EnableServiceLobby(std::function<void()> settings, std::function<void()> replay);

        bool LoggedIn() const;

        void SetConnectionObserver(std::function<void(const LobbySnapshot &)> observer) {
            connection_changed_ = std::move(observer);
        }

        void Show(bool visible);

        void SetWelcomeMusic(bool visible, bool official);

        void SetOfflineLobbyMusic(std::optional<bool> contest);

        void Resize(int width, int height);

        void Capture(HDC dc);

        void Save();

        void Disconnect();

        void ConfigureArtwork(const std::filesystem::path &directory, int width);

        void SetOriginalDisplay(bool enabled);

        bool HandleKey(UINT key);

        bool HandleExitVoteMessage(MSG &message) {
            return exit_vote_ && IsWindowVisible(exit_vote_) &&
                   (message.hwnd == exit_vote_ || IsChild(exit_vote_, message.hwnd)) &&
                   IsDialogMessageW(exit_vote_, &message);
        }

        void SetEffectsOnly(bool enabled) {
            StopAudio();
            sequencer_.set_effects_only(enabled);
        }

        void ApplyPreferences(const ClientPreferences &preferences, PreferenceChange change);

        void SetQuickAudio(bool mute_effects, bool mute_music);
        ULONGLONG LastEffectAt() const { return audio_.LastPlayedAt(); }
        StereoLevels MusicLevels() const { return lobby_music_.Levels(); }

    private:
        friend struct LobbyPresentationCheck;
        friend struct TextTableCheck;
        static LRESULT CALLBACK Procedure(HWND, UINT, WPARAM, LPARAM);

        static LRESULT CALLBACK ExitVoteProcedure(HWND, UINT, WPARAM, LPARAM);

        void UpdateExitVote();

        OnlineSequencerContext MakeContext() const;
        OnlineMotionCallbacks MakeMotion();
        void ApplyEffects(const OnlineSequencerEffects &fx);
        void SyncTimers();
        void ScheduleSound(unsigned id, unsigned delay = 0);

        void Command(int id);
        void UpdateActions();
        void Submit(std::size_t index);
        void CancelSelection();
        bool LocalTableInput() const;
        bool AutomaticInput() const;
        void CancelHandDrag();
        void PreferenceInput(bool double_click);

        bool Compact() const;
        RECT BoardBounds() const;
        void Update(const LobbySnapshot &snapshot);
        void Arrange();
        void Paint(HDC dc, bool table_only = false, HWND surface_parent = nullptr);
        std::vector<TableActionButton> Buttons();
        static int HitButton(const std::vector<TableActionButton> &buttons, int x, int y);
        void CancelButton();
        void CheckButtonCapture();

        void OnGameEvent(const NetworkMessage &message, const LobbySnapshot &snapshot);
        const LobbySnapshot &Snapshot() const;
        void PresentationTick();
        void ClearPresentationQueue();
        void OpeningTick(double now);
        void ActionClockTick();
        void AutomaticAction();
        void ApplyAutomaticSort();
        bool SuppressCallPrompt() const;

        void PlaySoundId(unsigned id, unsigned delay = 0);
        void StopSound(bool preserve_resume = false);
        void StopAudio();
        bool Moving() const { return sequencer_.Moving(table_.Animating()); }

        TableInteraction Interaction(const LobbySnapshot &snapshot);
        int HitAutomaticButton(int x, int y) const;
        void UpdateLobbyMusic(bool refresh = false);

        HWND parent_ = nullptr, window_ = nullptr;
        HFONT font_ = nullptr;
        HFONT exit_font_ = nullptr;
        HWND exit_vote_ = nullptr, exit_vote_text_ = nullptr, exit_yes_ = nullptr, exit_no_ = nullptr;
        ULONGLONG exit_vote_deadline_ = 0;
        PaintBuffer paint_buffer_;
        MotionTimer motion_timer_, opening_timer_;
        bool queue_timer_ = false, clock_timer_ = false, automatic_timer_ = false, caption_timer_ = false;
        std::wstring painted_countdown_;
        int painted_hover_ = -1, painted_auto_hover_ = -1;
        SIZE exit_size_{};
        int focused_button_ = -1, keyboard_button_ = -1;
        UINT keyboard_key_ = 0;
        std::filesystem::path config_;
        std::function<void(const std::string &)> log_;
        ReplayAudio audio_;
        BackgroundMusic lobby_music_;
        std::wstring lobby_music_key_;
        bool music_preview_ = false, welcome_music_ = false, official_welcome_ = false, third_party_music_ = false;
        std::optional<bool> offline_lobby_music_;
        ReplaySoundBank sounds_;
        bool sound_loaded_ = false;
        bool music_muted_ = false;
        std::string audio_error_;
        ClientPreferences preferences_;
        OnlineTableSequencer sequencer_;
        std::unique_ptr<NetworkClient> client_;
        ConnectionOptions login_options_;
        std::function<void(const LobbySnapshot &)> connection_changed_;
        OnlineTable table_;
        std::unique_ptr<MatchmakingView> matching_;
        std::unique_ptr<OfflineLobbyView> service_lobby_;
        std::unique_ptr<SettlementView> settlement_view_;
        std::unique_ptr<MatchIntroView> match_intro_;
        std::unique_ptr<ActionSelectionView> action_view_;
        HudInteraction hud_interaction_;
        bool artwork_painted_ = false;
        std::string artwork_error_;
        std::filesystem::path resource_directory_;
        int canvas_width_ = 1440;
        Image lobby_background_;
    };
} // namespace tenhou
