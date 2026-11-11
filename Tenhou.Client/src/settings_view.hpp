#pragma once
#include "client_preferences.hpp"
#include "replay_audio.hpp"
#include <commctrl.h>
#include <functional>

namespace tenhou {
    enum class PreferenceChange { Other, Sound, Artwork, Initial, MusicPreviewBegin, MusicPreviewEnd };

    class SettingsView {
    public:
        SettingsView(HWND parent, std::filesystem::path config, std::filesystem::path resources,
                     std::function<void(PreferenceChange)> changed, std::function<void(const std::string &)> log);

        ~SettingsView();

        void Open();

        HWND Window() const { return window_; }

        bool HandleMessage(MSG &message);

    private:
        struct Page {
            SettingsView *owner = nullptr;
            int id = 0;
        };

        static INT_PTR CALLBACK Procedure(HWND, UINT, WPARAM, LPARAM);

        static int CALLBACK SheetCallback(HWND, UINT, LPARAM);

        static LRESULT CALLBACK SheetProcedure(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

        INT_PTR Message(HWND page, int id, UINT message, WPARAM wparam, LPARAM lparam);

        void InitOther(HWND page);

        void InitSound(HWND page);

        void SoundList(HWND page);

        void Preview(HWND page);

        void SoundOverride(HWND page, int command);

        void Volume(HWND page);

        void InitMusic(HWND page);

        void MusicList(HWND page);

        void MusicPreview(HWND page);

        void MusicCommand(HWND page, int command);

        void MusicVolume(HWND page, int control);

        void InitReplay(HWND page);

        void ReplayCommand(HWND page, int command);

        void SaveReplaySettings(HWND page);

        void InitAppearance(HWND page, int id);

        void Appearance(HWND page, int id, int command);

        void BackgroundLayer(HWND page, bool save);

        unsigned SelectedSound(HWND page) const;

        HWND parent_ = nullptr, window_ = nullptr;
        std::filesystem::path config_, resources_;
        std::function<void(PreferenceChange)> changed_;
        std::function<void(const std::string &)> log_;
        std::array<Page, 6> pages_{};
        ReplayAudio audio_;
        ReplaySoundBank sounds_;
        bool initializing_ = false;
        bool music_previewing_ = false;
        bool owner_disabled_ = false;
        HWND owner_focus_ = nullptr;
        ClientPreferences appearance_;
        int background_layer_ = 0;
    };
} // namespace tenhou
