#include "application.hpp"
#include <cstring>
#include <cwctype>
#include "welcome_view.hpp"
#include "table_view.hpp"
#include "lobby_view.hpp"
#include "settings_view.hpp"
#include "window_layout.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace tenhou {
    namespace {
        constexpr const char *kWindowClass = "TENHOU_CLIENT";
        constexpr DWORD kWindowStyle = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        constexpr UINT kReplay = 4004, kWelcome = 4001, kSettings = 4023;

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
        lobby_.reset();
        if (IsMenu(client_menu_)) DestroyMenu(client_menu_);
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
        Check(SetProcessDPIAware() != FALSE, "SetProcessDPIAware");
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
        SetWindowTextW(window_, L"天鳳 / Windows版");
        client_menu_ = CreateMenu();
        Check(client_menu_ != nullptr, "Create client menu");
        AppendMenuW(client_menu_, MF_STRING, kSettings, L"设置...");
        AppendMenuW(client_menu_, MF_STRING, kReplay, L"打开牌谱...");
        AppendMenuW(client_menu_, MF_STRING, kWelcome, L"返回登录");
        Check(SetMenu(window_, client_menu_) != FALSE, "SetMenu");
        // Keep configured values as client dimensions after adding the new tools menu.
        RECT client_bounds{0, 0, width_, height_};
        Check(AdjustWindowRect(&client_bounds, kWindowStyle, TRUE) != FALSE, "AdjustWindowRect menu");
        SetWindowPos(window_, nullptr, 0, 0, client_bounds.right - client_bounds.left,
                     client_bounds.bottom - client_bounds.top,
                     SWP_NOMOVE | SWP_NOZORDER);
        welcome_ = std::make_unique<WelcomeView>(window_, [this](WelcomeView::Action action) {
                                                     if (action == WelcomeView::Action::Exit) PostMessageA(
                                                         window_, WM_CLOSE, 0, 0);
                                                     else if (action == WelcomeView::Action::Lobby || action ==
                                                              WelcomeView::Action::Register) {
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
                                                         connection.identity = NetworkUtf8(welcome_->Identity());
                                                         connection.gender = welcome_->Gender();
                                                         connection.password = NetworkUtf8(welcome_->Password());
                                                         connection.nickname = NetworkUtf8(welcome_->Nickname());
                                                         connection.register_account =
                                                                 action == WelcomeView::Action::Register;
                                                         connection.config_file = config_path_;
                                                         connection.native_service = true;
                                                         connection.interactive_games = true;
                                                         connection.match_games = true;
                                                         connection.recover_games = true;
                                                         connection.presentation = true;
                                                         connection.host = NetworkUtf8(
                                                             PreferenceString(
                                                                 config_path_, L"REBUILD", L"NativeHost",
                                                                 L"127.0.0.1"));
                                                         connection.port = GetPrivateProfileIntW(
                                                             L"REBUILD", L"NativePort", 10080, config_path_.c_str());
                                                         const auto account_key = Sha256(
                                                             Bytes(connection.identity.begin(),
                                                                   connection.identity.end())).substr(0, 16);
                                                         connection.session_file =
                                                                 config_path_.parent_path() / (
                                                                     L"native-session-" +
                                                                     std::to_wstring(connection.port) + (
                                                                         connection.identity == "NoName"
                                                                             ? L""
                                                                             : L"-" + NetworkWide(account_key)) +
                                                                     L".dat");
                                                         welcome_->SetLoginState(true, L"正在连接本机服务...");
                                                         lobby_->Login(connection);
                                                     } else settings_->Open();
                                                 }, [this](const std::string &message) { Log(message); });
        welcome_->LoadPreferences(config_path_);
        table_ = std::make_unique<TableView>(window_, [this](const std::string &message) { Log(message); },
                                             config_path_);
        table_->ConfigureArtwork(resource_directory_, screen_width_);
        lobby_ = std::make_unique<LobbyView>(window_, config_path_,
                                             [this](const std::string &message) { Log(message); });
        lobby_->ConfigureArtwork(resource_directory_, screen_width_);
        lobby_->SetConnectionObserver([this](const LobbySnapshot &s) {
            if (view_ != "welcome") return;
            if (s.phase == LobbyPhase::Lobby && s.native_service) {
                welcome_->ClearPassword();
                welcome_->SetLoginState(false, L"服务已确认登录");
                SetView("lobby");
            } else if (s.phase == LobbyPhase::Error) welcome_->SetLoginState(false, L"连接失败：" + NetworkWide(s.error));
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
        if (!options_.replay.empty()) table_->Load(options_.replay);
        SetView(view_);
        ShowWindow(window_, show == SW_HIDE ? SW_SHOWNORMAL : show);
        UpdateWindow(window_);
        MSG message{};
        BOOL result;
        while ((result = MotionTimer::GetMessage(&message)) > 0) {
            if (settings_->HandleMessage(message)) continue;
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
        InvalidateRect(window_, nullptr, FALSE);
    }

    void Application::LoadResources() {
        if (view_ == "welcome") welcome_->Load(resource_directory_, config_path_, screen_width_, screen_width_ * 3 / 4);
        else if (view_ == "replay") table_->Reload();
        else lobby_->ConfigureArtwork(resource_directory_, screen_width_);
    }

    void Application::SetView(const std::string &requested_view) {
        if (requested_view != "welcome" && requested_view != "lobby" && requested_view != "replay") throw
                std::runtime_error("页面不可用");
        if (welcome_ &&view_=="welcome" && requested_view != "welcome") welcome_->Save(config_path_);
        view_ = requested_view;
        welcome_->Show(view_ == "welcome");
        table_->Show(view_ == "replay");
        lobby_->Show(view_ == "lobby");
        EnableMenuItem(client_menu_, kReplay, MF_BYCOMMAND | (view_ == "lobby" ? MF_GRAYED : MF_ENABLED));
        EnableMenuItem(client_menu_, kWelcome, MF_BYCOMMAND | (view_ == "replay" ? MF_ENABLED : MF_GRAYED));
        Layout();
        LoadResources();
        Log("VIEW " + view_);
    }

    void Application::Paint(HDC dc) {
        RECT bounds{};
        GetClientRect(window_, &bounds);
        FillRect(dc, &bounds, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        if (view_ == "welcome" && welcome_) welcome_->Capture(dc);
        else if (view_ == "replay" && table_) table_->Capture(dc);
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
        switch (message) {
            case WM_APP + 1: throw std::runtime_error(reinterpret_cast<const char *>(lparam));
            case WM_SIZE: Layout();
                return 0;
            case WM_GETMINMAXINFO: {
                auto *bounds = reinterpret_cast<MINMAXINFO *>(lparam);
                RECT minimum{0, 0, kDefaultClientWidth, kDefaultClientHeight};
                Check(AdjustWindowRectEx(&minimum, static_cast<DWORD>(GetWindowLongPtrA(window_, GWL_STYLE)),
                                         GetMenu(window_) != nullptr,
                                         static_cast<DWORD>(GetWindowLongPtrA(window_, GWL_EXSTYLE))) != FALSE,
                      "AdjustWindowRectEx minimum");
                bounds->ptMinTrackSize = {minimum.right - minimum.left, minimum.bottom - minimum.top};
                bounds->ptMaxTrackSize = bounds->ptMinTrackSize;
                bounds->ptMaxSize = bounds->ptMinTrackSize;
                return 0;
            }
            case WM_SYSCOMMAND:
                if ((wparam & 0xfff0) == SC_SIZE || (wparam & 0xfff0) == SC_MAXIMIZE) return 0;
                return DefWindowProcA(window_, message, wparam, lparam);
            case WM_COMMAND:
                if (LOWORD(wparam) == kSettings) settings_->Open();
                else if (LOWORD(wparam) == kWelcome && view_ == "replay") SetView("welcome");
                else if (LOWORD(wparam) == kReplay && view_ != "lobby") {
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
                return 0;
            case WM_CLOSE:
                if (table_) table_->Show(false);
                if (lobby_) lobby_->Disconnect();
                SaveConfig();
                DestroyWindow(window_);
                return 0;
            case WM_DESTROY: RemovePropA(window_, instance_property_.c_str());
                PostQuitMessage(0);
                return 0;
            default: return DefWindowProcA(window_, message, wparam, lparam);
        }
    }
} // namespace tenhou
