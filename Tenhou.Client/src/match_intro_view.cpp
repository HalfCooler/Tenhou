#include "match_intro_view.hpp"
#include "resource_manager.hpp"
#include "background.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <charconv>
#include <sstream>
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

        std::array<std::wstring, 4> Ranks(const std::string &dan) {
            std::array<std::wstring, 4> out;
            out.fill(L"未提供");
            const wchar_t *labels[]{
                L"新人", L"９級", L"８級", L"７級", L"６級", L"５級", L"４級", L"３級", L"２級", L"１級",
                L"初段", L"二段", L"三段", L"四段", L"五段", L"六段", L"七段", L"八段", L"九段", L"十段", L"天鳳位"
            };
            std::istringstream stream(dan);
            std::string field;
            for (int i = 0; i < 4 && std::getline(stream, field, ','); ++i) {
                int value = -1;
                const auto parsed = std::from_chars(field.data(), field.data() + field.size(), value);
                if (parsed.ec == std::errc() && parsed.ptr == field.data() + field.size() && value >= 0 && value <= 20)
                    out[i] = labels[value];
            }
            return out;
        }

        void Text(Gdiplus::Graphics &g, const std::wstring &value, float x, float y, float w, float h,
                  float size, const wchar_t *face, Gdiplus::Color color = Gdiplus::Color(255, 255, 255, 255)) {
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

    MatchIntroView::MatchIntroView(HWND parent, std::function<void()> finished)
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

    void MatchIntroView::Configure(const std::filesystem::path &directory, int width) {
        Cancel();
        directory_ = directory;
        width_ = width;
        background_ = {};
        artwork_.Configure(directory, width);
    }

    void MatchIntroView::Move(RECT bounds) {
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

    void MatchIntroView::Arrange() {
        if (!ok_) return;
        RECT r{};
        GetClientRect(window_, &r);
        const float s = r.right / 1024.0f;
        // Screenshot: 540x400 confirmation panel; OK is 55x21, 351 below its top.
        MoveWindow(ok_, static_cast<int>((512 - 23) * s), static_cast<int>((384 - 200 + 351) * s),
                   std::max(1, static_cast<int>(55 * s)), std::max(1, static_cast<int>(21 * s)), TRUE);
    }

    void MatchIntroView::Begin(const ReplayState &state, unsigned type, const std::string &dan) {
        if (!state.initialized) return;
        Cancel();
        state_ = state;
        type_ = type;
        ranks_ = Ranks(dan);
        seconds_ = 5;
        resuming_ = false;
        EnableWindow(ok_, TRUE);
        ShowWindow(ok_, SW_SHOW);
        if (play_sound_) play_sound_(102);
        // First-use sound preparation can block. Start the five visible seconds
        // after it returns, so the first countdown beat is not shortened.
        deadline_ = GetTickCount64() + 5000;
        if (!SetTimer(window_, 1, 100, nullptr)) throw std::runtime_error("Cannot start match introduction countdown");
        active_ = true;
        HideSiblings();
        SetWindowPos(window_, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        SetFocus(ok_);
        InvalidateRect(window_, nullptr, FALSE);
    }

    void MatchIntroView::Resume(const ReplayState &state, unsigned type, const std::string &dan) {
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
        ranks_ = Ranks(dan);
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

    void MatchIntroView::PaintOk(HDC dc, RECT bounds, bool pressed) {
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

    void MatchIntroView::Paint(HDC dc) {
        RECT r{};
        GetClientRect(window_, &r);
        SetDCBrushColor(dc, RGB(24, 45, 44));
        FillRect(dc, &r, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        try {
            const int h = width_ * 3 / 4;
            if (background_.bgra.empty()) background_ = BuildBackground(ResourceManager(directory_).Path(AssetId::Background), width_, h);
            Gdiplus::Graphics g(dc);
            const float s = r.right / static_cast<float>(width_);
            g.ScaleTransform(s, s);
            g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
            Gdiplus::Bitmap bg(width_, h, width_ * 4, PixelFormat32bppARGB, background_.bgra.data());
            g.DrawImage(&bg, 0, 0);
            if (state_.initialized) {
                const auto hud = BuildOriginalHud(state_, width_);
                artwork_.Sprite(g, 589, hud.panel[0], hud.panel[1]);
                const int cx = static_cast<int>(hud.center[0]), cy = static_cast<int>(hud.center[1]);
                const float u = hud.scale;
                artwork_.Sprite(g, 407 + state_.round / 4 % 4, cx - static_cast<int>(30 * u),
                                cy - static_cast<int>(17 * u));
                artwork_.Sprite(g, 403 + state_.round % 4, cx - static_cast<int>(10 * u),
                                cy - static_cast<int>(17 * u));
                artwork_.Sprite(g, 411, cx + static_cast<int>(10 * u), cy - static_cast<int>(17 * u));
                artwork_.Sprite(g, 401, cx - static_cast<int>(18 * u), cy + static_cast<int>(6 * u), false, true);
                artwork_.Sprite(g, 402, cx + static_cast<int>(2 * u), cy + static_cast<int>(6 * u), false, true);
                Text(g, std::to_wstring(state_.sticks), cx - 12 * u, cy - 3 * u, 10 * u, 18 * u, 11 * u, L"System",
                     Gdiplus::Color(255, 0, 0, 0));
                Text(g, std::to_wstring(state_.honba), cx + 8 * u, cy - 3 * u, 10 * u, 18 * u, 11 * u, L"System",
                     Gdiplus::Color(255, 0, 0, 0));
            }
            const float u = width_ == 1440 ? 1.4f : 1.0f;
            artwork_.PaintChrome(g, type_);
            const float left = width_ / 2.0f - 270 * u, top = h / 2.0f - 200 * u;
            Gdiplus::SolidBrush shade(Gdiplus::Color(220, 0, 0, 0));
            g.FillRectangle(&shade, left, top, 540 * u, 400 * u);
            Gdiplus::Pen border(Gdiplus::Color(180, 180, 180), 1);
            g.DrawRectangle(&border, left, top, 540 * u, 400 * u);
            if (resuming_) artwork_.Sprite(g, 464, static_cast<int>(left + 270 * u), static_cast<int>(top + 71 * u),
                                           true, false);
            else Text(g, L"对局", left, top + 71 * u, 540 * u, 48 * u, 40 * u, L"KaiTi");
            // Absolute/relative seat 0 stays at the bottom; dealer only changes wind.
            constexpr int xs[]{175, 315, 175, 35}, ys[]{274, 239, 204, 239};
            for (int i: {2, 3, 1, 0}) {
                if (!SeatPresent(state_.seat_mask, i)) continue;
                const float x = left + xs[i] * u, y = top + ys[i] * u;
                Gdiplus::LinearGradientBrush card(Gdiplus::PointF(x, y), Gdiplus::PointF(x, y + 42 * u),
                                                  Gdiplus::Color(255, 34, 34, 34), Gdiplus::Color(136, 17, 17, 17));
                g.FillRectangle(&card, x, y, 190 * u, 42 * u);
                if (state_.initialized)
                    artwork_.Sprite(g, 412 + SeatWind(state_.seat_mask, state_.dealer, i), static_cast<int>(x + 30 * u),
                                    static_cast<int>(y + 21 * u), true, true);
                Text(g, Wide(state_.players[i].name), x + 62 * u, y + 2 * u, 105 * u, 19 * u, (40.0f / 3) * u,
                     L"System");
                Text(g, ranks_[i], x + 62 * u, y + 20 * u, 105 * u, 18 * u, (32.0f / 3) * u, L"System");
            }
            if (!resuming_) Text(g, std::to_wstring(seconds_), left + 304 * u, top + 350 * u, 9 * u, 10 * u, 8 * u,
                                 L"System");
        } catch (const std::exception &e) {
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(255, 190, 160));
            const auto message = Wide(std::string("Match start artwork failed: ") + e.what());
            r.left += 12;
            r.top += 12;
            DrawTextW(dc, message.c_str(), -1, &r, DT_WORDBREAK);
        }
    }

    void MatchIntroView::Capture(HDC dc) {
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

LRESULT CALLBACK MatchIntroView::Procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
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
                self->buffer_.Paint(window, [&](HDC dc) { self->Paint(dc); });
                return 0;
            }
            if (message == WM_PRINTCLIENT) {
                self->Paint(reinterpret_cast<HDC>(wparam));
                return 0;
            }
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }

LRESULT CALLBACK MatchIntroView::OkProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
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
