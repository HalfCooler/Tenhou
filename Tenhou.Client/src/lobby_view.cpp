#include "lobby_view.hpp"
#include "background.hpp"
#include "offline_lobby_view.hpp"
#include "original_button.hpp"
#include "settings_view.hpp"
#include "music_preferences.hpp"
#include "matchmaking_view.hpp"
#include "preference_actions.hpp"
#include "replay_effects.hpp"
#include "text_catalog.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <stdexcept>
#include <commctrl.h>

namespace tenhou {
    namespace {
        constexpr UINT_PTR kOpeningTimer = 12;
        constexpr wchar_t kLobbyWindow[] = L"TENHOU_REBUILD_LOBBY";

        std::wstring Text(HWND window) {
            const int length = GetWindowTextLengthW(window);
            std::wstring value(static_cast<std::size_t>(length) + 1, L'\0');
            GetWindowTextW(window, value.data(), length + 1);
            value.resize(static_cast<std::size_t>(length));
            return value;
        }

        LRESULT CALLBACK ActionButtonProcedure(HWND w, UINT m, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR data) {
            if (m == WM_LBUTTONDOWN || (m == WM_KEYDOWN && (wp == VK_SPACE || wp == VK_RETURN)))
                SendMessageW(GetParent(w), WM_APP + 26, static_cast<WPARAM>(GetDlgCtrlID(w)), 0);
            if (m == WM_ERASEBKGND) return 1; // WM_DRAWITEM supplies every pixel.
            if (data && (m == WM_LBUTTONDOWN || m == WM_LBUTTONUP)) {
                const auto *log = reinterpret_cast<const std::function<void(const std::string &)> *>(data);
                (*log)("ACTION POINTER button=" + std::to_string(GetDlgCtrlID(w)) + " message=" + std::to_string(m));
            }
            if (m == WM_MOUSEMOVE) {
                TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, w, 0};
                TrackMouseEvent(&track);
                InvalidateRect(w, nullptr, FALSE);
            }
            if (m == WM_MOUSELEAVE) InvalidateRect(w, nullptr, FALSE);
            return DefSubclassProc(w, m, wp, lp);
        }
    }

    OnlineSequencerContext LobbyView::MakeContext() const {
        OnlineSequencerContext ctx;
        ctx.tick_ms = GetTickCount64();
        ctx.anim_ms = AnimationNow();
        ctx.window_visible = window_ && IsWindowVisible(window_) != FALSE;
        ctx.intro_active = match_intro_ && match_intro_->Active();
        ctx.intro_resuming = match_intro_ && match_intro_->Resuming();
        ctx.settlement_animating = settlement_view_ && settlement_view_->Animating();
        ctx.audio_active = audio_.Active();
        ctx.audio_healthy = audio_error_.empty();
        ctx.canvas_width = canvas_width_;
        return ctx;
    }

    OnlineMotionCallbacks LobbyView::MakeMotion() {
        OnlineMotionCallbacks motion;
        motion.accept = [this](const OnlineState &game, const ReplayEvent &event, bool animate, double now) {
            table_.Accept(game, event, animate, now);
            if (table_.Animating()) motion_timer_.Start(window_, 6, kTableTickMs);
            else motion_timer_.Stop();
        };
        motion.begin_opening = [this](OpeningPresentation &opening, const OnlineState &game, bool wall) {
            table_.BeginOpening(opening, game, wall);
        };
        motion.set_auto_sort = [this](bool enabled, const OnlineState &game) {
            table_.SetAutoSort(enabled, game);
            if (sequencer_.animation_enabled() && table_.Animating()) motion_timer_.Start(window_, 6);
            else {
                table_.CancelAnimation();
                motion_timer_.Stop();
            }
        };
        motion.set_draw_end_slots = [this](unsigned slots) { table_.SetDrawEndSlots(slots); };
        motion.set_paused = [this](bool paused) {
            table_.SetPaused(paused);
            if (settlement_view_) settlement_view_->SetPaused(paused);
            if (match_intro_) match_intro_->SetPaused(paused);
        };
        motion.present_settlement = [this] { table_.PresentSettlement(); };
        motion.cancel_animation = [this] {
            table_.CancelAnimation();
            motion_timer_.Stop();
        };
        motion.animating = [this] { return table_.Animating(); };
        motion.manual_animation = [this] { return table_.ManualAnimation(); };
        motion.prepare = [this] { table_.Prepare(); };
        motion.refresh_preferences = [this] { table_.SetPreferences(preferences_, config_, false); };
        return motion;
    }

    void LobbyView::SyncTimers() {
        if (!window_ || !client_) return;
        if (sequencer_.opening().Active()) opening_timer_.Start(window_, kOpeningTimer, kTableTickMs);
        else opening_timer_.Stop();

        const bool queue_needed = sequencer_.QueueTimerNeeded();
        if (queue_needed && !queue_timer_) queue_timer_ = SetTimer(window_, 7, 15, nullptr) != 0;
        else if (!queue_needed && queue_timer_) {
            KillTimer(window_, 7);
            queue_timer_ = false;
        }

        const bool clock_needed = sequencer_.ClockTimerNeeded();
        if (clock_needed && !clock_timer_) clock_timer_ = SetTimer(window_, 3, 100, nullptr) != 0;
        else if (!clock_needed && clock_timer_) {
            KillTimer(window_, 3);
            clock_timer_ = false;
        }

        const bool auto_needed = sequencer_.AutoTimerNeeded(client_->snapshot());
        if (auto_needed && !automatic_timer_) automatic_timer_ = SetTimer(window_, 4, 100, nullptr) != 0;
        else if (!auto_needed && automatic_timer_) {
            KillTimer(window_, 4);
            automatic_timer_ = false;
        }

        const bool caption_needed = sequencer_.captions().Active();
        if (caption_needed && !caption_timer_) caption_timer_ = SetTimer(window_, 8, 15, nullptr) != 0;
        else if (!caption_needed && caption_timer_) {
            KillTimer(window_, 8);
            caption_timer_ = false;
        }
    }

    void LobbyView::ApplyEffects(const OnlineSequencerEffects &fx) {
        if (fx.reload_config_preferences) preferences_ = ClientPreferences::Load(config_);
        if (fx.release_capture && GetCapture() == window_) ReleaseCapture();
        if (fx.stop_all_sound) {
            if (match_intro_ && !fx.preserve_intro_resume) {
                if (match_intro_->Resuming()) log_("ONLINE RESUME PREPARATION END");
                match_intro_->Cancel();
            }
            if (!sequencer_.opening().Active()) opening_timer_.Stop();
            if (!table_.Animating()) motion_timer_.Stop();
            if (settlement_view_) settlement_view_->CancelPresentation();
            KillTimer(window_, 2);
            try { audio_.Stop(); } catch (const std::exception &e) {
                log_(std::string("ONLINE AUDIO RESET ERROR ") + e.what());
            }
        }
        if (fx.stop_audio_only) {
            try { audio_.Stop(); } catch (const std::exception &e) {
                log_(std::string("ONLINE AUDIO RESET ERROR ") + e.what());
            }
        }
        if (fx.cancel_settlement && settlement_view_) settlement_view_->CancelPresentation();
        if (fx.cancel_intro && match_intro_ && fx.intro.kind == OnlineIntroRequest::Kind::None)
            match_intro_->Cancel();

        for (const auto &entry: fx.logs) log_(entry);

        if (fx.notified_snapshot) {
            UpdateExitVote();
            if (matching_) matching_->SetSnapshot(*fx.notified_snapshot);
            if (service_lobby_) service_lobby_->SetServiceSnapshot(*fx.notified_snapshot);
        }

        if (fx.intro.kind == OnlineIntroRequest::Kind::BeginMatch && match_intro_) {
            RECT area{};
            GetClientRect(window_, &area);
            match_intro_->Move(area);
            match_intro_->Begin(fx.intro.state, fx.intro.type);
        }

        if (fx.settlement_sync.valid && settlement_view_) {
            const auto &sync = fx.settlement_sync;
            settlement_view_->SetContinuation(sync.continuation);
            settlement_view_->SetContext(sync.dealer, sync.honba, sync.sticks);
            settlement_view_->Set(sync.settlement, sync.names, sync.red);
            settlement_view_->SetConfirmEnabled(sync.confirm_enabled);
            settlement_view_->SetRoundFinished(sync.round_finished);
            settlement_view_->Show(sync.show);
            if (sync.present) settlement_view_->Present();
        }

        if (fx.arrange) {
            UpdateActionAvailability();
            Arrange();
        }

        if (fx.intro.kind == OnlineIntroRequest::Kind::BeginResume && match_intro_) {
            match_intro_->Resume(fx.intro.state, fx.intro.type);
        }

        if (fx.focus_action_view && action_view_) action_view_->Focus();
        if (fx.focus_window) SetFocus(window_);
        if (fx.confirm_match_start && client_) client_->ConfirmMatchStart();
        if (fx.submit_action && client_) client_->SubmitAction(fx.submit_action->first, fx.submit_action->second);

        if (fx.audio_queue_overflow) {
            audio_error_ = "Online sound queue capacity exceeded";
            log_("ONLINE AUDIO ERROR " + audio_error_);
            StopAudio();
        }
        for (unsigned sound_id: fx.play_sounds) ScheduleSound(sound_id, 0);
        if (fx.start_deferred_sound_timer && !sequencer_.deferred_sounds().empty())
            SetTimer(window_, 2, 25, nullptr);

        if (fx.result_timer_ms.has_value()) {
            if (*fx.result_timer_ms == 0) KillTimer(window_, 5);
            else if (!SetTimer(window_, 5, *fx.result_timer_ms, nullptr))
                throw std::runtime_error("Cannot start settlement delay");
        }
        if (fx.auto_discard_timer_ms.has_value()) {
            if (*fx.auto_discard_timer_ms == 0) KillTimer(window_, 10);
            else if (!SetTimer(window_, 10, *fx.auto_discard_timer_ms, nullptr))
                throw std::runtime_error("Cannot start automatic discard wait");
        }
        SyncTimers();

        if (fx.dirty_all) InvalidateRect(window_, nullptr, FALSE);
        else if (fx.dirty_board) {
            const auto bounds = BoardBounds();
            InvalidateRect(window_, &bounds, FALSE);
        }
        if (fx.update_lobby_music) UpdateLobbyMusic();
        if (fx.notified_snapshot && connection_changed_) connection_changed_(*fx.notified_snapshot);
    }

    void LobbyView::ApplyPreferences(const ClientPreferences &preferences, PreferenceChange change) {
        if (change == PreferenceChange::MusicPreviewBegin || change == PreferenceChange::MusicPreviewEnd) {
            music_preview_ = change == PreferenceChange::MusicPreviewBegin;
            if (music_preview_) lobby_music_.Stop();
            UpdateLobbyMusic();
            return;
        }
        preferences_ = preferences;
        settlement_view_->SetPeekEnabled(preferences.ResultPeek());
        settlement_view_->SetTileColor(preferences.TileColor());
        if (change == PreferenceChange::Artwork || change == PreferenceChange::Initial) {
            lobby_background_ = {};
            table_.SetPreferences(preferences, config_, change == PreferenceChange::Artwork);
            InvalidateRect(window_, nullptr, FALSE);
            if (service_lobby_ && !resource_directory_.empty())
                service_lobby_->ConfigureArtwork(resource_directory_, canvas_width_);
        }
        if (change == PreferenceChange::Sound) {
            sequencer_.set_sound_enabled(preferences.se_volume > 0);
            settlement_view_->SetSoundEnabled(sequencer_.EffectsEnabled());
            StopAudio();
            audio_.SetVolumePercent(preferences.se_volume);
            const auto voice_mode = GetPrivateProfileIntW(L"RESOURCE", L"SE_NOVOICE",
                                                          GetPrivateProfileIntW(
                                                              L"REBUILD", L"SoundEffectsOnly", 0, config_.c_str()),
                                                          config_.c_str());
            sequencer_.set_voice_mode(voice_mode);
            sound_loaded_ = false;
            audio_error_.clear();
            log_("ONLINE AUDIO SETTINGS enabled=" + std::to_string(sequencer_.sound_enabled()) +
                 " master=" + std::to_string(preferences.se_volume));
            UpdateLobbyMusic(true);
        }
    }

    LobbyView::LobbyView(HWND parent, const std::filesystem::path &config, std::function<void(const std::string &)> log)
        : parent_(parent), config_(config), log_(std::move(log)), audio_(log_), lobby_music_(log_) {
        WNDCLASSEXW wc{sizeof(wc)};
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpfnWndProc = Procedure;
        wc.style = CS_DBLCLKS;
        wc.lpszClassName = kLobbyWindow;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            throw std::runtime_error("Cannot register lobby view");
        font_ = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        window_ = CreateWindowExW(WS_EX_CONTROLPARENT, kLobbyWindow, L"", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                  0, 0, 1024, 768, parent, reinterpret_cast<HMENU>(220), wc.hInstance, this);
        if (!font_ || !window_) throw std::runtime_error("Cannot create lobby view");
        auto add = [&](int id, const wchar_t *type, const wchar_t *text, DWORD style) {
            const HWND control = CreateWindowExW(0, type, text, WS_CHILD | WS_VISIBLE | style,
                                                 0, 0, 100, 25, window_,
                                                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), wc.hInstance,
                                                 nullptr);
            if (!control) throw std::runtime_error("Cannot create lobby control");
            controls_[id] = control;
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
        };
        const wchar_t *buttons[]{L"打牌", TextLabel(TextKey::button_normal_chi).data(),
            TextLabel(TextKey::button_normal_pon).data(), TextLabel(TextKey::button_normal_kan).data(),
            TextLabel(TextKey::button_normal_riichi).data(), TextLabel(TextKey::button_normal_ron).data(),
            TextLabel(TextKey::button_normal_tsumo).data(), TextLabel(TextKey::button_normal_abort_draw).data(),
            TextLabel(TextKey::button_normal_pass).data(), L"次局"};
        for (int i = 1; i <= 8; ++i)
            add(5120 + i, L"BUTTON", buttons[i], WS_TABSTOP | (i >= 1 && i <= 8 ? BS_OWNERDRAW : BS_PUSHBUTTON));
        add(5151, L"BUTTON", TextLabel(TextKey::button_normal_nuki).data(), WS_TABSTOP | BS_OWNERDRAW);
        constexpr TextKey suits[]{TextKey::button_normal_man, TextKey::button_normal_pin, TextKey::button_normal_sou};
        for (int i = 0; i < 3; ++i) add(5170 + i, L"BUTTON", TextLabel(suits[i]).data(), WS_TABSTOP | BS_OWNERDRAW);
        add(5173, L"BUTTON", TextLabel(TextKey::button_normal_exchange).data(), WS_TABSTOP | BS_OWNERDRAW);
        add(5174, L"BUTTON", L"结算", WS_TABSTOP | BS_PUSHBUTTON);
        add(5129, L"BUTTON", L"下一盘", WS_TABSTOP | BS_PUSHBUTTON);
        for (int id: {5170, 5171, 5172, 5173, 5129})
            SetWindowSubclass(controls_[id], ActionButtonProcedure, 1, reinterpret_cast<DWORD_PTR>(&log_));
        SetWindowSubclass(controls_[5151], ActionButtonProcedure, 1, reinterpret_cast<DWORD_PTR>(&log_));
        for (int i = 1; i <= 8; ++i)
            SetWindowSubclass(controls_[5120 + i], ActionButtonProcedure, 1, reinterpret_cast<DWORD_PTR>(&log_));
        add(5137, L"BUTTON", L"返回大厅", WS_TABSTOP | BS_PUSHBUTTON);
        add(0xbc6, L"BUTTON", L"EXIT", WS_TABSTOP | BS_OWNERDRAW);
        SetWindowSubclass(controls_[0xbc6], ActionButtonProcedure, 1, reinterpret_cast<DWORD_PTR>(&log_));
        exit_font_ = CreateOriginalUiFont();
        WNDCLASSEXW vote_class{sizeof(vote_class)};
        vote_class.hInstance = wc.hInstance;
        vote_class.lpfnWndProc = ExitVoteProcedure;
        vote_class.lpszClassName = L"TENHOU_EXIT_VOTE";
        vote_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
        vote_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        if (!RegisterClassExW(&vote_class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            throw std::runtime_error("Cannot register exit vote");
        RECT vote_bounds{0, 0, 420, 190};
        AdjustWindowRectEx(&vote_bounds, WS_POPUP | WS_CAPTION, FALSE, WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT);
        exit_vote_ = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT, vote_class.lpszClassName, L"投票退出对局",
                                     WS_POPUP | WS_CAPTION, 0, 0, vote_bounds.right - vote_bounds.left,
                                     vote_bounds.bottom - vote_bounds.top, parent_, nullptr, wc.hInstance, this);
        const auto vote_control = [&](const wchar_t *type, const wchar_t *text, int id, int x, int y, int w, int h,
                                      DWORD style) {
            const auto child = CreateWindowExW(0, type, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h, exit_vote_,
                                               reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), wc.hInstance,
                                               nullptr);
            if (!child) throw std::runtime_error("Cannot create exit vote control");
            SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
            return child;
        };
        if (!exit_vote_ || !exit_font_) throw std::runtime_error("Cannot create exit vote window");
        exit_vote_text_ = vote_control(L"STATIC", L"", 5162, 20, 18, 380, 105, SS_LEFT);
        exit_yes_ = vote_control(L"BUTTON", L"同意退出", 5163, 60, 135, 130, 30, WS_TABSTOP | BS_PUSHBUTTON);
        exit_no_ = vote_control(L"BUTTON", L"继续对局", 5164, 225, 135, 130, 30, WS_TABSTOP | BS_DEFPUSHBUTTON);
        sequencer_.set_animation_enabled(GetPrivateProfileIntW(L"REBUILD", L"NetworkAnimation", 1, config_.c_str()) != 0);
        settlement_view_ = std::make_unique<SettlementView>(window_);
        settlement_view_->SetBackgroundPainter([this](HDC dc, RECT r) {
            if (client_ && Snapshot().game.initialized)
                table_.Paint(dc, r, Snapshot().game, false);
        });
        settlement_view_->SetConfirmHandler([this] {
            const auto &a = Snapshot().actions;
            if (!a.pending.empty()) return;
            const auto next = std::find_if(a.choices.begin(), a.choices.end(),
                                           [](const OnlineAction &c) { return c.kind == "next"; });
            if (a.enabled && next != a.choices.end()) Submit(static_cast<std::size_t>(next - a.choices.begin()));
            else if (Snapshot().phase == LobbyPhase::MatchComplete && Snapshot().game.complete) {
                client_->SubmitLobby(Snapshot().lobby_revision, "return");
            } else {
                sequencer_.set_show_results(false);
                Update(Snapshot());
            }
        });
        action_view_ = std::make_unique<ActionSelectionView>(
            window_, [this](std::uint64_t revision, std::size_t index) {
                if (!match_intro_->Active() && !sequencer_.opening().Active() && !Moving() &&
                    !sequencer_.show_results() && sequencer_.selection().Active() &&
                    revision == sequencer_.selection().revision && revision == Snapshot().actions.revision)
                    Submit(index);
            }, [this] { CancelSelection(); });
        match_intro_ = std::make_unique<MatchIntroView>(window_, [this] {
            ApplyEffects(sequencer_.OnMatchIntroFinished(client_->snapshot(), MakeContext(), MakeMotion()));
        });
        match_intro_->SetSoundPlayer([this](unsigned id) { PlaySoundId(id); });
        settlement_view_->SetSoundPlayer([this](unsigned id) { PlaySoundId(id); });
        settlement_view_->SetSoundStopper([this] {
            try { audio_.Stop(); } catch (const std::exception &e) {
                log_(std::string("ONLINE AUDIO RESET ERROR ") + e.what());
            }
        });
        settlement_view_->SetSoundEnabled(sequencer_.sound_enabled());
        settlement_view_->SetAnimationEnabled(sequencer_.animation_enabled());
        matching_ = std::make_unique<MatchmakingView>(window_, log_);
        client_ = std::make_unique<NetworkClient>([this](const LobbySnapshot &snapshot) { Update(snapshot); }, log_,
                                                  [this](const NetworkMessage &message, const LobbySnapshot &snapshot) {
                                                      OnGameEvent(message, snapshot);
                                                  });
        matching_->Bind([this](const std::string &command, unsigned value,
                               const std::map<std::string, std::string> &settings) {
            if (command == "match" || command == "cancel")
                client_->SubmitLobby(client_->snapshot().lobby_revision, command, value);
            else client_->NativeRequest(command, value, settings);
        });
        Update(Snapshot());
        Arrange();
    }

    LobbyView::~LobbyView() {
        lobby_music_.Stop();
        StopSound();
        service_lobby_.reset();
        matching_.reset();
        client_.reset();
        match_intro_.reset();
        action_view_.reset();
        settlement_view_.reset();
        if (window_ && IsWindow(window_)) DestroyWindow(window_);
        if (exit_vote_) DestroyWindow(exit_vote_);
        if (exit_font_) DeleteObject(exit_font_);
        if (font_) DeleteObject(font_);
    }

    void LobbyView::EnableServiceLobby(std::function<void()> settings, std::function<void()> replay) {
        service_lobby_ = std::make_unique<OfflineLobbyView>(window_, config_,
            [this, settings = std::move(settings), replay = std::move(replay)](OfflineLobbyView::Action action) {
                switch (action) {
                    case OfflineLobbyView::Action::Settings: settings(); break;
                    case OfflineLobbyView::Action::OpenReplay: replay(); break;
                    case OfflineLobbyView::Action::Logout:
                        Disconnect();
                        PostMessageW(parent_, WM_APP + 25, 0, 0);
                        break;
                    case OfflineLobbyView::Action::RoomConfirmed: UpdateLobbyMusic(); break;
                }
            }, log_, [this](const std::string &command, const std::wstring &value) {
                client_->SubmitOriginLobby(command, value);
            });
    }

    void LobbyView::Login(const ConnectionOptions &options) {
        if (client_->active()) client_->Disconnect();
        const bool reconnect = client_->snapshot().reconnect_available && options.recover_games &&
                               !options.register_account && options.host == login_options_.host &&
                               options.port == login_options_.port && options.identity == login_options_.identity &&
                               options.gender == login_options_.gender && options.origin_lobby == login_options_.origin_lobby;
        login_options_ = options;
        if (reconnect) client_->Reconnect(login_options_);
        else client_->Connect(login_options_);
    }

    bool LobbyView::LoggedIn() const {
        const auto phase = client_->snapshot().phase;
        return phase == LobbyPhase::Lobby || phase == LobbyPhase::Game || phase == LobbyPhase::Matching ||
               phase == LobbyPhase::Cancelling || phase == LobbyPhase::ChangingLobby || phase ==
               LobbyPhase::MatchComplete || phase == LobbyPhase::AwaitReturn;
    }

    void LobbyView::Show(bool visible) {
        if (!visible) {
            sequencer_.set_hover_point(-1, -1);
            ApplyEffects(sequencer_.ResetActionClock());
            ApplyEffects(sequencer_.CancelResultTimer());
            StopSound();
            ClearPresentationQueue();
            matching_->Reset();
            if (service_lobby_) service_lobby_->Show(false);
        }
        if (!visible && exit_vote_) ShowWindow(exit_vote_, SW_HIDE);
        if (!visible && client_->active()) client_->Disconnect();
        ShowWindow(window_, visible ? SW_SHOW : SW_HIDE);
        if (visible) Arrange();
        UpdateLobbyMusic();
        if (visible) PostMessageW(window_, WM_APP + 18, 0, 0);
    }

    void LobbyView::SetWelcomeMusic(bool visible, bool official) {
        welcome_music_ = visible;
        official_welcome_ = official;
        UpdateLobbyMusic();
    }

    void LobbyView::Disconnect() {
        if (client_->active() || Snapshot().reconnect_available) client_->Disconnect();
    }

    void LobbyView::Save() {
        if (!WritePrivateProfileStringW(L"REBUILD", L"NetworkAnimation",
                                        sequencer_.animation_enabled() ? L"1" : L"0", config_.c_str()))
            throw std::runtime_error("Cannot save server preferences");
    }

    void LobbyView::Resize(int width, int height) {
        MoveWindow(window_, 0, 0, width, height, TRUE);
        if (service_lobby_) service_lobby_->Resize(width, height);
        Arrange();
    }

    void LobbyView::ConfigureArtwork(const std::filesystem::path &directory, int width) {
        CancelHandDrag();
        sequencer_.captions().Clear();
        CancelSelection();
        SyncTimers();
        action_view_->Configure(directory, width);
        match_intro_->Configure(directory, width);
        StopSound();
        sounds_ = {};
        sound_loaded_ = false;
        audio_error_.clear();
        settlement_view_->Configure(directory, width);
        action_artwork_.Configure(directory, width);
        matching_->ConfigureArtwork(directory);
        if (service_lobby_) service_lobby_->ConfigureArtwork(directory, width);
        action_artwork_error_.clear();
        resource_directory_ = directory;
        UpdateLobbyMusic(true);
        canvas_width_ = width;
        lobby_background_ = {};
        table_.Configure(directory, width);
        artwork_error_.clear();
        artwork_painted_ = false;
        InvalidateRect(window_, nullptr, FALSE);
        PostMessageW(window_, WM_APP + 18, 0, 0);
    }

    void LobbyView::Arrange() {
        if (!matching_ || !client_) return;
        RECT r{};
        GetClientRect(window_, &r);
        const bool game = Snapshot().game.initialized;
        const auto &session = client_->snapshot();
        const bool service = service_lobby_ && session.origin_lobby.has_value();
        const bool lobby_ready = session.phase == LobbyPhase::Lobby || session.phase == LobbyPhase::Matching ||
                                 session.phase == LobbyPhase::Cancelling || session.phase == LobbyPhase::ChangingLobby;
        matching_->Move(r);
        matching_->Show(!game && !service && (!session.official || session.phase == LobbyPhase::Error));
        if (service_lobby_) service_lobby_->Show(service && !session.game.started && !session.recovering &&
                                                 lobby_ready && IsWindowVisible(window_));
        for (const auto &item: controls_) ShowWindow(item.second, SW_HIDE);
        const auto board = BoardBounds();
        settlement_view_->Move(board);
        action_view_->Move(board);
        const bool intro_active = match_intro_ && match_intro_->Active();
        const bool table_anim = table_.Animating();
        if (game && !sequencer_.show_results()) {
            if (Snapshot().variant_active) {
                const auto &s = Snapshot();
                const auto target = FitNativeCanvas(board, canvas_width_);
                const auto anchor = table_.ActionAnchor(s.game);
                const int stride = canvas_width_ == 1440 ? 154 : 110;
                const int bw = canvas_width_ == 1440 ? 140 : 100, bh = canvas_width_ == 1440 ? 50 : 36;
                const int y = anchor.y - (canvas_width_ == 1440 ? 63 : 45);
                const auto place = [&](int id, int x) {
                    MoveWindow(controls_.at(id), target.left + x * (target.right - target.left) / canvas_width_,
                               target.top + y * (target.bottom - target.top) / (canvas_width_ * 3 / 4),
                               bw * (target.right - target.left) / canvas_width_,
                               bh * (target.bottom - target.top) / (canvas_width_ * 3 / 4), TRUE);
                    ShowWindow(controls_.at(id), SW_SHOW);
                };
                if (s.variant.phase == VariantPhase::Missing && LocalTableInput() && !Moving())
                    for (int suit = 0; suit < 3; ++suit) {
                        const auto found = std::find_if(s.actions.choices.begin(), s.actions.choices.end(),
                            [suit](const OnlineAction &a) { return a.kind == "dingque" && a.suit == suit; });
                        if (found != s.actions.choices.end()) {
                            place(5170 + suit, anchor.x + 65 - (2 - suit) * stride);
                            EnableWindow(controls_.at(5170 + suit),
                                         sequencer_.IsActionButtonEnabled(5170 + suit, session, intro_active, table_anim));
                        }
                    }
                if (s.variant.phase == VariantPhase::Exchange && LocalTableInput() && !Moving() &&
                    sequencer_.selection().group == "exchange" &&
                    sequencer_.selection().Confirm(s.actions) != static_cast<std::size_t>(-1)) {
                    place(5173, anchor.x + 65);
                    EnableWindow(controls_.at(5173),
                                 sequencer_.IsActionButtonEnabled(5173, session, intro_active, table_anim));
                }
                if (!s.game.settlement.receipts.empty() || !s.last_settlement.receipts.empty()) {
                    MoveWindow(controls_.at(5174), 12, 50, 80, 26, TRUE);
                    ShowWindow(controls_.at(5174), SW_SHOW);
                }
                if (s.variant.phase == VariantPhase::Ended) {
                    place(5129, anchor.x + 65);
                    EnableWindow(controls_.at(5129),
                                 sequencer_.IsActionButtonEnabled(5129, session, intro_active, table_anim));
                }
            }
            if (Snapshot().phase == LobbyPhase::MatchComplete) {
                MoveWindow(controls_.at(5137), r.right - 180, r.bottom - 42, 160, 28, TRUE);
                ShowWindow(controls_.at(5137), SW_SHOW);
            }
            const auto target = FitNativeCanvas(board, canvas_width_);
            const auto anchor = table_.ActionAnchor(Snapshot().game);
            for (const auto &p: BuildActionPlacements(Snapshot().actions, canvas_width_, {anchor.x, anchor.y})) {
                if (SuppressCallPrompt()) continue;
                if (p.button == 5120 || p.button == 5129) continue;
                const int x = target.left + p.rect.left * (target.right - target.left) / canvas_width_;
                const int y = target.top + p.rect.top * (target.bottom - target.top) / (canvas_width_ * 3 / 4);
                const int w = (p.rect.right - p.rect.left) * (target.right - target.left) / canvas_width_;
                const int h = (p.rect.bottom - p.rect.top) * (target.bottom - target.top) / (canvas_width_ * 3 / 4);
                MoveWindow(controls_.at(p.button), x, y, w, h, TRUE);
                ShowWindow(controls_.at(p.button), SW_SHOW);
            }
        }
        action_view_->Set(sequencer_.selection(), Snapshot().actions, Snapshot().game,
                          game && !sequencer_.show_results() && !SuppressCallPrompt() && !intro_active &&
                          !sequencer_.opening().Active() && !Moving());
        match_intro_->Move(r);
        if (game && session.exit_allowed && session.phase == LobbyPhase::Game && !intro_active) {
            const auto target = FitNativeCanvas(board, canvas_width_);
            HDC dc = GetDC(window_);
            const auto old = SelectObject(dc, exit_font_);
            SIZE size{};
            GetTextExtentPoint32W(dc, L"EXIT", 4, &size);
            SelectObject(dc, old);
            ReleaseDC(window_, dc);
            const int y = std::min<int>(r.bottom - 22, target.bottom - (canvas_width_ == 1440 ? 57 : 32));
            MoveWindow(controls_.at(0xbc6), target.left + 10, y, size.cx + 16, size.cy + 8, TRUE);
            EnableWindow(controls_.at(0xbc6),
                         sequencer_.IsActionButtonEnabled(0xbc6, session, intro_active, table_anim));
            ShowWindow(controls_.at(0xbc6), SW_SHOW);
            BringWindowToTop(controls_.at(0xbc6));
        }
    }

    bool LobbyView::Compact() const { return client_ && Snapshot().game.initialized; }

    RECT LobbyView::BoardBounds() const {
        RECT r{};
        GetClientRect(window_, &r);
        return r;
    }

    void LobbyView::SetOriginalDisplay(bool enabled) {
        CancelHandDrag();
        sequencer_.captions().Clear();
        CancelSelection();
        StopSound();
        match_intro_->Cancel();
        (void) enabled;
        sequencer_.set_original_display(true);
        table_.SetOriginalDisplay(true);
        SyncTimers();
        Arrange();
        InvalidateRect(window_, nullptr, FALSE);
    }

    void LobbyView::Submit(std::size_t index) {
        if (!client_) return;
        ApplyEffects(sequencer_.Submit(index, client_->snapshot(), MakeContext(), table_.Animating()));
    }

    void LobbyView::PreferenceInput(bool double_click) {
        if (!LocalTableInput() || Moving() || sequencer_.selection().Active() ||
            sequencer_.pressed_tile() >= 0 || sequencer_.pressed_auto() >= 0 || sequencer_.hand_dragged())
            return;
        preferences_ = ClientPreferences::Load(config_);
        const auto &snapshot = Snapshot();
        const auto index = PreferenceShortcut(preferences_, double_click, snapshot.game, snapshot.actions);
        if (index != static_cast<std::size_t>(-1)) {
            log_(std::string("ONLINE SHORTCUT ") + (double_click ? "double" : "right") + " revision=" +
                 std::to_string(snapshot.actions.revision));
            Submit(index);
        }
    }

    void LobbyView::CancelSelection() {
        const auto fx = sequencer_.CancelSelection();
        if (client_ && action_view_) {
            action_view_->Set(sequencer_.selection(), Snapshot().actions, Snapshot().game, false);
            InvalidateRect(window_, nullptr, FALSE);
        }
        if (fx.release_capture && GetCapture() == window_) ReleaseCapture();
    }

    bool LobbyView::LocalTableInput() const {
        return client_ && sequencer_.LocalTableInput(client_->snapshot(), match_intro_ && match_intro_->Active());
    }

    bool LobbyView::AutomaticInput() const {
        return client_ && sequencer_.AutomaticInput(client_->snapshot());
    }

    void LobbyView::CancelHandDrag() {
        const auto fx = sequencer_.CancelHandDrag();
        if (fx.release_capture && GetCapture() == window_) ReleaseCapture();
    }

    bool LobbyView::HandleKey(UINT key) {
        if (!IsWindowVisible(window_) || !client_) return false;
        if (service_lobby_ && service_lobby_->HandleKey(key)) return true;
        if (settlement_view_->HandleKey(key)) return true;
        if (!sequencer_.selection().Active()) return key == VK_ESCAPE && !Snapshot().actions.pending.empty();
        if (key == VK_ESCAPE) {
            CancelSelection();
            UpdateActions();
            Arrange();
            SetFocus(window_);
            return true;
        }
        return false;
    }

    void LobbyView::ActionClockTick() {
        if (!client_) return;
        ApplyEffects(sequencer_.ActionClockTick(client_->snapshot(), MakeContext()));
    }

    TableInteraction LobbyView::Interaction(const LobbySnapshot &snapshot) {
        const auto [hx, hy] = sequencer_.hover_point();
        return sequencer_.BuildInteraction(
            snapshot, client_->snapshot(), match_intro_ && match_intro_->Active(), table_.Animating(),
            HitAutomaticButton(hx, hy),
            [&] { return table_.HitTile(BoardBounds(), snapshot.game, hx, hy); },
            GetTickCount64());
    }

    int LobbyView::HitAutomaticButton(int x, int y) const {
        if (!Compact() || !AutomaticInput()) return -1;
        const auto target = FitNativeCanvas(BoardBounds(), canvas_width_);
        if (x < target.left || x >= target.right || y < target.top || y >= target.bottom) return -1;
        const POINT point{
            (x - target.left) * canvas_width_ / (target.right - target.left),
            (y - target.top) * (canvas_width_ * 3 / 4) / (target.bottom - target.top)
        };
        for (int i = 0; i < 4; ++i) {
            const RECT r = OriginalAutoButtonRect(canvas_width_, i);
            if (PtInRect(&r, point)) return i;
        }
        return -1;
    }

    bool LobbyView::SuppressCallPrompt() const {
        return client_ && sequencer_.SuppressCallPrompt(client_->snapshot());
    }

    void LobbyView::AutomaticAction() {
        if (!client_) return;
        ApplyEffects(sequencer_.AutomaticAction(client_->snapshot(), MakeContext(), table_.Animating()));
    }

    void LobbyView::ApplyAutomaticSort() {
        if (!client_) return;
        ApplyEffects(sequencer_.ApplyAutomaticSort(client_->snapshot(), MakeMotion()));
    }

    void LobbyView::UpdateActionAvailability() {
        if (!client_) return;
        const auto &live = client_->snapshot();
        const bool intro_active = match_intro_ && match_intro_->Active();
        const bool table_anim = table_.Animating();
        for (int i = 1; i <= 8; ++i)
            EnableWindow(controls_[5120 + i],
                         sequencer_.IsActionButtonEnabled(5120 + i, live, intro_active, table_anim));
        EnableWindow(controls_[5151], sequencer_.IsActionButtonEnabled(5151, live, intro_active, table_anim));
    }

    void LobbyView::UpdateActions() {
        if (!client_) return;
        (void) sequencer_.UpdateActions(client_->snapshot(), Compact());
        UpdateActionAvailability();
    }

    void LobbyView::Command(int id) {
        if (!client_) return;
        if (id == 0xbc6) {
            if (sequencer_.pressed_action() == id) {
                sequencer_.pressed_action() = -1;
                if (sequencer_.button_revision() != Snapshot().actions.revision) return;
            }
            client_->ExitGame();
            return;
        }
        if (id == 5163 || id == 5164) {
            client_->ExitGame(id == 5163 ? 1 : 0);
            UpdateExitVote();
            return;
        }
        if (id == 5137) {
            client_->SubmitLobby(Snapshot().lobby_revision, "return");
            return;
        }
        ApplyEffects(sequencer_.HandleCommand(id, client_->snapshot(), Compact(), MakeContext(), MakeMotion()));
    }

    const LobbySnapshot &LobbyView::Snapshot() const {
        return sequencer_.Snapshot(client_->snapshot());
    }

    void LobbyView::ClearPresentationQueue() {
        ApplyEffects(sequencer_.ClearPresentationQueue(MakeMotion()));
    }

    void LobbyView::Update(const LobbySnapshot &incoming) {
        if (!client_) return;
        ApplyEffects(sequencer_.SyncSnapshot(incoming, client_->snapshot(), MakeContext(), MakeMotion()));
    }

    void LobbyView::StopSound(bool preserve_resume) {
        motion_timer_.Stop();
        if (match_intro_ && !preserve_resume) {
            if (match_intro_->Resuming()) log_("ONLINE RESUME PREPARATION END");
            match_intro_->Cancel();
        }
        opening_timer_.Stop();
        sequencer_.ResetForStopSound();
        table_.CancelAnimation();
        if (settlement_view_) settlement_view_->CancelPresentation();
        KillTimer(window_, 2);
        StopAudio();
    }

    void LobbyView::StopAudio() {
        sequencer_.deferred_sounds().clear();
        try { audio_.Stop(); } catch (const std::exception &e) {
            log_(std::string("ONLINE AUDIO RESET ERROR ") + e.what());
        }
    }

    void LobbyView::SetQuickAudio(bool mute_effects, bool mute_music) {
        if (sequencer_.effects_muted() != mute_effects) {
            sequencer_.set_effects_muted(mute_effects);
            if (mute_effects) StopAudio();
            settlement_view_->SetSoundEnabled(sequencer_.EffectsEnabled());
        }
        if (music_muted_ != mute_music) {
            music_muted_ = mute_music;
            if (mute_music) lobby_music_.Stop();
            UpdateLobbyMusic(true);
        }
    }

    void LobbyView::UpdateLobbyMusic(bool refresh) {
        std::wstring key;
        bool third_party_lobby = false;
        if (!music_muted_ && !music_preview_ && !resource_directory_.empty()) {
            if (welcome_music_) {
                key = L"BGM_LOBBY0";
                third_party_lobby = !official_welcome_;
            } else if (offline_lobby_music_) {
                key = *offline_lobby_music_ ? L"BGM_LOBBY1" : L"BGM_LOBBY0";
            } else if (client_ && IsWindowVisible(window_)) {
                const auto &s = client_->snapshot();
                const auto &game = sequencer_.displaying() ? sequencer_.displayed().game : s.game;
                if ((s.phase == LobbyPhase::Game || s.phase == LobbyPhase::MatchComplete) && game.initialized &&
                    (!s.recovering || s.silent_recovery)) {
                    const int last = static_cast<int>(((game.type & 8u) | 6u) >> 1) - ((game.type & 16u) ? 1 : 0);
                    int index = game.round < last ? (game.round < 4 ? 0 : 1) : game.round == last ? 2 : 3;
                    const auto reaches = std::count_if(game.players.begin(), game.players.end(),
                                                       [](const OnlinePlayer &p) { return p.reach != 0; });
                    if (reaches) index = reaches == 1 ? 4 : 5;
                    if (settlement_view_->FinalPage()) index = 9;
                    key = L"BGM_GAME" + std::to_wstring(index);
                } else if (s.phase == LobbyPhase::Lobby || s.phase == LobbyPhase::Matching ||
                           s.phase == LobbyPhase::Cancelling || s.phase == LobbyPhase::ChangingLobby) {
                    third_party_lobby = s.native_service && !s.origin_lobby;
                    const bool custom = !s.lobby.empty() && s.lobby.front() == 'C';
                    key = custom && !third_party_lobby ? L"BGM_LOBBY1" : L"BGM_LOBBY0";
                }
            }
        }
        if (!refresh && lobby_music_key_ == key && third_party_music_ == third_party_lobby) return;
        lobby_music_key_ = key;
        third_party_music_ = third_party_lobby;
        try {
            if (key.empty()) {
                lobby_music_.Select(window_);
                return;
            }
            const auto path = MusicPath(resource_directory_, config_, key, third_party_lobby);
            const int master = std::clamp(
                static_cast<int>(GetPrivateProfileIntW(L"RESOURCE", L"BGM_VOL", 100, config_.c_str())), 0, 200);
            const int level = std::clamp(
                static_cast<int>(GetPrivateProfileIntW(L"RESOURCE", MusicOption(key).c_str(), 60, config_.c_str())),
                0, 200);
            lobby_music_.Select(window_, path, key == L"BGM_LOBBY0", (master * .01) * (level * .01));
        } catch (const std::exception &e) { log_(std::string("LOBBY BGM ERROR ") + e.what()); }
    }

    void LobbyView::SetOfflineLobbyMusic(std::optional<bool> contest) {
        offline_lobby_music_ = contest;
        UpdateLobbyMusic();
    }

    void LobbyView::ScheduleSound(unsigned id, unsigned delay) {
        if (!sequencer_.EffectsEnabled() || !audio_error_.empty() || !IsWindowVisible(window_)) return;
        try {
            if (!sound_loaded_) {
                sounds_.Load(resource_directory_, canvas_width_, config_);
                sound_loaded_ = true;
                log_("ONLINE AUDIO BANK loaded");
            }
            audio_.Schedule(id, sounds_.Get(id), delay);
            if (!SetTimer(window_, 2, 25, nullptr)) throw std::runtime_error("Cannot start online sound timer");
        } catch (const std::exception &e) {
            audio_error_ = e.what();
            log_("ONLINE AUDIO ERROR " + audio_error_);
            StopAudio();
        }
    }

    void LobbyView::PlaySoundId(unsigned id, unsigned delay) {
        OnlineSequencerEffects fx;
        sequencer_.QueueOrEmitSound(id, fx, IsWindowVisible(window_) != FALSE, audio_error_.empty());
        if (fx.audio_queue_overflow) {
            audio_error_ = "Online sound queue capacity exceeded";
            log_("ONLINE AUDIO ERROR " + audio_error_);
            StopAudio();
            return;
        }
        for (unsigned sound_id: fx.play_sounds) ScheduleSound(sound_id, delay);
    }

    void LobbyView::OnGameEvent(const NetworkMessage &message, const LobbySnapshot &snapshot) {
        if (snapshot.silent_recovery && snapshot.recovering) return;
        if (!snapshot.variant_active && message.tag == "EXITVOTE" && snapshot.exit_voting)
            exit_vote_deadline_ = GetTickCount64() + snapshot.exit_vote_ms;
        ApplyEffects(sequencer_.OnGameEvent(message, snapshot, client_->snapshot(), MakeContext(), MakeMotion()));
    }

    void LobbyView::PresentationTick() {
        if (!client_) return;
        ApplyEffects(sequencer_.PresentationTick(client_->snapshot(), MakeContext(), MakeMotion()));
    }

    void LobbyView::OpeningTick(double now) {
        if (!client_) return;
        auto ctx = MakeContext();
        ctx.anim_ms = now;
        ApplyEffects(sequencer_.OpeningTick(client_->snapshot(), ctx));
    }

    void LobbyView::Paint(HDC dc, bool table_only) {
        RECT r{};
        GetClientRect(window_, &r);
        if (!table_only) {
            FillRect(dc, &r, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
            if (sequencer_.original_display()) {
                try {
                    if (lobby_background_.bgra.empty())
                        lobby_background_ = BuildPreferredBackground(
                            resource_directory_, config_, canvas_width_, canvas_width_ * 3 / 4, preferences_);
                    Gdiplus::Graphics graphics(dc);
                    auto &bg = lobby_background_;
                    Gdiplus::Bitmap bitmap(bg.width, bg.height, bg.width * 4, PixelFormat32bppARGB, bg.bgra.data());
                    graphics.DrawImage(&bitmap, 0, 0, r.right, r.bottom);
                } catch (const std::exception &) {
                    /* Table/connection status retains the actionable resource error. */
                }
            }
        }
        const auto old = SelectObject(dc, font_);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, sequencer_.original_display() ? RGB(245, 245, 235) : RGB(25, 35, 45));
        if (client_ && Snapshot().game.initialized && !sequencer_.show_results()) {
            auto bounds = BoardBounds();
            try {
                const auto interaction = Interaction(Snapshot());
                table_.Paint(dc, bounds, Snapshot().game, Snapshot().actions.enabled,
                             sequencer_.animation_enabled() ? &sequencer_.opening() : nullptr, &interaction,
                             AnimationNow(), &sequencer_.captions(), GetDpiForWindow(window_));
                artwork_error_.clear();
                painted_countdown_ = interaction.countdown;
                painted_hover_ = interaction.hovered;
                painted_auto_hover_ = interaction.hovered_button;
                if (!artwork_painted_) {
                    log_("ONLINE ARTWORK rendered");
                    artwork_painted_ = true;
                }
            } catch (const std::exception &error) {
                artwork_painted_ = false;
                if (artwork_error_ != error.what()) log_("ONLINE ARTWORK ERROR " + std::string(error.what()));
                artwork_error_ = error.what();
                const auto text = NetworkWide("Table artwork failed: " + artwork_error_);
                SetTextColor(dc, RGB(150, 35, 25));
                DrawTextW(dc, text.c_str(), -1, &bounds, DT_LEFT | DT_WORDBREAK);
            }
        }
        SelectObject(dc, old);
        if (Snapshot().table_paused) {
            const auto previous = SelectObject(dc, font_);
            SetTextColor(dc, RGB(255, 220, 120));
            SetBkMode(dc, TRANSPARENT);
            RECT message{20, 20, r.right - 20, 65};
            DrawTextW(dc, client_->snapshot().exit_voting ? L"对局已暂停，等待退出投票" : L"对局已暂停，等待断线玩家重新登录",
                      -1, &message, DT_CENTER | DT_WORDBREAK);
            SelectObject(dc, previous);
        }
    }

    void LobbyView::PaintAction(const DRAWITEMSTRUCT &item) {
        bool rendered = false;
        if (sequencer_.original_display() && ActionButtonText(item.CtlID, false)) {
            try {
                POINT point{};
                GetCursorPos(&point);
                ScreenToClient(item.hwndItem, &point);
                const bool hot = IsWindowEnabled(item.hwndItem) && PtInRect(&item.rcItem, point);
                const auto key = ActionButtonText(item.CtlID, hot);
                Gdiplus::Graphics graphics(item.hDC);
                const auto paint = [&](unsigned resource) {
                    const auto &im = action_artwork_.Get(resource);
                    Gdiplus::Bitmap bitmap(im.width, im.height, im.width * 4, PixelFormat32bppARGB,
                                           const_cast<BYTE *>(im.bgra.data()));
                    const int offset = item.itemState & ODS_SELECTED ? 1 : 0;
                    if (graphics.DrawImage(&bitmap, item.rcItem.left + offset, item.rcItem.top + offset,
                                           item.rcItem.right - item.rcItem.left,
                                           item.rcItem.bottom - item.rcItem.top) != Gdiplus::Ok)
                        throw std::runtime_error("Cannot draw native action button");
                };
                FillRect(item.hDC, &item.rcItem, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
                paint(440);
                if (!key) throw std::runtime_error("Unmapped action button text");
                auto &renderer = action_artwork_.Text();
                auto layout = renderer.Layout(*key);
                const auto &ink = layout.metrics.ink;
                const double sd = canvas_width_ == 1024 ? 1.0 / 1.4 : 1;
                const double fit = double(item.rcItem.right - item.rcItem.left) / (canvas_width_ == 1440 ? 140 : 100);
                TextRenderContext context;
                context.native_layout_scale = sd;
                context.viewport_scale_x = context.viewport_scale_y = fit;
                context.dpi = GetDpiForWindow(item.hwndItem);
                const float offset = item.itemState & ODS_SELECTED ? 1.f : 0.f;
                context.origin_x = (item.rcItem.left + item.rcItem.right) / 2.0 + offset;
                context.origin_y = (item.rcItem.top + item.rcItem.bottom) / 2.0 + offset;
                context.device_clip = Gdiplus::RectF(static_cast<float>(item.rcItem.left), static_cast<float>(item.rcItem.top),
                    static_cast<float>(item.rcItem.right - item.rcItem.left), static_cast<float>(item.rcItem.bottom - item.rcItem.top));
                SceneTextRun run;
                run.layout = layout;
                run.transform[4] = -ink.X - ink.Width / 2;
                run.transform[5] = -ink.Y - ink.Height / 2;
                renderer.Draw(graphics, context, run);
                if (item.itemState & ODS_DISABLED) {
                    Gdiplus::SolidBrush dim(Gdiplus::Color(150, 0, 0, 0));
                    graphics.FillRectangle(&dim, 0, 0, item.rcItem.right, item.rcItem.bottom);
                }
                rendered = true;
                action_artwork_error_.clear();
            } catch (const std::exception &e) {
                if (action_artwork_error_ != e.what()) log_(std::string("ACTION ARTWORK ERROR ") + e.what());
                action_artwork_error_ = e.what();
            }
        }
        if (!rendered) {
            RECT bounds = item.rcItem;
            DrawFrameControl(item.hDC, &bounds, DFC_BUTTON,
                             DFCS_BUTTONPUSH | ((item.itemState & ODS_SELECTED) ? DFCS_PUSHED : 0));
            const auto font = SelectObject(item.hDC, font_);
            SetBkMode(item.hDC, TRANSPARENT);
            SetTextColor(item.hDC, GetSysColor((item.itemState & ODS_DISABLED) ? COLOR_GRAYTEXT : COLOR_BTNTEXT));
            const auto text = Text(item.hwndItem);
            DrawTextW(item.hDC, text.c_str(), -1, &bounds, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(item.hDC, font);
        }
        if (item.itemState & ODS_FOCUS) {
            RECT r = item.rcItem;
            InflateRect(&r, -3, -3);
            DrawFocusRect(item.hDC, &r);
        }
    }

    void LobbyView::UpdateExitVote() {
        if (!client_ || !exit_vote_) return;
        const auto &s = client_->snapshot();
        if (!s.exit_voting || s.recovering || !IsWindowVisible(window_)) {
            ShowWindow(exit_vote_, SW_HIDE);
            KillTimer(window_, 11);
            return;
        }
        auto now = GetTickCount64();
        auto seconds = exit_vote_deadline_ > now ? (exit_vote_deadline_ - now + 999) / 1000 : 0;
        std::wstring text = L"是否终止本场对局？对局和出牌计时已暂停。\r\n剩余 " + std::to_wstring(seconds) + L" 秒（30 秒）\r\n同意 " +
                            std::to_wstring(s.exit_yes) + L" / 反对 " + std::to_wstring(s.exit_no) + L" / 真人 " +
                            std::to_wstring(s.exit_total) +
                            L"；需超过半数同意。\r\n" + (s.exit_choice < 0
                                                    ? L"请投票；未投票不计为同意。"
                                                    : s.exit_choice
                                                          ? L"你已同意退出，等待其他玩家。"
                                                          : L"你已选择继续，等待其他玩家。");
        SetWindowTextW(exit_vote_text_, text.c_str());
        EnableWindow(exit_yes_, s.exit_choice < 0 && !s.exit_ballot_pending && seconds > 0);
        EnableWindow(exit_no_, s.exit_choice < 0 && !s.exit_ballot_pending && seconds > 0);
        if (!IsWindowVisible(exit_vote_)) {
            RECT r{}, popup{};
            GetWindowRect(parent_, &r);
            GetWindowRect(exit_vote_, &popup);
            SetWindowPos(exit_vote_, HWND_TOP, r.left + (r.right - r.left - (popup.right - popup.left)) / 2,
                         r.top + (r.bottom - r.top - (popup.bottom - popup.top)) / 2, 0, 0,
                         SWP_NOSIZE | SWP_SHOWWINDOW);
            SetFocus(exit_no_);
        }
        SetTimer(window_, 11, 100, nullptr);
    }

    LRESULT CALLBACK LobbyView::ExitVoteProcedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
        auto *self = reinterpret_cast<LobbyView *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = static_cast<LobbyView *>(reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self && message == WM_COMMAND) {
            PostMessageW(self->window_, WM_COMMAND, wp, lp);
            return 0;
        }
        if (message == WM_CLOSE) return 0; // The server deadline resolves an abstention.
        return DefWindowProcW(window, message, wp, lp);
    }

    void LobbyView::Capture(HDC dc) {
        Paint(dc);
        matching_->Capture(dc);
        if (service_lobby_) service_lobby_->Capture(dc);
        settlement_view_->Capture(dc);
        POINT origin{};
        ClientToScreen(window_, &origin);
        for (const auto &item: controls_) {
            if (!IsWindowVisible(item.second)) continue;
            RECT r{};
            GetWindowRect(item.second, &r);
            const int saved = SaveDC(dc);
            SetViewportOrgEx(dc, r.left - origin.x, r.top - origin.y, nullptr);
            SendMessageW(item.second, WM_PRINT, reinterpret_cast<WPARAM>(dc),
                         PRF_CLIENT | PRF_NONCLIENT | PRF_ERASEBKGND);
            RestoreDC(dc, saved);
        }
        match_intro_->Capture(dc);
        action_view_->Capture(dc);
    }

    LRESULT CALLBACK LobbyView::Procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        auto *self = reinterpret_cast<LobbyView *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = static_cast<LobbyView *>(reinterpret_cast<CREATESTRUCTW *>(lparam)->lpCreateParams);
            self->window_ = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self) {
            try {
                if (message == WM_ERASEBKGND) return 1; // Paint supplies the complete background.
                if (message == WM_MOUSEMOVE || message == WM_MOUSELEAVE || message == WM_LBUTTONDOWN ||
                    (message == WM_TIMER && wparam == 9)) {
                    if (self->sequencer_.original_display() &&
                        self->hud_interaction_.Handle(window, self->BoardBounds(), 1440, message, lparam)) {
                        self->table_.SetRelativeScores(self->hud_interaction_.relative());
                        const auto bounds = self->BoardBounds();
                        InvalidateRect(window, &bounds, FALSE);
                    }
                    if (message == WM_TIMER) return 0;
                }
                if (message == WM_PRINTCLIENT) {
                    self->Paint(reinterpret_cast<HDC>(wparam));
                    return 0;
                }
                if (message == WM_DRAWITEM) {
                    const auto &item = *reinterpret_cast<DRAWITEMSTRUCT *>(lparam);
                    if (item.CtlID == 0xbc6) {
                        PaintOriginalButton(item, self->exit_font_, L"EXIT");
                        return TRUE;
                    }
                    if (item.CtlType == ODT_BUTTON &&
                        ((item.CtlID >= 5121 && item.CtlID <= 5128) || item.CtlID == 5151 ||
                         (item.CtlID >= 5170 && item.CtlID <= 5173))) {
                        self->PaintAction(item);
                        return TRUE;
                    }
                }
                if (message == WM_CTLCOLORSTATIC && self->sequencer_.original_display()) {
                    const auto dc = reinterpret_cast<HDC>(wparam);
                    SetTextColor(dc, RGB(245, 245, 235));
                    SetBkColor(dc, RGB(24, 45, 44));
                    SetDCBrushColor(dc, RGB(24, 45, 44));
                    return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));
                }
                if (message == WM_SIZE) {
                    self->Arrange();
                    return 0;
                }
                if (message == WM_APP + 16) {
                    self->UpdateActions();
                    self->Arrange();
                    return 0;
                }
                if (message == WM_APP + 17) {
                    self->ApplyAutomaticSort();
                    return 0;
                }
                if (message == WM_APP + 26) {
                    self->sequencer_.pressed_action() = static_cast<int>(wparam);
                    self->sequencer_.button_revision() = self->Snapshot().actions.revision;
                    return 0;
                }
                if (message == MM_WOM_DONE) {
                    self->lobby_music_.BufferDone();
                    return 0;
                }
                if (message == WM_APP + 18) {
                    self->UpdateLobbyMusic();
                    return 0;
                }
                if (message == WM_RBUTTONUP && self->sequencer_.selection().Active()) {
                    self->CancelSelection();
                    self->UpdateActions();
                    self->Arrange();
                    return 0;
                }
                if (message == WM_RBUTTONUP || message == WM_LBUTTONDBLCLK) {
                    const int x = static_cast<short>(LOWORD(lparam)), y = static_cast<short>(HIWORD(lparam));
                    const auto board = self->BoardBounds();
                    const POINT point{x, y};
                    if (PtInRect(&board, point) && self->HitAutomaticButton(x, y) < 0)
                        self->PreferenceInput(message == WM_LBUTTONDBLCLK);
                    return 0;
                }
                if (message == WM_MOUSEMOVE) {
                    const int x = static_cast<short>(LOWORD(lparam)), y = static_cast<short>(HIWORD(lparam));
                    self->sequencer_.set_hover_point(x, y);
                    TRACKMOUSEEVENT track{sizeof(TRACKMOUSEEVENT), TME_LEAVE, window, 0};
                    TrackMouseEvent(&track);
                    if (self->client_) {
                        if (self->sequencer_.pressed_tile() >= 0 && GetCapture() == window &&
                            self->LocalTableInput() &&
                            (!self->table_.Animating() || self->table_.ManualAnimation()) &&
                            !self->sequencer_.selection().Active()) {
                            const int target = self->table_.HitTile(self->BoardBounds(), self->Snapshot().game, x, y);
                            if (self->table_.MoveHandTile(self->sequencer_.pressed_tile(), target,
                                                          self->Snapshot().game)) {
                                self->sequencer_.set_automatic(0, false);
                                self->sequencer_.hand_dragged() = true;
                                if (self->sequencer_.animation_enabled() && self->table_.Animating())
                                    self->motion_timer_.Start(window, 6);
                                else self->table_.CancelAnimation();
                                self->UpdateActionAvailability();
                                self->log_("ONLINE HAND MOVE tile=" + std::to_string(self->sequencer_.pressed_tile()) +
                                           " target=" + std::to_string(target) + " sort=0");
                                InvalidateRect(window, nullptr, FALSE);
                            }
                        }
                        const auto interaction = self->Interaction(self->Snapshot());
                        if (interaction.hovered != self->painted_hover_ ||
                            interaction.hovered_button != self->painted_auto_hover_)
                            InvalidateRect(window, nullptr, FALSE);
                    }
                    return 0;
                }
                if (message == WM_MOUSELEAVE) {
                    self->sequencer_.set_hover_point(-1, -1);
                    InvalidateRect(window, nullptr, FALSE);
                    return 0;
                }
                if (message == WM_CAPTURECHANGED || message == WM_CANCELMODE) {
                    self->CancelHandDrag();
                    return 0;
                }
                if (message == WM_LBUTTONDOWN && (self->AutomaticInput() || self->LocalTableInput())) {
                    self->CancelHandDrag();
                    const int x = static_cast<short>(LOWORD(lparam)), y = static_cast<short>(HIWORD(lparam));
                    self->sequencer_.pressed_auto() = self->HitAutomaticButton(x, y);
                    self->sequencer_.pressed_tile() = self->sequencer_.pressed_auto() < 0 && self->LocalTableInput()
                                                          ? self->table_.HitTile(self->BoardBounds(),
                                                                                 self->Snapshot().game, x, y)
                                                          : -1;
                    self->sequencer_.pressed_revision() = self->Snapshot().actions.revision;
                    if (self->sequencer_.pressed_tile() >= 0 || self->sequencer_.pressed_auto() >= 0)
                        SetCapture(window);
                    return 0;
                }
                if (message == WM_TIMER && wparam == 11) {
                    self->UpdateExitVote();
                    return 0;
                }
                if (message == WM_TIMER && self->client_ && self->client_->snapshot().table_paused &&
                    wparam >= 2 && wparam <= 10)
                    return 0;
                if (message == WM_TIMER && wparam == 8) {
                    self->sequencer_.captions().Tick();
                    self->SyncTimers();
                    const auto bounds = self->BoardBounds();
                    InvalidateRect(window, &bounds, FALSE);
                    return 0;
                }
                if (message == WM_TIMER && wparam == 3) {
                    self->ActionClockTick();
                    if (self->Interaction(self->Snapshot()).countdown != self->painted_countdown_)
                        InvalidateRect(window, nullptr, FALSE);
                    return 0;
                }
                if (message == WM_TIMER && wparam == 7) {
                    self->PresentationTick();
                    return 0;
                }
                if (message == WM_TIMER && wparam == 10) {
                    KillTimer(window, 10);
                    self->AutomaticAction();
                    return 0;
                }
                if (message == WM_TIMER && wparam == 4) {
                    self->AutomaticAction();
                    return 0;
                }
                if (message == WM_TIMER && wparam == 5) {
                    if (self->client_) {
                        self->ApplyEffects(self->sequencer_.ResultTimerExpired(
                            self->client_->snapshot(), self->MakeContext(), self->MakeMotion()));
                    }
                    return 0;
                }
                if (message == WM_TIMER && wparam == kOpeningTimer) {
                    self->OpeningTick(AnimationNow());
                    return 0;
                }
                if (message == WM_TIMER && wparam == 2) {
                    if (const auto id = self->sequencer_.PopDeferredSound(self->match_intro_->Active()))
                        self->PlaySoundId(*id);
                    try { self->audio_.Poll(); } catch (const std::exception &e) {
                        self->audio_error_ = e.what();
                        self->log_("ONLINE AUDIO ERROR " + self->audio_error_);
                        self->StopAudio();
                    }
                    if (self->sequencer_.deferred_sounds().empty() && !self->audio_.Active())
                        KillTimer(window, 2);
                    return 0;
                }
                if (message == WM_TIMER && wparam == 6) {
                    const bool active = self->table_.Animating();
                    if (self->table_.Tick()) {
                        self->log_("ONLINE ANIMATION END");
                        self->UpdateActions();
                        self->Arrange();
                    }
                    if (!self->table_.Animating()) self->motion_timer_.Stop();
                    if (active) {
                        const auto bounds = self->BoardBounds();
                        InvalidateRect(window, &bounds, FALSE);
                    }
                    return 0;
                }
                if (message == WM_LBUTTONUP && self->client_ && !self->sequencer_.show_results()) {
                    const bool dragged = self->sequencer_.hand_dragged();
                    const int pressed_auto = self->sequencer_.pressed_auto();
                    const int pressed_tile = self->sequencer_.pressed_tile();
                    const auto pressed_revision = self->sequencer_.pressed_revision();
                    self->CancelHandDrag();
                    if (dragged) return 0;
                    const int button = self->HitAutomaticButton(static_cast<short>(LOWORD(lparam)),
                                                                static_cast<short>(HIWORD(lparam)));
                    if (button >= 0 && button == pressed_auto) {
                        const bool next_value = !self->sequencer_.automatic()[button];
                        self->sequencer_.set_automatic(button, next_value);
                        if (button == 0 && !self->sequencer_.sort_refresh_pending()) {
                            self->sequencer_.set_sort_refresh_pending(true);
                            self->UpdateActionAvailability();
                            PostMessageW(window, WM_APP + 17, 0, 0);
                        }
                        self->log_("ONLINE AUTO index=" + std::to_string(button) + " enabled=" +
                                   std::to_string(next_value));
                        if (self->SuppressCallPrompt()) self->CancelSelection();
                        self->UpdateActions();
                        self->Arrange();
                        self->SyncTimers();
                        const auto bounds = self->BoardBounds();
                        InvalidateRect(window, &bounds, FALSE);
                        return 0;
                    }
                    if (!self->LocalTableInput() || self->Moving() ||
                        pressed_revision != self->Snapshot().actions.revision)
                        return 0;
                    const auto bounds = self->BoardBounds();
                    const auto &snapshot = self->Snapshot();
                    const int tile = self->table_.HitTile(bounds, snapshot.game, static_cast<short>(LOWORD(lparam)),
                                                          static_cast<short>(HIWORD(lparam)));
                    if (tile != pressed_tile || tile < 0) return 0;
                    const auto &choices = snapshot.actions.choices;
                    if (self->sequencer_.selection().HandSelection()) {
                        const auto index = self->sequencer_.selection().Pick(snapshot.actions, tile);
                        self->log_("ACTION HAND PICK tile=" + std::to_string(tile) + " count=" +
                                   std::to_string(self->sequencer_.selection().selected.size()));
                        if (index != static_cast<std::size_t>(-1)) self->Submit(index);
                        else {
                            self->Arrange();
                            InvalidateRect(window, nullptr, FALSE);
                        }
                        return 0;
                    }
                    const std::string kind = self->sequencer_.action_filter() == "riichi" ? "riichi" : "discard";
                    for (std::size_t i = 0; i < choices.size(); ++i)
                        if (choices[i].kind == kind && choices[i].tiles == std::vector<int>{tile}) {
                            self->Submit(i);
                            break;
                        }
                    return 0;
                }
                if (message == WM_APP + 25) {
                    PostMessageW(self->parent_, WM_APP + 25, 0, 0);
                    return 0;
                }
                if (message == WM_COMMAND) {
                    self->Command(LOWORD(wparam));
                    return 0;
                }
                if (message == WM_PAINT) {
                    RECT update{};
                    GetUpdateRect(window, &update, FALSE);
                    const auto board = self->BoardBounds();
                    const bool table_only = self->Snapshot().game.initialized && !self->sequencer_.show_results() &&
                                            update.left >= board.left && update.top >= board.top &&
                                            update.right <= board.right && update.bottom <= board.bottom;
                    self->paint_buffer_.Paint(window, [&](HDC dc) { self->Paint(dc, table_only); });
                    return 0;
                }
            } catch (const std::exception &error) {
                const auto text = NetworkWide(error.what());
                self->log_("NETWORK view error: " + NetworkUtf8(text));
                return 0;
            }
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }
} // namespace tenhou
