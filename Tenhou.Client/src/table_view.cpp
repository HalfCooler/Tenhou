#include "table_view.hpp"
#include "gdi_table_renderer.hpp"
#include "settings_view.hpp"
#include "window_layout.hpp"
#include <commdlg.h>
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace tenhou {
    void TableView::ApplyPreferences(const ClientPreferences &preferences, PreferenceChange change) {
        preferences_ = preferences;
        settlement_view_->SetPeekEnabled(preferences.ResultPeek());
        settlement_view_->SetTileColor(preferences.TileColor());
        renderer_->SetPreferences(preferences, config_path_, change == PreferenceChange::Artwork);
        if (change == PreferenceChange::Artwork) {
            InvalidateRect(viewport_, nullptr, FALSE);
        }
        if (change == PreferenceChange::Sound) {
            audio_.SetVolumePercent(preferences.se_volume);
            const auto voice_mode = GetPrivateProfileIntW(
                L"RESOURCE", L"SE_NOVOICE",
                GetPrivateProfileIntW(L"REBUILD", L"SoundEffectsOnly", 0, config_path_.c_str()),
                config_path_.c_str());
            sequencer_.set_voice_mode(voice_mode);
            sound_loaded_ = false;
            audio_error_.clear();
        }
    }

    namespace {
        constexpr wchar_t kClass[] = L"TenhouRebuildOfflineTable";
        constexpr int kPath = 3101, kLoad = 3102, kPrevious = 3103, kNext = 3104;
        constexpr int kReset = 3105, kStatus = 3106, kEnd = 3107, kOpen = 3108;
        constexpr int kPlayback = 3110, kSpeed = 3111, kRounds = 3112;
        constexpr int kAnimate = 3115, kSound = 3116;
        constexpr int kResults = 3117, kBack = 3118;
        constexpr UINT_PTR kPlaybackTimer = 4;
        constexpr UINT_PTR kEffectsTimer = 5;
        constexpr UINT_PTR kMotionTimer = 6;
        constexpr UINT_PTR kCaptionTimer = 8;
        constexpr UINT_PTR kOpeningTimer = 10;
        constexpr UINT kIntervals[]{2000, 1000, 500, 250, 100};

        std::wstring Wide(const std::string &value) {
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

        void Text(HDC dc, const std::string &value, RECT bounds,
                  UINT format = DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS) {
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
    } // namespace

    TableView::TableView(HWND parent, std::function<void(const std::string &)> log, const std::filesystem::path &config)
        : parent_(parent), renderer_(std::make_unique<GdiTableRenderer>()), log_(std::move(log)), audio_(log_),
          config_path_(config) {
        renderer_->SetPreferences(preferences_, config_path_, false);
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
        viewport_ = CreateWindowExW(WS_EX_CONTROLPARENT, kClass, L"", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                    0, 0, width_, height_, parent_, reinterpret_cast<HMENU>(202), instance, this);
        if (!viewport_) throw std::runtime_error("Cannot create offline table viewport");
        const auto control = [&](const wchar_t *kind, const wchar_t *text, int id, DWORD style = 0) {
            const auto window = CreateWindowExW(std::wstring(kind) == L"EDIT" ? WS_EX_CLIENTEDGE : 0,
                                                kind, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 1, 1, viewport_,
                                                reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
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
        for (const auto *label: {L"0.5x", L"1x", L"2x", L"4x", L"10x"})
            SendMessageW(speed_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
        SendMessageW(speed_, CB_SETCURSEL, 1, 0);
        rounds_ = control(L"COMBOBOX", L"", kRounds, WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
        animate_control_ = control(L"BUTTON", L"动画", kAnimate, WS_TABSTOP | BS_AUTOCHECKBOX);
        sound_control_ = control(L"BUTTON", L"声音", kSound, WS_TABSTOP | BS_AUTOCHECKBOX);
        results_control_ = control(L"BUTTON", L"结算", kResults, WS_TABSTOP | BS_AUTOCHECKBOX);
        back_ = control(L"BUTTON", L"返回", kBack, WS_TABSTOP);
        settlement_view_ = std::make_unique<SettlementView>(viewport_);
        settlement_view_->SetBackgroundPainter([this](HDC dc, RECT r) {
            if (sequencer_.session() && sequencer_.session()->state().initialized) PaintTable(dc, r);
        });
        settlement_view_->SetConfirmHandler([this] {
            sequencer_.set_show_results(false);
            SendMessageW(results_control_, BM_SETCHECK, BST_UNCHECKED, 0);
            Update();
            InvalidateRect(viewport_, nullptr, FALSE);
        });
        match_intro_ = std::make_unique<MatchIntroView>(viewport_, [this] {
            ApplyEffects(sequencer_.OnMatchIntroFinished(AnimationNow()));
            log_("MATCH INTRO DONE tick=" + std::to_string(GetTickCount64()));
            Update();
        });
        match_intro_->SetSoundPlayer([this](unsigned id) { PlaySoundId(id); });
        settlement_view_->SetSoundPlayer([this](unsigned id) { PlaySoundId(id); });
        settlement_view_->SetSoundStopper([this] {
            try { audio_.Stop(); } catch (const std::exception &e) { SoundError(e); }
        });
        sequencer_.set_animation_enabled(GetPrivateProfileIntW(L"REBUILD", L"ReplayAnimation", 1, config_path_.c_str()) != 0);
        sequencer_.set_sound_enabled(GetPrivateProfileIntW(L"REBUILD", L"ReplaySound", 0, config_path_.c_str()) != 0);
        settlement_view_->SetSoundEnabled(EffectsEnabled());
        settlement_view_->SetAnimationEnabled(sequencer_.animation_enabled());
        SendMessageW(animate_control_, BM_SETCHECK, sequencer_.animation_enabled() ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(sound_control_, BM_SETCHECK, sequencer_.sound_enabled() ? BST_CHECKED : BST_UNCHECKED, 0);
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

    TileGeometryCatalog *TableView::EnsureGeometry() {
        const std::vector<int> empty_red;
        const auto &red = sequencer_.session() ? sequencer_.session()->state().red_tiles : empty_red;
        return &renderer_->EnsureGeometry(red);
    }

    unsigned TableView::CurrentSpeedInterval() const {
        if (!speed_) return kIntervals[1];
        const auto index = SendMessageW(speed_, CB_GETCURSEL, 0, 0);
        return index >= 0 && index < 5 ? kIntervals[index] : kIntervals[1];
    }

    void TableView::SyncTimers() {
        if (!viewport_ || !IsWindow(viewport_)) return;
        if (sequencer_.opening().Active()) opening_timer_.Start(viewport_, kOpeningTimer, kTableTickMs);
        else opening_timer_.Stop();
        if (sequencer_.presentation().Active()) motion_timer_.Start(viewport_, kMotionTimer, kTableTickMs);
        else motion_timer_.Stop();
        if (sequencer_.captions().Active()) SetTimer(viewport_, kCaptionTimer, 15, nullptr);
        else KillTimer(viewport_, kCaptionTimer);
        if (sequencer_.draw_end_ticks()) SetTimer(viewport_, kEffectsTimer, 15, nullptr);
        else if (!audio_.Active()) KillTimer(viewport_, kEffectsTimer);
    }

    void TableView::ApplyEffects(const ReplaySequencerEffects &effects) {
        if (!effects.artwork_error.empty()) artwork_error_ = effects.artwork_error;
        for (const auto &entry: effects.logs) log_(entry);
        if (effects.cancel_settlement && settlement_view_) settlement_view_->CancelPresentation();
        if (effects.cancel_intro && match_intro_) match_intro_->Cancel();
        if (effects.stop_audio) {
            try { audio_.Stop(); } catch (const std::exception &e) { SoundError(e); }
        }
        if (effects.sync_results_checkbox && results_control_)
            SendMessageW(results_control_, BM_SETCHECK, effects.results_checked ? BST_CHECKED : BST_UNCHECKED, 0);
        if (effects.playback_ended) {
            if (viewport_) KillTimer(viewport_, kPlaybackTimer);
            if (playback_) SetWindowTextW(playback_, L"播放");
        }
        for (unsigned id: effects.play_sounds) PlaySoundId(id);
        if (effects.begin_match_intro && match_intro_ && sequencer_.session()) {
            settlement_view_->Show(false);
            match_intro_->Move({0, 0, width_, height_});
            match_intro_->Begin(sequencer_.session()->state(), effects.intro_type);
        }
        SyncTimers();
        if (effects.update_ui) Update();
        if (viewport_) {
            if (effects.dirty_all) InvalidateRect(viewport_, nullptr, FALSE);
            else if (effects.dirty_table) {
                const auto bounds = BoardBounds();
                InvalidateRect(viewport_, &bounds, FALSE);
            }
        }
    }

    void TableView::Show(bool visible) {
        if (!visible) {
            Stop();
            Update();
        }
        ShowWindow(viewport_, visible ? SW_SHOW : SW_HIDE);
    }

    void TableView::ConfigureArtwork(const std::filesystem::path &directory, int width) {
        match_intro_->Configure(directory, width);
        settlement_view_->Configure(directory, width);
        const bool was_playing = sequencer_.playing();
        Stop();
        if (resource_directory_ != directory || canvas_width_ != width) {
            renderer_->Configure(directory, width);
            renderer_->SetPreferences(preferences_, config_path_, false);
            artwork_loaded_logged_ = false;
            artwork_error_.clear();
            resource_directory_ = directory;
            canvas_width_ = width;
            sounds_ = {};
            sound_loaded_ = false;
            audio_error_.clear();
            InvalidateRect(viewport_, nullptr, FALSE);
        }
        if (was_playing) Update();
    }

    void TableView::Resize(int width, int height) {
        width = kDefaultClientWidth;
        height = kDefaultClientHeight;
        width_ = width;
        height_ = height;
        MoveWindow(viewport_, 0, 0, width, height, TRUE);
        RECT client{};
        GetClientRect(viewport_, &client);
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
        MoveWindow(back_, 682, 112, 74, 27, TRUE);
        MoveWindow(results_control_, width - 100, 148, 84, 27, TRUE);
        settlement_view_->Move({16, 204, width - 16, std::max(254, height - 48)});
        const bool compact = Compact();
        for (HWND control: {path_, load_}) ShowWindow(control, compact ? SW_HIDE : SW_SHOW);
        if (compact) {
            const HWND nav[]{reset_, previous_, next_, end_, playback_};
            const int widths[]{78, 78, 64, 64, 64};
            int x = 12;
            for (int i = 0; i < 5; ++i) {
                MoveWindow(nav[i], x, 8, widths[i], 26, TRUE);
                x += widths[i] + 4;
            }
            MoveWindow(speed_, x, 8, 62, 180, TRUE);
            x += 66;
            MoveWindow(rounds_, x, 8, std::max(80, width - x - 12), 240, TRUE);
            MoveWindow(open_, 12, 42, 74, 26, TRUE);
            MoveWindow(back_, 92, 42, 74, 26, TRUE);
            MoveWindow(animate_control_, 210, 42, 80, 26, TRUE);
            MoveWindow(sound_control_, 298, 42, 76, 26, TRUE);
            MoveWindow(results_control_, width - 100, 42, 88, 26, TRUE);
            MoveWindow(status_, 12, height - 26, width - 24, 22, TRUE);
        }
        settlement_view_->Move(BoardBounds());
        match_intro_->Move({0, 0, width_, height_});
        InvalidateRect(viewport_, nullptr, FALSE);
    }

    bool TableView::Compact() const {
        return sequencer_.original_display() && sequencer_.session() && sequencer_.session()->state().initialized;
    }

    RECT TableView::BoardBounds() const {
        return Compact() ? RECT{8, 78, width_ - 8, height_ - 30} : RECT{16, 204, width_ - 16, height_ - 48};
    }

    void TableView::SetOriginalDisplay(bool enabled) {
        Stop();
        sequencer_.set_original_display(enabled);
        RECT r{};
        GetClientRect(parent_, &r);
        Resize(r.right, r.bottom);
    }

    void TableView::SetEffectsOnly(bool enabled) {
        try { audio_.Stop(); } catch (const std::exception &e) { SoundError(e); }
        sequencer_.set_effects_only(enabled);
    }

    void TableView::SetEffectsMuted(bool muted) {
        if (sequencer_.effects_muted() == muted) return;
        sequencer_.set_effects_muted(muted);
        settlement_view_->SetSoundEnabled(EffectsEnabled());
        if (muted) {
            try { audio_.Stop(); } catch (const std::exception &e) { SoundError(e); }
        }
    }

    void TableView::Load(const std::filesystem::path &path) {
        Stop();
        error_.clear();
        artwork_error_.clear();
        audio_error_.clear();
        SetWindowTextW(path_, path.c_str());
        try {
            auto log = ReplayLog::Load(path);
            bool red = true;
            for (const auto &event: log.events)
                if (event.tag == "GO" && event.attributes.count("type"))
                    red = (std::stoul(event.attributes.at("type")) & 2) == 0;
            std::vector<std::size_t> positions;
            std::vector<std::wstring> labels;
            for (std::size_t i = 0; i < log.events.size(); ++i)
                if (log.events[i].kind == ReplayEventKind::Init) {
                    positions.push_back(i + 1);
                    std::istringstream seed(log.events[i].attributes.at("seed"));
                    int round = 0, honba = 0;
                    char comma = 0;
                    seed >> round >> comma >> honba;
                    const wchar_t *winds[]{L"东", L"南", L"西", L"北"};
                    labels.push_back(std::wstring(winds[(round / 4) % 4]) + std::to_wstring(round % 4 + 1) + L"局" +
                                     (honba ? L" " + std::to_wstring(honba) + L"本场" : L""));
                }
            sequencer_.SetSession(std::make_unique<ReplaySession>(std::move(log)), std::move(positions));
            sequencer_.set_red_tiles(red);
            SendMessageW(rounds_, CB_RESETCONTENT, 0, 0);
            for (const auto &label: labels)
                SendMessageW(rounds_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
            renderer_->Prepare();
            painted_ = false;
            log_("REPLAY LOADED events=" + std::to_string(sequencer_.session()->log().events.size()));
        } catch (const std::exception &error) {
            error_ = error.what();
            log_("REPLAY ERROR " + error_);
        }
        Update();
    }

    void TableView::Reload() {
        Stop();
        renderer_->Configure(resource_directory_, canvas_width_);
        renderer_->SetPreferences(preferences_, config_path_, false);
        artwork_loaded_logged_ = false;
        artwork_error_.clear();
        sounds_ = {};
        sound_loaded_ = false;
        audio_error_.clear();
        if (GetWindowTextLengthW(path_)) Command(kLoad);
        else Update();
    }

    void TableView::Update() {
        const auto *session = sequencer_.session();
        const bool result_available = session && !session->state().settlement.receipts.empty();
        EnableWindow(results_control_, result_available);
        if (!result_available) {
            sequencer_.set_show_results(false);
            SendMessageW(results_control_, BM_SETCHECK, BST_UNCHECKED, 0);
        }
        if (session) {
            std::array<std::string, 4> names;
            for (int i = 0; i < 4; ++i) names[i] = session->state().players[i].name;
            settlement_view_->SetContext(session->state().dealer, session->state().honba, session->state().sticks);
            settlement_view_->Set(session->state().settlement, names, sequencer_.red_tiles());
        }
        const bool show_results = sequencer_.show_results() && result_available;
        settlement_view_->Show(show_results);
        if (sequencer_.consume_result_pending() && show_results) settlement_view_->Present();
        const bool before = session && session->position() > 0;
        const bool after = session && session->position() < session->log().events.size();
        EnableWindow(previous_, before);
        EnableWindow(reset_, before);
        EnableWindow(next_, after);
        EnableWindow(end_, after);
        if (!after && sequencer_.playing()) {
            Stop();
            log_("REPLAY PLAYBACK end");
        }
        EnableWindow(playback_, after);
        const auto &round_positions = sequencer_.round_positions();
        EnableWindow(rounds_, session && !round_positions.empty());
        int selected_round = -1;
        if (session)
            for (std::size_t i = 0; i < round_positions.size(); ++i)
                if (round_positions[i] <= session->position()) selected_round = static_cast<int>(i);
        SendMessageW(rounds_, CB_SETCURSEL, selected_round, 0);
        auto status = session
                          ? "cursor " + std::to_string(session->position()) + "/" +
                                std::to_string(session->log().events.size())
                          : error_.empty()
                                ? "No replay loaded"
                                : "Load failed";
        if (session && !error_.empty()) status += " | Load failed; previous replay kept";
        if (sequencer_.playing()) status += " | Playing";
        if (!audio_error_.empty()) status += " | Sound error (F5 retries): " + audio_error_;
        SetWindowTextW(status_, Wide(status).c_str());
        if (session)
            log_("REPLAY CURSOR " + std::to_string(session->position()) + "/" +
                 std::to_string(session->log().events.size()));
        RECT r{};
        GetClientRect(parent_, &r);
        Resize(r.right, r.bottom);
    }

    void TableView::Command(int id) {
        if (id == kBack) {
            Stop();
            PostMessageW(parent_, WM_APP + 25, 0, 0);
            return;
        }
        if (id == kResults) {
            Stop();
            sequencer_.set_show_results(SendMessageW(results_control_, BM_GETCHECK, 0, 0) == BST_CHECKED);
            Update();
            return;
        }
        if (id == kAnimate || id == kSound) {
            if (id == kAnimate) sequencer_.presentation().Cancel();
            else {
                try { audio_.Stop(); } catch (const std::exception &e) { SoundError(e); }
            }
            sequencer_.set_animation_enabled(SendMessageW(animate_control_, BM_GETCHECK, 0, 0) == BST_CHECKED);
            sequencer_.set_sound_enabled(SendMessageW(sound_control_, BM_GETCHECK, 0, 0) == BST_CHECKED);
            settlement_view_->SetSoundEnabled(EffectsEnabled());
            settlement_view_->SetAnimationEnabled(sequencer_.animation_enabled());
            if (!sequencer_.animation_enabled() && !EffectsEnabled()) {
                sequencer_.opening().Cancel();
                opening_timer_.Stop();
            }
            if (id == kSound) {
                sounds_ = {};
                sound_loaded_ = false;
                audio_error_.clear();
            }
            SyncTimers();
            Update();
            return;
        }
        if (id == kPlayback) {
            if (sequencer_.playing()) Stop();
            else Start();
            Update();
            return;
        }
        if (id == kSpeed) {
            if (sequencer_.playing()) {
                Stop();
                Start();
            }
            Update();
            return;
        }
        if (id == kRounds) {
            Stop();
            const auto index = SendMessageW(rounds_, CB_GETCURSEL, 0, 0);
            const auto &round_positions = sequencer_.round_positions();
            if (sequencer_.session() && index >= 0 && static_cast<std::size_t>(index) < round_positions.size()) {
                sequencer_.session()->Seek(round_positions[static_cast<std::size_t>(index)]);
                Update();
            }
            return;
        }
        if (id == kLoad) {
            std::wstring value(GetWindowTextLengthW(path_) + 1, L' ');
            GetWindowTextW(path_, value.data(), static_cast<int>(value.size()));
            value.resize(value.size() - 1);
            Load(std::filesystem::path(value));
        } else if (id == kOpen) {
            Stop();
            Update();
            wchar_t file[32768]{};
            OPENFILENAMEW dialog{sizeof(OPENFILENAMEW)};
            dialog.hwndOwner = parent_;
            dialog.lpstrFilter = L"Tenhou replay (*.xml;*.mjlog;*.gz)\0*.xml;*.mjlog;*.gz\0All files\0*.*\0\0";
            dialog.lpstrFile = file;
            dialog.nMaxFile = 32768;
            dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
            if (GetOpenFileNameW(&dialog)) Load(std::filesystem::path(file));
            else if (CommDlgExtendedError()) throw std::runtime_error("Open replay dialog failed");
        } else if (sequencer_.session() && (id == kPrevious || id == kNext || id == kReset || id == kEnd)) {
            Stop();
            auto *session = sequencer_.session();
            if (id == kPrevious && session->position()) session->Seek(session->position() - 1);
            else if (id == kNext) Advance();
            else if (id == kReset) session->Reset();
            else if (id == kEnd) session->Seek(session->log().events.size());
            Update();
        }
    }

    void TableView::Stop() {
        if (viewport_) KillTimer(viewport_, kPlaybackTimer);
        if (playback_) SetWindowTextW(playback_, L"播放");
        ApplyEffects(sequencer_.Stop());
    }

    void TableView::Start() {
        const auto index = SendMessageW(speed_, CB_GETCURSEL, 0, 0);
        if (index < 0 || index >= 5) return;
        const unsigned interval = kIntervals[index];
        ReplaySequencerEffects fx;
        if (!sequencer_.Start(interval, GetTickCount64(), IsWindowVisible(viewport_) != FALSE, fx)) return;
        if (!SetTimer(viewport_, kPlaybackTimer, interval, nullptr)) {
            sequencer_.Stop();
            throw std::runtime_error("Cannot start replay timer");
        }
        SetWindowTextW(playback_, L"暂停");
        ApplyEffects(fx);
    }

    void TableView::Tick() {
        ApplyEffects(sequencer_.PlaybackTick(
            CurrentSpeedInterval(), canvas_width_, artwork_error_.empty(),
            GetTickCount64(), AnimationNow(),
            match_intro_->Active(), settlement_view_->Animating(), audio_.Active(),
            [this] { return EnsureGeometry(); }));
    }

    void TableView::SaveSettings() {
        if (!WritePrivateProfileStringW(L"REBUILD", L"ReplayAnimation",
                                        sequencer_.animation_enabled() ? L"1" : L"0", config_path_.c_str()) ||
            !WritePrivateProfileStringW(L"REBUILD", L"ReplaySound",
                                        sequencer_.sound_enabled() ? L"1" : L"0", config_path_.c_str()))
            throw std::runtime_error("Cannot save replay effects settings");
    }

    void TableView::CancelEffects() {
        ApplyEffects(sequencer_.CancelEffects());
    }

    void TableView::SoundError(const std::exception &error) {
        audio_error_ = error.what();
        log_("AUDIO ERROR " + audio_error_);
        if (status_) {
            const auto *session = sequencer_.session();
            const auto cursor = session
                                    ? "cursor " + std::to_string(session->position()) + "/" +
                                          std::to_string(session->log().events.size()) + " | "
                                    : std::string();
            SetWindowTextW(status_, Wide(cursor + "Sound error (F5 retries): " + audio_error_).c_str());
        }
        try { audio_.Stop(); } catch (const std::exception &e) { log_(std::string("AUDIO RESET ERROR ") + e.what()); }
        InvalidateRect(viewport_, nullptr, FALSE);
    }

    void TableView::Advance() {
        ApplyEffects(sequencer_.Advance(
            CurrentSpeedInterval(), canvas_width_, artwork_error_.empty(),
            GetTickCount64(), AnimationNow(), [this] { return EnsureGeometry(); }));
    }

    void TableView::PlaySoundId(unsigned id, unsigned delay) {
        if (!EffectsEnabled() || !audio_error_.empty()) return;
        try {
            if (!sound_loaded_) {
                sounds_.Load(resource_directory_, canvas_width_, config_path_);
                sound_loaded_ = true;
                log_("AUDIO BANK loaded");
            }
            audio_.Schedule(id, sounds_.Get(id), delay);
            if (!SetTimer(viewport_, kEffectsTimer, sequencer_.draw_end_ticks() ? 15 : 25, nullptr))
                throw std::runtime_error("Cannot start replay effects timer");
        } catch (const std::exception &e) {
            SoundError(e);
        }
    }

    void TableView::EffectsTick() {
        ApplyEffects(sequencer_.EffectsTick());
        try { audio_.Poll(); } catch (const std::exception &e) { SoundError(e); }
        if (!sequencer_.draw_end_ticks() && !audio_.Active()) KillTimer(viewport_, kEffectsTimer);
    }

    void TableView::OpeningTick() {
        ApplyEffects(sequencer_.OpeningTick(AnimationNow(), match_intro_->Active()));
    }

    void TableView::PaintTable(HDC dc, RECT bounds) {
        try {
            const auto &session_state = sequencer_.session()->state();
            const auto &geometry = renderer_->EnsureGeometry(session_state.red_tiles);
            const auto frame = BuildReplayTableRenderFrame(
                canvas_width_, {bounds.left, bounds.top, bounds.right, bounds.bottom}, session_state, geometry,
                sequencer_.presentation(), sequencer_.opening(), sequencer_.captions(),
                sequencer_.red_tiles(), sequencer_.original_display(), sequencer_.animation_enabled(),
                hud_interaction_.relative(), sequencer_.draw_end_ticks(), AnimationNow(),
                GetDpiForWindow(viewport_));
            renderer_->RenderToDc(dc, frame);
            if (!artwork_loaded_logged_) {
                artwork_loaded_logged_ = true;
                log_("REPLAY ARTWORK canvas=" + std::to_string(canvas_width_));
            }
            artwork_error_.clear();
        } catch (const std::exception &error) {
            if (artwork_error_ != error.what()) log_("REPLAY ARTWORK ERROR " + std::string(error.what()));
            artwork_error_ = error.what();
            const bool was_playing = sequencer_.playing();
            Stop();
            if (was_playing) Update();
            SetTextColor(dc, RGB(255, 177, 160));
            Text(dc, "牌桌资源加载失败：" + artwork_error_ + "\n请修复资源后按 F5 重试。", bounds,
                 DT_LEFT | DT_WORDBREAK);
        }
    }

    void TableView::Paint(HDC dc, bool table_only) {
        if (!table_only) Fill(dc, {0, 0, width_, height_}, RGB(23, 45, 44));
        const auto old_font = SelectObject(dc, font_);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(245, 245, 235));
        if (!Compact()) Text(dc, "Offline replay | Original tile artwork", {16, 12, width_ - 304, 34});
        const auto *session = sequencer_.session();
        if (!error_.empty() && !session) {
            SetTextColor(dc, RGB(255, 177, 160));
            Text(dc, "Replay loading failed:\n\n" + error_ + "\n\nChoose another file, then Load.",
                 {24, 164, width_ - 24, height_ - 56}, DT_LEFT | DT_WORDBREAK);
        } else if (!session) {
            Text(dc, "打开本机 XML 或 gzip 牌谱文件，使用前进、回退、播放、倍速及跳局查看对局。",
                 {24, 164, width_ - 24, height_ - 56}, DT_LEFT | DT_WORDBREAK);
        } else {
            const auto &state = session->state();
            std::ostringstream summary;
            summary << "Round " << state.round + 1 << " | Honba " << state.honba << " | Sticks " << state.sticks
                    << " | Dora: ";
            for (int tile: state.dora_indicators) summary << CompactTile(tile, sequencer_.red_tiles()) << ' ';
            if (!state.initialized) summary << " | Awaiting INIT";
            if (state.ended) summary << " | " << state.result;
            if (!Compact()) Text(dc, summary.str(), {16, 148, width_ - 110, 168});
            SetTextColor(dc, error_.empty() ? RGB(245, 245, 235) : RGB(255, 177, 160));
            if (!audio_error_.empty()) SetTextColor(dc, RGB(255, 177, 160));
            if (!Compact())
                Text(dc, !audio_error_.empty()
                             ? "Sound error (F5 retries): " + audio_error_
                             : error_.empty()
                                   ? "Event: " + (state.last_event.empty()
                                                      ? "Beginning (no events applied)"
                                                      : state.last_event)
                                   : "Load failed (previous replay kept): " + error_,
                     {16, 174, width_ - 16, 194});
            const auto board = BoardBounds();
            const int clip = SaveDC(dc);
            IntersectClipRect(dc, 0, board.top, width_, board.bottom);
            if (state.initialized) PaintTable(dc, board);
            else {
                SetTextColor(dc, RGB(225, 235, 230));
                Text(dc, "等待首局 INIT 事件，点击“前进”或“播放”开始查看牌桌。",
                     {board.left + 8, board.top + 8, board.right - 8, board.bottom - 8}, DT_LEFT | DT_WORDBREAK);
            }
            RestoreDC(dc, clip);
        }
        SetTextColor(dc, RGB(225, 235, 230));
        if (!Compact())
            Text(dc, "All active hands visible | Gold outline: drawn tile / riichi",
                 {16, height_ - 36, width_ - 16, height_ - 15});
        SelectObject(dc, old_font);
        if (!painted_) {
            log_("PAINT offline replay rendered");
            painted_ = true;
        }
    }

    void TableView::Capture(HDC dc) {
        SendMessageW(viewport_, WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_NONCLIENT);
        Paint(dc);
        settlement_view_->Capture(dc);
        POINT origin{0, 0};
        ClientToScreen(viewport_, &origin);
        for (HWND control: {
                 path_, load_, open_, previous_, next_, reset_, end_, status_, playback_, speed_, rounds_,
                 animate_control_, sound_control_, results_control_, back_
             }) {
            if (!IsWindowVisible(control)) continue;
            RECT position{};
            GetWindowRect(control, &position);
            const int saved = SaveDC(dc);
            SetViewportOrgEx(dc, position.left - origin.x, position.top - origin.y, nullptr);
            SendMessageW(control, WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT | PRF_NONCLIENT | PRF_ERASEBKGND);
            RestoreDC(dc, saved);
        }
        match_intro_->Capture(dc);
    }

    LRESULT CALLBACK TableView::ViewProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        auto *view = reinterpret_cast<TableView *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            view = static_cast<TableView *>(reinterpret_cast<CREATESTRUCTW *>(lparam)->lpCreateParams);
            view->viewport_ = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
        }
        if (view) {
            try {
                if (message == WM_ERASEBKGND) return 1;
                if (message == WM_MOUSEMOVE || message == WM_MOUSELEAVE || message == WM_LBUTTONDOWN ||
                    (message == WM_TIMER && wparam == 9)) {
                    if (message == WM_MOUSEMOVE) {
                        TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, window, 0};
                        TrackMouseEvent(&track);
                    }
                    if (view->sequencer_.original_display() &&
                        view->hud_interaction_.Handle(window, view->BoardBounds(), view->canvas_width_, message, lparam)) {
                        const auto bounds = view->BoardBounds();
                        InvalidateRect(window, &bounds, FALSE);
                    }
                    if (message == WM_TIMER) return 0;
                }
                if (message == WM_TIMER && wparam == kCaptionTimer) {
                    if (view->sequencer_.CaptionTick()) {
                        if (!view->sequencer_.captions().Active()) KillTimer(window, kCaptionTimer);
                        const auto bounds = view->BoardBounds();
                        InvalidateRect(window, &bounds, FALSE);
                    } else {
                        KillTimer(window, kCaptionTimer);
                    }
                    return 0;
                }
                if (message == WM_COMMAND) {
                    const int id = LOWORD(wparam), notification = HIWORD(wparam);
                    if ((id == kSpeed || id == kRounds) ? notification == CBN_SELCHANGE : notification == BN_CLICKED)
                        view->Command(id);
                    return 0;
                }
                if (message == WM_TIMER && wparam == kPlaybackTimer) {
                    view->Tick();
                    return 0;
                }
                if (message == WM_TIMER && wparam == kEffectsTimer) {
                    view->EffectsTick();
                    return 0;
                }
                if (message == WM_TIMER && wparam == kOpeningTimer) {
                    view->OpeningTick();
                    return 0;
                }
                if (message == WM_TIMER && wparam == kMotionTimer) {
                    view->ApplyEffects(view->sequencer_.MotionTick(AnimationNow()));
                    return 0;
                }
                if (message == WM_PRINTCLIENT) {
                    view->Paint(reinterpret_cast<HDC>(wparam));
                    return 0;
                }
                if (message == WM_PAINT) {
                    RECT update{};
                    GetUpdateRect(window, &update, FALSE);
                    const auto board = view->BoardBounds();
                    const bool table_only = update.left >= board.left && update.top >= board.top &&
                                            update.right <= board.right && update.bottom <= board.bottom;
                    view->paint_buffer_.Paint(window, [&](HDC dc) { view->Paint(dc, table_only); });
                    return 0;
                }
            } catch (const std::exception &error) {
                view->Stop();
                view->error_ = error.what();
                view->log_("REPLAY ERROR " + view->error_);
                view->Update();
                return 0;
            }
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }
} // namespace tenhou
