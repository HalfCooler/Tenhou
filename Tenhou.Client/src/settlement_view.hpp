#pragma once
#include "settlement.hpp"
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

        void SetPaused(bool paused) {
            if (paused && !paused_at_) paused_at_ = GetTickCount64();
            else if (!paused && paused_at_) {
                if (confirm_deadline_) confirm_deadline_ += GetTickCount64() - paused_at_;
                paused_at_ = 0;
            }
        }

        void SetContext(int dealer, int honba, int sticks);

        void SetBackgroundPainter(std::function<void(HDC, RECT)> paint) { background_painter_ = std::move(paint); }
        void SetConfirmHandler(std::function<void()> confirm) { confirm_handler_ = std::move(confirm); }

        void SetConfirmEnabled(bool enabled) {
            if (confirm_enabled_ != enabled) {
                confirm_enabled_ = enabled;
                Arrange();
            }
        }

        void SetContinuation(bool available) {
            if (continuation_ != available) {
                continuation_ = available;
                Arrange();
            }
        }

        void Present();

        void CancelPresentation();

        bool Animating() const { return shown_yaku_ != static_cast<std::size_t>(-1) && !Complete(); }
        bool FinalPage() const { return visible_ && state_.has_final && selected_ == state_.receipts.size(); }

    private:
        static LRESULT CALLBACK Procedure(HWND, UINT, WPARAM, LPARAM);

        void Select();

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

        std::size_t YakuCount() const;

        bool UraPending() const;

        bool Complete() const;

        void ArmPresentationTimer();

        std::size_t PreviewTiles() const;

        HWND window_ = nullptr, choices_ = nullptr, details_ = nullptr, ok_ = nullptr;
        ULONGLONG confirm_deadline_ = 0;
        ULONGLONG paused_at_ = 0;
        int confirm_seconds_ = -1;
        bool confirm_ready_ = false;
        bool text_details_ = false;
        HFONT font_ = nullptr;
        PaintBuffer paint_buffer_;
        SettlementState state_;
        std::array<std::string, 4> names_;
        bool red_ = true;
        std::size_t selected_ = 0;
        std::filesystem::path directory_;
        int canvas_ = 1440, tile_top_ = 88, text_top_ = 200;
        TableArtwork artwork_;
        std::array<float, 3> tile_color_{0, 0, 38.0f / 255.0f};
        DisplayArtwork display_artwork_;
        std::string resource_error_;
        std::function<void(unsigned)> play_sound_;
        std::function<void()> stop_sound_;
        std::function<void(HDC, RECT)> background_painter_;
        std::function<void()> confirm_handler_;
        int dealer_ = 0, honba_ = -1, sticks_ = -1;
        bool confirm_enabled_ = true;
        bool continuation_ = false;
        bool visible_ = false;
        bool peeking_ = false;
        bool peek_enabled_ = true;
        HWND peek_focus_ = nullptr;
        bool sound_enabled_ = false;
        bool animation_enabled_ = true;
        bool ura_revealed_ = true;
        bool totals_revealed_ = true, ura_pause_done_ = false;
        unsigned presentation_frames_ = 0;
        std::size_t shown_yaku_ = static_cast<std::size_t>(-1), shown_tiles_ = static_cast<std::size_t>(-1);
    };
} // namespace tenhou
