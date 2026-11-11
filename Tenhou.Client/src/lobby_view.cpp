#include "lobby_view.hpp"
#include "settings_view.hpp"
#include "music_preferences.hpp"
#include "matchmaking_view.hpp"
#include "preference_actions.hpp"
#include "replay_effects.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <charconv>
#include <stdexcept>
#include <commctrl.h>

namespace tenhou {
void LobbyView::ApplyPreferences(const ClientPreferences& preferences, PreferenceChange change) {
    if(change==PreferenceChange::MusicPreviewBegin || change==PreferenceChange::MusicPreviewEnd) { music_preview_=change==PreferenceChange::MusicPreviewBegin; StopLobbyMusic(); if(!music_preview_) UpdateLobbyMusic(); return; }
    preferences_=preferences;
    settlement_view_->SetPeekEnabled(preferences.ResultPeek());
    settlement_view_->SetTileColor(preferences.TileColor());
    if (change==PreferenceChange::Artwork || change==PreferenceChange::Initial) {
        lobby_background_ = {};
        table_.SetPreferences(preferences,config_,change==PreferenceChange::Artwork); InvalidateRect(window_,nullptr,FALSE);
    }
    if (change==PreferenceChange::Sound) {
        audio_.SetVolumePercent(preferences.se_volume);
        const auto voice_mode=GetPrivateProfileIntW(L"RESOURCE",L"SE_NOVOICE",
            GetPrivateProfileIntW(L"REBUILD",L"SoundEffectsOnly",0,config_.c_str()),config_.c_str());
        effects_only_=voice_mode==1; individual_voices_=voice_mode==2;
        sound_loaded_=false; audio_error_.clear();
        StopLobbyMusic(); UpdateLobbyMusic();
    }
}
namespace {
constexpr wchar_t kLobbyWindow[] = L"TENHOU_REBUILD_LOBBY";
std::wstring Text(HWND window) {
    const int length=GetWindowTextLengthW(window);
    std::wstring value(static_cast<std::size_t>(length)+1,L'\0');
    GetWindowTextW(window,value.data(),length+1); value.resize(static_cast<std::size_t>(length)); return value;
}
LRESULT CALLBACK ActionButtonProcedure(HWND w, UINT m, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR data) {
    if (m == WM_ERASEBKGND) return 1; // WM_DRAWITEM supplies every pixel.
    if (data && (m == WM_LBUTTONDOWN || m == WM_LBUTTONUP)) {
        const auto* log = reinterpret_cast<const std::function<void(const std::string&)>*>(data);
        (*log)("ACTION POINTER button="+std::to_string(GetDlgCtrlID(w))+" message="+std::to_string(m));
    }
    if (m == WM_MOUSEMOVE) { TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,w,0}; TrackMouseEvent(&track); InvalidateRect(w,nullptr,FALSE); }
    if (m == WM_MOUSELEAVE) InvalidateRect(w,nullptr,FALSE);
    return DefSubclassProc(w,m,wp,lp);
}

}
LobbyView::LobbyView(HWND parent, const std::filesystem::path& config, std::function<void(const std::string&)> log)
    : parent_(parent), config_(config), log_(std::move(log)), audio_(log_), lobby_music_(log_) {
    WNDCLASSEXW wc{sizeof(wc)}; wc.hInstance = GetModuleHandleW(nullptr); wc.lpfnWndProc = Procedure; wc.style=CS_DBLCLKS;
    wc.lpszClassName = kLobbyWindow; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) throw std::runtime_error("Cannot register lobby view");
    font_ = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    window_ = CreateWindowExW(WS_EX_CONTROLPARENT, kLobbyWindow, L"", WS_CHILD | WS_CLIPCHILDREN, 0, 0, 1024, 768,
        parent, reinterpret_cast<HMENU>(220), wc.hInstance, this);
    if (!font_ || !window_) throw std::runtime_error("Cannot create lobby view");
    auto add = [&](int id, const wchar_t* type, const wchar_t* text, DWORD style) {
        const HWND control = CreateWindowExW(0, type, text, WS_CHILD | WS_VISIBLE | style,
            0, 0, 100, 25, window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), wc.hInstance, nullptr);
        if (!control) throw std::runtime_error("Cannot create lobby control");
        controls_[id] = control; SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
    };
    const wchar_t* buttons[]{L"打牌", L"吃", L"碰", L"杠", L"立直", L"荣和", L"自摸", L"流局", L"跳过", L"次局"};
    for (int i = 1; i <= 8; ++i) add(5120 + i, L"BUTTON", buttons[i], WS_TABSTOP | (i >= 1 && i <= 8 ? BS_OWNERDRAW : BS_PUSHBUTTON));
    add(5151,L"BUTTON",L"北",WS_TABSTOP|BS_OWNERDRAW);
    SetWindowSubclass(controls_[5151],ActionButtonProcedure,1,reinterpret_cast<DWORD_PTR>(&log_));
    for (int i = 1; i <= 8; ++i) SetWindowSubclass(controls_[5120+i],ActionButtonProcedure,1,reinterpret_cast<DWORD_PTR>(&log_));
    add(5137,L"BUTTON",L"返回大厅",WS_TABSTOP|BS_PUSHBUTTON);
    add(0xbc6,L"BUTTON",L"EXIT",WS_TABSTOP|BS_OWNERDRAW);
    SetWindowSubclass(controls_[0xbc6],ActionButtonProcedure,1,reinterpret_cast<DWORD_PTR>(&log_));
    LOGFONTW exit_font{}; GetObjectW(GetStockObject(DEFAULT_GUI_FONT),sizeof(exit_font),&exit_font);
    exit_font.lfHeight=-12; lstrcpyW(exit_font.lfFaceName,L"MS UI Gothic"); exit_font_=CreateFontIndirectW(&exit_font);
    WNDCLASSEXW vote_class{sizeof(vote_class)}; vote_class.hInstance=wc.hInstance; vote_class.lpfnWndProc=ExitVoteProcedure;
    vote_class.lpszClassName=L"TENHOU_EXIT_VOTE"; vote_class.hCursor=LoadCursor(nullptr,IDC_ARROW);
    vote_class.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_BTNFACE+1);
    if(!RegisterClassExW(&vote_class) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) throw std::runtime_error("Cannot register exit vote");
    RECT vote_bounds{0,0,420,190};
    AdjustWindowRectEx(&vote_bounds,WS_POPUP|WS_CAPTION,FALSE,WS_EX_DLGMODALFRAME|WS_EX_CONTROLPARENT);
    exit_vote_=CreateWindowExW(WS_EX_DLGMODALFRAME|WS_EX_CONTROLPARENT,vote_class.lpszClassName,L"投票退出对局",
        WS_POPUP|WS_CAPTION,0,0,vote_bounds.right-vote_bounds.left,vote_bounds.bottom-vote_bounds.top,parent_,nullptr,wc.hInstance,this);
    const auto vote_control=[&](const wchar_t* type,const wchar_t* text,int id,int x,int y,int w,int h,DWORD style) {
        const auto child=CreateWindowExW(0,type,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,exit_vote_,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),wc.hInstance,nullptr);
        if(!child) throw std::runtime_error("Cannot create exit vote control");
        SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(font_),FALSE); return child;
    };
    if(!exit_vote_ || !exit_font_) throw std::runtime_error("Cannot create exit vote window");
    exit_vote_text_=vote_control(L"STATIC",L"",5162,20,18,380,105,SS_LEFT);
    exit_yes_=vote_control(L"BUTTON",L"同意退出",5163,60,135,130,30,WS_TABSTOP|BS_PUSHBUTTON);
    exit_no_=vote_control(L"BUTTON",L"继续对局",5164,225,135,130,30,WS_TABSTOP|BS_DEFPUSHBUTTON);
    sound_enabled_ = GetPrivateProfileIntW(L"REBUILD", L"NetworkSound", 0, config_.c_str()) != 0;
    animation_enabled_ = GetPrivateProfileIntW(L"REBUILD", L"NetworkAnimation", 1, config_.c_str()) != 0;
    settlement_view_ = std::make_unique<SettlementView>(window_);
    settlement_view_->SetBackgroundPainter([this](HDC dc,RECT r) {
        if (client_ && Snapshot().game.initialized)
            table_.Paint(dc,r,Snapshot().game,false);
    });
    settlement_view_->SetConfirmHandler([this] {
        const auto& a=Snapshot().actions;
        if (!a.pending.empty()) return;
        const auto next=std::find_if(a.choices.begin(),a.choices.end(),[](const OnlineAction& c) { return c.kind=="next"; });
        if (a.enabled && next!=a.choices.end()) Submit(static_cast<std::size_t>(next-a.choices.begin()));
        else if (Snapshot().phase==LobbyPhase::MatchComplete && Snapshot().game.complete) {
            client_->SubmitLobby(Snapshot().lobby_revision,"return");
        } else {
            show_results_=false;
            Update(Snapshot());
        }
    });
    action_view_ = std::make_unique<ActionSelectionView>(window_, [this](std::uint64_t revision, std::size_t index) {
        if (!match_intro_->Active() && !opening_.Active() && !Moving() && !show_results_ && selection_.Active() &&
            revision == selection_.revision && revision == Snapshot().actions.revision) Submit(index);
    }, [this] { CancelSelection(); });
    match_intro_ = std::make_unique<MatchIntroView>(window_, [this] {
        opening_start_ = GetTickCount64();
        if (start_barrier_) client_->ConfirmMatchStart();
        else if (start_sound_pending_) { start_sound_pending_=false; PlaySoundId(112); }
        log_("ONLINE MATCH INTRO DONE"); Update(Snapshot());
    });
    match_intro_->SetSoundPlayer([this](unsigned id) { PlaySoundId(id); });
    settlement_view_->SetSoundPlayer([this](unsigned id) { PlaySoundId(id); });
    settlement_view_->SetSoundStopper([this] {
        try { audio_.Stop(); } catch (const std::exception& e) { log_(std::string("ONLINE AUDIO RESET ERROR ") + e.what()); }
    });
    settlement_view_->SetSoundEnabled(sound_enabled_);
    settlement_view_->SetAnimationEnabled(animation_enabled_);
    matching_ = std::make_unique<MatchmakingView>(window_,log_);
    client_ = std::make_unique<NetworkClient>([this](const LobbySnapshot& snapshot) { Update(snapshot); }, log_,
        [this](const NetworkMessage& message, const LobbySnapshot& snapshot) { OnGameEvent(message, snapshot); });
    matching_->Bind([this](const std::string& command,unsigned value,const std::map<std::string,std::string>& settings) {
        if(command=="match" || command=="cancel") client_->SubmitLobby(client_->snapshot().lobby_revision,command,value);
        else client_->NativeRequest(command,value,settings);
    });
    Update(Snapshot()); Arrange();
}
LobbyView::~LobbyView() {
    StopSound();
    matching_.reset();
    client_.reset();
    match_intro_.reset();
    action_view_.reset();
    settlement_view_.reset();
    if (window_ && IsWindow(window_)) DestroyWindow(window_);
    if(exit_vote_) DestroyWindow(exit_vote_);
    if(exit_font_) DeleteObject(exit_font_);
    if (font_) DeleteObject(font_);
}
void LobbyView::Login(const ConnectionOptions& options) {
    if(client_->active()) client_->Disconnect();
    login_options_=options;
    if(client_->snapshot().reconnect_available && options.recover_games) client_->Reconnect(login_options_);
    else client_->Connect(login_options_);
}
bool LobbyView::LoggedIn() const {
    const auto phase=client_->snapshot().phase;
    return phase==LobbyPhase::Lobby || phase==LobbyPhase::Game || phase==LobbyPhase::Matching ||
        phase==LobbyPhase::Cancelling || phase==LobbyPhase::ChangingLobby || phase==LobbyPhase::MatchComplete || phase==LobbyPhase::AwaitReturn;
}
void LobbyView::Show(bool visible) {
    if (!visible) { hover_point_ = {-1,-1}; ResetActionClock(); CancelResultTimer(); StopSound(); }
    if (!visible) { ClearPresentationQueue(); matching_->Reset(); }
    if (!visible && exit_vote_) ShowWindow(exit_vote_,SW_HIDE);
    if (!visible && client_->active()) client_->Disconnect();
    ShowWindow(window_, visible ? SW_SHOW : SW_HIDE);
    if (visible) PostMessageW(window_,WM_APP+18,0,0);
}
void LobbyView::Disconnect() { if (client_->active() || Snapshot().reconnect_available) client_->Disconnect(); }
void LobbyView::Save() {
    if (!WritePrivateProfileStringW(L"REBUILD", L"NetworkSound", sound_enabled_ ? L"1" : L"0", config_.c_str()) ||
        !WritePrivateProfileStringW(L"REBUILD", L"NetworkAnimation", animation_enabled_ ? L"1" : L"0", config_.c_str()))
        throw std::runtime_error("Cannot save server preferences");
}
void LobbyView::Resize(int width, int height) { MoveWindow(window_, 0, 0, width, height, TRUE); Arrange(); }
void LobbyView::ConfigureArtwork(const std::filesystem::path& directory, int width) {
    CancelHandDrag(); captions_.Clear(); KillTimer(window_,8);
    CancelSelection();
    action_view_->Configure(directory,width);
    match_intro_->Configure(directory, width);
    StopSound(); sounds_ = {}; sound_loaded_ = false; audio_error_.clear();
    settlement_view_->Configure(directory, width);
    action_artwork_.Configure(directory, width); action_artwork_error_.clear();
    resource_directory_ = directory; canvas_width_ = width; lobby_background_ = {};
    table_.Configure(directory, width); artwork_error_.clear(); artwork_painted_ = false; InvalidateRect(window_, nullptr, FALSE);
    PostMessageW(window_,WM_APP+18,0,0);
}
void LobbyView::Arrange() {
    if (!matching_ || !client_) return;
    RECT r{}; GetClientRect(window_,&r);
    const bool game=Snapshot().game.initialized;
    matching_->Move(r); matching_->Show(!game);
    for (const auto& item:controls_) ShowWindow(item.second,SW_HIDE);
    const auto board=BoardBounds();
    settlement_view_->Move(board); action_view_->Move(board);
    if (game && !show_results_) {
        if (Snapshot().phase==LobbyPhase::MatchComplete) {
            MoveWindow(controls_.at(5137),r.right-180,r.bottom-42,160,28,TRUE); ShowWindow(controls_.at(5137),SW_SHOW);
        }
        const auto target=FitNativeCanvas(board,canvas_width_);
        for (const auto& p:BuildActionPlacements(Snapshot().actions,canvas_width_,table_.ActionAnchor(Snapshot().game))) {
            if (SuppressCallPrompt()) continue;
            // Only original action artwork; no inspector, discard or next toolbar.
            if (p.button==5120 || p.button==5129) continue;
            const int x=target.left+p.rect.left*(target.right-target.left)/canvas_width_;
            const int y=target.top+p.rect.top*(target.bottom-target.top)/(canvas_width_*3/4);
            const int w=(p.rect.right-p.rect.left)*(target.right-target.left)/canvas_width_;
            const int h=(p.rect.bottom-p.rect.top)*(target.bottom-target.top)/(canvas_width_*3/4);
            MoveWindow(controls_.at(p.button),x,y,w,h,TRUE); ShowWindow(controls_.at(p.button),SW_SHOW);
        }

    }
    action_view_->Set(selection_,Snapshot().actions,Snapshot().game,
        game && !show_results_ && !SuppressCallPrompt() && !match_intro_->Active() && !opening_.Active() && !Moving());
    // Introduction owns the final sibling visibility until its finish callback.
    match_intro_->Move(r);
    const auto& live=client_->snapshot();
    if(game && live.exit_allowed && live.phase==LobbyPhase::Game && !match_intro_->Active()) {
        const auto target=FitNativeCanvas(board,canvas_width_);
        // Original 0047d7c5: x=10, y=004a6450, preserve measured text size.
        HDC dc=GetDC(window_); const auto old=SelectObject(dc,exit_font_); SIZE size{};
        GetTextExtentPoint32W(dc,L"EXIT",4,&size); SelectObject(dc,old); ReleaseDC(window_,dc);
        const int y=std::min<int>(r.bottom-22,target.bottom-(canvas_width_==1440?57:32));
        MoveWindow(controls_.at(0xbc6),target.left+10,y,size.cx+16,size.cy+8,TRUE);
        EnableWindow(controls_.at(0xbc6),!live.exit_pending && !live.exit_voting && !live.recovering);
        ShowWindow(controls_.at(0xbc6),SW_SHOW); BringWindowToTop(controls_.at(0xbc6));
    }
}
bool LobbyView::Compact() const { return client_ && Snapshot().game.initialized; }
RECT LobbyView::BoardBounds() const { RECT r{}; GetClientRect(window_,&r);
    return r;

}
void LobbyView::SetOriginalDisplay(bool enabled) {
    CancelHandDrag(); captions_.Clear(); KillTimer(window_,8);
    CancelSelection();
    StopSound();
    match_intro_->Cancel();
    (void)enabled; original_display_ = true; table_.SetOriginalDisplay(true);
    Arrange(); InvalidateRect(window_, nullptr, FALSE);
}
void LobbyView::Submit(std::size_t index) {
    if(client_->snapshot().table_paused || client_->snapshot().exit_pending) return;
    if (match_intro_->Active() || opening_.Active() || Moving() || Snapshot().actions.revision!=client_->snapshot().actions.revision) return;
    const auto& a = Snapshot().actions;
    if (!a.enabled || !a.pending.empty() || index >= a.choices.size()) return;
    if (show_results_ && (a.choices[index].kind != "next" || settlement_view_->Animating())) return;
    if (a.choices[index].kind == "riichi") { AnnounceRiichi(); suppress_self_reach_ = true; }
    client_->SubmitAction(action_revision_, index);
}
void LobbyView::PreferenceInput(bool double_click) {
    if (!LocalTableInput() || Moving() || selection_.Active() || pressed_tile_>=0 || pressed_auto_>=0 || hand_dragged_) return;
    preferences_=ClientPreferences::Load(config_); // Original 0047abe8/0047b5fd reads FLAGS0 at the input branch.
    const auto& snapshot=Snapshot();
    const auto index=PreferenceShortcut(preferences_,double_click,snapshot.game,snapshot.actions);
    if (index!=static_cast<std::size_t>(-1)) {
        log_(std::string("ONLINE SHORTCUT ")+(double_click?"double":"right")+" revision="+std::to_string(snapshot.actions.revision));
        Submit(index);
    }
}
void LobbyView::AnnounceRiichi() {
    const auto& s = Snapshot();
    if (riichi_sound_revision_ == s.actions.revision) return;
    riichi_sound_revision_ = s.actions.revision;
    ReplayEvent event; event.kind = ReplayEventKind::Reach; event.attributes["step"] = "1";
    event.who = 0;
    if (captions_.Accept(event)) { SetTimer(window_,8,15,nullptr); InvalidateRect(window_,nullptr,FALSE); }
    for (auto id : ReplaySounds(event,s.game.players[0].gender,effects_only_,individual_voices_)) PlaySoundId(id);
    log_("ONLINE RIICHI ANNOUNCE revision="+std::to_string(riichi_sound_revision_));
}
void LobbyView::CancelResultTimer() {
    KillTimer(window_,5); result_deadline_ = 0;
}
void LobbyView::CancelSelection() {
    if (selection_.group == "riichi") riichi_sound_revision_ = 0;
    selection_.Clear(); action_filter_ = "discard";
    if (client_ && action_view_) { action_view_->Set(selection_,Snapshot().actions,Snapshot().game,false); InvalidateRect(window_,nullptr,FALSE); }
}
bool LobbyView::LocalTableInput() const {
    if (!client_ || text_details_ || show_results_ || match_intro_->Active() || opening_.Active() || start_barrier_) return false;
    const auto& s = Snapshot();
    return s.phase == LobbyPhase::Game && s.game.initialized && s.actions.enabled && !s.game.ended && !s.recovering && !client_->snapshot().table_paused && !client_->snapshot().exit_pending;
}
bool LobbyView::AutomaticInput() const {
    // Original WC_ITEM_CHECK children have their own input path (004a9080),
    // independent of hand actions, animation and the local start barrier.
    if (!client_ || text_details_ || show_results_) return false;
    const auto& s=Snapshot();
    return s.phase==LobbyPhase::Game && s.game.initialized && !s.recovering && !client_->snapshot().table_paused && !client_->snapshot().exit_pending;
}
void LobbyView::CancelHandDrag() {
    pressed_tile_ = pressed_auto_ = -1; hand_dragged_ = false;
    if (GetCapture() == window_) ReleaseCapture();
}
bool LobbyView::HandleKey(UINT key) {
    if (!IsWindowVisible(window_) || !client_) return false;
    if (settlement_view_->HandleKey(key)) return true;
    if (!selection_.Active()) return key == VK_ESCAPE && !Snapshot().actions.pending.empty();
    if (key == VK_ESCAPE) { CancelSelection(); UpdateActions(); Arrange(); SetFocus(window_); return true; }
    return false;
}
void LobbyView::ResetActionClock() {
    clock_active_ = false; clock_deadline_ = 0; clock_warning_second_ = -1; KillTimer(window_, 3);
}
void LobbyView::ActionClockTick() {
    if (!clock_active_ || !clock_deadline_ || !client_) return;
    const auto& s = client_->snapshot();
    if (s.actions.revision!=clock_revision_ || !s.actions.pending.empty() || s.game.ended || s.recovering || s.actions.choices.empty()) { ResetActionClock(); return; }
    const auto now=GetTickCount64();
    const auto remaining=now>=clock_deadline_?0:clock_deadline_-now;
    const int second=static_cast<int>((remaining+999)/1000);
    // FUN_004a9c40: SE_TICK tests total base + reserve <= 2000ms,
    // includes zero, and never sounds for an infinite/finished action.
    if (second<=2 && second!=clock_warning_second_) { clock_warning_second_=second; PlaySoundId(108); }
    if (!remaining) KillTimer(window_,3);
}
void LobbyView::ReceiveActionClock(const NetworkMessage& message, const LobbySnapshot& snapshot) {
    if (message.tag != "CLOCK") return;
    ResetActionClock();
    if (snapshot.phase != LobbyPhase::Game || !snapshot.actions.enabled || snapshot.actions.choices.empty() ||
        snapshot.game.ended || snapshot.recovering) return;
    const auto it = message.attributes.find("ms"); if (it == message.attributes.end()) return;
    int ms = 0; const auto& text = it->second;
    const auto parsed = std::from_chars(text.data(), text.data()+text.size(), ms);
    if (parsed.ec != std::errc() || parsed.ptr != text.data()+text.size() || ms < -1 || ms > 86400000) return;
    clock_active_ = true; clock_revision_ = snapshot.actions.revision;
    clock_base_ms_=-1;
    if(const auto base=message.attributes.find("base_ms");base!=message.attributes.end()) {
        int value=0;const auto read=std::from_chars(base->second.data(),base->second.data()+base->second.size(),value);
        if(read.ec==std::errc() && read.ptr==base->second.data()+base->second.size() && value>=0 && value<=300000) clock_base_ms_=value;
    }
    clock_deadline_ = ms < 0 ? 0 : GetTickCount64() + static_cast<ULONGLONG>(ms);
    clock_base_deadline_=GetTickCount64()+static_cast<ULONGLONG>(std::max(0,clock_base_ms_));
    if (clock_deadline_) SetTimer(window_, 3, 100, nullptr);
}
TableInteraction LobbyView::Interaction(const LobbySnapshot& snapshot) {
    TableInteraction result;
    result.automatic = automatic_;
    result.hovered_button = HitAutomaticButton(hover_point_.x, hover_point_.y);
    if (!LocalTableInput()) return result;
    result.hovered = table_.HitTile(BoardBounds(), snapshot.game, hover_point_.x, hover_point_.y);
    // Local hover/manual order remain available during opponent queue waits.
    // Playable colours and requests still require the presented live revision.
    if (Moving() || snapshot.actions.revision != client_->snapshot().actions.revision) return result;
    const std::string kind = action_filter_ == "riichi" ? "riichi" : "discard";
    for (const auto& choice : snapshot.actions.choices) if (choice.kind == kind && choice.tiles.size() == 1)
        result.playable.push_back(choice.tiles[0]);
    if (selection_.HandSelection()) result.playable = selection_.Selectable(snapshot.actions);
    result.selected = selection_.selected;
    result.selecting = selection_.HandSelection();
    if (clock_active_ && clock_revision_ == snapshot.actions.revision && snapshot.actions.pending.empty()) {
        const auto now = GetTickCount64();
        if(!clock_deadline_) result.countdown=L"\u221e+0";
        else {
            const auto remaining=clock_deadline_>now?clock_deadline_-now:0;
            const auto base=clock_base_ms_<0?remaining:std::min(remaining,clock_base_deadline_>now?clock_base_deadline_-now:0);
            result.countdown=std::to_wstring((base+999)/1000)+L"+"+std::to_wstring((remaining-base+999)/1000);
        }
    }
    return result;
}
int LobbyView::HitAutomaticButton(int x, int y) const {
    if (!Compact() || !AutomaticInput()) return -1;
    const auto target = FitNativeCanvas(BoardBounds(), canvas_width_);
    if (x < target.left || x >= target.right || y < target.top || y >= target.bottom) return -1;
    const POINT point{(x-target.left)*canvas_width_/(target.right-target.left),
        (y-target.top)*(canvas_width_*3/4)/(target.bottom-target.top)};
    for (int i = 0; i < 4; ++i) { const RECT r = OriginalAutoButtonRect(canvas_width_,i); if (PtInRect(&r,point)) return i; }
    return -1;
}
bool LobbyView::SuppressCallPrompt() const {
    const auto& a=Snapshot().actions;
    return (automatic_[1] || automatic_[2]) &&
        std::any_of(a.choices.begin(),a.choices.end(),[](const OnlineAction& c) { return c.kind=="skip"; }) &&
        std::none_of(a.choices.begin(),a.choices.end(),[](const OnlineAction& c) { return c.kind=="ron"; });
}
void LobbyView::AutomaticAction() {
    const auto& s = Snapshot();
    if (!IsWindowVisible(window_) || match_intro_->Active() || opening_.Active() || Moving() || s.phase != LobbyPhase::Game ||
        !s.actions.enabled || !s.actions.pending.empty() || s.game.ended || s.recovering || client_->snapshot().table_paused) return;
    const auto& choices = s.actions.choices;
    for (std::size_t i = 0; i < choices.size(); ++i) if (automatic_[3] && (choices[i].kind == "ron" || choices[i].kind == "tsumo")) { Submit(i); return; }
    const bool ron = std::any_of(choices.begin(), choices.end(), [](const OnlineAction& c) { return c.kind == "ron"; });
    for (std::size_t i = 0; i < choices.size(); ++i) {
        const auto& c = choices[i];
        if ((automatic_[1] || automatic_[2]) && !ron && c.kind == "skip") { Submit(i); return; }
        // Preserve explicit win/kan/riichi decisions; tsumogiri only handles an ordinary draw.
        if ((automatic_[2] || s.game.players[0].reach >= 2) && c.kind == "discard" && c.tiles == std::vector<int>{s.game.players[0].drawn_tile} &&
            std::all_of(choices.begin(),choices.end(),[](const OnlineAction& a) { return a.kind == "discard"; })) {
            if (!automatic_discard_at_ || GetTickCount64()<automatic_discard_at_) return;
            log_("ONLINE AUTO DISCARD tile="+std::to_string(s.game.players[0].drawn_tile)+" tick="+std::to_string(GetTickCount64()));
            Submit(i); return;
        }
    }
}
void LobbyView::UpdateAutomaticTimer() {
    const auto& snapshot = Snapshot();
    const bool needed = snapshot.actions.enabled && !snapshot.actions.choices.empty() && !snapshot.game.ended &&
        (automatic_[1] || automatic_[2] || automatic_[3] || snapshot.game.players[0].reach >= 2);
    if (needed && !automatic_timer_) automatic_timer_ = SetTimer(window_,4,100,nullptr) != 0;
    else if (!needed && automatic_timer_) { KillTimer(window_,4); automatic_timer_ = false; }
}
void LobbyView::ApplyAutomaticSort() {
    if (!sort_refresh_pending_) return;
    
    sort_refresh_pending_ = false;
    table_.SetAutoSort(automatic_[0],Snapshot().game);
    if (animation_enabled_ && table_.Animating()) motion_timer_.Start(window_,6);
    else table_.CancelAnimation();
    UpdateActionAvailability();
    const auto bounds = BoardBounds(); InvalidateRect(window_,&bounds,FALSE);
}
void LobbyView::UpdateActionAvailability() {
    const auto& a = Snapshot().actions;
    const auto group = [](const std::string& kind) { return kind.find("kan") != std::string::npos ? std::string("kan") : kind; };
    const char* kinds[]{"discard", "chi", "pon", "kan", "riichi", "ron", "tsumo", "draw", "skip", "next"};
    const bool ready = !SuppressCallPrompt() && !match_intro_->Active() && !opening_.Active() && !Moving() && !show_results_ && !Snapshot().table_paused;
    for (int i = 1; i <= 8; ++i) EnableWindow(controls_[5120+i],ready &&
        std::any_of(a.choices.begin(),a.choices.end(),[&](const OnlineAction& c) { return group(c.kind)==kinds[i]; }));
    EnableWindow(controls_[5151],ready && !start_barrier_ && std::any_of(a.choices.begin(),a.choices.end(),[](const OnlineAction& c) { return c.kind=="nuki"; }));
}
void LobbyView::UpdateActions() {
    const auto& actions=Snapshot().actions;
    const bool selecting=selection_.Active();
    selection_.Sync(actions); action_revision_=actions.revision;
    if (selecting && !selection_.Active()) action_filter_="discard";
    UpdateActionAvailability();
}
void LobbyView::Command(int id) {
    if(id==0xbc6) { client_->ExitGame(); return; }
    if(id==5163 || id==5164) { client_->ExitGame(id==5163?1:0); UpdateExitVote(); return; }
    if (id==5137) { client_->SubmitLobby(Snapshot().lobby_revision,"return"); return; }
    if(client_->snapshot().table_paused) return;
    if (id == 5150) { CancelSelection(); UpdateActions(); Arrange(); return; }
    if (SuppressCallPrompt() && id>=5121 && id<=5128) return;
    if ((id >= 5120 && id < 5130) || id==5151) {
        if (match_intro_->Active() || opening_.Active() || Moving() || (show_results_ && (id != 5129 || settlement_view_->Animating()))) return;
        const char* kinds[]{"discard", "chi", "pon", "kan", "riichi", "ron", "tsumo", "draw", "skip", "next"};
        const auto& choices = Snapshot().actions.choices;
        const std::string wanted = id==5151?"nuki":kinds[id - 5120];
        if (std::none_of(choices.begin(), choices.end(), [&](const OnlineAction& c) {
            return c.kind == wanted || (wanted == "kan" && c.kind.find("kan") != std::string::npos);
        })) return;
        if (id == 5124) AnnounceRiichi();
        action_filter_ = wanted; UpdateActions();
        if (Compact() && id == 5123) {
            // Kan has no hand-pair selection: prefer the drawn tile's kan.
            const int drawn = Snapshot().game.players[0].drawn_tile;
            auto chosen = choices.end();
            for (auto it = choices.begin(); it != choices.end(); ++it) if (ActionGroup(it->kind) == "kan") {
                if (chosen == choices.end()) chosen = it;
                if (std::find(it->tiles.begin(), it->tiles.end(), drawn) != it->tiles.end()) { chosen = it; break; }
            }
            if (chosen != choices.end()) Submit(static_cast<std::size_t>(chosen-choices.begin()));
            return;
        }
        if (id == 5121 || id == 5122) {
            const auto& snapshot = Snapshot();
            selection_.Open(snapshot.actions,wanted);
            const auto unique = selection_.UniqueCall(snapshot.actions,(snapshot.game.type & 2) == 0);
            if (unique != static_cast<std::size_t>(-1)) { Submit(unique); return; }
        }
        if (Compact() && (id == 5121 || id == 5122 || id == 5124)) {
            log_("ACTION SELECT group="+wanted+" revision="+std::to_string(action_revision_));
            if (id == 5124) selection_.Open(Snapshot().actions,wanted);
            Arrange(); SetFocus(window_);
            InvalidateRect(window_,nullptr,FALSE); return;
        }
        CancelSelection(); action_filter_ = wanted;
        if (id >= 5125) {
            for (std::size_t i=0;i<choices.size();++i) if (choices[i].kind==wanted) { Submit(i); break; }
        }
        return;
    }

}
const LobbySnapshot& LobbyView::Snapshot() const {
    return displaying_ ? displayed_ : client_->snapshot();
}
void LobbyView::ClearPresentationQueue() {
    CancelHandDrag(); captions_.Clear(); KillTimer(window_,8);
    presentation_queue_.clear(); pause_ticks_=draw_end_ticks_=0; displaying_=false;
    automatic_discard_at_=0; KillTimer(window_,10);
    table_.SetDrawEndSlots(14); KillTimer(window_,7);
}
void LobbyView::Update(const LobbySnapshot& incoming) {
    
    const bool paused=client_->snapshot().table_paused;
    if(paused && !paused_at_) { paused_at_=GetTickCount64(); CancelHandDrag(); CancelSelection(); }
    else if(!paused && paused_at_) {
        const auto now=GetTickCount64(), delay=now-paused_at_;
        const auto remaining=[&](ULONGLONG deadline) { return static_cast<UINT>(deadline>now?deadline-now:1); };
        if(result_deadline_) result_deadline_+=delay;
        if(automatic_discard_at_) automatic_discard_at_+=delay;
        if(result_deadline_) SetTimer(window_,5,remaining(result_deadline_),nullptr);
        if(automatic_discard_at_) SetTimer(window_,10,remaining(automatic_discard_at_),nullptr);
        paused_at_=0;
    }
    table_.SetPaused(paused); settlement_view_->SetPaused(paused);
    match_intro_->SetPaused(paused);
    UpdateExitVote();
    if (!incoming.game.initialized && displaying_) {
        ClearPresentationQueue(); CancelSelection(); CancelResultTimer(); show_results_=false;
    }
    if(displaying_) displayed_.table_paused=client_->snapshot().table_paused;
    if(matching_) matching_->SetSnapshot(incoming);
    if (incoming.phase==LobbyPhase::Disconnected || incoming.phase==LobbyPhase::Error || incoming.recovering) ClearPresentationQueue();
    if (displaying_ && presentation_queue_.empty() && incoming.game.events==displayed_.game.events) displayed_=incoming;
    const auto& snapshot=displaying_ ? displayed_ : incoming;
    if ((pressed_auto_ >= 0 ? !AutomaticInput() : !LocalTableInput()) || (pressed_tile_ >= 0 && std::find(snapshot.game.players[0].known_hand.begin(),
        snapshot.game.players[0].known_hand.end(),pressed_tile_) == snapshot.game.players[0].known_hand.end())) CancelHandDrag();
    if (snapshot.actions.revision != riichi_sound_revision_) riichi_sound_revision_ = 0;
    if (snapshot.phase != LobbyPhase::Game || snapshot.recovering || snapshot.game.ended || show_results_) CancelSelection();
    const auto& live=client_->snapshot();
    if (clock_active_ && (live.phase != LobbyPhase::Game || live.recovering || live.table_paused ||
        live.actions.revision != clock_revision_ || !live.actions.pending.empty() || live.actions.choices.empty())) ResetActionClock();
    UpdateAutomaticTimer();
    if (snapshot.phase == LobbyPhase::Disconnected || snapshot.phase == LobbyPhase::Error || snapshot.recovering) {
        CancelResultTimer(); suppress_self_reach_ = false; StopSound(snapshot.recovering && match_intro_->Resuming());
    }
    if (snapshot.phase == LobbyPhase::Disconnected) { match_dan_.clear(); match_intro_pending_ = true; resume_preparation_pending_ = false; }
    if (snapshot.actions.revision != sound_action_revision_) {
        sound_action_revision_ = snapshot.actions.revision;
        if (snapshot.phase == LobbyPhase::Game && snapshot.actions.enabled && !SuppressCallPrompt() &&
            std::any_of(snapshot.actions.choices.begin(), snapshot.actions.choices.end(), [](const OnlineAction& c) {
                return c.kind == "chi" || c.kind == "pon" || c.kind.find("kan") != std::string::npos ||
                    c.kind == "riichi" || c.kind == "tsumo" || c.kind == "ron" || c.kind == "nuki";
            }) && event_sound_allowed_) PlaySoundId(106);
    }
    const auto& settlement = snapshot.game.settlement.receipts.empty() ? snapshot.last_settlement : snapshot.game.settlement;
    const bool result_available = !settlement.receipts.empty();
    if (!result_available) { CancelResultTimer(); show_results_ = false; }
    std::array<std::string, 4> result_names;
    for (int i = 0; i < 4; ++i) result_names[i] = snapshot.game.settlement.receipts.empty() ? snapshot.last_result_names[i] : snapshot.game.players[i].name;
    const bool history=snapshot.game.settlement.receipts.empty();
    settlement_view_->SetContinuation(!history && snapshot.actions.enabled && snapshot.game.ended && !snapshot.game.complete);
    settlement_view_->SetContext(history?snapshot.last_result_dealer:snapshot.game.dealer,
        history?snapshot.last_result_honba:snapshot.game.honba,history?snapshot.last_result_sticks:snapshot.game.sticks);
    settlement_view_->Set(settlement, result_names, snapshot.game.settlement.receipts.empty() ? snapshot.last_result_red : (snapshot.game.type & 2) == 0);
    settlement_view_->SetConfirmEnabled(!paused && !client_->snapshot().exit_pending && snapshot.actions.pending.empty() &&
        (!snapshot.game.complete || snapshot.phase==LobbyPhase::MatchComplete));
    settlement_view_->Show(show_results_ && result_available);
    if (result_pending_ && show_results_ && result_available) settlement_view_->Present();
    result_pending_ = false;
    UpdateActions(); Arrange();
    if (incoming.recovering && (!incoming.native_service || resume_preparation_pending_) &&
        incoming.game.started && original_display_ && !text_details_ && IsWindowVisible(window_)) {
        const bool beginning = !match_intro_->Resuming();
        match_intro_->Resume(OnlineTableState(incoming.game), incoming.game.type, match_dan_);
        if (beginning) log_("ONLINE RESUME PREPARATION BEGIN");
    }
    InvalidateRect(window_, nullptr, FALSE);
    UpdateLobbyMusic();
    if(connection_changed_) connection_changed_(incoming);
}
void LobbyView::StopSound(bool preserve_resume) {
    StopLobbyMusic();
    motion_timer_.Stop();
    if (match_intro_ && !preserve_resume) {
        if (match_intro_->Resuming()) log_("ONLINE RESUME PREPARATION END");
        match_intro_->Cancel();
    }
    opening_.Cancel(); deferred_sounds_.clear();
    table_.CancelAnimation();
    result_pending_ = false;
    if (settlement_view_) settlement_view_->CancelPresentation();
    KillTimer(window_, 2);
    StopAudio();
}
void LobbyView::StopAudio() {
    deferred_sounds_.clear();
    try { audio_.Stop(); } catch (const std::exception& e) { log_(std::string("ONLINE AUDIO RESET ERROR ") + e.what()); }
}
void LobbyView::StopLobbyMusic() {
    if (lobby_music_key_.empty()) return;
    try { lobby_music_.Stop(); } catch (const std::exception& e) { log_(std::string("LOBBY BGM STOP ERROR ")+e.what()); }
    lobby_music_key_.clear(); log_("LOBBY BGM STOP");
}
void LobbyView::UpdateLobbyMusic() {
    if(music_preview_) { StopLobbyMusic(); return; }
    if (!client_) return;
    const auto& s=client_->snapshot();
    const bool lobby=s.phase==LobbyPhase::Lobby || s.phase==LobbyPhase::Matching ||
        s.phase==LobbyPhase::Cancelling || s.phase==LobbyPhase::ChangingLobby;
    const auto& game=displaying_?displayed_.game:s.game;
    const bool playing=(s.phase==LobbyPhase::Game || s.phase==LobbyPhase::MatchComplete) && game.initialized && !s.recovering;
    if (!IsWindowVisible(window_) || (!lobby && !playing) || resource_directory_.empty()) { StopLobbyMusic(); return; }
    const bool custom=!s.lobby.empty() && s.lobby.front()=='C';
    std::wstring key=custom?L"BGM_LOBBY1":L"BGM_LOBBY0";
    if(playing) {
        const int last=static_cast<int>(((game.type&8u)|6u)>>1)-((game.type&16u)?1:0);
        int index=game.round<last?(game.round<4?0:1):game.round==last?2:3;
        const auto reaches=std::count_if(game.players.begin(),game.players.end(),[](const OnlinePlayer& p){return p.reach!=0;});
        if(reaches) index=reaches==1?4:5;
        if(settlement_view_->FinalPage()) index=9;
        key=L"BGM_GAME"+std::to_wstring(index);
    }
    const auto option=MusicOption(key);
    const auto source=PreferenceString(config_,L"RESOURCE",key.c_str(),L"default");
    const auto identity=NetworkUtf8(key)+":"+NetworkUtf8(source);
    if (lobby_music_key_==identity) return;
    StopLobbyMusic(); lobby_music_key_=identity;
    if (_wcsicmp(source.c_str(),L"none")==0) return;
    try {
        const auto path=MusicPath(resource_directory_,config_,key); if(path.empty()) return;
        const int master=std::clamp(static_cast<int>(GetPrivateProfileIntW(L"RESOURCE",L"BGM_VOL",100,config_.c_str())),0,200);
        const int level=std::clamp(static_cast<int>(GetPrivateProfileIntW(L"RESOURCE",option.c_str(),60,config_.c_str())),0,200);
        lobby_music_.SetVolumePercent(master);
        const auto clip=std::make_shared<SoundClip>(DecodeMusic(ReadFile(path)));
        lobby_music_.Play(0,clip,true,level*.01);
        log_("LOBBY BGM START file="+path.filename().string()+" master="+std::to_string(master)+" level="+std::to_string(level)+" loop=1");
    } catch (const std::exception& e) { log_(std::string("LOBBY BGM ERROR ")+e.what()); }
}
void LobbyView::PlaySoundId(unsigned id, unsigned delay) {
    if (!sound_enabled_ || !audio_error_.empty() || !IsWindowVisible(window_)) return;
    if (opening_.Active() && id != 104 && id != 105 && id != 102 && id != 112) {
        if (deferred_sounds_.size() < 256) deferred_sounds_.push_back(id);
        else { audio_error_ = "Online sound queue capacity exceeded"; log_("ONLINE AUDIO ERROR " + audio_error_); StopAudio(); }
        return;
    }
    try {
        if (!sound_loaded_) { sounds_.Load(resource_directory_, canvas_width_, config_); sound_loaded_ = true; log_("ONLINE AUDIO BANK loaded"); }
        audio_.Schedule(id, sounds_.Get(id), delay);
        if (!SetTimer(window_, 2, 25, nullptr)) throw std::runtime_error("Cannot start online sound timer");
    } catch (const std::exception& e) {
        audio_error_ = e.what(); log_("ONLINE AUDIO ERROR " + audio_error_); StopAudio();
    }
}
void LobbyView::OnGameEvent(const NetworkMessage& message, const LobbySnapshot& snapshot) {
    // Native RESTORE also repairs a stale action within a running connection.
    // Only a login/reconnect opens the original recovery preparation window.
    if (message.tag=="RESTORE") resume_preparation_pending_=match_intro_pending_;
    if(message.tag=="EXITVOTE" && snapshot.exit_voting) exit_vote_deadline_=GetTickCount64()+snapshot.exit_vote_ms;
    if(message.tag=="TABLEEND") { ClearPresentationQueue(); CancelSelection(); CancelResultTimer(); show_results_=false; StopSound(); start_barrier_=false; match_intro_pending_=true; return; }
    if(message.tag=="EXITVOTE" || message.tag=="PAUSED") return;
    if (message.tag=="INIT" || message.tag=="REINIT") {
        // ASM 00493133: INIT and REINIT share the preference initialization.
        preferences_=ClientPreferences::Load(config_);
        // User-authorized per-hand defaults. Original INIT clears fa2/fa3
        // and reloads fa4 from FLAGS0; fa1 originally persists after creation.
        automatic_={true,false,false,false};
        sort_refresh_pending_=false;
        table_.SetAutoSort(true,snapshot.game);
        table_.CancelAnimation();
        log_("ONLINE AUTO RESET sort=1 no_call=0 tsumogiri=0 win=0");
    }
    ReceiveActionClock(message,snapshot);
    const auto& tag=message.tag;
    const bool reset=tag=="GO" || tag=="INIT" || tag=="REINIT";
    if (reset || snapshot.recovering || !IsWindowVisible(window_)) ClearPresentationQueue();
    if (tag=="INIT" && IsWindowVisible(window_)) { displayed_=snapshot; displaying_=true; }
    const bool tile=!tag.empty() && std::string("TUVWtuvwDEFGdefg").find(tag[0])!=std::string::npos &&
        std::all_of(tag.begin()+1,tag.end(),[](char c){return c>='0' && c<='9';});
    const bool event=tile || tag=="N" || tag=="DORA" || tag=="REACH" || tag=="FURITEN" || tag=="AGARI" || tag=="RYUUKYOKU" || tag=="PROF";
    if (snapshot.game.initialized && !snapshot.recovering && IsWindowVisible(window_) && (event || !presentation_queue_.empty())) {
        // Immutable public snapshots keep packet bursts from replacing a visible
        // discard/draw. Receive/validation and the server clock keep running.
        presentation_queue_.push_back({message,snapshot,sound_enabled_ && audio_error_.empty()});
        SetTimer(window_,7,15,nullptr);
        if (!opening_.Active() && !match_intro_->Active() && !pause_ticks_ && !draw_end_ticks_ && !table_.Animating()) PresentationTick();
        return;
    }
    if (displaying_) displayed_=snapshot;
    event_sound_allowed_=true;
    PresentGameEvent(message,snapshot);
}
void LobbyView::PresentationTick() {
    if(client_->snapshot().table_paused) return;
    if (opening_.Active() || match_intro_->Active() || start_barrier_) return;
    if (pause_after_motion_ && table_.Animating()) return;
    if (pause_ticks_) { --pause_ticks_; return; }
    if (table_.Animating()) return;
    if (draw_end_ticks_) {
        const unsigned before=14-draw_end_ticks_/2;
        --draw_end_ticks_;
        const unsigned slots=14-draw_end_ticks_/2;
        table_.SetDrawEndSlots(slots);
        if (slots!=before) {
            const unsigned seats=DrawEndChangedSeats(OnlineTableState(Snapshot().game),slots-1);
            if (seats && event_sound_allowed_) PlaySoundId(104);
            log_("ONLINE DRAW SLOT index="+std::to_string(slots-1)+" seats="+std::to_string(seats)+" tick="+std::to_string(GetTickCount64()));
        }
        InvalidateRect(window_,nullptr,FALSE);
        if (draw_end_ticks_) return;
        log_("ONLINE DRAW TABLE DONE tick="+std::to_string(GetTickCount64()));
        // FUN_004aad30 posts WM_TIMER/100 immediately for RYUUKYOKU.
        result_deadline_=GetTickCount64(); SetTimer(window_,5,1,nullptr);
        return;
    }
    if (presentation_queue_.empty()) { KillTimer(window_,7); Update(client_->snapshot()); return; }
    auto item=std::move(presentation_queue_.front()); presentation_queue_.pop_front();
    displayed_=std::move(item.snapshot); displaying_=true;
    event_sound_allowed_=item.sound_allowed;
    log_("ONLINE PRESENT tag="+item.message.tag+" tick="+std::to_string(GetTickCount64()));
    PresentGameEvent(item.message,displayed_);
    Update(displayed_);
}
void LobbyView::PresentGameEvent(const NetworkMessage& message, const LobbySnapshot& snapshot) {
    if(message.tag=="RESTORED") {
        StopSound(); match_intro_pending_=false; sound_action_revision_=snapshot.actions.revision;
        resume_preparation_pending_=false;
        show_results_=snapshot.game.ended;
        table_.SetPreferences(preferences_,config_,false);
        table_.Accept(snapshot.game,ReplayEvent{},false); return;
    }
    if (message.tag == "GO") { match_intro_->Cancel(); match_intro_pending_ = true; start_barrier_=false; start_sound_pending_=false; }
    if (message.tag == "PREPARE") { start_barrier_=true; return; }
    if (message.tag == "START") {
        start_barrier_=false; opening_start_=GetTickCount64();
        if (start_sound_pending_) { start_sound_pending_=false; PlaySoundId(112); }
        log_("ONLINE ALL READY START"); UpdateActions(); Arrange(); return;
    }
    if (message.tag == "UN" && message.attributes.count("dan")) match_dan_ = message.attributes.at("dan");
    if (message.tag == "AGARI" || message.tag == "RYUUKYOKU" || message.tag == "PROF") match_intro_->Cancel();
    if (message.tag == "REINIT") {
        CancelResultTimer(); suppress_self_reach_ = false;
        match_intro_pending_ = false;
        sound_action_revision_ = snapshot.actions.revision;
        table_.Accept(snapshot.game,ReplayEvent{},false);
        StopSound(); return; // Restored choices do not replay the availability hint.
    }
    if (!snapshot.game.initialized || snapshot.recovering || !IsWindowVisible(window_)) return;
    ReplayEvent event; event.tag = message.tag; event.attributes = message.attributes;
    if (event.tag == "INIT") { CancelResultTimer(); suppress_self_reach_ = false; StopSound(); event.kind = ReplayEventKind::Init; }
    else if (event.tag == "N") event.kind = ReplayEventKind::Meld;
    else if (event.tag == "REACH") event.kind = ReplayEventKind::Reach;
    else if (event.tag == "AGARI") event.kind = ReplayEventKind::Win;
    else if (event.tag == "RYUUKYOKU") event.kind = ReplayEventKind::DrawEnd;
    else if (!event.tag.empty() && std::string("TUVWtuvwDEFGdefg").find(event.tag[0]) != std::string::npos &&
        std::all_of(event.tag.begin() + 1, event.tag.end(), [](char c) { return c >= '0' && c <= '9'; })) {
        const auto found = std::string("TUVWtuvwDEFGdefg").find(event.tag[0]);
        event.kind = found < 8 ? ReplayEventKind::Draw : ReplayEventKind::Discard;
        event.who = static_cast<int>(found % 4);
    } else return;
    const bool result = event.kind == ReplayEventKind::Win || event.kind == ReplayEventKind::DrawEnd;
    if (event.attributes.count("who")) event.who = std::stoi(event.attributes.at("who"));
    // Keep captions independent of audio and tile motion cancellation.
    if (!(event.kind == ReplayEventKind::Reach && event.who == 0 && suppress_self_reach_) && captions_.Accept(event)) {
        SetTimer(window_,8,15,nullptr);
        log_("ONLINE CAPTION id="+std::to_string(ActionCaptionResource(event))+" who="+std::to_string(event.who));
    }
    pause_ticks_=ActionPauseTicks(event);
    pause_after_motion_=event.kind==ReplayEventKind::Draw;
    const bool replaced = table_.Animating();
    try {
        table_.Accept(snapshot.game,event,animation_enabled_ && !text_details_ && !show_results_ && !opening_.Active());
        if (table_.Animating()) {
            log_("ONLINE ANIMATION BEGIN tag="+event.tag+" replaced="+std::to_string(replaced));
            motion_timer_.Start(window_,6);
        }
        else motion_timer_.Stop();
    } catch (const std::exception& e) { log_(std::string("ONLINE ANIMATION FALLBACK ")+e.what()); table_.CancelAnimation(); motion_timer_.Stop(); }
    if (event.kind==ReplayEventKind::Draw && event.who==0) {
        // FUN_00491fe0 -> WC_ITEM_COUNT, auto flag at +0x25. The native
        // first timer beat is 1000ms from draw dispatch, including movement.
        automatic_discard_at_=GetTickCount64()+kAutomaticDiscardWaitMs;
        if (!SetTimer(window_,10,kAutomaticDiscardWaitMs,nullptr)) throw std::runtime_error("Cannot start automatic discard wait");
        log_("ONLINE AUTO DRAW WAIT until="+std::to_string(automatic_discard_at_));
    } else if (event.kind==ReplayEventKind::Init || event.kind==ReplayEventKind::DrawEnd ||
        event.kind==ReplayEventKind::Win || (event.kind==ReplayEventKind::Discard && event.who==0)) {
        automatic_discard_at_=0; KillTimer(window_,10);
    }
    result_pending_ = result;
    if (result) {
        if (opening_.Active() || snapshot.game.settlement.receipts.size() == 1) StopAudio();
        opening_.Cancel();
        deferred_sounds_.clear();
        settlement_view_->CancelPresentation();
        if (event.kind==ReplayEventKind::DrawEnd && !show_results_) {
            draw_end_ticks_=28; table_.SetDrawEndSlots(0);
            SetTimer(window_,7,15,nullptr);
            log_("ONLINE DRAW TABLE BEGIN tick="+std::to_string(GetTickCount64()));
        }
        if (event.kind == ReplayEventKind::Win && !show_results_ && !result_deadline_) {
            // FUN_004aad30 / ASM 004aaf28: AGARI waits 0x7d0 ms.
            // This display timer is independent of audio and the action clock.
            constexpr unsigned delay = 2000;
            result_deadline_ = GetTickCount64()+delay;
            if (!SetTimer(window_,5,delay,nullptr)) throw std::runtime_error("Cannot start settlement delay");
            log_("ONLINE RESULT WAIT ms="+std::to_string(delay)+" tick="+std::to_string(GetTickCount64()));
        }
    }
    const bool intro = event.kind==ReplayEventKind::Init && match_intro_pending_ && original_display_ && !text_details_ && snapshot.actions.enabled;
    if (event.kind==ReplayEventKind::Init && (intro || start_barrier_)) start_sound_pending_=!ReplaySounds(event).empty();
    else if (event_sound_allowed_) PlayEventSound(event,snapshot);
    if (event.kind == ReplayEventKind::Init && (animation_enabled_ || sound_enabled_)) {
        opening_start_ = GetTickCount64();
        table_.BeginOpening(opening_,snapshot.game,original_display_ && !text_details_);
        if (!intro && !start_barrier_ && opening_.Tick() && opening_.sound()) PlaySoundId(opening_.sound());
        log_("ONLINE OPENING BEGIN wall=" + std::to_string(opening_.wall_tiles()));
        SetTimer(window_,2,opening_.Active()?opening_.interval():(draw_end_ticks_?15:25),nullptr);
    }
    if (event.kind == ReplayEventKind::Init && match_intro_pending_) {
        match_intro_pending_ = false;
        if (original_display_ && !text_details_ && snapshot.actions.enabled) {
            RECT area{}; GetClientRect(window_, &area);
            match_intro_->Move(area); match_intro_->Begin(OnlineTableState(snapshot.game), snapshot.game.type, match_dan_);
            // The confirmation countdown is idle UI time: front-load the table
            // pack read here instead of the first visible table frame.
            table_.Prepare();
            UpdateActions(); log_("ONLINE MATCH INTRO BEGIN");
        }
    }
    // Native PREPARE is per hand; the match introduction is only per match.
    // Once INIT has been installed, a hand without an introduction is ready
    // immediately. Keep the opening frozen until the authority sends START.
    if (event.kind==ReplayEventKind::Init && start_barrier_ && !match_intro_->Active() && !snapshot.recovering)
        client_->ConfirmMatchStart();
}
void LobbyView::PlayEventSound(const ReplayEvent& event, const LobbySnapshot& snapshot) {
    const char gender = event.who >= 0 && event.who < 4 ? snapshot.game.players[event.who].gender : '?';
    const bool own_reach = event.kind == ReplayEventKind::Reach && event.who == 0 && event.attributes.at("step") == "1";
    if (!(own_reach && suppress_self_reach_))
        for (auto id : ReplaySounds(event,gender,effects_only_,individual_voices_)) PlaySoundId(id);
    if (own_reach) suppress_self_reach_ = false;
}
void LobbyView::Paint(HDC dc, bool table_only) {
    
    RECT r{}; GetClientRect(window_, &r);
    if (!table_only) {
    FillRect(dc, &r, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
    if (original_display_) {
        try {
            if (lobby_background_.bgra.empty()) lobby_background_ = BuildPreferredBackground(resource_directory_, config_, canvas_width_, canvas_width_*3/4, preferences_);
            Gdiplus::Graphics graphics(dc);
            auto& bg = lobby_background_;
            Gdiplus::Bitmap bitmap(bg.width, bg.height, bg.width*4, PixelFormat32bppARGB, bg.bgra.data());
            graphics.DrawImage(&bitmap, 0, 0, r.right, r.bottom);
        } catch (const std::exception&) { /* Table/connection status retains the actionable resource error. */ }
    }
    }
    const auto old = SelectObject(dc, font_); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, original_display_ ? RGB(245, 245, 235) : RGB(25, 35, 45));
    if (client_ && Snapshot().game.initialized && !text_details_ && !show_results_) {
        auto bounds = BoardBounds();
        try {
            const auto interaction = Interaction(Snapshot());
            table_.Paint(dc, bounds, Snapshot().game, Snapshot().actions.enabled,
                animation_enabled_ ? &opening_ : nullptr, &interaction); artwork_error_.clear();
            if (original_display_) captions_.Paint(dc,FitNativeCanvas(bounds,canvas_width_),canvas_width_,action_artwork_);
            painted_countdown_ = interaction.countdown;
            painted_hover_ = interaction.hovered; painted_auto_hover_ = interaction.hovered_button;
            if (!artwork_painted_) { log_("ONLINE ARTWORK rendered"); artwork_painted_ = true; }
        }
        catch (const std::exception& error) {
            artwork_painted_ = false;
            if (artwork_error_ != error.what()) log_("ONLINE ARTWORK ERROR " + std::string(error.what()));
            artwork_error_ = error.what();
            const auto text = NetworkWide("Table artwork failed: " + artwork_error_);
            SetTextColor(dc, RGB(150, 35, 25)); DrawTextW(dc, text.c_str(), -1, &bounds, DT_LEFT | DT_WORDBREAK);
        }
    }
    SelectObject(dc, old);
    if(Snapshot().table_paused) {
        const auto previous=SelectObject(dc,font_); SetTextColor(dc,RGB(255,220,120)); SetBkMode(dc,TRANSPARENT);
        RECT message{20,20,r.right-20,65}; DrawTextW(dc,client_->snapshot().exit_voting?L"对局已暂停，等待退出投票":L"对局已暂停，等待断线玩家重新登录",-1,&message,DT_CENTER|DT_WORDBREAK); SelectObject(dc,previous);
    }
}
void LobbyView::PaintAction(const DRAWITEMSTRUCT& item) {
    bool rendered = false;
    if (original_display_) {
        try {
            constexpr unsigned ids[]{443, 441, 442, 446, 444, 445, 447, 449};
            POINT point{}; GetCursorPos(&point); ScreenToClient(item.hwndItem,&point);
            const bool hot = IsWindowEnabled(item.hwndItem) && PtInRect(&item.rcItem,point);
            const unsigned id = (item.CtlID==5151?448:ids[item.CtlID - 5121]) + (hot ? 9 : 0);
            Gdiplus::Graphics graphics(item.hDC);
            const auto paint = [&](unsigned resource) {
                const auto& im = action_artwork_.Get(resource);
                Gdiplus::Bitmap bitmap(im.width, im.height, im.width*4, PixelFormat32bppARGB, const_cast<BYTE*>(im.bgra.data()));
                const int offset = item.itemState & ODS_SELECTED ? 1 : 0;
                if (graphics.DrawImage(&bitmap, item.rcItem.left + offset, item.rcItem.top + offset,
                    item.rcItem.right - item.rcItem.left, item.rcItem.bottom - item.rcItem.top) != Gdiplus::Ok)
                    throw std::runtime_error("Cannot draw native action button");
            };
            FillRect(item.hDC, &item.rcItem, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            paint(440); paint(id);
            if (item.itemState & ODS_DISABLED) { Gdiplus::SolidBrush dim(Gdiplus::Color(150,0,0,0)); graphics.FillRectangle(&dim,0,0,item.rcItem.right,item.rcItem.bottom); }
            rendered = true; action_artwork_error_.clear();
        } catch (const std::exception& e) {
            if (action_artwork_error_ != e.what()) log_(std::string("ACTION ARTWORK ERROR ") + e.what());
            action_artwork_error_ = e.what();
        }
    }
    if (!rendered) {
        RECT bounds = item.rcItem;
        DrawFrameControl(item.hDC, &bounds, DFC_BUTTON, DFCS_BUTTONPUSH | ((item.itemState & ODS_SELECTED) ? DFCS_PUSHED : 0));
        const auto font = SelectObject(item.hDC, font_); SetBkMode(item.hDC, TRANSPARENT);
        SetTextColor(item.hDC, GetSysColor((item.itemState & ODS_DISABLED) ? COLOR_GRAYTEXT : COLOR_BTNTEXT));
        const auto text = Text(item.hwndItem);
        DrawTextW(item.hDC, text.c_str(), -1, &bounds, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(item.hDC, font);
    }
    if (item.itemState & ODS_FOCUS) { RECT r = item.rcItem; InflateRect(&r, -3, -3); DrawFocusRect(item.hDC, &r); }
}
void LobbyView::UpdateExitVote() {
    if(!client_ || !exit_vote_) return;
    const auto& s=client_->snapshot();
    if(!s.exit_voting || s.recovering || !IsWindowVisible(window_)) {
        ShowWindow(exit_vote_,SW_HIDE); KillTimer(window_,11); return;
    }
    auto now=GetTickCount64(); auto seconds=exit_vote_deadline_>now?(exit_vote_deadline_-now+999)/1000:0;
    std::wstring text=L"是否终止本场对局？对局和出牌计时已暂停。\r\n剩余 "+std::to_wstring(seconds)+L" 秒（30 秒）\r\n同意 "+
        std::to_wstring(s.exit_yes)+L" / 反对 "+std::to_wstring(s.exit_no)+L" / 真人 "+std::to_wstring(s.exit_total)+
        L"；需超过半数同意。\r\n"+(s.exit_choice<0?L"请投票；未投票不计为同意。":s.exit_choice?L"你已同意退出，等待其他玩家。":L"你已选择继续，等待其他玩家。");
    SetWindowTextW(exit_vote_text_,text.c_str());
    EnableWindow(exit_yes_,s.exit_choice<0 && !s.exit_ballot_pending && seconds>0);
    EnableWindow(exit_no_,s.exit_choice<0 && !s.exit_ballot_pending && seconds>0);
    if(!IsWindowVisible(exit_vote_)) {
        RECT r{},popup{}; GetWindowRect(parent_,&r); GetWindowRect(exit_vote_,&popup);
        SetWindowPos(exit_vote_,HWND_TOP,r.left+(r.right-r.left-(popup.right-popup.left))/2,r.top+(r.bottom-r.top-(popup.bottom-popup.top))/2,0,0,SWP_NOSIZE|SWP_SHOWWINDOW);
        SetFocus(exit_no_);
    }
    SetTimer(window_,11,100,nullptr);
}
LRESULT CALLBACK LobbyView::ExitVoteProcedure(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    auto* self=reinterpret_cast<LobbyView*>(GetWindowLongPtrW(window,GWLP_USERDATA));
    if(message==WM_NCCREATE) { self=static_cast<LobbyView*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams); SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self)); }
    if(self && message==WM_COMMAND) { PostMessageW(self->window_,WM_COMMAND,wp,lp); return 0; }
    if(message==WM_CLOSE) return 0; // The server deadline resolves an abstention.
    return DefWindowProcW(window,message,wp,lp);
}
void LobbyView::PaintExit(const DRAWITEMSTRUCT& item) {
    // WC_ITEM_BTN (004a7920): white upper half, gray lower gradient,
    // disabled gray, text extent +16/+8. Font/rasterization is a rebuild adapter.
    const auto r=item.rcItem; const bool enabled=IsWindowEnabled(item.hwndItem)!=FALSE;
    const int width=r.right-r.left,height=r.bottom-r.top;
    for(int y=0;y<height;++y) {
        int value=enabled?(y<height/2?255:221+34*(y-height/2)/std::max(1,height-height/2-1)):204;
        RECT line{0,y,width,y+1}; const auto brush=CreateSolidBrush(RGB(value,value,value)); FillRect(item.hDC,&line,brush); DeleteObject(brush);
    }
    RECT text=r; if(item.itemState&ODS_SELECTED) OffsetRect(&text,1,1);
    const auto old=SelectObject(item.hDC,exit_font_); SetBkMode(item.hDC,TRANSPARENT); SetTextColor(item.hDC,enabled?RGB(34,34,34):RGB(102,102,102));
    DrawTextW(item.hDC,L"EXIT",4,&text,DT_CENTER|DT_VCENTER|DT_SINGLELINE); SelectObject(item.hDC,old);
    POINT point{}; GetCursorPos(&point); ScreenToClient(item.hwndItem,&point);
    const auto border=CreateSolidBrush(enabled && PtInRect(&r,point)?RGB(51,153,255):RGB(136,136,136)); FrameRect(item.hDC,&r,border); DeleteObject(border);
    if(item.itemState&ODS_FOCUS) { InflateRect(&text,-2,-2); DrawFocusRect(item.hDC,&text); }
}
void LobbyView::Capture(HDC dc) {
    Paint(dc); matching_->Capture(dc); settlement_view_->Capture(dc); POINT origin{}; ClientToScreen(window_, &origin);
    for (const auto& item : controls_) {
        if (!IsWindowVisible(item.second)) continue;
        RECT r{}; GetWindowRect(item.second, &r); const int saved = SaveDC(dc);
        SetViewportOrgEx(dc, r.left - origin.x, r.top - origin.y, nullptr);
        SendMessageW(item.second, WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT | PRF_NONCLIENT | PRF_ERASEBKGND);
        RestoreDC(dc, saved);
    }
    match_intro_->Capture(dc);
    action_view_->Capture(dc);
}
LRESULT CALLBACK LobbyView::Procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* self = reinterpret_cast<LobbyView*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        self = static_cast<LobbyView*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        self->window_ = window; SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (self) {
        try {
            if (message == WM_ERASEBKGND) return 1; // Paint supplies the complete background.
            if (message==WM_MOUSEMOVE || message==WM_MOUSELEAVE || message==WM_LBUTTONDOWN || (message==WM_TIMER && wparam==9)) {
                if (self->original_display_ && self->hud_interaction_.Handle(window,self->BoardBounds(),1440,message,lparam)) {
                    self->table_.SetRelativeScores(self->hud_interaction_.relative());
                    const auto bounds=self->BoardBounds(); InvalidateRect(window,&bounds,FALSE);
                }
                if (message==WM_TIMER) return 0;
            }
            if (message == WM_PRINTCLIENT) { self->Paint(reinterpret_cast<HDC>(wparam)); return 0; }
            if (message == WM_DRAWITEM) {
                const auto& item = *reinterpret_cast<DRAWITEMSTRUCT*>(lparam);
                if(item.CtlID==0xbc6) { self->PaintExit(item); return TRUE; }
                if (item.CtlType == ODT_BUTTON && ((item.CtlID >= 5121 && item.CtlID <= 5128) || item.CtlID==5151)) { self->PaintAction(item); return TRUE; }
            }
            if (message == WM_CTLCOLORSTATIC && self->original_display_) {
                const auto dc = reinterpret_cast<HDC>(wparam);
                SetTextColor(dc, RGB(245, 245, 235)); SetBkColor(dc, RGB(24, 45, 44));
                SetDCBrushColor(dc, RGB(24, 45, 44));
                return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));
            }
            if (message == WM_SIZE) { self->Arrange(); return 0; }
            if (message == WM_APP+16) { self->UpdateActions(); self->Arrange(); return 0; }
            if (message == WM_APP+17) { self->ApplyAutomaticSort(); return 0; }
            if (message == WM_APP+18) { self->UpdateLobbyMusic(); return 0; }
            if (message == WM_RBUTTONUP && self->selection_.Active()) { self->CancelSelection(); self->UpdateActions(); self->Arrange(); return 0; }
            if (message == WM_RBUTTONUP || message == WM_LBUTTONDBLCLK) {
                const int x=static_cast<short>(LOWORD(lparam)),y=static_cast<short>(HIWORD(lparam));
                const auto board=self->BoardBounds(); const POINT point{x,y};
                // Original automatic controls are child HWNDs, so their double
                // clicks never reach the table procedure. Ours are painted.
                if (PtInRect(&board,point) && self->HitAutomaticButton(x,y)<0) self->PreferenceInput(message==WM_LBUTTONDBLCLK);
                return 0;
            }
            if (message == WM_MOUSEMOVE) {
                self->hover_point_ = {static_cast<short>(LOWORD(lparam)), static_cast<short>(HIWORD(lparam))};
                TRACKMOUSEEVENT track{sizeof(TRACKMOUSEEVENT), TME_LEAVE, window, 0}; TrackMouseEvent(&track);
                if (self->client_) {
                    if (self->pressed_tile_ >= 0 && GetCapture() == window && self->LocalTableInput() &&
                        (!self->table_.Animating() || self->table_.ManualAnimation()) && !self->selection_.Active()) {
                        const int target = self->table_.HitTile(self->BoardBounds(),self->Snapshot().game,self->hover_point_.x,self->hover_point_.y);
                        if (self->table_.MoveHandTile(self->pressed_tile_,target,self->Snapshot().game)) {
                            self->automatic_[0] = false; self->hand_dragged_ = true;
                            if (self->animation_enabled_ && self->table_.Animating()) self->motion_timer_.Start(window,6);
                            else self->table_.CancelAnimation();
                            self->UpdateActionAvailability();
                            self->log_("ONLINE HAND MOVE tile="+std::to_string(self->pressed_tile_)+" target="+std::to_string(target)+" sort=0");
                            InvalidateRect(window,nullptr,FALSE);
                        }
                    }
                    const auto interaction = self->Interaction(self->Snapshot());
                    if (interaction.hovered != self->painted_hover_ || interaction.hovered_button != self->painted_auto_hover_)
                        InvalidateRect(window, nullptr, FALSE);
                }
                return 0;
            }
            if (message == WM_MOUSELEAVE) { self->hover_point_ = {-1,-1}; InvalidateRect(window, nullptr, FALSE); return 0; }
            if (message == WM_CAPTURECHANGED || message == WM_CANCELMODE) { self->CancelHandDrag(); return 0; }
            if (message == WM_LBUTTONDOWN && (self->AutomaticInput() || self->LocalTableInput())) {
                self->CancelHandDrag();
                const int x = static_cast<short>(LOWORD(lparam)), y = static_cast<short>(HIWORD(lparam));
                self->pressed_auto_ = self->HitAutomaticButton(x,y);
                self->pressed_tile_ = self->pressed_auto_ < 0 && self->LocalTableInput() ? self->table_.HitTile(self->BoardBounds(),self->Snapshot().game,x,y) : -1;
                self->pressed_revision_ = self->Snapshot().actions.revision;
                if (self->pressed_tile_ >= 0 || self->pressed_auto_ >= 0) SetCapture(window);
                return 0;
            }
            if(message==WM_TIMER && wparam==11) { self->UpdateExitVote(); return 0; }
            if(message==WM_TIMER && self->client_ && self->client_->snapshot().table_paused && wparam>=2 && wparam<=10) return 0;
            if (message == WM_TIMER && wparam == 8) {
                self->captions_.Tick();
                if (!self->captions_.Active()) KillTimer(window,8);
                const auto bounds = self->BoardBounds(); InvalidateRect(window,&bounds,FALSE); return 0;
            }
            if (message == WM_TIMER && wparam == 3) {
                self->ActionClockTick();
                if (self->Interaction(self->Snapshot()).countdown != self->painted_countdown_)
                    InvalidateRect(window, nullptr, FALSE);
                return 0;
            }
            if (message == WM_TIMER && wparam == 7) { self->PresentationTick(); return 0; }
            if (message == WM_TIMER && wparam == 10) {
                KillTimer(window,10);
                self->AutomaticAction(); return 0;
            }
            if (message == WM_TIMER && wparam == 4) { self->AutomaticAction(); return 0; }
            if (message == WM_TIMER && wparam == 5) {
                if (!self->result_deadline_ || GetTickCount64() < self->result_deadline_) return 0;
                self->CancelResultTimer();
                const auto& s = self->Snapshot();
                if (!s.game.settlement.receipts.empty() && IsWindowVisible(window) &&
                    (s.phase == LobbyPhase::Game || s.phase == LobbyPhase::MatchComplete)) {
                    self->show_results_ = true;
                    self->log_("ONLINE RESULT SHOW tick="+std::to_string(GetTickCount64()));
                    self->Update(s);
                }
                return 0;
            }
            if (message == WM_TIMER && wparam == 2) {
                if (self->match_intro_->Active() || self->start_barrier_) { self->audio_.Poll(); return 0; }
                if (self->opening_.Active()) {
                    
                    if (self->opening_.Tick() && self->opening_.sound()) self->PlaySoundId(self->opening_.sound());
                    SetTimer(window,2,self->opening_.interval(),nullptr);
                    if (!self->opening_.Active()) { self->log_("ONLINE OPENING DONE"); self->UpdateActions(); self->Arrange(); }
                    InvalidateRect(window, nullptr, FALSE);
                } else if (!self->deferred_sounds_.empty()) {
                    const auto id = self->deferred_sounds_.front(); self->deferred_sounds_.erase(self->deferred_sounds_.begin());
                    self->PlaySoundId(id);
                }
                try { self->audio_.Poll(); } catch (const std::exception& e) {
                    self->audio_error_ = e.what(); self->log_("ONLINE AUDIO ERROR " + self->audio_error_); self->StopAudio();
                }
                if (!self->opening_.Active() && self->deferred_sounds_.empty() && !self->audio_.Active()) KillTimer(window, 2);
                return 0;
            }
            if (message == WM_TIMER && wparam == 6) {
                
                const bool active = self->table_.Animating();
                if (self->table_.Tick()) {
                    self->log_("ONLINE ANIMATION END"); self->UpdateActions(); self->Arrange();
                }
                if (!self->table_.Animating()) self->motion_timer_.Stop();
                if (active) { const auto bounds = self->BoardBounds(); InvalidateRect(window,&bounds,FALSE); }
                return 0;
            }
            if (message == WM_LBUTTONUP && self->client_ && !self->text_details_ && !self->show_results_) {
                const bool dragged = self->hand_dragged_;
                const int pressed_auto = self->pressed_auto_, pressed_tile = self->pressed_tile_;
                const auto pressed_revision = self->pressed_revision_;
                self->CancelHandDrag();
                if (dragged) return 0;
                const int button = self->HitAutomaticButton(static_cast<short>(LOWORD(lparam)),static_cast<short>(HIWORD(lparam)));
                if (button >= 0 && button == pressed_auto) {
                    
                    self->automatic_[button] = !self->automatic_[button];
                    
                    if (button == 0 && !self->sort_refresh_pending_) {
                        // Original 0047bc37 marks sorting dirty; the render
                        // timer consumes the current checkbox state later.
                        // One pending message coalesces clicks into that state.
                        self->sort_refresh_pending_ = true;
                        self->UpdateActionAvailability();
                        PostMessageW(window,WM_APP+17,0,0);
                    }
                    self->log_("ONLINE AUTO index=" + std::to_string(button) + " enabled=" + std::to_string(self->automatic_[button]));
                    if (self->SuppressCallPrompt()) self->CancelSelection();
                    self->UpdateActions(); self->Arrange();
                    self->UpdateAutomaticTimer();
                    const auto bounds = self->BoardBounds(); InvalidateRect(window,&bounds,FALSE);
                    return 0;
                }
                if (!self->LocalTableInput() || self->Moving() || pressed_revision != self->Snapshot().actions.revision) return 0;
                const auto bounds = self->BoardBounds();
                const auto& snapshot = self->Snapshot();
                const int tile = self->table_.HitTile(bounds, snapshot.game, static_cast<short>(LOWORD(lparam)), static_cast<short>(HIWORD(lparam)));
                if (tile != pressed_tile || tile < 0) return 0;
                const auto& choices = snapshot.actions.choices;
                if (self->selection_.HandSelection()) {
                    const auto index = self->selection_.Pick(snapshot.actions,tile);
                    self->log_("ACTION HAND PICK tile="+std::to_string(tile)+" count="+std::to_string(self->selection_.selected.size()));
                    if (index != static_cast<std::size_t>(-1)) self->Submit(index);
                    else InvalidateRect(window,nullptr,FALSE);
                    return 0;
                }
                const std::string kind = self->action_filter_ == "riichi" ? "riichi" : "discard";
                for (std::size_t i = 0; i < choices.size(); ++i) if (choices[i].kind == kind && choices[i].tiles == std::vector<int>{tile}) { self->Submit(i); break; }
                return 0;
            }
            if (message==WM_APP+25) { PostMessageW(self->parent_,WM_APP+25,0,0); return 0; }
            if (message == WM_COMMAND) { self->Command(LOWORD(wparam)); return 0; }
            if (message == WM_PAINT) {
                RECT update{}; GetUpdateRect(window,&update,FALSE);
                const auto board = self->BoardBounds();
                const bool table_only = self->Snapshot().game.initialized && !self->show_results_ && !self->text_details_ &&
                    update.left>=board.left && update.top>=board.top && update.right<=board.right && update.bottom<=board.bottom;
                self->paint_buffer_.Paint(window, [&](HDC dc) { self->Paint(dc,table_only); });
                return 0;
            }
        } catch (const std::exception& error) {
            const auto text = NetworkWide(error.what());
            self->log_("NETWORK view error: " + NetworkUtf8(text)); return 0;
        }
    }
    return DefWindowProcW(window, message, wparam, lparam);
}
} // namespace tenhou
