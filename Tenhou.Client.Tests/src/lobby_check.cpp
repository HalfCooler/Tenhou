#include "lobby_view.hpp"
#include "settings_view.hpp"
#include <commctrl.h>
#include <fstream>
#include <iostream>

// Interactive local-only harness: the production LobbyView and transport, with a
// synthetic guest session and isolated configuration. No account dialog or user data.
int main(int argc, char **argv) {
    if (argc != 3) { std::cerr << "Usage: tenhou_lobby_check LOCAL_PORT CHECK_DIRECTORY\n"; return 1; }
    try {
        Gdiplus::GdiplusStartupInput input; ULONG_PTR token = 0;
        if (Gdiplus::GdiplusStartup(&token, &input, nullptr) != Gdiplus::Ok) return 1;
        INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES}; InitCommonControlsEx(&controls);
        const auto directory = std::filesystem::absolute(argv[2]); std::filesystem::create_directories(directory);
        const auto config = directory / "config.ini";
        std::ofstream log(directory / "client.log");
        WNDCLASSEXW wc{sizeof(wc)}; wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = L"TENHOU_LOBBY_CHECK";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpfnWndProc = [](HWND w, UINT m, WPARAM wp, LPARAM lp) -> LRESULT {
            if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
            return DefWindowProcW(w, m, wp, lp);
        };
        RegisterClassExW(&wc);
        const HWND window = CreateWindowExW(0, wc.lpszClassName, L"Tenhou Sichuan UI Check", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            80, 50, 1200, 900, nullptr, nullptr, wc.hInstance, nullptr);
        {
            tenhou::LobbyView view(window, config, [&](const std::string &text) { log << text << '\n'; log.flush(); });
            view.ConfigureArtwork(std::filesystem::absolute(argv[0]).parent_path() / "resources", 1440);
            tenhou::ClientPreferences preferences; preferences.se_volume = 0;
            view.ApplyPreferences(preferences, tenhou::PreferenceChange::Initial);
            RECT r{}; GetClientRect(window, &r); view.Resize(r.right, r.bottom); view.Show(true);
            tenhou::ConnectionOptions options;
            options.port = static_cast<unsigned>(std::stoul(argv[1])); options.native_service = true;
            options.interactive_games = options.match_games = options.recover_games = true;
            options.config_file = config; options.session_file = directory / "session.ini";
            view.Login(options);
            MSG message{};
            while (GetMessageW(&message, nullptr, 0, 0) > 0) {
                if (message.message == WM_KEYDOWN && view.HandleKey(static_cast<UINT>(message.wParam))) continue;
                TranslateMessage(&message); DispatchMessageW(&message);
            }
        }
        Gdiplus::GdiplusShutdown(token); return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
