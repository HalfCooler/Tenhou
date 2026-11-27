#include "match_intro_view.hpp"
#include "resource_manager.hpp"
#include "background.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <stdexcept>

namespace tenhou {
    namespace {
        constexpr wchar_t kClass[] = L"TenhouRebuildMatchIntro";

        std::wstring Wide(const std::string &value) {
            const int count = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
            std::wstring out(count, L' ');
            if (count) MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), out.data(), count);
            return out;
        }

        void Text(Gdiplus::Graphics &g, const std::wstring &value, const float x, const float y, const float w, const float h,
                  const float size, const wchar_t *face, const Gdiplus::Color color = Gdiplus::Color(255, 255, 255, 255)) {
            Gdiplus::FontFamily family(face);
            Gdiplus::Font font(family.IsAvailable() ? &family : Gdiplus::FontFamily::GenericSansSerif(), size,
                               Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            Gdiplus::StringFormat format;
            format.SetAlignment(Gdiplus::StringAlignmentCenter);
            format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
            format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
            Gdiplus::SolidBrush brush(color);
            g.DrawString(value.c_str(), -1, &font, Gdiplus::RectF(x, y, w, h), &format, &brush);
        }
    }

    MatchIntroView::MatchIntroView(const HWND parent, std::function<void()> finished)
        : parent_(parent), finished_(std::move(finished)) {
        const auto instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent, GWLP_HINSTANCE));
        WNDCLASSEXW wc{sizeof(wc)};
        wc.hInstance = instance;
        wc.lpfnWndProc = Procedure;
        wc.lpszClassName = kClass;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) throw std::runtime_error(
            "Cannot register match introduction");
        window_ = CreateWindowExW(WS_EX_CONTROLPARENT, kClass, L"Match start", WS_CHILD | WS_CLIPCHILDREN,
                                  0, 0, 1, 1, parent, reinterpret_cast<HMENU>(5500), instance, this);
        if (!window_) throw std::runtime_error("Cannot create match introduction");
        ok_ = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                              0, 0, 1, 1, window_, reinterpret_cast<HMENU>(IDOK), instance, nullptr);
        if (!ok_) throw std::runtime_error("Cannot create match introduction OK");
        SetWindowLongPtrW(ok_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        ok_procedure_ = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(ok_, GWLP_WNDPROC,
                                                                    reinterpret_cast<LONG_PTR>(OkProcedure)));
        SendMessageW(ok_, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), FALSE);
    }

    MatchIntroView::~MatchIntroView() {
        Cancel();
        if (IsWindow(window_)) DestroyWindow(window_);
    }

    void MatchIntroView::Configure(const std::filesystem::path &directory, const int width) {
        Cancel();
        directory_ = directory;
        width_ = width;
        background_ = {};
        artwork_.Configure(directory, width);
    }

    void MatchIntroView::Move(const RECT bounds) {
        bounds_ = FitNativeCanvas(bounds, width_);
        MoveWindow(window_, bounds_.left, bounds_.top, bounds_.right - bounds_.left, bounds_.bottom - bounds_.top,
                   TRUE);
        Arrange();
        if (active_) HideSiblings();
    }

    void MatchIntroView::HideSiblings() {
        for (HWND child = GetWindow(parent_, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
            if (child == window_ || !(GetWindowLongPtrW(child, GWL_STYLE) & WS_VISIBLE)) continue;
            if (std::find(hidden_.begin(), hidden_.end(), child) == hidden_.end()) hidden_.push_back(child);
            ShowWindow(child, SW_HIDE);
        }
    }

    void MatchIntroView::Arrange() const
    {
        if (!ok_) return;
        RECT r{};
        GetClientRect(window_, &r);
        const float s = r.right / 1024.0f;
        // Screenshot: 540x400 confirmation panel; OK is 55x21, 351 below its top.
        MoveWindow(ok_, static_cast<int>((512 - 23) * s), static_cast<int>((384 - 200 + 351) * s),
                   std::max(1, static_cast<int>(55 * s)), std::max(1, static_cast<int>(21 * s)), TRUE);
    }

    void MatchIntroView::Begin(const ReplayState &state, const unsigned type) {
        if (!state.initialized) return;
        Cancel();
        state_ = state;
        type_ = type;
        seconds_ = kReadySeconds;
        resuming_ = false;
        EnableWindow(ok_, TRUE);
        ShowWindow(ok_, SW_SHOW);
        if (play_sound_) play_sound_(102);
        // Start the visible countdown after first-use sound preparation.
        deadline_ = GetTickCount64() + kReadySeconds * 1000;
        if (!SetTimer(window_, 1, 100, nullptr)) throw std::runtime_error("Cannot start match introduction countdown");
        active_ = true;
        HideSiblings();
        SetWindowPos(window_, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        SetFocus(ok_);
        InvalidateRect(window_, nullptr, FALSE);
    }

    void MatchIntroView::Resume(const ReplayState &state, const unsigned type) {
        // Original SAIKAI: 004ab91f uses a blank, disabled button and does not
        // post the TAIKYOKU-only 8302 countdown. INIT/REINIT closes the popup.
        const bool beginning = !Resuming();
        if (beginning) {
            Cancel();
            resuming_ = true;
            active_ = true;
            EnableWindow(ok_, FALSE);
            ShowWindow(ok_, SW_HIDE);
        }
        state_ = state;
        type_ = type;
        HideSiblings();
        SetWindowPos(window_, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        InvalidateRect(window_, nullptr, FALSE);
        // Recovery can finish in the same network dispatch. Present the first
        // waiting frame synchronously; do not add an artificial minimum duration.
        if (beginning) UpdateWindow(window_);
    }

    void MatchIntroView::Cancel() {
        active_ = false;
        resuming_ = false;
        paused_at_ = 0;
        if (window_) {
            KillTimer(window_, 1);
            ShowWindow(window_, SW_HIDE);
        }
        for (HWND child: hidden_) if (IsWindow(child) && GetParent(child) == parent_) ShowWindow(child, SW_SHOW);
        hidden_.clear();
    }

    void MatchIntroView::Finish() {
        if (!active_ || resuming_ || paused_at_) return;
        Cancel();
        SetFocus(parent_);
        if (finished_) finished_();
    }

    void MatchIntroView::PaintOk(const HDC dc, const RECT bounds, const bool pressed) {
        Gdiplus::Graphics g(dc);
        const int w = bounds.right - bounds.left, h = bounds.bottom - bounds.top;
        Gdiplus::LinearGradientBrush fill(Gdiplus::Point(0, 0), Gdiplus::Point(0, h),
                                          Gdiplus::Color(255, pressed ? 220 : 255, pressed ? 220 : 255,
                                                         pressed ? 220 : 255), Gdiplus::Color(255, 222, 222, 222));
        g.FillRectangle(&fill, bounds.left, bounds.top, w, h);
        Gdiplus::Pen border(Gdiplus::Color(255, 150, 150, 150));
        g.DrawRectangle(&border, bounds.left, bounds.top, w - 1, h - 1);
        Text(g, L"OK", static_cast<float>(bounds.left), static_cast<float>(bounds.top), static_cast<float>(w),
             static_cast<float>(h),
             h * .57f, L"Segoe UI", Gdiplus::Color(255, 20, 20, 20));
    }

    void MatchIntroView::Paint(const HDC dc) {
        RECT r{};
        GetClientRect(window_, &r);
        SetDCBrushColor(dc, RGB(24, 45, 44));
        FillRect(dc, &r, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        try {
            const int h = width_ * 3 / 4;
            if (background_.bgra.empty()) background_ = BuildBackground(ResourceManager(directory_).Path(AssetId::Background), width_, h);
            const int ui_w = width_ == 1024 ? 1024 : 1440;
            const int ui_h = ui_w * 3 / 4;
            const float hud_scale = static_cast<float>(width_) / static_cast<float>(ui_w);
            FinalScene scene(artwork_.Text(), width_ == 1024, GetDpiForWindow(window_));
            const float hd = width_ == 1024 ? 1.4f : 1.f;
            const auto image = [&](std::string key, Gdiplus::RectF bounds, std::function<void(Gdiplus::Graphics &)> draw) {
                bounds.X *= hd; bounds.Y *= hd; bounds.Width *= hd; bounds.Height *= hd;
                scene.Image(std::move(key), bounds, [hd, draw](Gdiplus::Graphics &g) { g.ScaleTransform(hd, hd); draw(g); });
            };
            {
                Gdiplus::Graphics g(dc);
                const float s = r.right / static_cast<float>(width_);
                g.ScaleTransform(s, s);
                g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
                Gdiplus::Bitmap bg(width_, h, width_ * 4, PixelFormat32bppARGB, background_.bgra.data());
                g.DrawImage(&bg, 0, 0);
                g.ScaleTransform(hud_scale, hud_scale);
                if (state_.initialized) {
                    const auto hud = BuildOriginalHud(state_, ui_w);
                    artwork_.Sprite(g, 589, hud.panel[0], hud.panel[1]);
                    const int cx = static_cast<int>(hud.center[0]), cy = static_cast<int>(hud.center[1]);
                    const float u = hud.scale;
                    const TextKey winds[]{TextKey::center_east, TextKey::center_south, TextKey::center_west, TextKey::center_north};
                    const TextKey hands[]{TextKey::center_hand_one, TextKey::center_hand_two, TextKey::center_hand_three, TextKey::center_hand_four};
                    scene.Text(artwork_.TextRun(winds[state_.round / 4 % 4], cx - 30 * u, cy - 17 * u));
                    scene.Text(artwork_.TextRun(hands[state_.round % 4], cx - 10 * u, cy - 17 * u));
                    scene.Text(artwork_.TextRun(TextKey::center_hand_unit, cx + 10 * u, cy - 17 * u));
                    if (!state_.variant) {
                        artwork_.Sprite(g, 401, cx - static_cast<int>(18 * u), cy + static_cast<int>(6 * u), false, true);
                        artwork_.Sprite(g, 402, cx + static_cast<int>(2 * u), cy + static_cast<int>(6 * u), false, true);
                        Text(g, std::to_wstring(state_.sticks), cx - 12 * u, cy - 3 * u, 10 * u, 18 * u, 11 * u, L"System",
                             Gdiplus::Color(255, 0, 0, 0));
                        Text(g, std::to_wstring(state_.honba), cx + 8 * u, cy - 3 * u, 10 * u, 18 * u, 11 * u, L"System",
                             Gdiplus::Color(255, 0, 0, 0));
                    }
                }
                artwork_.PaintChromeBackground(g);
            }
            artwork_.AppendChrome(scene, type_, {true, false, false, false}, state_.variant);
            artwork_.AppendStatus(scene, state_);
            const float u = ui_w == 1440 ? 1.4f : 1.0f;
            const float left = ui_w / 2.0f - 270 * u, top = ui_h / 2.0f - 200 * u;
            image("intro-panel", {left, top, 540 * u, 400 * u}, [=](Gdiplus::Graphics &g) {
                Gdiplus::SolidBrush shade(Gdiplus::Color(220, 0, 0, 0));
                g.FillRectangle(&shade, left, top, 540 * u, 400 * u);
                Gdiplus::Pen border(Gdiplus::Color(180, 180, 180), 1);
                g.DrawRectangle(&border, left, top, 540 * u, 400 * u);
            });
            scene.Text(artwork_.TextRun(resuming_ ? TextKey::title_resume : TextKey::title_match,
                left + 270 * u, top + 71 * u, true));
            // Absolute/relative seat 0 stays at the bottom; dealer only changes wind.
            constexpr int xs[]{175, 315, 175, 35}, ys[]{274, 239, 204, 239};
            for (int i: {2, 3, 1, 0}) {
                if (!SeatPresent(state_.seat_mask, i)) continue;
                const float x = left + xs[i] * u, y = top + ys[i] * u;
                image("intro-seat-" + std::to_string(i), {x, y, 190 * u, 42 * u}, [=](Gdiplus::Graphics &g) {
                    Gdiplus::LinearGradientBrush card(Gdiplus::PointF(x, y), Gdiplus::PointF(x, y + 42 * u),
                        Gdiplus::Color(255, 34, 34, 34), Gdiplus::Color(136, 17, 17, 17));
                    g.FillRectangle(&card, x, y, 190 * u, 42 * u);
                    Text(g, Wide(state_.players[i].name), x + 62 * u, y + 2 * u, 105 * u, 19 * u, (40.0f / 3) * u, L"System");
                    const auto rank = state_.players[i].rank;
                    Text(g, rank < 0 ? L"未提供" : Wide(SeatRankLabel(rank)), x + 62 * u, y + 20 * u, 105 * u, 18 * u, (32.0f / 3) * u, L"System");
                });
                constexpr TextKey winds[]{TextKey::seat_neutral_east, TextKey::seat_neutral_south,
                    TextKey::seat_neutral_west, TextKey::seat_neutral_north};
                if (state_.initialized) scene.Text(artwork_.TextRun(winds[SeatWind(state_.seat_mask, state_.dealer, i)],
                    x + 30 * u, y + 21 * u, true, true));
            }
            if (!resuming_) image("intro-countdown-" + std::to_string(seconds_),
                {left + 304 * u, top + 350 * u, 16 * u, 10 * u}, [=](Gdiplus::Graphics &g) {
                    Text(g, std::to_wstring(seconds_), left + 304 * u, top + 350 * u, 16 * u, 10 * u, 8 * u, L"System");
                });
            scene.Paint(dc, ui_w, ui_h, r.right, r.bottom, r);
        } catch (const std::exception &e) {
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(255, 190, 160));
            const auto message = Wide(std::string("Match start artwork failed: ") + e.what());
            r.left += 12;
            r.top += 12;
            DrawTextW(dc, message.c_str(), -1, &r, DT_WORDBREAK);
        }
    }

    void MatchIntroView::Capture(const HDC dc) {
        if (!active_) return;
        const int saved = SaveDC(dc);
        SetViewportOrgEx(dc, bounds_.left, bounds_.top, nullptr);
        Paint(dc);
        if (resuming_) {
            RestoreDC(dc, saved);
            return;
        }
        RECT button{};
        GetWindowRect(ok_, &button);
        MapWindowPoints(nullptr, window_, reinterpret_cast<POINT *>(&button), 2);
        SetViewportOrgEx(dc, bounds_.left + button.left, bounds_.top + button.top, nullptr);
        PaintOk(dc, {0, 0, button.right - button.left, button.bottom - button.top}, false);
        RestoreDC(dc, saved);
    }

LRESULT CALLBACK MatchIntroView::Procedure(const HWND window, const UINT message, const WPARAM wparam, const LPARAM lparam) {
        auto *self = reinterpret_cast<MatchIntroView *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = static_cast<MatchIntroView *>(reinterpret_cast<CREATESTRUCTW *>(lparam)->lpCreateParams);
            self->window_ = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self) {
            if (message == WM_ERASEBKGND) return 1;
            if (message == WM_SIZE) {
                self->Arrange();
                return 0;
            }
            if (message == WM_DRAWITEM && wparam == IDOK) {
                const auto &item = *reinterpret_cast<DRAWITEMSTRUCT *>(lparam);
                self->PaintOk(item.hDC, item.rcItem, (item.itemState & ODS_SELECTED) != 0);
                return TRUE;
            }
            if ((message == WM_COMMAND && LOWORD(wparam) == IDOK) || (message == WM_KEYDOWN && wparam == VK_RETURN)) {
                self->Finish();
                return 0;
            }
            if (message == WM_TIMER && self->active_ && !self->resuming_) {
                if (self->paused_at_) return 0;
                const auto now = GetTickCount64();
                if (now >= self->deadline_) self->Finish();
                else {
                    const int seconds = static_cast<int>((self->deadline_ - now + 999) / 1000);
                    if (seconds != self->seconds_) {
                        self->seconds_ = seconds;
                        if (self->play_sound_) self->play_sound_(102);
                        InvalidateRect(window, nullptr, FALSE);
                    }
                }
                return 0;
            }
            if (message == WM_PAINT) {
                self->buffer_.Paint(window, [&](const HDC dc) { self->Paint(dc); });
                return 0;
            }
            if (message == WM_PRINTCLIENT) {
                self->Paint(reinterpret_cast<HDC>(wparam));
                return 0;
            }
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }

LRESULT CALLBACK MatchIntroView::OkProcedure(const HWND window, const UINT message, const WPARAM wparam, const LPARAM lparam) {
        auto *self = reinterpret_cast<MatchIntroView *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (self) {
            if (message == WM_GETDLGCODE && wparam == VK_RETURN) return DLGC_WANTALLKEYS;
            if (message == WM_KEYDOWN && wparam == VK_RETURN) {
                self->Finish();
                return 0;
            }
            return CallWindowProcW(self->ok_procedure_, window, message, wparam, lparam);
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }
} // namespace tenhou
