#pragma once
#include "replay.hpp"
#include "table_canvas_renderer.hpp"
#include "table_sequencer.hpp"
#include "replay_audio.hpp"
#include "settlement_view.hpp"
#include "original_display.hpp"
#include "paint_buffer.hpp"
#include "motion_timer.hpp"
#include "match_intro_view.hpp"
#include "action_captions.hpp"
#include "client_preferences.hpp"
#include <windows.h>
#include <functional>
#include <memory>

namespace tenhou {
    enum class PreferenceChange;

    // Offline replay UI shell delegating timeline & choreography to ReplayTableSequencer.
    class TableView {
    public:
        TableView(HWND parent, std::function<void(const std::string &)> log, const std::filesystem::path &config);

        ~TableView();

        void Show(bool visible);

        void Resize(int width, int height);

        void Load(const std::filesystem::path &path);

        void Reload();

        void ConfigureArtwork(const std::filesystem::path &directory, int width);

        void Capture(HDC dc);

        bool Ready() const { return error_.empty() && artwork_error_.empty() && audio_error_.empty() && painted_; }

        void SaveSettings();

        void SetOriginalDisplay(bool enabled);

        void SetEffectsOnly(bool enabled);
        void SetEffectsMuted(bool muted);
        ULONGLONG LastEffectAt() const { return audio_.LastPlayedAt(); }

        void ApplyPreferences(const ClientPreferences &preferences, PreferenceChange change);

        bool HandleKey(UINT key) { return settlement_view_ && settlement_view_->HandleKey(key); }

    private:
        friend struct TextTableCheck;
        static LRESULT CALLBACK ViewProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

        void ApplyEffects(const ReplaySequencerEffects &effects);
        void SyncTimers();
        TileGeometryCatalog *EnsureGeometry();
        unsigned CurrentSpeedInterval() const;

        void Command(int id);
        void Update();
        void Paint(HDC dc, bool table_only = false, HWND surface_parent = nullptr);
        void PaintTable(HDC dc, RECT bounds, HWND surface_parent = nullptr);

        void Stop();
        void Start();
        void Tick();
        void Advance();
        void EffectsTick();
        void OpeningTick();
        void CancelEffects();

        void SoundError(const std::exception &error);
        void PlaySoundId(unsigned id, unsigned delay = 0);
        bool EffectsEnabled() const { return sequencer_.EffectsEnabled(); }

        bool Compact() const;
        RECT BoardBounds() const;

        HWND parent_ = nullptr, viewport_ = nullptr, path_ = nullptr, status_ = nullptr;
        HWND load_ = nullptr, open_ = nullptr, previous_ = nullptr, next_ = nullptr, reset_ = nullptr, end_ = nullptr;
        HWND playback_ = nullptr, speed_ = nullptr, rounds_ = nullptr;
        HWND animate_control_ = nullptr, sound_control_ = nullptr;
        HWND results_control_ = nullptr, back_ = nullptr;
        std::unique_ptr<SettlementView> settlement_view_;
        std::unique_ptr<MatchIntroView> match_intro_;
        HudInteraction hud_interaction_;
        HFONT font_ = nullptr;
        PaintBuffer paint_buffer_;
        std::unique_ptr<ITableCanvasRenderer> renderer_;
        MotionTimer motion_timer_, opening_timer_;
        std::function<void(const std::string &)> log_;
        ReplayAudio audio_;
        ReplaySoundBank sounds_;
        std::filesystem::path config_path_;
        std::string audio_error_;
        bool sound_loaded_ = false;
        ClientPreferences preferences_;
        ReplayTableSequencer sequencer_;
        std::string error_;
        std::string artwork_error_;
        std::filesystem::path resource_directory_;
        int canvas_width_ = 1440;
        bool artwork_loaded_logged_ = false;
        bool painted_ = false;
        int width_ = 1024, height_ = 768;
    };
} // namespace tenhou
