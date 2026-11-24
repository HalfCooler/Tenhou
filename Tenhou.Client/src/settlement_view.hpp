#pragma once
#include "settlement_sequencer.hpp"
#include "table_scene.hpp"
#include "original_display.hpp"
#include "paint_buffer.hpp"
#include "settlement_scene.hpp"
#include <windows.h>
#include <functional>

namespace tenhou {
    class SettlementView {
    public:
        explicit SettlementView(HWND parent);

        ~SettlementView();

        void Set(const SettlementState &state, const std::array<std::string, 4> &names, bool red);
        void SetRoundFinished(bool finished);

        void Show(bool visible);

        void Move(RECT bounds);

        void Configure(const std::filesystem::path &directory, int width);

        void Capture(HDC dc);

        void SetSoundPlayer(std::function<void(unsigned)> play) { play_sound_ = std::move(play); }
        void SetSoundStopper(std::function<void()> stop) { stop_sound_ = std::move(stop); }

        void SetSoundEnabled(bool enabled);

        void SetAnimationEnabled(bool enabled);

        void SetPeekEnabled(bool enabled);

        void SetTileColor(const std::array<float, 3> &rgb) {
            tile_color_ = rgb;
            artwork_.SetColor(rgb);
            InvalidateRect(window_, nullptr, FALSE);
        }

        bool HandleKey(UINT key);

        void SetPaused(bool paused) { sequencer_.SetPaused(paused, GetTickCount64()); }

        void SetContext(int dealer, int honba, int sticks);

        void SetBackgroundPainter(std::function<void(HDC, RECT)> paint) { background_painter_ = std::move(paint); }
        void SetConfirmHandler(std::function<void()> confirm) { confirm_handler_ = std::move(confirm); }

        void SetConfirmEnabled(bool enabled);

        void SetContinuation(bool available);

        void Present();

        void CancelPresentation();

        bool Animating() const { return sequencer_.Animating(); }
        bool FinalPage() const { return sequencer_.FinalPage(); }

    private:
        friend struct SettlementPresentationCheck;
        static LRESULT CALLBACK Procedure(HWND, UINT, WPARAM, LPARAM);

        void ApplyEffects(const SettlementEffects &effects);

        void Select();
        void ChangePatternPage(int step);

        void Arrange();

        void Peek(bool down);

        void Paint(HDC dc);

        void PresentationTick();

        void ConfirmTick();

        void UpdateText();

        void Confirm();

        void Menu(POINT point);

        SettlementLayout Layout();

        RECT Target() const;

        HWND window_ = nullptr, choices_ = nullptr, details_ = nullptr, ok_ = nullptr;
        HWND previous_patterns_ = nullptr, next_patterns_ = nullptr;
        HFONT font_ = nullptr;
        PaintBuffer paint_buffer_;
        SettlementSequencer sequencer_;
        std::filesystem::path directory_;
        int canvas_ = 1440;
        TableArtwork artwork_;
        std::array<float, 3> tile_color_{0, 0, 38.0f / 255.0f};
        DisplayArtwork display_artwork_;
        std::string resource_error_;
        std::function<void(unsigned)> play_sound_;
        std::function<void()> stop_sound_;
        std::function<void(HDC, RECT)> background_painter_;
        std::function<void()> confirm_handler_;
        bool peeking_ = false;
        bool peek_enabled_ = true;
        HWND peek_focus_ = nullptr;
    };
} // namespace tenhou
