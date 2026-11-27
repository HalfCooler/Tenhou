#pragma once
#include "resource_pack.hpp"
#include <windows.h>
#include <array>
#include <cstdint>
#include <fstream>
#include <memory>
#include <optional>
#include <string>

namespace Gdiplus {
    class Bitmap;
}

namespace tenhou {
    class WelcomeView;
    class TableView;
    class LobbyView;
    class OfflineLobbyView;
    class SettingsView;
    enum class PreferenceChange;

    struct Options {
        std::filesystem::path config_directory;
        std::filesystem::path resource_directory;
        std::filesystem::path replay;
        bool offline_lobby = false, service_lobby = false;
        int initial_width = 0, initial_height = 0;
    };

    struct FreeDeleter {
        void operator()(void* ptr) const noexcept {
            if (ptr != nullptr) LocalFree(ptr);
        }
    };

    Options ParseOptions(int argc, char **argv);

    class Application {
    public:
        explicit Application(Options options);

        ~Application();

        int Run(HINSTANCE instance, int show);

    private:
        static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

        LRESULT HandleMessage(UINT message, WPARAM wparam, LPARAM lparam);

        void Layout();

        void LoadResources() const;

        void SetView(const std::string &view);

        void Login(bool register_account);
        void OpenReplay();

        void Paint(HDC dc) const;

        void SaveConfig();

        void ApplyPreferences(PreferenceChange change);
        void UpdateAudioMeter();
        void PaintAudioMeter(const DRAWITEMSTRUCT &item) const;
        void ShowAudioMenu();
        void SetupSystemMenu() const;
        void SyncSystemMenu() const;
        void UpdateWindowTitle() const;
        void ApplyClientSizePreset(int client_width, int client_height);
        void SetFullscreen(bool enabled);
        int ResolveTableWidth(int client_width, int client_height) const;
        void ApplyScreenWidth(int next_width);
        void SyncAutoScreenWidth(int client_width, int client_height);

        void OpenLog();
        void RotateLogIfNeeded();
        void Log(const std::string &message);

        static void Check(bool success, const char *action);

        Options options_;
        std::filesystem::path executable_directory_, config_path_, resource_directory_, log_path_;
        std::ofstream log_;
        std::uint64_t log_bytes_ = 0;
        HWND window_ = nullptr, audio_meter_ = nullptr;
        HFONT font_ = nullptr;
        HANDLE mutex_ = nullptr;
        std::wstring instance_property_;
        ULONG_PTR gdiplus_token_ = 0;
        std::unique_ptr<WelcomeView> welcome_;
        std::unique_ptr<TableView> table_;
        std::unique_ptr<LobbyView> lobby_;
        std::unique_ptr<OfflineLobbyView> offline_lobby_;
        std::unique_ptr<SettingsView> settings_;
        struct PendingServerError {
            std::string caption, message;
            bool fatal = false;
        };
        std::uint64_t last_server_error_receipt_ = 0;
        std::optional<PendingServerError> pending_server_error_;
        int width_ = 1440, height_ = 1080;
        bool fullscreen_ = false;
        bool start_maximized_ = false, start_fullscreen_ = false;
        WINDOWPLACEMENT windowed_placement_{sizeof(WINDOWPLACEMENT)};
        DWORD windowed_style_ = 0, windowed_ex_style_ = 0;
        std::string view_;
        int configured_screen_width_ = 0; // 0 = auto ("default"), or 1024/1440/1920/2640.
        int screen_width_ = 1440;
        bool original_display_ = true;
        bool sound_effects_only_ = false;
        bool mute_effects_ = false, mute_music_ = false;
        std::array<int, 3> audio_bars_{};
        float music_meter_range_ = .1f;
        std::string error_;
        int exit_code_ = 0;
    };
} // namespace tenhou
