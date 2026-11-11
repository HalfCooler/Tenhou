#pragma once
#include "resource_pack.hpp"
#include "object_pack.hpp"
#include <windows.h>
#include <fstream>
#include <memory>

namespace Gdiplus {
    class Bitmap;
}

namespace tenhou {
    class WelcomeView;
    class TableView;
    class LobbyView;
    class SettingsView;
    enum class PreferenceChange;

    struct Options {
        std::filesystem::path config_directory;
        std::filesystem::path resource_directory;
        std::filesystem::path replay;
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

        void LoadResources();

        void SetView(const std::string &view);

        void Paint(HDC dc);

        void SaveConfig();

        void ApplyPreferences(PreferenceChange change);

        void Log(const std::string &message);

        void Check(bool success, const char *action) const;

        Options options_;
        std::filesystem::path executable_directory_, config_path_, resource_directory_;
        std::ofstream log_;
        HWND window_ = nullptr;
        HMENU client_menu_ = nullptr;
        HFONT font_ = nullptr;
        HANDLE mutex_ = nullptr;
        std::string instance_property_;
        ULONG_PTR gdiplus_token_ = 0;
        std::unique_ptr<WelcomeView> welcome_;
        std::unique_ptr<TableView> table_;
        std::unique_ptr<LobbyView> lobby_;
        std::unique_ptr<SettingsView> settings_;
        int width_ = 1440, height_ = 1080;
        std::string view_;
        int screen_width_ = 1440;
        bool original_display_ = true;
        bool sound_effects_only_ = false;
        std::string error_;
        int exit_code_ = 0;
    };
} // namespace tenhou
