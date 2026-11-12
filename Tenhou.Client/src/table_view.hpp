#pragma once
#include "replay.hpp"
#include "table_scene.hpp"
#include "replay_effects.hpp"
#include "replay_audio.hpp"
#include "settlement_view.hpp"
#include "original_display.hpp"
#include "retained_scene.hpp"
#include "cached_hud.hpp"
#include "motion_timer.hpp"
#include "match_intro_view.hpp"
#include "action_captions.hpp"
#include "client_preferences.hpp"
#include "tile_shadow.hpp"
#include <windows.h>
#include <functional>
#include <memory>

namespace tenhou {
enum class PreferenceChange;
// Offline inspection UI with native resource artwork and optional text details.
class TableView {
public:
    TableView(HWND parent, std::function<void(const std::string&)> log, const std::filesystem::path& config);
    ~TableView();
    void Show(bool visible);
    void Resize(int width, int height);
    void Load(const std::filesystem::path& path);
    void Reload();
    void ConfigureArtwork(const std::filesystem::path& directory, int width);
    void Capture(HDC dc);
    bool Ready() const { return error_.empty() && artwork_error_.empty() && audio_error_.empty() && painted_; }
    void SaveSettings();
    void SetOriginalDisplay(bool enabled);
    void SetEffectsOnly(bool enabled);
    void ApplyPreferences(const ClientPreferences& preferences, PreferenceChange change);
    bool HandleKey(UINT key) { return settlement_view_ && settlement_view_->HandleKey(key); }
private:
    void PresentMatchStart();
    static LRESULT CALLBACK ViewProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void Command(int id);
    void Update();
    void Paint(HDC dc, bool table_only = false);
    void PaintPlayer(HDC dc, int seat, RECT bounds);
    void PaintTable(HDC dc, RECT bounds);
    void Stop();
    void Start();
    void Tick();
    void Advance();
    void EffectsTick();
    void CancelEffects();
    void SoundError(const std::exception& error);
    void PlaySoundId(unsigned id, unsigned delay = 0);
    void UpdateScroll();
    HWND parent_ = nullptr, viewport_ = nullptr, path_ = nullptr, status_ = nullptr;
    HWND load_ = nullptr, open_ = nullptr, previous_ = nullptr, next_ = nullptr, reset_ = nullptr, end_ = nullptr;
    HWND playback_ = nullptr, speed_ = nullptr, rounds_ = nullptr;
    HWND animate_control_ = nullptr, sound_control_ = nullptr;
    HWND results_control_ = nullptr;
    std::unique_ptr<SettlementView> settlement_view_;
    std::unique_ptr<MatchIntroView> match_intro_;
    bool show_results_ = false;
    HudInteraction hud_interaction_;
    HFONT font_ = nullptr;
    PaintBuffer paint_buffer_;
    RetainedScene native_buffer_;
    CachedHud hud_cache_;
    TileShadowLayer shadow_layer_;
    MotionTimer motion_timer_;
    std::function<void(const std::string&)> log_;
    ReplayAudio audio_;
    ReplaySoundBank sounds_;
    std::filesystem::path config_path_;
    std::string audio_error_;
    bool sound_loaded_ = false, sound_enabled_ = false, animation_enabled_ = true;
    bool effects_only_ = false;
    bool individual_voices_ = false;
    ClientPreferences preferences_;
    OpeningPresentation opening_;
    ULONGLONG opening_start_ = 0;
    bool start_sound_pending_ = false;
    bool result_pending_ = false;
    unsigned draw_end_ticks_ = 0;
    TablePresentation presentation_;
    ActionCaptions captions_;
    ULONGLONG next_event_at_ = 0;
    std::unique_ptr<ReplaySession> session_;
    std::string error_;
    std::string artwork_error_;
    std::filesystem::path resource_directory_;
    int canvas_width_ = 1440;
    TableArtwork artwork_;
    Image table_background_;
    DisplayArtwork display_artwork_;
    bool original_display_ = true;
    bool Compact() const;
    RECT BoardBounds() const;
    std::vector<std::size_t> round_positions_;
    bool playing_ = false, text_details_ = false, red_tiles_ = true;
    bool painted_ = false;
    int width_ = 1024, height_ = 768;
    int scroll_ = 0, board_height_ = 610;
};
} // namespace tenhou
