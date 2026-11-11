#include "table_view.hpp"
#include "tile_shadow.hpp"
#include "settings_view.hpp"
#include "wall_scene.hpp"
#include "background.hpp"
#include "window_layout.hpp"
#include <commdlg.h>
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace tenhou {
void TableView::ApplyPreferences(const ClientPreferences& preferences, PreferenceChange change) {
    preferences_=preferences;
    settlement_view_->SetPeekEnabled(preferences.ResultPeek());
    artwork_.SetColor(preferences.TileColor()); settlement_view_->SetTileColor(preferences.TileColor());
    display_artwork_.SetPanelEnabled((preferences.flags&0x2000)==0);
    if (change==PreferenceChange::Artwork) {
        auto background=BuildPreferredBackground(resource_directory_,config_path_,canvas_width_,canvas_width_*3/4,preferences);
        table_background_=std::move(background);
        native_buffer_.Invalidate(); hud_cache_.Invalidate(); InvalidateRect(viewport_,nullptr,FALSE);
    }
    if (change==PreferenceChange::Sound) {
        audio_.SetVolumePercent(preferences.se_volume);
        const auto voice_mode=GetPrivateProfileIntW(L"RESOURCE",L"SE_NOVOICE",
            GetPrivateProfileIntW(L"REBUILD",L"SoundEffectsOnly",0,config_path_.c_str()),config_path_.c_str());
        effects_only_=voice_mode==1; individual_voices_=voice_mode==2;
        sound_loaded_=false; audio_error_.clear();
    }
}
namespace {
constexpr wchar_t kClass[] = L"TenhouRebuildOfflineTable";
constexpr int kPath = 3101, kLoad = 3102, kPrevious = 3103, kNext = 3104;
constexpr int kReset = 3105, kStatus = 3106, kEnd = 3107, kOpen = 3108;
constexpr int kPlayback = 3110, kSpeed = 3111, kRounds = 3112;
constexpr int kAnimate = 3115, kSound = 3116;
constexpr int kResults = 3117;
constexpr UINT_PTR kPlaybackTimer = 4;
constexpr UINT_PTR kEffectsTimer = 5;
constexpr UINT_PTR kMotionTimer = 6;
constexpr UINT kIntervals[]{2000, 1000, 500, 250, 100};
std::wstring Wide(const std::string& value) {
    if (value.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(size, L' ');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size);
    return result;
}
void Fill(HDC dc, RECT bounds, COLORREF color) {
    const auto brush = CreateSolidBrush(color);
    FillRect(dc, &bounds, brush);
    DeleteObject(brush);
}
void Text(HDC dc, const std::string& value, RECT bounds, UINT format = DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS) {
    const auto text = Wide(value);
    DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &bounds, format);
}
std::string CompactTile(int tile, bool red = true) {
    if (tile >= 108 && tile < 136) return std::string(1, "ESWNPFC"[tile / 4 - 27]);
    if (red && tile == 16) return "0m";
    if (red && tile == 52) return "0p";
    if (red && tile == 88) return "0s";
    auto label = TileLabel(tile);
    if (!red && !label.empty() && label.back() == '*') label.pop_back();
    return label;
}
void Tile(HDC dc, int tile, RECT bounds, bool claimed = false, bool reach = false, bool drawn = false, bool red = true) {
    Fill(dc, bounds, claimed ? RGB(146, 168, 166) : drawn ? RGB(255, 230, 152) : RGB(250, 249, 238));
    const auto border = CreateSolidBrush(reach ? RGB(220, 50, 50) : RGB(90, 110, 100));
    FrameRect(dc, &bounds, border);
    DeleteObject(border);
    SetTextColor(dc, tile / 4 >= 27 ? RGB(130, 30, 30) : RGB(25, 45, 60));
    Text(dc, CompactTile(tile, red), bounds, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}
} // namespace

TableView::TableView(HWND parent, std::function<void(const std::string&)> log, const std::filesystem::path& config)
    : parent_(parent), log_(std::move(log)), audio_(log_), config_path_(config) {
    const auto instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent, GWLP_HINSTANCE));
    WNDCLASSEXW definition{sizeof(WNDCLASSEXW)};
    definition.hInstance = instance;
    definition.lpfnWndProc = ViewProc;
    definition.lpszClassName = kClass;
    definition.hCursor = LoadCursor(nullptr, IDC_ARROW);
    if (!RegisterClassExW(&definition) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        throw std::runtime_error("Cannot register offline table window");
    font_ = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    if (!font_) throw std::runtime_error("Cannot create offline table font");
    viewport_ = CreateWindowExW(WS_EX_CONTROLPARENT, kClass, L"", WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL,
                                0, 0, width_, height_, parent_, reinterpret_cast<HMENU>(202), instance, this);
    if (!viewport_) throw std::runtime_error("Cannot create offline table viewport");
    const auto control = [&](const wchar_t* kind, const wchar_t* text, int id, DWORD style = 0) {
        const auto window = CreateWindowExW(std::wstring(kind) == L"EDIT" ? WS_EX_CLIENTEDGE : 0,
            kind, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 1, 1, viewport_, reinterpret_cast<HMENU>(id), instance, nullptr);
        if (!window) throw std::runtime_error("Cannot create offline table control");
        SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
        return window;
    };
    path_ = control(L"EDIT", L"", kPath, WS_TABSTOP | ES_AUTOHSCROLL);
    SendMessageW(path_, EM_LIMITTEXT, 32767, 0);
    load_ = control(L"BUTTON", L"载入", kLoad, WS_TABSTOP);
    open_ = control(L"BUTTON", L"打开...", kOpen, WS_TABSTOP);
    previous_ = control(L"BUTTON", L"后退", kPrevious, WS_TABSTOP);
    next_ = control(L"BUTTON", L"前进", kNext, WS_TABSTOP);
    reset_ = control(L"BUTTON", L"起点", kReset, WS_TABSTOP);
    end_ = control(L"BUTTON", L"终点", kEnd, WS_TABSTOP);
    playback_ = control(L"BUTTON", L"播放", kPlayback, WS_TABSTOP);
    speed_ = control(L"COMBOBOX", L"", kSpeed, WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
    for (const auto* label : {L"0.5x", L"1x", L"2x", L"4x", L"10x"})
        SendMessageW(speed_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
    SendMessageW(speed_, CB_SETCURSEL, 1, 0);
    rounds_ = control(L"COMBOBOX", L"", kRounds, WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
    animate_control_ = control(L"BUTTON", L"动画", kAnimate, WS_TABSTOP | BS_AUTOCHECKBOX);
    sound_control_ = control(L"BUTTON", L"声音", kSound, WS_TABSTOP | BS_AUTOCHECKBOX);
    results_control_ = control(L"BUTTON", L"结算", kResults, WS_TABSTOP | BS_AUTOCHECKBOX);
    settlement_view_ = std::make_unique<SettlementView>(viewport_);
    settlement_view_->SetBackgroundPainter([this](HDC dc,RECT r) {
        if (session_ && session_->state().initialized) PaintTable(dc,r);
    });
    settlement_view_->SetConfirmHandler([this] {
        show_results_=false; SendMessageW(results_control_,BM_SETCHECK,BST_UNCHECKED,0);
        Update();InvalidateRect(viewport_,nullptr,FALSE);
    });
    match_intro_ = std::make_unique<MatchIntroView>(viewport_, [this] {
        opening_start_ = GetTickCount64();
        if (start_sound_pending_) { start_sound_pending_=false; PlaySoundId(112); }
        log_("MATCH INTRO DONE"); Update();
    });
    match_intro_->SetSoundPlayer([this](unsigned id) { PlaySoundId(id); });
    settlement_view_->SetSoundPlayer([this](unsigned id) { PlaySoundId(id); });
    settlement_view_->SetSoundStopper([this] { try { audio_.Stop(); } catch (const std::exception& e) { SoundError(e); } });
    animation_enabled_ = GetPrivateProfileIntW(L"REBUILD", L"ReplayAnimation", 1, config_path_.c_str()) != 0;
    sound_enabled_ = GetPrivateProfileIntW(L"REBUILD", L"ReplaySound", 0, config_path_.c_str()) != 0;
    settlement_view_->SetSoundEnabled(sound_enabled_);
    settlement_view_->SetAnimationEnabled(animation_enabled_);
    SendMessageW(animate_control_, BM_SETCHECK, animation_enabled_ ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(sound_control_, BM_SETCHECK, sound_enabled_ ? BST_CHECKED : BST_UNCHECKED, 0);
    status_ = control(L"STATIC", L"", kStatus);
    Update();
    Resize(width_, height_);
}

TableView::~TableView() {
    Stop();
    match_intro_.reset();
    settlement_view_.reset();
    if (viewport_ && IsWindow(viewport_)) DestroyWindow(viewport_);
    if (font_) DeleteObject(font_);
}
void TableView::Show(bool visible) {
    if (!visible) { Stop(); Update(); }
    ShowWindow(viewport_, visible ? SW_SHOW : SW_HIDE);
}
void TableView::ConfigureArtwork(const std::filesystem::path& directory, int width) {
    match_intro_->Configure(directory, width);
    settlement_view_->Configure(directory, width);
    display_artwork_.Configure(directory, width);
    const bool was_playing = playing_;
    Stop();
    if (resource_directory_ != directory || canvas_width_ != width) {
        artwork_ = {}; table_background_ = {}; artwork_error_.clear();
        artwork_.SetColor(preferences_.TileColor());
        resource_directory_ = directory; canvas_width_ = width;
        sounds_ = {}; sound_loaded_ = false; audio_error_.clear();
        InvalidateRect(viewport_, nullptr, FALSE);
    }
    if (was_playing) Update();
}
void TableView::Resize(int width, int height) {
    // Reload, display switches and text-details toggles also call Resize with
    // parent dimensions. Keep these paths on the same fixed content area.
    width = kDefaultClientWidth;
    height = kDefaultClientHeight;
    width_ = width;
    height_ = height;
    MoveWindow(viewport_, 0, 0, width, height, TRUE);
    RECT client{}; GetClientRect(viewport_, &client);
    width_ = client.right;
    width = width_;
    if (!path_) return;
    MoveWindow(path_, 16, 42, std::max(100, width - 190), 27, TRUE);
    MoveWindow(load_, width - 166, 42, 64, 27, TRUE);
    MoveWindow(open_, width - 94, 42, 78, 27, TRUE);
    const HWND buttons[]{reset_, previous_, next_, end_};
    for (int i = 0; i < 4; ++i) MoveWindow(buttons[i], 16 + i * 100, 78, 92, 27, TRUE);
    MoveWindow(status_, 424, 82, std::max(100, width - 440), 22, TRUE);
    MoveWindow(playback_, 16, 112, 80, 27, TRUE);
    MoveWindow(speed_, 104, 112, 80, 180, TRUE);
    MoveWindow(rounds_, 198, 112, 210, 240, TRUE);
    MoveWindow(animate_control_, 526, 112, 78, 27, TRUE);
    MoveWindow(sound_control_, 608, 112, 68, 27, TRUE);
    MoveWindow(results_control_, width - 100, 148, 84, 27, TRUE);
    settlement_view_->Move({16, 204, width - 16, std::max(254, height - 48)});
    const bool compact = Compact();
    for (HWND control : {path_, load_}) ShowWindow(control, compact ? SW_HIDE : SW_SHOW);
    if (compact) {
        const HWND nav[]{reset_, previous_, next_, end_, playback_};
        const int widths[]{78, 78, 64, 64, 64}; int x = 12;
        for (int i = 0; i < 5; ++i) { MoveWindow(nav[i], x, 8, widths[i], 26, TRUE); x += widths[i] + 4; }
        MoveWindow(speed_, x, 8, 62, 180, TRUE); x += 66;
        MoveWindow(rounds_, x, 8, std::max(80, width - x - 12), 240, TRUE);
        MoveWindow(open_, 12, 42, 74, 26, TRUE);
        MoveWindow(animate_control_, 210, 42, 80, 26, TRUE); MoveWindow(sound_control_, 298, 42, 76, 26, TRUE);
        MoveWindow(results_control_, width - 100, 42, 88, 26, TRUE);
        MoveWindow(status_, 12, height - 26, width - 24, 22, TRUE);
    }
    settlement_view_->Move(BoardBounds());
    match_intro_->Move({0, 0, width_, height_});
    UpdateScroll();
    InvalidateRect(viewport_, nullptr, FALSE);
}
bool TableView::Compact() const { return original_display_ && session_ && session_->state().initialized && !text_details_; }
RECT TableView::BoardBounds() const { return Compact() ? RECT{8, 78, width_ - 8, height_ - 30} : RECT{16, 204, width_ - 16, height_ - 48}; }
void TableView::SetOriginalDisplay(bool enabled) {
    Stop();
    original_display_ = enabled;
    RECT r{}; GetClientRect(parent_, &r); Resize(r.right, r.bottom);
}
void TableView::SetEffectsOnly(bool enabled) {
    try { audio_.Stop(); } catch (const std::exception& e) { SoundError(e); }
    effects_only_ = enabled;
    individual_voices_ = false;
}
void TableView::PresentMatchStart() {
    if (!session_ || !session_->state().initialized) return;
    std::string dan; unsigned type = red_tiles_ ? 1 : 3;
    for (std::size_t i = 0; i < session_->position(); ++i) {
        const auto& e = session_->log().events[i];
        if (e.tag == "UN" && e.attributes.count("dan")) dan = e.attributes.at("dan");
        if (e.tag == "GO" && e.attributes.count("type")) type = static_cast<unsigned>(std::stoul(e.attributes.at("type")));
    }
    show_results_ = false; SendMessageW(results_control_, BM_SETCHECK, BST_UNCHECKED, 0);
    settlement_view_->Show(false);
    match_intro_->Move({0, 0, width_, height_}); match_intro_->Begin(session_->state(), type, dan);
    log_("MATCH INTRO BEGIN");
}
void TableView::Load(const std::filesystem::path& path) {
    Stop();
    SetWindowTextW(path_, path.c_str());
    error_.clear();
    try {
        if (path.empty()) throw std::runtime_error("Choose a four-player Tenhou mjloggm XML or gzip file.");
        auto candidate = std::make_unique<ReplaySession>(ReplayLog::Load(path));
        bool red = true;
        for (const auto& event : candidate->log().events) if (event.tag == "GO") {
            red = (std::stoi(event.attributes.at("type")) & 2) == 0; break;
        }
        std::vector<std::size_t> positions;
        std::vector<std::wstring> labels;
        for (std::size_t i = 0; i < candidate->log().events.size(); ++i) {
            const auto& event = candidate->log().events[i];
            if (event.kind != ReplayEventKind::Init) continue;
            positions.push_back(i + 1);
            std::istringstream seed(event.attributes.at("seed"));
            int round = 0, honba = 0; char comma = 0;
            seed >> round >> comma >> honba;
            const char* winds[]{"East", "South", "West", "North"};
            labels.push_back(Wide(std::to_string(positions.size()) + ". " + winds[(round / 4) % 4] + " " +
                std::to_string(round % 4 + 1) + " / honba " + std::to_string(honba)));
        }
        // Load validates every state transition before exposing a candidate.
        session_ = std::move(candidate);
        red_tiles_ = red;
        round_positions_ = std::move(positions);
        SendMessageW(rounds_, CB_RESETCONTENT, 0, 0);
        for (const auto& label : labels) SendMessageW(rounds_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        // Front-load the tile pack read/background build between the replay
        // load and the first positioned paint; first paint retries lazily.
        try {
            if (!artwork_.width()) artwork_.Load(resource_directory_, canvas_width_);
            if (table_background_.bgra.empty())
                table_background_ = BuildPreferredBackground(resource_directory_,config_path_,canvas_width_,canvas_width_*3/4,preferences_);
        } catch (const std::exception&) { }
        painted_ = false;
        scroll_ = 0;
        log_("REPLAY LOADED events=" + std::to_string(session_->log().events.size()));
    } catch (const std::exception& error) {
        error_ = error.what();
        log_("REPLAY ERROR " + error_);
    }
    Update();
}
void TableView::Reload() {
    display_artwork_.Configure(resource_directory_, canvas_width_);
    Stop(); artwork_ = {}; table_background_ = {}; artwork_error_.clear();
    artwork_.SetColor(preferences_.TileColor());
    sounds_ = {}; sound_loaded_ = false; audio_error_.clear();
    if (GetWindowTextLengthW(path_)) Command(kLoad); else Update();
}
void TableView::UpdateScroll() {
    const int visible = std::max(1, height_ - 204 - 48);
    int card_height = std::max(180, (visible - 10) / 2);
    if (session_) {
        const int per_row = std::max(1, ((width_ - 42) / 2 - 16) / 28);
        for (const auto& player : session_->state().players) {
            const int rows = (static_cast<int>(player.discards.size()) + per_row - 1) / per_row;
            card_height = std::max(card_height, 107 + std::max(1, rows) * 23 + 20 + static_cast<int>(player.melds.size()) * 20 + 8);
        }
    }
    board_height_ = card_height * 2 + 10;
    SCROLLINFO info{sizeof(SCROLLINFO)};
    info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
    info.nMin = 0; info.nMax = board_height_ - 1;
    info.nPage = static_cast<UINT>(session_ && text_details_ ? visible : board_height_);
    scroll_ = std::clamp(scroll_, 0, std::max(0, board_height_ - visible));
    info.nPos = scroll_;
    SetScrollInfo(viewport_, SB_VERT, &info, TRUE);
}
void TableView::Update() {
    const bool result_available = session_ && !session_->state().settlement.receipts.empty();
    EnableWindow(results_control_, result_available);
    if (!result_available) { show_results_ = false; SendMessageW(results_control_, BM_SETCHECK, BST_UNCHECKED, 0); }
    if (session_) {
        std::array<std::string, 4> names;
        for (int i = 0; i < 4; ++i) names[i] = session_->state().players[i].name;
        settlement_view_->SetContext(session_->state().dealer,session_->state().honba,session_->state().sticks);
        settlement_view_->Set(session_->state().settlement, names, red_tiles_);
    }
    settlement_view_->Show(show_results_ && result_available);
    if (result_pending_ && show_results_ && result_available) settlement_view_->Present();
    result_pending_ = false;
    const bool before = session_ && session_->position() > 0;
    const bool after = session_ && session_->position() < session_->log().events.size();
    EnableWindow(previous_, before); EnableWindow(reset_, before);
    EnableWindow(next_, after); EnableWindow(end_, after);
    if (!after && playing_) {
        KillTimer(viewport_, kPlaybackTimer); playing_ = false;
        SetWindowTextW(playback_, L"播放");
        log_("REPLAY PLAYBACK end");
    }
    EnableWindow(playback_, after);
    EnableWindow(rounds_, session_ && !round_positions_.empty());
    int selected_round = -1;
    if (session_) for (std::size_t i = 0; i < round_positions_.size(); ++i)
        if (round_positions_[i] <= session_->position()) selected_round = static_cast<int>(i);
    SendMessageW(rounds_, CB_SETCURSEL, selected_round, 0);
    auto status = session_ ? "cursor " + std::to_string(session_->position()) + "/" +
        std::to_string(session_->log().events.size()) : error_.empty() ? "No replay loaded" : "Load failed";
    if (session_ && !error_.empty()) status += " | Load failed; previous replay kept";
    if (playing_) status += " | Playing";
    if (!audio_error_.empty()) status += " | Sound error (F5 retries): " + audio_error_;
    SetWindowTextW(status_, Wide(status).c_str());
    if (session_) log_("REPLAY CURSOR " + std::to_string(session_->position()) + "/" + std::to_string(session_->log().events.size()));
    RECT r{}; GetClientRect(parent_, &r); Resize(r.right, r.bottom);
}
void TableView::Command(int id) {
    if (id == kResults) {
        Stop(); show_results_ = SendMessageW(results_control_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        Update(); return;
    }
    if (id == kAnimate || id == kSound) {
        if (id == kAnimate) presentation_.Cancel();
        else { try { audio_.Stop(); } catch (const std::exception& e) { SoundError(e); } }
        animation_enabled_ = SendMessageW(animate_control_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        sound_enabled_ = SendMessageW(sound_control_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        settlement_view_->SetSoundEnabled(sound_enabled_);
        settlement_view_->SetAnimationEnabled(animation_enabled_);
        if (!animation_enabled_ && !sound_enabled_) opening_.Cancel();
        if (id == kSound) { sounds_ = {}; sound_loaded_ = false; audio_error_.clear(); }
        Update(); return;
    }
    if (id == kPlayback) {
        if (playing_) Stop(); else Start();
        Update(); return;
    }
    if (id == kSpeed) {
        if (playing_) { Stop(); Start(); }
        Update(); return;
    }
    if (id == kRounds) {
        Stop();
        const auto index = SendMessageW(rounds_, CB_GETCURSEL, 0, 0);
        if (session_ && index >= 0 && static_cast<std::size_t>(index) < round_positions_.size()) {
            session_->Seek(round_positions_[static_cast<std::size_t>(index)]); Update();
        }
        return;
    }
    if (id == kLoad) {
        std::wstring value(GetWindowTextLengthW(path_) + 1, L' ');
        GetWindowTextW(path_, value.data(), static_cast<int>(value.size()));
        value.resize(value.size() - 1);
        Load(std::filesystem::path(value));
    } else if (id == kOpen) {
        Stop(); Update();
        wchar_t file[32768]{};
        OPENFILENAMEW dialog{sizeof(OPENFILENAMEW)};
        dialog.hwndOwner = parent_;
        dialog.lpstrFilter = L"Tenhou replay (*.xml;*.mjlog;*.gz)\0*.xml;*.mjlog;*.gz\0All files\0*.*\0\0";
        dialog.lpstrFile = file; dialog.nMaxFile = 32768;
        dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
        if (GetOpenFileNameW(&dialog)) Load(std::filesystem::path(file));
        else if (CommDlgExtendedError()) throw std::runtime_error("Open replay dialog failed");
    } else if (session_ && (id == kPrevious || id == kNext || id == kReset || id == kEnd)) {
        Stop();
        if (id == kPrevious && session_->position()) session_->Seek(session_->position() - 1);
        else if (id == kNext) Advance();
        else if (id == kReset) session_->Reset();
        else if (id == kEnd) session_->Seek(session_->log().events.size());
        Update();
    }
}
void TableView::Stop() {
    if (viewport_) KillTimer(viewport_, kPlaybackTimer);
    if (playing_) log_("REPLAY PLAYBACK paused");
    playing_ = false;
    if (playback_) SetWindowTextW(playback_, L"播放");
    CancelEffects();
}
void TableView::Start() {
    if (!session_ || session_->position() >= session_->log().events.size() || !IsWindowVisible(viewport_)) return;
    const auto index = SendMessageW(speed_, CB_GETCURSEL, 0, 0);
    if (index < 0 || index >= 5) return;
    if (!SetTimer(viewport_, kPlaybackTimer, kIntervals[index], nullptr))
        throw std::runtime_error("Cannot start replay timer");
    playing_ = true;
    next_event_at_ = GetTickCount64() + kIntervals[index];
    SetWindowTextW(playback_, L"暂停");
    log_("REPLAY PLAYBACK interval_ms=" + std::to_string(kIntervals[index]));
}
void TableView::Tick() {
    if (match_intro_->Active() || opening_.Active() || draw_end_ticks_ || (show_results_ && (settlement_view_->Animating() || audio_.Active()))) return;
    if (!playing_ || !session_ || GetTickCount64() < next_event_at_) return;
    const auto index = SendMessageW(speed_, CB_GETCURSEL, 0, 0);
    next_event_at_ = GetTickCount64() + kIntervals[index];
    Advance(); Update();
}
void TableView::SaveSettings() {
    if (!WritePrivateProfileStringW(L"REBUILD", L"ReplayAnimation", animation_enabled_ ? L"1" : L"0", config_path_.c_str()) ||
        !WritePrivateProfileStringW(L"REBUILD", L"ReplaySound", sound_enabled_ ? L"1" : L"0", config_path_.c_str()))
        throw std::runtime_error("Cannot save replay effects settings");
}
void TableView::CancelEffects() {
    captions_.Clear(); KillTimer(viewport_,8);
    motion_timer_.Stop();
    start_sound_pending_ = false;
    if (match_intro_) match_intro_->Cancel();
    opening_.Cancel();
    draw_end_ticks_=0;
    result_pending_ = false;
    if (settlement_view_) settlement_view_->CancelPresentation();
    if (viewport_) KillTimer(viewport_, kEffectsTimer);
    if (presentation_.Active()) log_("ANIMATION settled");
    presentation_.Cancel();
    try { audio_.Stop(); } catch (const std::exception& e) { SoundError(e); }
    if (viewport_) InvalidateRect(viewport_, nullptr, FALSE);
}
void TableView::SoundError(const std::exception& error) {
    audio_error_ = error.what(); log_("AUDIO ERROR " + audio_error_);
    if (status_) {
        const auto cursor = session_ ? "cursor " + std::to_string(session_->position()) + "/" + std::to_string(session_->log().events.size()) + " | " : std::string();
        SetWindowTextW(status_, Wide(cursor + "Sound error (F5 retries): " + audio_error_).c_str());
    }
    try { audio_.Stop(); } catch (const std::exception& e) { log_(std::string("AUDIO RESET ERROR ") + e.what()); }
    InvalidateRect(viewport_, nullptr, FALSE);
}
void TableView::Advance() {
    if (!session_ || session_->position() >= session_->log().events.size()) return;
    // Cancel the old receipt before dispatching the new event's sounds. A later
    // Set/Select must not reset the voice just emitted for the new hand/winner.
    settlement_view_->CancelPresentation();
    if (opening_.Active()) { opening_.Cancel(); audio_.Stop(); }
    // Settle the previous presentation, without mutating the replay state.
    motion_timer_.Stop();
    presentation_.Cancel();
    const auto before = session_->state();
    const auto event = session_->log().events[session_->position()];
    session_->Step();
    if (captions_.Accept(event)) SetTimer(viewport_,8,15,nullptr);
    
    result_pending_ = event.kind == ReplayEventKind::Win || event.kind == ReplayEventKind::DrawEnd;
    draw_end_ticks_=event.kind==ReplayEventKind::DrawEnd ? 28 : 0;
    if (draw_end_ticks_) SetTimer(viewport_,kEffectsTimer,15,nullptr);
    if (animation_enabled_ && !text_details_ && artwork_error_.empty()) {
        try {
            if (!artwork_.width()) artwork_.Load(resource_directory_, canvas_width_);
            unsigned duration = event.kind == ReplayEventKind::Init ? 520 : event.kind == ReplayEventKind::Meld ? 360 : 240;
            if (playing_) {
                const auto index = SendMessageW(speed_, CB_GETCURSEL, 0, 0);
                duration = std::min(duration, kIntervals[index] * 3 / 4);
            }
            presentation_.Begin(before, session_->state(), event, duration, canvas_width_, [&](const TableTile& tile) {
                const auto& image = artwork_.Get(tile, red_tiles_);
                const auto offset=artwork_.Offset(tile);
                return std::make_pair(image.x+offset.x,image.y+offset.y);
            },AnimationNow());
            if (presentation_.Active()) {
                log_("ANIMATION BEGIN cursor=" + std::to_string(session_->position()) + " duration=" + std::to_string(duration));
            }
        } catch (const std::exception& e) {
            artwork_error_ = e.what(); log_("REPLAY ARTWORK ERROR " + artwork_error_); presentation_.Cancel();
        }
    }
    const int who = event.kind == ReplayEventKind::Win ? std::stoi(event.attributes.at("who")) : event.who;
    const char gender = who >= 0 && who < 4 ? session_->state().players[who].gender : '?';
    const bool intro = event.kind==ReplayEventKind::Init && !before.initialized && playing_ && animation_enabled_ && original_display_ && !text_details_;
    if (intro) start_sound_pending_=!ReplaySounds(event,gender,effects_only_,individual_voices_).empty();
    else for (auto id : ReplaySounds(event, gender, effects_only_, individual_voices_)) PlaySoundId(id);
    if (event.kind == ReplayEventKind::Init && (animation_enabled_ || sound_enabled_)) {
        presentation_.Cancel();
        opening_start_ = GetTickCount64();
        if (!artwork_.width()) artwork_.Load(resource_directory_,canvas_width_);
        opening_.Begin(session_->state(), original_display_ && !text_details_, &artwork_);
        if (!intro && opening_.Tick() && opening_.sound()) PlaySoundId(opening_.sound());
        log_("OPENING BEGIN wall=" + std::to_string(opening_.wall_tiles()));
    }
    if (event.kind == ReplayEventKind::Init && !before.initialized && playing_ && animation_enabled_ && original_display_ && !text_details_)
        PresentMatchStart();
    if (presentation_.Active()) motion_timer_.Start(viewport_, kMotionTimer);
    if (opening_.Active() || draw_end_ticks_ || (sound_enabled_ && audio_error_.empty()))
        if (!SetTimer(viewport_, kEffectsTimer, opening_.Active()?opening_.interval():(draw_end_ticks_?15:25), nullptr)) throw std::runtime_error("Cannot start replay effects timer");
}
void TableView::PlaySoundId(unsigned id, unsigned delay) {
    if (!sound_enabled_ || !audio_error_.empty()) return;
    try {
        if (!sound_loaded_) { sounds_.Load(resource_directory_, canvas_width_, config_path_); sound_loaded_ = true; log_("AUDIO BANK loaded"); }
        audio_.Schedule(id, sounds_.Get(id), delay);
        if (!SetTimer(viewport_, kEffectsTimer, opening_.Active()?opening_.interval():(draw_end_ticks_?15:25), nullptr)) throw std::runtime_error("Cannot start replay effects timer");
    } catch (const std::exception& e) { SoundError(e); }
}
void TableView::EffectsTick() {
    if (draw_end_ticks_) {
        const unsigned before=14-draw_end_ticks_/2;
        --draw_end_ticks_;
        const unsigned slots=14-draw_end_ticks_/2;
        if (slots!=before) {
            const unsigned seats=DrawEndChangedSeats(session_->state(),slots-1);
            if (seats) PlaySoundId(104);
            log_("REPLAY DRAW SLOT index="+std::to_string(slots-1)+" seats="+std::to_string(seats));
        }
        if (!draw_end_ticks_) {
            log_("REPLAY DRAW TABLE DONE");
            show_results_=true; result_pending_=true;
            SendMessageW(results_control_,BM_SETCHECK,BST_CHECKED,0); Update();
        }
        InvalidateRect(viewport_,nullptr,FALSE);
    }
    if (match_intro_->Active()) { try { audio_.Poll(); } catch (const std::exception& e) { SoundError(e); } return; }
    if (opening_.Active()) {
        
        if (opening_.Tick() && opening_.sound()) PlaySoundId(opening_.sound());
        SetTimer(viewport_,kEffectsTimer,opening_.interval(),nullptr);
        if (!opening_.Active() && animation_enabled_) log_("ANIMATION END opening");
        if (!opening_.Active()) log_("OPENING DONE");
        InvalidateRect(viewport_, nullptr, FALSE);
    }
    try { audio_.Poll(); } catch (const std::exception& e) { SoundError(e); }
    if (!opening_.Active() && !draw_end_ticks_ && !audio_.Active()) KillTimer(viewport_, kEffectsTimer);
}

void TableView::PaintPlayer(HDC dc, int seat, RECT bounds) {
    const int saved = SaveDC(dc);
    IntersectClipRect(dc, bounds.left, bounds.top, bounds.right, bounds.bottom);
    const auto& state = session_->state();
    const auto& player = state.players[seat];
    Fill(dc, bounds, state.turn == seat ? RGB(45, 103, 90) : RGB(37, 76, 70));
    SetTextColor(dc, RGB(245, 245, 235));
    std::ostringstream header;
    header << "Seat " << seat << "  " << player.score;
    if (seat == state.dealer) header << "  [dealer]";
    if (player.reach) header << "  [riichi " << player.reach << "]";
    if (std::find(state.winners.begin(), state.winners.end(), seat) != state.winners.end()) header << "  [win]";
    if(!player.nuki.empty()) header << "  [north x" << player.nuki.size() << "]";
    header << "  " << player.name;
    Text(dc, header.str(), {bounds.left + 8, bounds.top + 7, bounds.right - 8, bounds.top + 27});
    const int available = bounds.right - bounds.left - 16;
    const int tile_width = std::min(32, std::max(19, available / 14 - 2));
    const int start = bounds.left + 8;
    const int top = bounds.top + 32;
    auto hand = player.hand;
    std::sort(hand.begin(), hand.end());
    for (std::size_t i = 0; i < hand.size(); ++i) {
        const int x = start + static_cast<int>(i) * (tile_width + 2);
        Tile(dc, hand[i], {x, top, x + tile_width, top + 29}, false, false, hand[i] == player.drawn_tile, red_tiles_);
    }
    SetTextColor(dc, RGB(225, 235, 230));
    Text(dc, "River (red border: riichi; muted: claimed)", {start, top + 33, bounds.right - 8, top + 51});
    const int river_width = 26;
    const int per_row = std::max(1, available / (river_width + 2));
    for (std::size_t i = 0; i < player.discards.size(); ++i) {
        const auto& item = player.discards[i];
        const int x = start + static_cast<int>(i) % per_row * (river_width + 2);
        const int y = top + 52 + static_cast<int>(i) / per_row * 23;
        Tile(dc, item.tile, {x, y, x + river_width, y + 21}, item.claimed, item.riichi, false, red_tiles_);
    }
    SetTextColor(dc, RGB(225, 235, 230));
    const int river_rows = (static_cast<int>(player.discards.size()) + per_row - 1) / per_row;
    const int meld_top = top + 55 + std::max(1, river_rows) * 23;
    Text(dc, player.melds.empty() ? "Melds: none" : "Melds:", {start, meld_top, bounds.right - 8, meld_top + 18});
    int meld_y = meld_top + 20;
    for (const auto& meld : player.melds) {
        std::string melds = std::string(MeldKindName(meld.kind)) + ": ";
        for (int tile : meld.tiles) melds += CompactTile(tile, red_tiles_) + " ";
        melds += "(seat " + std::to_string(meld.from) + ")";
        Text(dc, melds, {start, meld_y, bounds.right - 8, meld_y + 20});
        meld_y += 20;
    }
    RestoreDC(dc, saved);
}

void TableView::PaintTable(HDC dc, RECT bounds) {
    try {
        if (!artwork_.width() || table_background_.bgra.empty()) {
            TableArtwork candidate;
            candidate.Load(resource_directory_, canvas_width_); candidate.SetColor(preferences_.TileColor());
            auto background = BuildPreferredBackground(resource_directory_,config_path_,canvas_width_,canvas_width_*3/4,preferences_);
            artwork_ = std::move(candidate); table_background_ = std::move(background); native_buffer_.Invalidate(); hud_cache_.Invalidate();
            log_("REPLAY ARTWORK canvas=" + std::to_string(canvas_width_));
        }
        const int w = canvas_width_, h = w * 3 / 4;
        artwork_.BeginFrame();
        auto state = opening_.Active() && animation_enabled_ ? opening_.Frame() : session_->state();
        artwork_.SetRedTiles(state.red_tiles);
        state.relative_scores=hud_interaction_.relative();
        if (draw_end_ticks_) state.draw_end_slots=14-draw_end_ticks_/2;
        const bool red = red_tiles_;
        const double unit = double(w)/1024.0;
        const unsigned placed_count = opening_.Active() && animation_enabled_ ? opening_.placed() : 136;
        std::vector<PositionedTile> scene;
        if (opening_.Active() && animation_enabled_) scene = opening_.Scene(artwork_,red);
        else if (presentation_.Active()) scene = presentation_.Frame(AnimationNow());
        else for (const auto& tile : BuildTableScene(state)) {
            const auto& item = artwork_.Get(tile,red);
            const auto offset=artwork_.Offset(tile);
            scene.push_back({tile,item.x+offset.x,item.y+offset.y});
        }
        SortTableScene(scene,artwork_,red);
        if (original_display_) InsertNearWall(scene,artwork_,state,red,placed_count,true);
        const auto shadows=BuildTileShadows(scene,artwork_,ShadowPack::Embedded());
        const auto shadow_damage=TileShadowDamage(scene,shadows,artwork_,red);
        std::vector<RECT> atomic_layers;
        if (original_display_) {
            const auto hud = BuildOriginalHud(state,w);
            const int radius = static_cast<int>(128*hud.scale);
            const int cx = static_cast<int>(hud.center[0]), cy = static_cast<int>(hud.center[1]);
            atomic_layers.push_back({cx-radius,cy-radius,cx+radius,cy+radius});
        }
        native_buffer_.Render(dc,w,FitNativeCanvas(bounds,w),scene,artwork_,red,
            WallMetadata(state,placed_count,original_display_,red),TableMetadata(state)+std::to_string(original_display_)+std::to_string(red),
            [&](HDC memory) {
                Gdiplus::Graphics graphics(memory);
                Gdiplus::Bitmap background(w,h,w*4,PixelFormat32bppARGB,table_background_.bgra.data());
                if (graphics.DrawImage(&background,0,0) != Gdiplus::Ok) throw std::runtime_error("Cannot draw table background");
            },[&](HDC memory,RECT region) {
            std::vector<RECT> highlights;
            {
                Gdiplus::Graphics graphics(memory); ClipGraphics(graphics,region);
                shadow_layer_.Paint(graphics,shadows,preferences_.tile_shadow);
                for (const auto& placed : scene) {
                    const auto& command = placed.tile;
                    const auto& item = artwork_.Get(command, red);
                    if (!Overlaps(region, TileBounds(placed, item.image.width, item.image.height))) continue;
                    Image shaded;
                    const auto* pixels=item.image.bgra.data();
                    if(const auto* shadow=FindTileShadow(shadows,placed); shadow && shadow->received) {
                        shaded=ShadeTile(item.image,placed.x-shadow->dx,placed.y-shadow->dy,*shadow->received,preferences_.tile_shadow);
                        pixels=shaded.bgra.data();
                    }
                    Gdiplus::Bitmap image(item.image.width, item.image.height, item.image.width * 4,
                        PixelFormat32bppARGB, const_cast<BYTE*>(pixels));
                    const int x = placed.x;
                    const int y = placed.y;
                    if (graphics.DrawImage(&image, x, y) != Gdiplus::Ok) {
                        throw std::runtime_error("Cannot draw table tile");
                    }
                    if (command.drawn || command.riichi) highlights.push_back({x, y, x + item.image.width, y + item.image.height});
                }
                // Indicator strip uses original face images with a new caption/position.
                for (std::size_t i = 0; i < state.dora_indicators.size(); ++i) {
                    if (original_display_ && state.wall.known) break;
                    const auto& item = artwork_.Get({'k', 0, state.dora_indicators[i], 0}, red);
                    Gdiplus::Bitmap image(item.image.width, item.image.height, item.image.width * 4,
                        PixelFormat32bppARGB, const_cast<BYTE*>(item.image.bgra.data()));
                    const int x = static_cast<int>((original_display_ ? 28 + i * 40 : 410 + i * 40) * unit), y = static_cast<int>((original_display_ ? 28 : 345) * unit);
                    if (graphics.DrawImage(&image, x, y) != Gdiplus::Ok) throw std::runtime_error("Cannot draw dora indicator");
                }
            }
            const auto highlight = CreateSolidBrush(RGB(235, 185, 74));
            if (!original_display_) for (const auto& rect : highlights) FrameRect(memory, &rect, highlight);
            DeleteObject(highlight);
            if (original_display_) {
                if (Overlaps(region,atomic_layers.front())) {
                    const auto draw_hud = [&](HDC target) { Gdiplus::Graphics g(target); display_artwork_.PaintHud(g,state); };
                    if (RetainedScene::Reference()) draw_hud(memory);
                    else hud_cache_.Paint(memory,w,atomic_layers.front(),TableMetadata(state),draw_hud);
                }
                Gdiplus::Graphics graphics(memory); ClipGraphics(graphics,region);
                unsigned type = 0;
                for (const auto& event : session_->log().events) if (event.tag == "GO" && event.attributes.count("type")) { type = static_cast<unsigned>(std::stoul(event.attributes.at("type"))); break; }
                display_artwork_.PaintChrome(graphics, type);
                display_artwork_.PaintStatus(graphics,state);
                display_artwork_.PaintNukiCounts(graphics,state,artwork_);
            } else {
                const HFONT label_font = CreateFontW(-static_cast<int>(22 * unit), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
                const auto previous = SelectObject(memory, label_font ? label_font : font_);
                SetBkMode(memory, TRANSPARENT); SetTextColor(memory, RGB(250, 246, 225));
                const auto caption = [&](const std::string& text, int x, int y, int width, UINT align = DT_CENTER) {
                    Text(memory, text, {static_cast<LONG>(x * unit), static_cast<LONG>(y * unit),
                        static_cast<LONG>((x + width) * unit), static_cast<LONG>((y + 28) * unit)}, align | DT_SINGLELINE | DT_END_ELLIPSIS);
                };
                constexpr int label_x[]{220, 776, 280, 12}, label_y[]{740, 580, 84, 580};
                for (int seat = 0; seat < 4; ++seat) {
                    if(!SeatPresent(state.seat_mask,seat)) continue;
                    const auto& player = state.players[seat];
                    const char* winds[]{"E", "S", "W", "N"};
                    std::string label = std::string(winds[SeatWind(state.seat_mask,state.dealer,seat)]) + " " + std::to_string(player.score);
                    if (state.turn == seat) label += " >";
                    if (player.reach) label += " RIICHI";
                    if (std::find(state.winners.begin(), state.winners.end(), seat) != state.winners.end()) label += " WIN";
                    caption(label, label_x[seat], label_y[seat], seat % 2 ? 235 : 500);
                    caption(player.name, label_x[seat], label_y[seat] - (seat == 2 ? -28 : 26), seat % 2 ? 235 : 500);
                }
                const char* round_winds[]{"East", "South", "West", "North"};
                caption(std::string(round_winds[(state.round / 4) % 4]) + " " + std::to_string(state.round % 4 + 1), 405, 270, 210);
                caption("Honba " + std::to_string(state.honba) + " | Sticks " + std::to_string(state.sticks), 380, 297, 260);
                caption("Dora indicators", 400, 322, 230);
                SelectObject(memory, previous);
                if (label_font) DeleteObject(label_font);
            }
        },{},{},{},atomic_layers,shadow_damage);
        if (original_display_) captions_.Paint(dc,FitNativeCanvas(bounds,w),w,display_artwork_);
        artwork_error_.clear();
    } catch (const std::exception& error) {
        if (artwork_error_ != error.what()) log_("REPLAY ARTWORK ERROR " + std::string(error.what()));
        artwork_error_ = error.what();
        const bool was_playing = playing_;
        Stop();
        if (was_playing) Update();
        SetTextColor(dc, RGB(255, 177, 160));
        Text(dc, "牌桌资源加载失败：" + artwork_error_ + "\n请修复资源后按 F5 重试。", bounds, DT_LEFT | DT_WORDBREAK);
    }
}

void TableView::Paint(HDC dc, bool table_only) {
    
    if (!table_only) Fill(dc, {0, 0, width_, height_}, RGB(23, 45, 44));
    const auto old_font = SelectObject(dc, font_);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(245, 245, 235));
    if (!Compact()) Text(dc, "Offline replay | Original tile artwork", {16, 12, width_ - 304, 34});
    if (!error_.empty() && !session_) {
        SetTextColor(dc, RGB(255, 177, 160));
        Text(dc, "Replay loading failed:\n\n" + error_ + "\n\nChoose another file, then Load.",
            {24, 164, width_ - 24, height_ - 56}, DT_LEFT | DT_WORDBREAK);
    } else if (!session_) {
        Text(dc, "打开本机 XML 或 gzip 牌谱文件，使用前进、回退、播放、倍速及跳局查看对局。",
            {24, 164, width_ - 24, height_ - 56}, DT_LEFT | DT_WORDBREAK);
    } else {
        const auto& state = session_->state();
        std::ostringstream summary;
        summary << "Round " << state.round + 1 << " | Honba " << state.honba << " | Sticks " << state.sticks << " | Dora: ";
        for (int tile : state.dora_indicators) summary << CompactTile(tile, red_tiles_) << ' ';
        if (!state.initialized) summary << " | Awaiting INIT";
        if (state.ended) summary << " | " << state.result;
        if (!Compact()) Text(dc, summary.str(), {16, 148, width_ - 110, 168});
        SetTextColor(dc, error_.empty() ? RGB(245, 245, 235) : RGB(255, 177, 160));
        if (!audio_error_.empty()) SetTextColor(dc, RGB(255, 177, 160));
        if (!Compact()) Text(dc, !audio_error_.empty() ? "Sound error (F5 retries): " + audio_error_ : error_.empty() ? "Event: " + (state.last_event.empty() ? "Beginning (no events applied)" : state.last_event) : "Load failed (previous replay kept): " + error_,
             {16, 174, width_ - 16, 194});
        const int left = 16, top = BoardBounds().top, gap = 10;
        const int card_width = (width_ - 32 - gap) / 2;
        const int card_height = (board_height_ - gap) / 2;
        const int clip = SaveDC(dc);
        IntersectClipRect(dc, 0, top, width_, BoardBounds().bottom);
        if (!text_details_ && state.initialized) PaintTable(dc, BoardBounds());
        else for (int seat = 0; seat < 4; ++seat) {
            if(!SeatPresent(state.seat_mask,seat)) continue;
            const int x = left + seat % 2 * (card_width + gap);
            const int y = top + seat / 2 * (card_height + gap) - scroll_;
            PaintPlayer(dc, seat, {x, y, x + card_width, y + card_height});
        }
        RestoreDC(dc, clip);
    }
    SetTextColor(dc, RGB(225, 235, 230));
    if (!Compact()) Text(dc, text_details_ ? "m=man, p=pin, s=sou; E/S/W/N=wind; P/F/C=white/green/red; 0=red five | Gold: drawn tile" :
         "All active hands visible | Gold outline: drawn tile / riichi | Text details: full event inspection",
         {16, height_ - 36, width_ - 16, height_ - 15});
    SelectObject(dc, old_font);
    if (!painted_) { log_("PAINT offline replay rendered"); painted_ = true; }
}
void TableView::Capture(HDC dc) {
    SendMessageW(viewport_, WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_NONCLIENT);
    Paint(dc);
    settlement_view_->Capture(dc);
    POINT origin{0, 0};
    ClientToScreen(viewport_, &origin);
    for (HWND control : {path_, load_, open_, previous_, next_, reset_, end_, status_, playback_, speed_, rounds_, animate_control_, sound_control_, results_control_}) {
        if (!IsWindowVisible(control)) continue;
        RECT position{}; GetWindowRect(control, &position);
        const int saved = SaveDC(dc);
        SetViewportOrgEx(dc, position.left - origin.x, position.top - origin.y, nullptr);
        SendMessageW(control, WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT | PRF_NONCLIENT | PRF_ERASEBKGND);
        RestoreDC(dc, saved);
    }
    match_intro_->Capture(dc);
}
LRESULT CALLBACK TableView::ViewProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* view = reinterpret_cast<TableView*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        view = static_cast<TableView*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        view->viewport_ = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
    }
    if (view) {
        try {
            if (message == WM_ERASEBKGND) return 1; // Paint supplies the complete background.
            if (message==WM_MOUSEMOVE || message==WM_MOUSELEAVE || message==WM_LBUTTONDOWN || (message==WM_TIMER && wparam==9)) {
                if (message==WM_MOUSEMOVE) { TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,window,0}; TrackMouseEvent(&track); }
                if (view->original_display_ && view->hud_interaction_.Handle(window,view->BoardBounds(),view->canvas_width_,message,lparam)) {
                    const auto bounds=view->BoardBounds(); InvalidateRect(window,&bounds,FALSE);
                }
                if (message==WM_TIMER) return 0;
            }
            if (message == WM_TIMER && wparam == 8) {
                view->captions_.Tick();
                if (!view->captions_.Active()) KillTimer(window,8);
                const auto bounds = view->BoardBounds(); InvalidateRect(window,&bounds,FALSE); return 0;
            }
            if (message == WM_COMMAND) {
                const int id = LOWORD(wparam), notification = HIWORD(wparam);
                if ((id == kSpeed || id == kRounds) ? notification == CBN_SELCHANGE : notification == BN_CLICKED)
                    view->Command(id);
                return 0;
            }
            if (message == WM_TIMER && wparam == kPlaybackTimer) { view->Tick(); return 0; }
            if (message == WM_TIMER && wparam == kEffectsTimer) { view->EffectsTick(); return 0; }
            if (message == WM_TIMER && wparam == kMotionTimer) {
                
                const bool active = view->presentation_.Active();
                if (view->presentation_.Tick(AnimationNow())) view->log_("ANIMATION END");
                if (!view->presentation_.Active()) view->motion_timer_.Stop();
                if (active) { const auto bounds = view->BoardBounds(); InvalidateRect(window,&bounds,FALSE); }
                return 0;
            }
            if (message == WM_PRINTCLIENT) { view->Paint(reinterpret_cast<HDC>(wparam)); return 0; }
            if (message == WM_MOUSEWHEEL || message == WM_VSCROLL) {
                if (!view->session_ || !view->text_details_) return 0;
                int position = view->scroll_;
                if (message == WM_MOUSEWHEEL) position -= GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA * 54;
                else {
                    const auto command = LOWORD(wparam);
                    if (command == SB_LINEUP) position -= 24;
                    else if (command == SB_LINEDOWN) position += 24;
                    else if (command == SB_PAGEUP) position -= std::max(1, view->height_ - 252);
                    else if (command == SB_PAGEDOWN) position += std::max(1, view->height_ - 252);
                    else if (command == SB_TOP) position = 0;
                    else if (command == SB_BOTTOM) position = view->board_height_;
                    else if (command == SB_THUMBTRACK || command == SB_THUMBPOSITION) {
                        SCROLLINFO info{sizeof(SCROLLINFO)}; info.fMask = SIF_TRACKPOS;
                        GetScrollInfo(window, SB_VERT, &info); position = info.nTrackPos;
                    }
                }
                view->scroll_ = position; view->UpdateScroll(); InvalidateRect(window, nullptr, FALSE); return 0;
            }
            if (message == WM_PAINT) {
                RECT update{}; GetUpdateRect(window,&update,FALSE);
                const auto board = view->BoardBounds();
                const bool table_only = update.left>=board.left && update.top>=board.top && update.right<=board.right && update.bottom<=board.bottom;
                view->paint_buffer_.Paint(window, [&](HDC dc) { view->Paint(dc,table_only); });
                return 0;
            }
        } catch (const std::exception& error) {
            view->Stop();
            view->error_ = error.what();
            view->log_("REPLAY ERROR " + view->error_); view->Update();
            return 0;
        }
    }
    return DefWindowProcW(window, message, wparam, lparam);
}
} // namespace tenhou
