#pragma once
#include "original_display.hpp"
#include "paint_buffer.hpp"
#include <functional>

namespace tenhou {
    // A local presentation barrier. Acknowledging never sends a protocol frame.
    class MatchIntroView {
    public:
        MatchIntroView(HWND parent, std::function<void()> finished);

        ~MatchIntroView();

        void Configure(const std::filesystem::path &directory, int width);

        void Move(RECT bounds);

        void Begin(const ReplayState &state, unsigned type, const std::string &dan);

        void Resume(const ReplayState &state, unsigned type, const std::string &dan);

        void Cancel();

        void Finish();

        bool Active() const { return active_; }
        bool Resuming() const { return active_ && resuming_; }

        void SetPaused(bool paused) {
            if (paused && !paused_at_) paused_at_ = GetTickCount64();
            else if (!paused && paused_at_) {
                deadline_ += GetTickCount64() - paused_at_;
                paused_at_ = 0;
            }
        }

        void Capture(HDC dc);

        void SetSoundPlayer(std::function<void(unsigned)> play) { play_sound_ = std::move(play); }

    private:
        static LRESULT CALLBACK Procedure(HWND, UINT, WPARAM, LPARAM);

        static LRESULT CALLBACK OkProcedure(HWND, UINT, WPARAM, LPARAM);

        void Paint(HDC dc);

        void Arrange();

        void HideSiblings();

        void PaintOk(HDC dc, RECT bounds, bool pressed);

        HWND parent_ = nullptr, window_ = nullptr, ok_ = nullptr;
        WNDPROC ok_procedure_ = nullptr;
        std::function<void()> finished_;
        std::function<void(unsigned)> play_sound_;
        DisplayArtwork artwork_;
        PaintBuffer buffer_;
        Image background_;
        std::filesystem::path directory_;
        ReplayState state_;
        std::array<std::wstring, 4> ranks_;
        unsigned type_ = 0;
        int width_ = 1440, seconds_ = 5;
        ULONGLONG deadline_ = 0;
        ULONGLONG paused_at_ = 0;
        bool active_ = false;
        bool resuming_ = false;
        RECT bounds_{};
        std::vector<HWND> hidden_;
    };
} // namespace tenhou
