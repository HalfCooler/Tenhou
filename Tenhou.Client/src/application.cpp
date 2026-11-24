#include "application.hpp"
#include <cstring>
#include <cwctype>
#include "welcome_view.hpp"
#include "table_view.hpp"
#include "lobby_view.hpp"
#include "offline_lobby_view.hpp"
#include "settings_view.hpp"
#include "window_layout.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace tenhou {
    namespace {
        constexpr const char *kWindowClass = "TENHOU_CLIENT";
        constexpr DWORD kWindowStyle = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        constexpr UINT kReplay = 4004, kWelcome = 4001, kSettings = 4023;
        constexpr UINT kOfflineLobby = 4024;
        constexpr UINT kAudioMeter = 4030, kToggleEffects = 4031, kToggleMusic = 4032;
        constexpr UINT_PTR kAudioTimer = 1;
        constexpr int kMeterWidth = 14, kMeterHeight = 16, kMeterInterior = 14;
        constexpr unsigned kMeterPeriodMs = 50;

        std::filesystem::path ExecutableDirectory() {
            std::vector<char> buffer(32768);
            const DWORD size = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            if (!size || size >= buffer.size()) throw std::runtime_error("Cannot resolve executable directory");
            return std::filesystem::path(std::string(buffer.data(), size)).parent_path();
        }
    } // namespace

    Options ParseOptions(int argc, char **argv) {
        (void) argv;
        Options options;
        int count = 0;
        auto **arguments = CommandLineToArgvW(GetCommandLineW(), &count);
        if (!arguments || count != argc) {
            if (arguments) LocalFree(arguments);
            throw std::runtime_error("无法读取启动参数");
        }
        try {
            for (int i = 1; i < count; ++i) {
                const std::wstring name = arguments[i];
                if (name == L"--offline-lobby") {
                    options.offline_lobby = true;
                    continue;
                }
                if (name == L"--service-lobby") {
                    options.service_lobby = true;
                    continue;
                }
                if (name != L"--config-dir" && name != L"--resources" && name != L"--replay") throw std::runtime_error(
                    "不支持的启动参数");
                if (++i >= count) throw std::runtime_error("启动参数缺少路径");
                const auto value = std::filesystem::absolute(arguments[i]);
                if (name == L"--config-dir") options.config_directory = value;
                else if (name == L"--resources") options.resource_directory = value;
                else options.replay = value;
            }
        } catch (...) {
            LocalFree(arguments);
            throw;
        }
        LocalFree(arguments);
        return options;
    }

    Application::Application(Options options) : options_(std::move(options)) {
        executable_directory_ = ExecutableDirectory();
        if (options_.config_directory.empty()) options_.config_directory = executable_directory_;
        std::filesystem::create_directories(options_.config_directory);
        config_path_ = options_.config_directory / L"config.ini";
        width_ = kDefaultClientWidth;
        height_ = kDefaultClientHeight;
        screen_width_ = 1440;
        view_ = options_.replay.empty() ? "welcome" : "replay";
        sound_effects_only_ = GetPrivateProfileIntW(L"REBUILD", L"SoundEffectsOnly", 0, config_path_.c_str()) != 0;
        resource_directory_ = options_.resource_directory.empty()
                                  ? std::filesystem::path(PreferenceString(
                                      config_path_, L"REBUILD", L"ResourceDirectory",
                                      (executable_directory_ / L"resources").c_str()))
                                  : options_.resource_directory;
        if (resource_directory_.is_relative()) resource_directory_ = options_.config_directory / resource_directory_;
        resource_directory_ = std::filesystem::absolute(resource_directory_).lexically_normal();
    }

    Application::~Application() {
        settings_.reset();
        PlaySoundA(nullptr, nullptr, 0);
        welcome_.reset();
        table_.reset();
        offline_lobby_.reset();
        lobby_.reset();
        if (gdiplus_token_) Gdiplus::GdiplusShutdown(gdiplus_token_);
        if (font_) DeleteObject(font_);
        if (mutex_) CloseHandle(mutex_);
    }


    void Application::Log(const std::string &message) {
        SYSTEMTIME time{};
        GetLocalTime(&time);
        log_ << time.wHour << ':' << time.wMinute << ':' << time.wSecond << ' ' << message << '\n';
        log_.flush();
    }

    void Application::Check(bool success, const char *action) const {
        if (!success) throw std::runtime_error(
            std::string(action) + " failed; Win32 error=" + std::to_string(GetLastError()));
    }

    int Application::Run(HINSTANCE instance, int show) {
        Check(SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) != FALSE,
              "SetProcessDpiAwarenessContext");
        // Separate identity from the original client. Atomic mutex also covers startup races.
        SetLastError(ERROR_SUCCESS);
        auto canonical = std::filesystem::weakly_canonical(options_.config_directory).wstring();
        std::transform(canonical.begin(), canonical.end(), canonical.begin(), [](wchar_t c) {
            return static_cast<wchar_t>(towlower(c));
        });
        std::vector<std::uint8_t> identity(canonical.size() * sizeof(wchar_t));
        std::memcpy(identity.data(), canonical.data(), identity.size());
        const std::string instance_name = "Local\\TenhouRebuild.Config." + Sha256(identity);
        instance_property_ = instance_name;
        mutex_ = CreateMutexA(nullptr, FALSE, instance_name.c_str());
        const DWORD mutex_error = GetLastError();
        Check(mutex_ != nullptr, "CreateMutexA");
        if (mutex_error == ERROR_ALREADY_EXISTS) {
            EnumWindows([](HWND window, LPARAM context)-> BOOL {
                if (GetPropA(window, reinterpret_cast<const char *>(context))) {
                    ShowWindow(window, SW_RESTORE);
                    SetForegroundWindow(window);
                    return FALSE;
                }
                return TRUE;
            }, reinterpret_cast<LPARAM>(instance_name.c_str()));
            return 0;
        }
        log_.open(options_.config_directory / "rebuild.log", std::ios::app);
        if (!log_) throw std::runtime_error("Cannot open rebuild.log in " + options_.config_directory.string());
        Log("START x86; 本机麻将客户端 0.27.0");
        Log("CONFIG " + config_path_.string());
        Log("RESOURCES " + resource_directory_.string());
        Gdiplus::GdiplusStartupInput startup;
        if (Gdiplus::GdiplusStartup(&gdiplus_token_, &startup, nullptr) != Gdiplus::Ok)
            throw std::runtime_error("GDI+ startup failed");
        font_ = CreateFontA(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
        Check(font_ != nullptr, "CreateFontA");
        WNDCLASSEXA window_class{sizeof(WNDCLASSEXA)};
        window_class.lpfnWndProc = WindowProc;
        window_class.hInstance = instance;
        window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
        window_class.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        window_class.lpszClassName = kWindowClass;
        Check(RegisterClassExA(&window_class) != 0, "RegisterClassExA");
        RECT bounds{0, 0, width_, height_};
        Check(AdjustWindowRect(&bounds, kWindowStyle, FALSE) != FALSE, "AdjustWindowRect");
        window_ = CreateWindowExA(0, kWindowClass, "Tenhou",
                                  kWindowStyle | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
                                  bounds.right - bounds.left, bounds.bottom - bounds.top,
                                  nullptr, nullptr, instance, this);
        Check(window_ != nullptr, "CreateWindowExA");
        Check(SetPropA(window_, instance_property_.c_str(), reinterpret_cast<HANDLE>(1)) != FALSE,
              "Set instance identity");
        SetWindowTextW(window_, L"Tenhou Rebuild / Windows x64");
        DragAcceptFiles(window_, TRUE);
        RECT client_bounds{0, 0, width_, height_};
        Check(AdjustWindowRectExForDpi(&client_bounds, kWindowStyle, FALSE, 0, GetDpiForWindow(window_)) != FALSE,
              "AdjustWindowRectExForDpi");
        SetWindowPos(window_, nullptr, 0, 0, client_bounds.right - client_bounds.left,
                     client_bounds.bottom - client_bounds.top,
                     SWP_NOMOVE | SWP_NOZORDER);
        welcome_ = std::make_unique<WelcomeView>(window_, [this](WelcomeView::Action action) {
            switch (action) {
                case WelcomeView::Action::Exit: PostMessageA(window_, WM_CLOSE, 0, 0); break;
                case WelcomeView::Action::Settings: settings_->Open(); break;
                case WelcomeView::Action::Lobby: Login(false); break;
                case WelcomeView::Action::Register: Login(true); break;
                case WelcomeView::Action::ServerChanged:
                    lobby_->SetWelcomeMusic(view_ == "welcome", welcome_->OfficialServer());
                    break;
            }
        }, [this](const std::string &message) { Log(message); });
        welcome_->LoadPreferences(config_path_);
        table_ = std::make_unique<TableView>(window_, [this](const std::string &message) { Log(message); },
                                             config_path_);
        table_->ConfigureArtwork(resource_directory_, screen_width_);
        lobby_ = std::make_unique<LobbyView>(window_, config_path_,
                                             [this](const std::string &message) { Log(message); });
        lobby_->EnableServiceLobby([this] { settings_->Open(); },
            [this] { PostMessageW(window_, WM_APP + 27, 0, 0); });
        lobby_->ConfigureArtwork(resource_directory_, screen_width_);
        if (options_.offline_lobby) {
            offline_lobby_ = std::make_unique<OfflineLobbyView>(window_, config_path_, [this](OfflineLobbyView::Action action) {
                switch (action) {
                    case OfflineLobbyView::Action::Settings: settings_->Open(); break;
                    case OfflineLobbyView::Action::OpenReplay: PostMessageW(window_, WM_COMMAND, kReplay, 0); break;
                    case OfflineLobbyView::Action::Logout: PostMessageW(window_, WM_APP + 25, 0, 0); break;
                    case OfflineLobbyView::Action::RoomConfirmed:
                        lobby_->SetOfflineLobbyMusic(offline_lobby_->contest());
                        break;
                }
            }, [this](const std::string &message) { Log(message); });
        }
        lobby_->SetConnectionObserver([this](const LobbySnapshot &s) {
            if (s.server_error_receipt != 0 && s.server_error_receipt != last_server_error_receipt_) {
                last_server_error_receipt_ = s.server_error_receipt;
                const auto notice = FormatServerError(s.server_error_code, s.server_error_text);
                pending_server_error_ = PendingServerError{notice.caption, notice.message, s.server_error_fatal};
                PostMessageW(window_, WM_APP + 28, 0, 0);
            }
            if (view_ != "welcome") return;
            if ((s.native_service || s.official) &&
                (s.phase == LobbyPhase::Lobby || (s.phase == LobbyPhase::AwaitRecovery && s.game.started) ||
                 s.phase == LobbyPhase::Game)) {
                welcome_->ClearPassword();
                welcome_->SetLoginState(false, L"服务已确认登录");
                SetView("lobby");
            } else if (s.phase == LobbyPhase::Error) {
                if (s.server_error_receipt != 0) {
                    const auto notice = FormatServerError(s.server_error_code, s.server_error_text);
                    welcome_->SetLoginState(false, notice.message);
                } else {
                    welcome_->SetLoginState(false, L"连接失败：" + NetworkWide(s.error));
                }
            }
        });
        settings_ = std::make_unique<SettingsView>(window_, config_path_, resource_directory_,
                                                   [this](PreferenceChange change) { ApplyPreferences(change); },
                                                   [this](const std::string &message) { Log(message); });
        table_->SetOriginalDisplay(original_display_);
        lobby_->SetOriginalDisplay(original_display_);
        table_->SetEffectsOnly(sound_effects_only_);
        lobby_->SetEffectsOnly(sound_effects_only_);
        ApplyPreferences(PreferenceChange::Other);
        ApplyPreferences(PreferenceChange::Sound);
        ApplyPreferences(PreferenceChange::Initial);
        mute_effects_ = GetPrivateProfileIntW(L"REBUILD", L"MuteSE", 0, config_path_.c_str()) != 0;
        mute_music_ = GetPrivateProfileIntW(L"REBUILD", L"MuteBGM", 0, config_path_.c_str()) != 0;
        lobby_->SetQuickAudio(mute_effects_, mute_music_);
        table_->SetEffectsMuted(mute_effects_);
        audio_meter_ = CreateWindowExW(0, L"BUTTON", L"立体声音量与音频开关",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW, 0, 0, kMeterWidth, kMeterHeight,
            window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAudioMeter)), instance, nullptr);
        Check(audio_meter_ != nullptr && SetTimer(window_, kAudioTimer, kMeterPeriodMs, nullptr) != 0, "Create audio meter");
        if (!options_.replay.empty()) table_->Load(options_.replay);
        SetView(options_.offline_lobby && options_.replay.empty() ? "offline_lobby" : view_);
        ShowWindow(window_, show == SW_HIDE ? SW_SHOWNORMAL : show);
        UpdateWindow(window_);
        MSG message{};
        BOOL result;
        while ((result = MotionTimer::GetMessage(&message)) > 0) {
            if (settings_->HandleMessage(message)) continue;
            if (message.message == WM_KEYDOWN && offline_lobby_ &&
                offline_lobby_->HandleKey(static_cast<UINT>(message.wParam))) continue;
            if (lobby_ &&lobby_->HandleExitVoteMessage(message)) continue;
            if (message.message == WM_KEYDOWN && lobby_ && lobby_->HandleKey(static_cast<UINT>(message.wParam)))
                continue;
            if (message.message == WM_KEYDOWN && table_ && table_->HandleKey(static_cast<UINT>(message.wParam)))
                continue;
            if (message.message == WM_KEYDOWN && message.wParam == VK_F5) {
                LoadResources();
                continue;
            }
            if (!IsDialogMessageA(window_, &message)) {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }
        }
        if (result == -1) {
            Log("ERROR GetMessageA");
            return 1;
        }
        Log("EXIT code=" + std::to_string(exit_code_));
        return exit_code_;
    }


    void Application::Layout() {
        const RECT client{0, 0, kDefaultClientWidth, kDefaultClientHeight};
        if (welcome_) welcome_->Resize(client.right, client.bottom);
        if (table_) table_->Resize(client.right, client.bottom);
        if (lobby_) lobby_->Resize(client.right, client.bottom);
        if (offline_lobby_) offline_lobby_->Resize(client.right, client.bottom);
        // WC_SOUND, at (canvas width - 20, canvas height - 57 + 4) in HD.
        if (audio_meter_) SetWindowPos(audio_meter_, HWND_TOP, client.right - 20, client.bottom - 53,
                                      kMeterWidth, kMeterHeight, SWP_NOACTIVATE);
        InvalidateRect(window_, nullptr, FALSE);
    }

    void Application::UpdateAudioMeter() {
        if (IsIconic(window_)) return;
        const auto last_effect = view_ == "replay" ? table_->LastEffectAt() : lobby_->LastEffectAt();
        const auto music = lobby_->MusicLevels();
        // WC_SOUND's music range adapts to the louder BGM channel. SE has its own decay.
        if (std::max(music[0], music[1]) > music_meter_range_) music_meter_range_ *= 1.04f;
        else if (music_meter_range_ > .05f) music_meter_range_ *= .99f;
        auto next = audio_bars_;
        for (unsigned channel = 0; channel < music.size(); ++channel)
            next[channel] = static_cast<int>(std::min(1.f, music[channel] / music_meter_range_) * kMeterInterior);
        const auto elapsed = last_effect ? GetTickCount64() - last_effect : 0;
        next[2] = last_effect ? static_cast<int>(kMeterInterior * std::pow(.8, double(elapsed / kMeterPeriodMs))) : 0;
        if (next == audio_bars_) return;
        audio_bars_ = next;
        InvalidateRect(audio_meter_, nullptr, FALSE);
    }

    void Application::PaintAudioMeter(const DRAWITEMSTRUCT &item) {
        const auto &bounds = item.rcItem;
        const auto brush = reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH));
        SetDCBrushColor(item.hDC, item.itemState & (ODS_SELECTED | ODS_FOCUS) ? RGB(51, 153, 255) : RGB(51, 51, 51));
        FrameRect(item.hDC, &bounds, brush);
        for (int channel = 0; channel < 3; ++channel)
            for (int row = 0; row < kMeterInterior; ++row) {
                const int x = bounds.left + 1 + 4 * channel, y = bounds.bottom - 2 - row;
                const RECT line{x, y, x + 4, y + 1};
                const int grey = 17 + 34 * row / (kMeterInterior - 1);
                const bool lit = row < audio_bars_[channel];
                const int green = row + 1 == audio_bars_[channel] ? 204 : 17 + 68 * row / (kMeterInterior - 1);
                SetDCBrushColor(item.hDC, lit ? RGB(0, green, 0) : RGB(grey, grey, grey));
                FillRect(item.hDC, &line, brush);
            }
    }

    void Application::ShowAudioMenu() {
        const HMENU menu = CreatePopupMenu();
        Check(menu != nullptr, "Create audio menu");
        AppendMenuW(menu, MF_STRING | (mute_effects_ ? MF_UNCHECKED : MF_CHECKED), kToggleEffects, L"SE");
        AppendMenuW(menu, MF_STRING | (mute_music_ ? MF_UNCHECKED : MF_CHECKED), kToggleMusic, L"BGM");
        RECT anchor{};
        GetWindowRect(audio_meter_, &anchor);
        const auto command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                                            anchor.right, anchor.top, 0, window_, nullptr);
        DestroyMenu(menu);
        if (command == kToggleEffects) {
            WritePreferenceInt(config_path_, L"REBUILD", L"MuteSE", !mute_effects_);
            mute_effects_ = !mute_effects_;
        } else if (command == kToggleMusic) {
            WritePreferenceInt(config_path_, L"REBUILD", L"MuteBGM", !mute_music_);
            mute_music_ = !mute_music_;
        } else return;
        lobby_->SetQuickAudio(mute_effects_, mute_music_);
        table_->SetEffectsMuted(mute_effects_);
        UpdateAudioMeter();
    }

    void Application::LoadResources() {
        if (view_ == "welcome") welcome_->Load(resource_directory_, config_path_, screen_width_, screen_width_ * 3 / 4);
        else if (view_ == "replay") table_->Reload();
        else if (view_ == "offline_lobby") offline_lobby_->ConfigureArtwork(resource_directory_);
        else lobby_->ConfigureArtwork(resource_directory_, screen_width_);
    }

    void Application::SetView(const std::string &requested_view) {
        if (requested_view != "welcome" && requested_view != "lobby" && requested_view != "replay" &&
            (requested_view != "offline_lobby" || !offline_lobby_)) throw
                std::runtime_error("页面不可用");
        if (welcome_ &&view_=="welcome" && requested_view != "welcome") welcome_->Save(config_path_);
        view_ = requested_view;
        if (offline_lobby_ && view_ != "offline_lobby") offline_lobby_->Show(false);
        welcome_->Show(view_ == "welcome");
        table_->Show(view_ == "replay");
        lobby_->Show(view_ == "lobby");
        lobby_->SetWelcomeMusic(view_ == "welcome", welcome_->OfficialServer());
        lobby_->SetOfflineLobbyMusic(view_ == "offline_lobby" ? std::optional<bool>(offline_lobby_->contest()) : std::nullopt);
        Layout();
        LoadResources();
        if (view_ == "offline_lobby") offline_lobby_->Show(true);
        Log("VIEW " + view_);
    }

    void Application::Login(bool register_account) {
        if (welcome_->Connecting()) {
            lobby_->Disconnect();
            welcome_->SetLoginState(false, L"已取消连接");
            return;
        }
        if (lobby_->LoggedIn()) {
            SetView("lobby");
            return;
        }
        ConnectionOptions connection;
        connection.official = welcome_->OfficialServer();
        if (connection.official && !welcome_->HasOfficialId()) {
            welcome_->SetLoginState(false, L"请输入 19 位天凤官方 ID（IDxxxxxxxx-xxxxxxxx）或 NoName");
            return;
        }
        connection.host = connection.official ? "b.tenhou.net" : NetworkUtf8(welcome_->Host());
        connection.port = connection.official ? 10080 : welcome_->Port();
        if (connection.host.empty() || !connection.port) {
            welcome_->SetLoginState(false, L"请输入服务器主机和 1～65535 之间的端口");
            return;
        }
        connection.identity = NetworkUtf8(connection.official ? welcome_->OfficialIdentity() : welcome_->Identity());
        connection.gender = welcome_->Gender();
        connection.password = NetworkUtf8(welcome_->Password());
        connection.nickname = NetworkUtf8(welcome_->Nickname());
        connection.register_account = register_account;
        connection.config_file = config_path_;
        connection.native_service = !connection.official;
        connection.origin_lobby = connection.official || options_.service_lobby;
        connection.interactive_games = true;
        connection.match_games = true;
        connection.recover_games = true;
        connection.auto_reconnect = true;
        connection.presentation = true;
        // The earlier numeric default and localhost identify the same local service.
        const auto session_host = connection.host == "localhost" ? "127.0.0.1" : connection.host;
        const auto session_key = session_host + ":" + connection.identity;
        const auto account_key = Sha256(Bytes(session_key.begin(), session_key.end())).substr(0, 16);
        connection.session_file = config_path_.parent_path() / (
            std::wstring(connection.official ? L"official-session-" : L"platform-session-") +
            std::to_wstring(connection.port) + L"-" + NetworkWide(account_key) + L".txt");
        // Persist login ID immediately on login attempt (matching tenhou.exe.c:110711-110719).
        welcome_->Save(config_path_);
        welcome_->SetLoginState(true, connection.official ? L"正在连接天凤官方服务器..." : L"正在连接第三方服务器...");
        lobby_->Login(connection);
    }

    void Application::OpenReplay() {
        wchar_t file[32768]{};
        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = window_;
        dialog.lpstrFilter = L"麻将牌谱 (*.xml;*.mjlog)\0*.xml;*.mjlog\0所有文件\0*.*\0\0";
        dialog.lpstrFile = file;
        dialog.nMaxFile = 32768;
        dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetOpenFileNameW(&dialog)) {
            table_->Load(file);
            SetView("replay");
        } else if (CommDlgExtendedError()) throw std::runtime_error("无法打开牌谱文件选择窗口");
    }

    void Application::Paint(HDC dc) {
        RECT bounds{};
        GetClientRect(window_, &bounds);
        FillRect(dc, &bounds, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        if (view_ == "welcome" && welcome_) welcome_->Capture(dc);
        else if (view_ == "replay" && table_) table_->Capture(dc);
        else if (view_ == "offline_lobby" && offline_lobby_) offline_lobby_->Capture(dc);
        else if (lobby_) lobby_->Capture(dc);
    }

    void Application::SaveConfig() {
        if (welcome_) welcome_->Save(config_path_);
        if (table_) table_->SaveSettings();
        if (lobby_) lobby_->Save();
        const auto relative = resource_directory_.lexically_relative(options_.config_directory);
        const auto resources = relative.empty() ? resource_directory_ : relative;
        WritePreference(config_path_, L"REBUILD", L"ResourceDirectory", resources.c_str());
        WritePreferenceInt(config_path_, L"REBUILD", L"Width", kDefaultClientWidth);
        WritePreferenceInt(config_path_, L"REBUILD", L"Height", kDefaultClientHeight);
        WritePreferenceInt(config_path_, L"REBUILD", L"ScreenWidth", 1440);
        Log("CONFIG saved");
    }

    void Application::ApplyPreferences(PreferenceChange change) {
        const auto preferences = ClientPreferences::Load(config_path_);
        if (change == PreferenceChange::Other) welcome_->SetHiddenId(preferences.hidden_id);
        if (change == PreferenceChange::Sound) {
            const int mode = static_cast<int>(GetPrivateProfileIntW(L"RESOURCE", L"SE_NOVOICE",
                                                                    sound_effects_only_ ? 1 : 0, config_path_.c_str()));
            sound_effects_only_ = mode == 1;
        }
        table_->ApplyPreferences(preferences, change);
        lobby_->ApplyPreferences(preferences, change);
        if (change == PreferenceChange::Artwork && view_ == "offline_lobby")
            offline_lobby_->ConfigureArtwork(resource_directory_);
    }


LRESULT CALLBACK Application::WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        auto *application = reinterpret_cast<Application *>(GetWindowLongPtrA(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            application = static_cast<Application *>(reinterpret_cast<CREATESTRUCTA *>(lparam)->lpCreateParams);
            application->window_ = window;
            SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(application));
        }
        if (!application) return DefWindowProcA(window, message, wparam, lparam);
        // C++ exceptions must never unwind through a Win32 callback boundary.
        try { return application->HandleMessage(message, wparam, lparam); } catch (const std::exception &error) {
            application->error_ = error.what();
            application->Log("ERROR " + application->error_);
            application->exit_code_ = 1;
            if (message == WM_CLOSE) DestroyWindow(window);
            else PostMessageA(window, WM_CLOSE, 0, 0);
            return 0;
        }
    }

    LRESULT Application::HandleMessage(UINT message, WPARAM wparam, LPARAM lparam) {
        if (message == WM_APP + 25) {
            SetView("welcome");
            return 0;
        }
        if (message == WM_APP + 27 && view_ == "lobby") {
            OpenReplay();
            return 0;
        }
        if (message == WM_APP + 28) {
            if (pending_server_error_) {
                const auto err = std::move(*pending_server_error_);
                pending_server_error_.reset();
                MessageBoxW(window_, err.message.c_str(), err.caption.c_str(), MB_OK);
                if (err.fatal && view_ == "lobby") SetView("welcome");
            }
            return 0;
        }
        if (message == WM_DROPFILES) {
            const auto drop = reinterpret_cast<HDROP>(wparam);
            wchar_t file[32768]{};
            if (DragQueryFileW(drop, 0, file, static_cast<UINT>(std::size(file))) && view_ != "lobby") {
                table_->Load(file);
                SetView("replay");
            }
            DragFinish(drop);
            return 0;
        }
        switch (message) {
            case WM_APP + 1: throw std::runtime_error(reinterpret_cast<const char *>(lparam));
            case WM_TIMER:
                if (wparam == kAudioTimer) UpdateAudioMeter();
                return 0;
            case WM_DRAWITEM:
                if (wparam == kAudioMeter) {
                    PaintAudioMeter(*reinterpret_cast<const DRAWITEMSTRUCT *>(lparam));
                    return TRUE;
                }
                return DefWindowProcA(window_, message, wparam, lparam);
            case WM_CONTEXTMENU:
                if (audio_meter_ && reinterpret_cast<HWND>(wparam) == audio_meter_) {
                    ShowAudioMenu();
                    return 0;
                }
                return DefWindowProcA(window_, message, wparam, lparam);
            case WM_SIZE: Layout();
                return 0;
            case WM_DPICHANGED: {
                // The fixed 1440x1080 client is already in physical pixels. Only
                // the non-client chrome follows monitor DPI; do not scale twice.
                const auto *suggested = reinterpret_cast<const RECT *>(lparam);
                RECT frame{0, 0, kDefaultClientWidth, kDefaultClientHeight};
                Check(AdjustWindowRectExForDpi(&frame, kWindowStyle, GetMenu(window_) != nullptr, 0,
                                               HIWORD(wparam)) != FALSE, "AdjustWindowRectExForDpi change");
                Check(SetWindowPos(window_, nullptr, suggested->left, suggested->top,
                                   frame.right - frame.left, frame.bottom - frame.top,
                                   SWP_NOZORDER | SWP_NOACTIVATE) != FALSE, "SetWindowPos DPI change");
                Layout();
                RedrawWindow(window_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
                return 0;
            }
            case WM_GETMINMAXINFO: {
                auto *bounds = reinterpret_cast<MINMAXINFO *>(lparam);
                RECT minimum{0, 0, kDefaultClientWidth, kDefaultClientHeight};
                Check(AdjustWindowRectExForDpi(&minimum, static_cast<DWORD>(GetWindowLongPtrA(window_, GWL_STYLE)),
                                         GetMenu(window_) != nullptr,
                                         static_cast<DWORD>(GetWindowLongPtrA(window_, GWL_EXSTYLE)),
                                         GetDpiForWindow(window_)) != FALSE, "AdjustWindowRectExForDpi minimum");
                bounds->ptMinTrackSize = {minimum.right - minimum.left, minimum.bottom - minimum.top};
                bounds->ptMaxTrackSize = bounds->ptMinTrackSize;
                bounds->ptMaxSize = bounds->ptMinTrackSize;
                return 0;
            }
            case WM_SYSCOMMAND:
                if ((wparam & 0xfff0) == SC_SIZE || (wparam & 0xfff0) == SC_MAXIMIZE) return 0;
                return DefWindowProcA(window_, message, wparam, lparam);
            case WM_COMMAND:
                if (LOWORD(wparam) == kAudioMeter && HIWORD(wparam) == BN_CLICKED) ShowAudioMenu();
                else if (LOWORD(wparam) == kSettings) settings_->Open();
                else if (LOWORD(wparam) == kWelcome && (view_ == "replay" || view_ == "offline_lobby")) SetView("welcome");
                else if (LOWORD(wparam) == kOfflineLobby && offline_lobby_ && (view_ == "welcome" || view_ == "replay"))
                    SetView("offline_lobby");
                else if (LOWORD(wparam) == kReplay && view_ != "lobby") OpenReplay();
                return 0;
            case WM_CLOSE:
                if (table_) table_->Show(false);
                if (lobby_) lobby_->Disconnect();
                SaveConfig();
                DestroyWindow(window_);
                return 0;
            case WM_DESTROY: KillTimer(window_, kAudioTimer);
                RemovePropA(window_, instance_property_.c_str());
                PostQuitMessage(0);
                return 0;
            default: return DefWindowProcA(window_, message, wparam, lparam);
        }
    }
} // namespace tenhou
