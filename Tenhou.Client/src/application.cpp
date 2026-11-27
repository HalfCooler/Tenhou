#include "application.hpp"
#include <cstring>
#include <cwctype>
#include "welcome_view.hpp"
#include "table_view.hpp"
#include "lobby_view.hpp"
#include "offline_lobby_view.hpp"
#include "settings_view.hpp"
#include "window_layout.hpp"
#include "monotonic_clock.hpp"
#include "shared_assets.hpp"
#include "win32_utf.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <commdlg.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace tenhou {
    namespace {
        constexpr wchar_t kWindowClass[] = L"TENHOU_CLIENT";
        constexpr DWORD kWindowStyle = WS_OVERLAPPEDWINDOW;
        constexpr UINT kReplay = 4004, kWelcome = 4001, kSettings = 4023;
        constexpr UINT kOfflineLobby = 4024;
        constexpr UINT kAudioMeter = 4030, kToggleEffects = 4031, kToggleMusic = 4032;
        // System menu custom commands (must keep low 4 bits zero per WM_SYSCOMMAND docs).
        constexpr UINT kSysSize1024 = 0x1010;
        constexpr UINT kSysSize1280 = 0x1020;
        constexpr UINT kSysSize1440 = 0x1030;
        constexpr UINT kSysSize1920 = 0x1040;
        constexpr UINT kSysSize2560 = 0x1050;
        constexpr UINT kSysFullscreen = 0x1060;
        constexpr UINT_PTR kAudioTimer = 1;
        constexpr int kMeterWidth = 14, kMeterHeight = 16, kMeterInterior = 14;
        constexpr unsigned kMeterPeriodMs = 50;
        constexpr std::uint64_t kMaxLogBytes = static_cast<uint64_t>(4) * 1024 * 1024;

        std::filesystem::path ExecutableDirectory() {
            std::vector<wchar_t> buffer(32768);
            const DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            if (!size || size >= buffer.size()) throw std::runtime_error("Cannot resolve executable directory");
            return std::filesystem::path(std::wstring(buffer.data(), size)).parent_path();
        }
    } // namespace

    Options ParseOptions(const int argc, char **argv) {
        (void) argv;
        Options options;
        int count = 0;
        const std::unique_ptr<wchar_t*, FreeDeleter> arguments(CommandLineToArgvW(GetCommandLineW(), &count));
        if (!arguments || count != argc) {
            throw std::runtime_error("无法读取启动参数");
        }
        try {
            for (int i = 1; i < count; ++i) {
                const std::wstring name = arguments.get()[i];
                if (name == L"--offline-lobby") {
                    options.offline_lobby = true;
                    continue;
                }
                if (name == L"--service-lobby") {
                    options.service_lobby = true;
                    continue;
                }
                if (name == L"--width" || name == L"--height") {
                    if (++i >= count) throw std::runtime_error("启动参数缺少数值");
                    const int v = std::stoi(arguments.get()[i]);
                    if (name == L"--width") options.initial_width = v;
                    else options.initial_height = v;
                    continue;
                }
                if (name != L"--config-dir" && name != L"--resources" && name != L"--replay") throw std::runtime_error(
                    "不支持的启动参数");
                if (++i >= count) throw std::runtime_error("启动参数缺少路径");
                const auto value = std::filesystem::absolute(arguments.get()[i]);
                if (name == L"--config-dir") options.config_directory = value;
                else if (name == L"--resources") options.resource_directory = value;
                else options.replay = value;
            }
        } catch (...) {
            throw;
        }
        return options;
    }

    Application::Application(Options options) : options_(std::move(options)) {
        executable_directory_ = ExecutableDirectory();
        if (options_.config_directory.empty()) options_.config_directory = executable_directory_;
        std::filesystem::create_directories(options_.config_directory);
        config_path_ = options_.config_directory / L"config.ini";
        const auto raw_screen_size = PreferenceString(config_path_, "STATUS", "SCREENSIZE");
        if (raw_screen_size.empty()) WritePreference(config_path_, "STATUS", "SCREENSIZE", "default");
        configured_screen_width_ = ParseScreenSizeSetting(raw_screen_size);
        const int default_w = configured_screen_width_ ? configured_screen_width_ : kDefaultClientWidth;
        const int default_h = default_w * 3 / 4;
        const int saved_w = PreferenceInt(config_path_, "REBUILD", "Width", default_w);
        const int saved_h = PreferenceInt(config_path_, "REBUILD", "Height", default_h);
        width_ = std::clamp(options_.initial_width > 0 ? options_.initial_width : saved_w, kMinClientWidth, 7680);
        height_ = std::clamp(options_.initial_height > 0 ? options_.initial_height : saved_h, kMinClientHeight, 4320);
        start_maximized_ = options_.initial_width <= 0 && options_.initial_height <= 0 &&
                           PreferenceInt(config_path_, "REBUILD", "Maximized", 0) != 0;
        start_fullscreen_ = options_.initial_width <= 0 && options_.initial_height <= 0 &&
                            PreferenceInt(config_path_, "REBUILD", "Fullscreen", 0) != 0;
        screen_width_ = ResolveTableWidth(width_, height_);
        view_ = options_.replay.empty() ? "welcome" : "replay";
        sound_effects_only_ = PreferenceInt(config_path_, "REBUILD", "SoundEffectsOnly", 0) != 0;
        resource_directory_ = options_.resource_directory.empty()
                                  ? PathFromUtf8(PreferenceString(
                                      config_path_, "REBUILD", "ResourceDirectory",
                                      PathToUtf8(executable_directory_ / L"resources")))
                                  : options_.resource_directory;
        if (resource_directory_.is_relative()) resource_directory_ = options_.config_directory / resource_directory_;
        resource_directory_ = std::filesystem::absolute(resource_directory_).lexically_normal();
    }

    Application::~Application() {
        settings_.reset();
        welcome_.reset();
        table_.reset();
        offline_lobby_.reset();
        lobby_.reset();
        SharedAssets().Clear();
        if (log_.is_open()) log_.flush();
        if (gdiplus_token_) Gdiplus::GdiplusShutdown(gdiplus_token_);
        if (font_) DeleteObject(font_);
        if (mutex_) CloseHandle(mutex_);
    }

    void Application::OpenLog() {
        log_path_ = options_.config_directory / L"rebuild.log";
        std::error_code ec;
        const auto existing = std::filesystem::file_size(log_path_, ec);
        log_bytes_ = ec ? 0 : existing;
        if (log_bytes_ >= kMaxLogBytes) {
            auto backup = log_path_;
            backup += L".1";
            std::filesystem::remove(backup, ec);
            ec.clear();
            std::filesystem::rename(log_path_, backup, ec);
            log_bytes_ = 0;
        }
        log_.open(log_path_, std::ios::app | std::ios::binary);
        if (!log_) throw std::runtime_error("Cannot open rebuild.log in " + PathToUtf8(options_.config_directory));
    }

    void Application::RotateLogIfNeeded() {
        if (log_bytes_ < kMaxLogBytes || log_path_.empty()) return;
        log_.flush();
        log_.close();
        std::error_code ec;
        auto backup = log_path_;
        backup += L".1";
        std::filesystem::remove(backup, ec);
        ec.clear();
        std::filesystem::rename(log_path_, backup, ec);
        log_bytes_ = 0;
        log_.open(log_path_, std::ios::trunc | std::ios::binary);
    }

    void Application::Log(const std::string &message) {
        if (!log_.is_open()) return;
        RotateLogIfNeeded();
        SYSTEMTIME time{};
        GetLocalTime(&time);
        std::ostringstream line;
        line << time.wHour << ':' << time.wMinute << ':' << time.wSecond << ' ' << message << '\n';
        const auto text = line.str();
        log_.write(text.data(), static_cast<std::streamsize>(text.size()));
        log_bytes_ += text.size();
        if (message.rfind("ERROR", 0) == 0 || message.rfind("START", 0) == 0 ||
            message.rfind("EXIT", 0) == 0 || message.find("ERROR") != std::string::npos) {
            log_.flush();
        }
    }

    void Application::Check(const bool success, const char *action)
    {
        if (!success) throw std::runtime_error(
            std::string(action) + " failed; Win32 error=" + std::to_string(GetLastError()));
    }

    int Application::Run(const HINSTANCE instance, const int show) {
        Check(SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) != FALSE,
              "SetProcessDpiAwarenessContext");
        // Separate identity from the original client. Atomic mutex also covers startup races.
        SetLastError(ERROR_SUCCESS);
        auto canonical = std::filesystem::weakly_canonical(options_.config_directory).wstring();
        std::transform(canonical.begin(), canonical.end(), canonical.begin(), [](const wchar_t c) {
            return static_cast<wchar_t>(towlower(c));
        });
        std::vector<std::uint8_t> identity(canonical.size() * sizeof(wchar_t));
        std::memcpy(identity.data(), canonical.data(), identity.size());
        const std::wstring instance_name = Utf8ToWide("Local\\TenhouRebuild.Config." + Sha256(identity));
        instance_property_ = instance_name;
        mutex_ = CreateMutexW(nullptr, FALSE, instance_name.c_str());
        const DWORD mutex_error = GetLastError();
        Check(mutex_ != nullptr, "CreateMutexW");
        if (mutex_error == ERROR_ALREADY_EXISTS) {
            EnumWindows([](const HWND window, const LPARAM context) -> BOOL {
                if (GetPropW(window, reinterpret_cast<const wchar_t *>(context))) {
                    ShowWindow(window, SW_RESTORE);
                    SetForegroundWindow(window);
                    return FALSE;
                }
                return TRUE;
            }, reinterpret_cast<LPARAM>(instance_name.c_str()));
            return 0;
        }
        OpenLog();
        Log("START x64; Tenhou Rebuild Client 0.27.0");
        Log("CONFIG " + PathToUtf8(config_path_));
        Log("RESOURCES " + PathToUtf8(resource_directory_));
        const Gdiplus::GdiplusStartupInput startup;
        if (Gdiplus::GdiplusStartup(&gdiplus_token_, &startup, nullptr) != Gdiplus::Ok)
            throw std::runtime_error("GDI+ startup failed");
        font_ = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        Check(font_ != nullptr, "CreateFontW");
        WNDCLASSEXW window_class{sizeof(WNDCLASSEXW), 0};
        window_class.lpfnWndProc = WindowProc;
        window_class.hInstance = instance;
        window_class.hCursor = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));
        window_class.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        window_class.lpszClassName = kWindowClass;
        Check(RegisterClassExW(&window_class) != 0, "RegisterClassExW");
        RECT bounds{0, 0, width_, height_};
        Check(AdjustWindowRect(&bounds, kWindowStyle, FALSE) != FALSE, "AdjustWindowRect");
        window_ = CreateWindowExW(0, kWindowClass, L"Tenhou Rebuild / Windows x64",
                                  kWindowStyle | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
                                  bounds.right - bounds.left, bounds.bottom - bounds.top,
                                  nullptr, nullptr, instance, this);
        Check(window_ != nullptr, "CreateWindowExW");
        Check(SetPropW(window_, instance_property_.c_str(), reinterpret_cast<HANDLE>(1)) != FALSE,
              "Set instance identity");
        DragAcceptFiles(window_, TRUE);
        RECT client_bounds{0, 0, width_, height_};
        const UINT init_dpi = GetDpiForWindow(window_);
        Check(AdjustWindowRectExForDpi(&client_bounds, kWindowStyle, FALSE, 0, init_dpi) != FALSE,
              "AdjustWindowRectExForDpi");
        int outer_w = client_bounds.right - client_bounds.left;
        int outer_h = client_bounds.bottom - client_bounds.top;
        // Clamp initial non-override window cleanly inside the monitor work area.
        if (options_.initial_width <= 0 && options_.initial_height <= 0) {
            MONITORINFO mi{sizeof(MONITORINFO), {}, {}, 0};
            if (GetMonitorInfoW(MonitorFromWindow(window_, MONITOR_DEFAULTTONEAREST), &mi)) {
                const int work_w = mi.rcWork.right - mi.rcWork.left;
                const int work_h = mi.rcWork.bottom - mi.rcWork.top;
                const int chrome_w = outer_w - width_;
                const int chrome_h = outer_h - height_;
                const int max_cw = std::max(kMinClientWidth, work_w - chrome_w);
                const int max_ch = std::max(kMinClientHeight, work_h - chrome_h);
                if (width_ > max_cw || height_ > max_ch) {
                    const double scale = std::min(static_cast<double>(max_cw) / width_, static_cast<double>(max_ch) / height_);
                    width_ = std::max(kMinClientWidth, static_cast<int>(width_ * scale));
                    height_ = std::max(kMinClientHeight, static_cast<int>(height_ * scale));
                    outer_w = width_ + chrome_w;
                    outer_h = height_ + chrome_h;
                }
            }
        }
        SetWindowPos(window_, nullptr, 0, 0, outer_w, outer_h, SWP_NOMOVE | SWP_NOZORDER);
        screen_width_ = ResolveTableWidth(width_, height_);
        Log("SCREENSIZE setting=" + std::string(FormatScreenSizeSetting(configured_screen_width_)) +
            " canvas=" + std::to_string(screen_width_) + "x" + std::to_string(screen_width_ * 3 / 4));
        SetupSystemMenu();
        UpdateWindowTitle();
        welcome_ = std::make_unique<WelcomeView>(window_, [this](const WelcomeView::Action action) {
            switch (action) {
                case WelcomeView::Action::Exit: PostMessageW(window_, WM_CLOSE, 0, 0); break;
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
            offline_lobby_ = std::make_unique<OfflineLobbyView>(window_, config_path_, [this](const OfflineLobbyView::Action action) {
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
                    welcome_->SetLoginState(false, Utf8ToWide(notice.message));
                } else {
                    welcome_->SetLoginState(false, L"连接失败：" + Utf8ToWide(s.error));
                }
            }
        });
        settings_ = std::make_unique<SettingsView>(window_, config_path_, resource_directory_,
                                                   [this](const PreferenceChange change) { ApplyPreferences(change); },
                                                   [this](const std::string &message) { Log(message); });
        table_->SetOriginalDisplay(original_display_);
        lobby_->SetOriginalDisplay(original_display_);
        table_->SetEffectsOnly(sound_effects_only_);
        lobby_->SetEffectsOnly(sound_effects_only_);
        ApplyPreferences(PreferenceChange::Other);
        ApplyPreferences(PreferenceChange::Sound);
        ApplyPreferences(PreferenceChange::Initial);
        mute_effects_ = PreferenceInt(config_path_, "REBUILD", "MuteSE", 0) != 0;
        mute_music_ = PreferenceInt(config_path_, "REBUILD", "MuteBGM", 0) != 0;
        lobby_->SetQuickAudio(mute_effects_, mute_music_);
        table_->SetEffectsMuted(mute_effects_);
        audio_meter_ = CreateWindowExW(0, L"BUTTON", L"立体声音量与音频开关",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW, 0, 0, kMeterWidth, kMeterHeight,
            window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAudioMeter)), instance, nullptr);
        Check(audio_meter_ != nullptr && SetTimer(window_, kAudioTimer, kMeterPeriodMs, nullptr) != 0, "Create audio meter");
        if (!options_.replay.empty()) table_->Load(options_.replay);
        SetView(options_.offline_lobby && options_.replay.empty() ? "offline_lobby" : view_);
        const int effective_show = show == SW_HIDE ? SW_SHOWNORMAL : start_maximized_ && !start_fullscreen_ ? SW_SHOWMAXIMIZED : show;
        ShowWindow(window_, effective_show);
        if (start_fullscreen_) SetFullscreen(true);
        UpdateWindow(window_);
        MSG message{};
        BOOL result;
        while ((result = MotionTimer::GetMessage(&message)) > 0) {
            if (settings_->HandleMessage(message)) continue;
            if (message.message == WM_KEYDOWN && message.wParam == VK_F11) { SetFullscreen(!fullscreen_); continue; }
            if (message.message == WM_SYSKEYDOWN && message.wParam == VK_RETURN && message.lParam & (1 << 29)) { SetFullscreen(!fullscreen_); continue; }
            if (message.message == WM_KEYDOWN && offline_lobby_ && offline_lobby_->HandleKey(static_cast<UINT>(message.wParam))) continue;
            if (lobby_ && lobby_->HandleExitVoteMessage(message)) continue;
            if (message.message == WM_KEYDOWN && lobby_ && lobby_->HandleKey(static_cast<UINT>(message.wParam))) continue;
            if (message.message == WM_KEYDOWN && table_ && table_->HandleKey(static_cast<UINT>(message.wParam))) continue;
            if (message.message == WM_KEYDOWN && message.wParam == VK_F5) { LoadResources(); continue; }
            if (!IsDialogMessageW(window_, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
        }
        if (result == -1) {
            Log("ERROR GetMessageW");
            return 1;
        }
        Log("EXIT code=" + std::to_string(exit_code_));
        return exit_code_;
    }

    void Application::SetupSystemMenu() const
    {
        const HMENU sys = GetSystemMenu(window_, FALSE);
        if (!sys) return;
        AppendMenuW(sys, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(sys, MF_STRING, kSysSize1024, L"窗口尺寸：1024×768");
        AppendMenuW(sys, MF_STRING, kSysSize1280, L"窗口尺寸：1280×960");
        AppendMenuW(sys, MF_STRING, kSysSize1440, L"窗口尺寸：1440×1080（标准）");
        AppendMenuW(sys, MF_STRING, kSysSize1920, L"窗口尺寸：1920×1440");
        AppendMenuW(sys, MF_STRING, kSysSize2560, L"窗口尺寸：2560×1440（宽屏）");
        AppendMenuW(sys, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(sys, MF_STRING, kSysFullscreen, L"无边框全屏\tAlt+Enter / F11");
        SyncSystemMenu();
    }

    void Application::SyncSystemMenu() const {
        const HMENU sys = GetSystemMenu(window_, FALSE);
        if (!sys) return;
        CheckMenuItem(sys, kSysFullscreen, MF_BYCOMMAND | (fullscreen_ ? MF_CHECKED : MF_UNCHECKED));
    }

    void Application::UpdateWindowTitle() const {
        if (!window_) return;
        const std::wstring title = L"Tenhou Rebuild / Windows x64 [" +
                                   std::to_wstring(screen_width_) + L"x" +
                                   std::to_wstring(screen_width_ * 3 / 4) + L"]";
        SetWindowTextW(window_, title.c_str());
    }

    void Application::ApplyClientSizePreset(int client_width, int client_height) {
        if (fullscreen_) SetFullscreen(false);
        if (IsZoomed(window_)) ShowWindow(window_, SW_RESTORE);
        RECT frame{0, 0, client_width, client_height};
        const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(window_, GWL_STYLE));
        if (const DWORD ex_style = static_cast<DWORD>(GetWindowLongPtrW(window_, GWL_EXSTYLE)); AdjustWindowRectExForDpi(&frame, style, GetMenu(window_) != nullptr, ex_style, GetDpiForWindow(window_))) {
            int outer_w = frame.right - frame.left;
            int outer_h = frame.bottom - frame.top;
            const int chrome_w = outer_w - client_width;
            const int chrome_h = outer_h - client_height;
            MONITORINFO mi{sizeof(MONITORINFO), {}, {}, 0};
            if (GetMonitorInfoW(MonitorFromWindow(window_, MONITOR_DEFAULTTONEAREST), &mi)) {
                const int max_cw = std::max(kMinClientWidth, static_cast<int>(mi.rcWork.right - mi.rcWork.left) - chrome_w);
                const int max_ch = std::max(kMinClientHeight, static_cast<int>(mi.rcWork.bottom - mi.rcWork.top) - chrome_h);
                if (client_width > max_cw || client_height > max_ch) {
                    const double scale = std::min(static_cast<double>(max_cw) / client_width, static_cast<double>(max_ch) / client_height);
                    client_width = std::max(kMinClientWidth, static_cast<int>(client_width * scale));
                    client_height = std::max(kMinClientHeight, static_cast<int>(client_height * scale));
                    outer_w = client_width + chrome_w;
                    outer_h = client_height + chrome_h;
                }
            }
            width_ = client_width;
            height_ = client_height;
            SetWindowPos(window_, nullptr, 0, 0, outer_w, outer_h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            Layout();
        }
    }

    void Application::SetFullscreen(const bool enabled) {
        if (fullscreen_ == enabled || !window_) return;
        if (enabled) {
            windowed_style_ = static_cast<DWORD>(GetWindowLongPtrW(window_, GWL_STYLE));
            windowed_ex_style_ = static_cast<DWORD>(GetWindowLongPtrW(window_, GWL_EXSTYLE));
            windowed_placement_.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(window_, &windowed_placement_);
            MONITORINFO mi{sizeof(MONITORINFO), {}, {}, 0};
            if (!GetMonitorInfoW(MonitorFromWindow(window_, MONITOR_DEFAULTTONEAREST), &mi)) return;
            SetWindowLongPtrW(window_, GWL_STYLE, (windowed_style_ & ~(WS_OVERLAPPEDWINDOW)) | WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN);
            SetWindowLongPtrW(window_, GWL_EXSTYLE, windowed_ex_style_ & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
            SetWindowPos(window_, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            fullscreen_ = true;
        } else {
            SetWindowLongPtrW(window_, GWL_STYLE, windowed_style_ ? windowed_style_ : (kWindowStyle | WS_CLIPCHILDREN));
            SetWindowLongPtrW(window_, GWL_EXSTYLE, windowed_ex_style_);
            SetWindowPlacement(window_, &windowed_placement_);
            SetWindowPos(window_, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            fullscreen_ = false;
        }
        SyncSystemMenu();
        Layout();
    }

    int Application::ResolveTableWidth(const int client_width, const int client_height) const {
        if (IsSupportedTableWidth(configured_screen_width_)) return configured_screen_width_;
        return SelectAutoTableWidth(client_width, client_height);
    }

    void Application::ApplyScreenWidth(const int next_width) {
        if (!IsSupportedTableWidth(next_width) || next_width == screen_width_) return;
        screen_width_ = next_width;
        if (table_) table_->ConfigureArtwork(resource_directory_, screen_width_);
        if (lobby_) lobby_->ConfigureArtwork(resource_directory_, screen_width_);
        if (offline_lobby_) offline_lobby_->ConfigureArtwork(resource_directory_, screen_width_);
        if (view_ == "welcome" && welcome_) welcome_->Load(resource_directory_, config_path_, screen_width_, screen_width_ * 3 / 4);
        const auto preferences = ClientPreferences::Load(config_path_);
        if (table_) table_->ApplyPreferences(preferences, PreferenceChange::Artwork);
        if (lobby_) lobby_->ApplyPreferences(preferences, PreferenceChange::Artwork);
        UpdateWindowTitle();
        Log("SCREENSIZE active=" + std::to_string(screen_width_) + "x" + std::to_string(screen_width_ * 3 / 4) +
            " setting=" + std::string(FormatScreenSizeSetting(configured_screen_width_)));
    }

    void Application::SyncAutoScreenWidth(const int client_width, const int client_height) {
        if (configured_screen_width_ != 0) return;
        if (const int target = SelectAutoTableWidth(client_width, client_height); target != screen_width_) ApplyScreenWidth(target);
    }

    void Application::Layout() {
        RECT client{};
        GetClientRect(window_, &client);
        const int cw = std::max(1, static_cast<int>(client.right));
        const int ch = std::max(1, static_cast<int>(client.bottom));
        if (!fullscreen_ && !IsIconic(window_) && !IsZoomed(window_) && cw >= kMinClientWidth && ch >= kMinClientHeight) {
            width_ = cw;
            height_ = ch;
        }
        if (table_ && lobby_) SyncAutoScreenWidth(cw, ch);
        if (welcome_) welcome_->Resize(cw, ch);
        if (table_) table_->Resize(cw, ch);
        if (lobby_) lobby_->Resize(cw, ch);
        if (offline_lobby_) offline_lobby_->Resize(cw, ch);
        // WC_SOUND, at (canvas width - 20, canvas height - 57 + 4) in HD; anchor inside client bounds on resize.
        if (audio_meter_) SetWindowPos(audio_meter_, HWND_TOP, std::max(4, cw - 20), std::max(4, ch - 53),
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
        const auto now = MonotonicMs();
        const auto elapsed = last_effect && now >= last_effect ? now - last_effect : 0;
        next[2] = last_effect ? static_cast<int>(kMeterInterior * std::pow(.8, static_cast<double>(elapsed) / kMeterPeriodMs)) : 0;
        if (next == audio_bars_) return;
        audio_bars_ = next;
        InvalidateRect(audio_meter_, nullptr, FALSE);
    }

    void Application::PaintAudioMeter(const DRAWITEMSTRUCT &item) const {
        const auto &bounds = item.rcItem;
        const auto brush = static_cast<HBRUSH>(GetStockObject(DC_BRUSH));
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
            WritePreferenceInt(config_path_, "REBUILD", "MuteSE", !mute_effects_);
            mute_effects_ = !mute_effects_;
        } else if (command == kToggleMusic) {
            WritePreferenceInt(config_path_, "REBUILD", "MuteBGM", !mute_music_);
            mute_music_ = !mute_music_;
        } else return;
        lobby_->SetQuickAudio(mute_effects_, mute_music_);
        table_->SetEffectsMuted(mute_effects_);
        UpdateAudioMeter();
    }

    void Application::LoadResources() const {
        if (view_ == "welcome") welcome_->Load(resource_directory_, config_path_, screen_width_, screen_width_ * 3 / 4);
        else if (view_ == "replay") table_->Reload();
        else if (view_ == "offline_lobby") offline_lobby_->ConfigureArtwork(resource_directory_, screen_width_);
        else lobby_->ConfigureArtwork(resource_directory_, screen_width_);
    }

    void Application::SetView(const std::string &requested_view) {
        if (requested_view != "welcome" && requested_view != "lobby" && requested_view != "replay" &&
            (requested_view != "offline_lobby" || !offline_lobby_)) throw
                std::runtime_error("页面不可用");
        if (welcome_ && view_ == "welcome" && requested_view != "welcome") welcome_->Save(config_path_);
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

    void Application::Login(const bool register_account) {
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
            welcome_->SetLoginState(false, L"请输入 19 位天凤官方 ID 或 NoName");
            return;
        }
        connection.host = connection.official ? "b.tenhou.net" : WideToUtf8(welcome_->Host());
        connection.port = connection.official ? 10080 : welcome_->Port();
        if (connection.host.empty() || !connection.port) {
            welcome_->SetLoginState(false, L"请输入服务器主机和 1～65535 之间的端口");
            return;
        }
        connection.identity = WideToUtf8(connection.official ? welcome_->OfficialIdentity() : welcome_->Identity());
        connection.gender = welcome_->Gender();
        connection.password = WideToUtf8(welcome_->Password());
        connection.nickname = WideToUtf8(welcome_->Nickname());
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
            std::to_wstring(connection.port) + L"-" + Utf8ToWide(account_key) + L".txt");
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

    void Application::Paint(const HDC dc) const
    {
        RECT bounds{};
        GetClientRect(window_, &bounds);
        FillRect(dc, &bounds, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
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
        WritePreference(config_path_, "REBUILD", "ResourceDirectory", PathToUtf8(resources));
        WritePreferenceInt(config_path_, "REBUILD", "Width", std::clamp(width_, kMinClientWidth, 7680));
        WritePreferenceInt(config_path_, "REBUILD", "Height", std::clamp(height_, kMinClientHeight, 4320));
        WritePreferenceInt(config_path_, "REBUILD", "Maximized", (!fullscreen_ && window_ && IsZoomed(window_)) ? 1 : 0);
        WritePreferenceInt(config_path_, "REBUILD", "Fullscreen", fullscreen_ ? 1 : 0);
        WritePreferenceInt(config_path_, "REBUILD", "ScreenWidth", screen_width_);
        Log("CONFIG saved");
    }

    void Application::ApplyPreferences(const PreferenceChange change) {
        if (change == PreferenceChange::ScreenSize) {
            configured_screen_width_ = ParseScreenSizeSetting(PreferenceString(config_path_, "STATUS", "SCREENSIZE"));
            if (configured_screen_width_ != 0) {
                ApplyClientSizePreset(configured_screen_width_, configured_screen_width_ * 3 / 4);
                ApplyScreenWidth(configured_screen_width_);
            } else {
                RECT client{};
                GetClientRect(window_, &client);
                ApplyScreenWidth(SelectAutoTableWidth(std::max(1, static_cast<int>(client.right)),
                                                      std::max(1, static_cast<int>(client.bottom))));
            }
            return;
        }
        const auto preferences = ClientPreferences::Load(config_path_);
        if (change == PreferenceChange::Other) welcome_->SetHiddenId(preferences.hidden_id);
        if (change == PreferenceChange::Sound) {
            const int mode = PreferenceInt(config_path_, "RESOURCE", "SE_NOVOICE",
                                           sound_effects_only_ ? 1 : 0);
            sound_effects_only_ = mode == 1;
        }
        table_->ApplyPreferences(preferences, change);
        lobby_->ApplyPreferences(preferences, change);
        if (change == PreferenceChange::Artwork && view_ == "offline_lobby")
            offline_lobby_->ConfigureArtwork(resource_directory_, screen_width_);
    }


    LRESULT CALLBACK Application::WindowProc(const HWND window, const UINT message, const WPARAM wparam, const LPARAM lparam) {
        auto *application = reinterpret_cast<Application *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            application = static_cast<Application *>(reinterpret_cast<CREATESTRUCTW *>(lparam)->lpCreateParams);
            application->window_ = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(application));
        }
        if (!application) return DefWindowProcW(window, message, wparam, lparam);
        // C++ exceptions must never unwind through a Win32 callback boundary.
        try { return application->HandleMessage(message, wparam, lparam); } catch (const std::exception &error) {
            application->error_ = error.what();
            application->Log("ERROR " + application->error_);
            application->exit_code_ = 1;
            if (message == WM_CLOSE) DestroyWindow(window);
            else PostMessageW(window, WM_CLOSE, 0, 0);
            return 0;
        }
    }

    LRESULT Application::HandleMessage(const UINT message, const WPARAM wparam, const LPARAM lparam) {
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
                MessageBoxW(window_, Utf8ToWide(err.message).c_str(), Utf8ToWide(err.caption).c_str(), MB_OK);
                if (err.fatal && view_ == "lobby") SetView("welcome");
            }
            return 0;
        }
        if (message == WM_DROPFILES) {
            const auto drop = reinterpret_cast<HDROP>(wparam);
            wchar_t file[32768]{};
            if (DragQueryFileW(drop, 0, file, std::size(file)) && view_ != "lobby") {
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
                return DefWindowProcW(window_, message, wparam, lparam);
            case WM_CONTEXTMENU:
                if (audio_meter_ && reinterpret_cast<HWND>(wparam) == audio_meter_) {
                    ShowAudioMenu();
                    return 0;
                }
                return DefWindowProcW(window_, message, wparam, lparam);
            case WM_SIZE: Layout();
                return 0;
            case WM_DPICHANGED: {
                if (!fullscreen_) {
                    const auto *suggested = reinterpret_cast<const RECT *>(lparam);
                    SetWindowPos(window_, nullptr, suggested->left, suggested->top,
                                 suggested->right - suggested->left, suggested->bottom - suggested->top,
                                 SWP_NOZORDER | SWP_NOACTIVATE);
                }
                Layout();
                RedrawWindow(window_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
                return 0;
            }
            case WM_GETMINMAXINFO: {
                if (!fullscreen_) {
                    auto *bounds = reinterpret_cast<MINMAXINFO *>(lparam);
                    RECT minimum{0, 0, kMinClientWidth, kMinClientHeight};
                    const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(window_, GWL_STYLE));
                    if (const DWORD ex_style = static_cast<DWORD>(GetWindowLongPtrW(window_, GWL_EXSTYLE));
                        AdjustWindowRectExForDpi(&minimum, style, GetMenu(window_) != nullptr, ex_style, GetDpiForWindow(window_))) {
                        bounds->ptMinTrackSize = {minimum.right - minimum.left, minimum.bottom - minimum.top};
                    }
                }
                return 0;
            }
            case WM_SYSCOMMAND: {
                switch (static_cast<UINT>(wparam & 0xfff0u)) {
                    case kSysSize1024: ApplyClientSizePreset(1024, 768); return 0;
                    case kSysSize1280: ApplyClientSizePreset(1280, 960); return 0;
                    case kSysSize1440: ApplyClientSizePreset(1440, 1080); return 0;
                    case kSysSize1920: ApplyClientSizePreset(1920, 1440); return 0;
                    case kSysSize2560: ApplyClientSizePreset(2560, 1440); return 0;
                    case kSysFullscreen: SetFullscreen(!fullscreen_); return 0;
                    default: return DefWindowProcW(window_, message, wparam, lparam);
                }
            }
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
                RemovePropW(window_, instance_property_.c_str());
                PostQuitMessage(0);
                return 0;
            default: return DefWindowProcW(window_, message, wparam, lparam);
        }
    }
} // namespace tenhou
