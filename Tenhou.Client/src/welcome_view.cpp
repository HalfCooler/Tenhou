#include "welcome_view.hpp"
#include "resource_manager.hpp"
#include "background.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <charconv>
#include <stdexcept>

namespace tenhou {
    struct WelcomeView::Control {
        WelcomeView *owner = nullptr;
        HWND window = nullptr;
        int id = 0, width = 0, height = 0;
        bool dropdown = false, anchor = false, pressed = false, hovered = false;
        std::wstring text;
    };

    namespace {
        constexpr const wchar_t *kView = L"TENHOU_REBUILD_VIEW";
        constexpr const wchar_t *kPanel = L"TENHOU_REBUILD_WELCOME";
        constexpr const wchar_t *kButton = L"TENHOU_REBUILD_ITEM_BTN";

        void Fill(HDC dc, RECT rectangle, COLORREF color) {
            HBRUSH brush = CreateSolidBrush(color);
            FillRect(dc, &rectangle, brush);
            DeleteObject(brush);
        }

        void ImageAt(HDC dc, const Image &image, int x, int y) {
            if (image.bgra.empty()) return;
            Gdiplus::Graphics graphics(dc);
            Gdiplus::Bitmap bitmap(image.width, image.height, image.width * 4, PixelFormat32bppARGB,
                                   const_cast<BYTE *>(image.bgra.data()));
            if (bitmap.GetLastStatus() != Gdiplus::Ok || graphics.DrawImage(&bitmap, x, y, image.width, image.height) !=
                Gdiplus::Ok)
                throw std::runtime_error("Welcome image drawing failed");
        }
    } // namespace

    WelcomeView::WelcomeView(HWND parent, std::function<void(Action)> action,
                             std::function<void(const std::string &)> log)
        : parent_(parent), action_(std::move(action)), log_(std::move(log)) {
        const auto instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent, GWLP_HINSTANCE));
        for (const auto &item: std::vector<std::pair<const wchar_t *, WNDPROC> >{
                 {kView, ViewProc}, {kPanel, PanelProc}, {kButton, ButtonProc}
             }) {
            WNDCLASSEXW definition{sizeof(WNDCLASSEXW)};
            definition.hInstance = instance;
            definition.lpfnWndProc = item.second;
            definition.lpszClassName = item.first;
            definition.hCursor = LoadCursor(nullptr, IDC_ARROW);
            if (!RegisterClassExW(&definition) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
                throw std::runtime_error("Cannot register welcome window class");
        }
        font_ = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"MS UI Gothic");
        if (!font_) throw std::runtime_error("Cannot create original welcome font");
        viewport_ = CreateWindowExW(WS_EX_CONTROLPARENT, kView, L"", WS_CHILD | WS_CLIPCHILDREN, 0, 0, 1024, 768,
                                    parent_, reinterpret_cast<HMENU>(200), instance, this);
        panel_ = CreateWindowExW(WS_EX_CONTROLPARENT, kPanel, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 728,
                                 330,
                                 viewport_, reinterpret_cast<HMENU>(201), instance, this);
        if (!viewport_ || !panel_) throw std::runtime_error("Cannot create welcome viewport/panel");
        HDC dc = GetDC(panel_);
        const auto old_font = SelectObject(dc, font_);
        SIZE unit{};
        GetTextExtentPoint32W(dc, L"0", 1, &unit);
        edit_ = CreateWindowExW(0, L"EDIT", L"NoName", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                0, 0, 160, unit.cy, panel_, reinterpret_cast<HMENU>(3002), instance, nullptr);
        SendMessageW(edit_, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
        SendMessageW(edit_, EM_LIMITTEXT, 32, 0);
        nickname_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                    260, 132, 210, 22, panel_, reinterpret_cast<HMENU>(3008), instance, nullptr);
        password_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_PASSWORD, 260, 164, 210,
                                    22, panel_, reinterpret_cast<HMENU>(3007), instance, nullptr);
        if (!nickname_ || !password_) throw std::runtime_error("Cannot create account fields");
        for (auto field: {nickname_, password_})
            SendMessageW(field, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
        SendMessageW(nickname_, EM_LIMITTEXT, 16, 0);
        SendMessageW(password_, EM_LIMITTEXT, 64, 0);
        struct Definition {
            int id;
            const wchar_t *text;
            bool dropdown, anchor, disabled;
        };
        for (const auto &definition: std::vector<Definition>{
                 {3001, L"注册账号", false, false, false}, {3003, L"男", true, false, false},
                 {1, L"  登录  ", false, false, false}, {3004, L"设置", false, false, false},
                 {2, L"退出", false, false, false}, {3006, L"使用说明", false, true, false}
             }) {
            auto control = std::make_unique<Control>();
            control->owner = this;
            control->id = definition.id;
            control->text = definition.text;
            control->dropdown = definition.dropdown;
            control->anchor = definition.anchor;
            SIZE size{};
            GetTextExtentPoint32W(dc, definition.text, static_cast<int>(control->text.size()), &size);
            control->width = size.cx + (control->anchor ? 0 : 16 + (control->dropdown ? 6 : 0));
            control->height = size.cy + (control->anchor ? 0 : 8);
            control->window = CreateWindowExW(0, kButton, definition.text,
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | (definition.disabled
                                                  ? WS_DISABLED
                                                  : 0),
                                              0, 0, control->width, control->height, panel_,
                                              reinterpret_cast<HMENU>(definition.id), instance, control.get());
            if (!control->window) throw std::runtime_error("Cannot create welcome button");
            controls_.push_back(std::move(control));
        }
        SelectObject(dc, old_font);
        ReleaseDC(panel_, dc);
        Arrange();
    }

    WelcomeView::~WelcomeView() {
        if (viewport_ &&IsWindow(viewport_)) DestroyWindow(viewport_);
        if (font_) DeleteObject(font_);
    }

    void WelcomeView::Load(const std::filesystem::path &resources, const std::filesystem::path &config,
                           int screen_width, int screen_height) {
        (void) config;
        error_.clear();
        banner_ = {};
        background_ = {};
        try {
            const auto pack = LoadResourcePack(ResourceManager(resources).Path(AssetId::DisplayPack));
            const auto found = std::find_if(pack.entries.begin(), pack.entries.end(),
                                            [](const Resource &entry) { return entry.id == 591; });
            if (found == pack.entries.end()) throw std::runtime_error("Original welcome frame resource 591 is missing");
            banner_ = DecodeImage(*found);
            if (banner_.width != 728 || banner_.height != 330) throw std::runtime_error(
                "Original welcome frame dimensions changed");
            background_ = BuildBackground(ResourceManager(resources).Path(AssetId::Background), screen_width, screen_height);
            log_("WELCOME frame=591 size=728x330 canvas=" + std::to_string(screen_width) + "x" + std::to_string(
                     screen_height));
        } catch (const std::exception &error) {
            error_ = error.what();
            log_("ERROR " + error_);
        }
        InvalidateRect(viewport_, nullptr, TRUE);
        InvalidateRect(panel_, nullptr, TRUE);
    }

    void WelcomeView::LoadPreferences(const std::filesystem::path &config) {
        wchar_t name[80]{};
        GetPrivateProfileStringW(L"REBUILD", L"WelcomeName", L"NoName", name, 80, config.c_str());
        SetWindowTextW(edit_, name);
        SetGender(GetPrivateProfileIntW(L"REBUILD", L"WelcomeFemale", 0, config.c_str()) ? 'F' : 'M');
    }

    void WelcomeView::Arrange() {
        if (!panel_ || !edit_ || controls_.size() != 6) return;
        RECT viewport{};
        GetClientRect(viewport_, &viewport);
        SetWindowPos(panel_, nullptr, (viewport.right - 728) / 2, (viewport.bottom - 330) / 2, 728, 330, SWP_NOZORDER);
        int total = 160;
        for (unsigned i = 0; i < 5; ++i) total += controls_[i]->width;
        // Exact gaps from 004a44d0: 5,3,3,1,1; original centering uses728-4.
        int x = (724 - total) / 2;
        SetWindowPos(controls_[0]->window, nullptr, x, 265 - controls_[0]->height / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        x += controls_[0]->width + 5;
        RECT edit{};
        GetClientRect(edit_, &edit);
        SetWindowPos(edit_, nullptr, x, 265 - edit.bottom / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        x += 160 + 3;
        const int row_top = 265 - controls_[2]->height / 2;
        for (unsigned i = 1; i < 5; ++i) {
            SetWindowPos(controls_[i]->window, nullptr, x, row_top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            x += controls_[i]->width + (i < 2 ? 3 : 1);
        }
        SetWindowPos(controls_[5]->window, nullptr, (728 - controls_[5]->width) / 2, 307,
                     0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    void WelcomeView::Resize(int width, int height) {
        MoveWindow(viewport_, 0, 0, width, height, TRUE);
        Arrange();
    }

    void WelcomeView::Show(bool visible) { ShowWindow(viewport_, visible ? SW_SHOW : SW_HIDE); }

    void WelcomeView::SetGender(char gender) {
        gender_ = gender;
        if (controls_.size() < 2) return;
        controls_[1]->text = gender == 'F' ? L"女" : L"男";
        InvalidateRect(controls_[1]->window, nullptr, FALSE);
    }

    void WelcomeView::Save(const std::filesystem::path &config) {
        wchar_t name[80]{};
        GetWindowTextW(edit_, name, 80);
        if (!masked_identity_.empty()) wcscpy_s(name, L"NoName");
        // Remember the local account name, never its password.
        if (!WritePrivateProfileStringW(L"REBUILD", L"WelcomeName", name, config.c_str()) ||
            !WritePrivateProfileStringW(L"REBUILD", L"WelcomeFemale", gender_ == 'F' ? L"1" : L"0", config.c_str()))
            throw std::runtime_error("Cannot save login preferences");
    }

    std::wstring WelcomeView::Identity() const {
        wchar_t name[80]{};
        GetWindowTextW(edit_, name, 80);
        return !masked_identity_.empty() && std::wstring(name) == L"ID########-########" ? masked_identity_ : name;
    }

    std::wstring WelcomeView::Password() const {
        wchar_t value[129]{};
        GetWindowTextW(password_, value, 129);
        return value;
    }

    std::wstring WelcomeView::Nickname() const {
        wchar_t value[65]{};
        GetWindowTextW(nickname_, value, 65);
        return value;
    }

    void WelcomeView::ClearPassword() { SetWindowTextW(password_, L""); }

    void WelcomeView::SetHiddenId(bool hidden) {
        const auto identity = Identity();
        masked_identity_.clear();
        if (hidden && !identity.empty() && identity != L"NoName") {
            masked_identity_ = identity;
            SetWindowTextW(edit_, L"ID########-########");
        } else SetWindowTextW(edit_, identity.c_str());
    }

    void WelcomeView::SetLoginState(bool connecting, const std::wstring &status) {
        connecting_ = connecting;
        login_status_ = status;
        EnableWindow(edit_, !connecting);
        EnableWindow(controls_[1]->window, !connecting);
        EnableWindow(password_, !connecting);
        EnableWindow(nickname_, !connecting);
        EnableWindow(controls_[0]->window, !connecting);
        controls_[2]->text = connecting ? L"  取消  " : L"  登录  ";
        SetWindowTextW(controls_[2]->window, controls_[2]->text.c_str());
        InvalidateRect(controls_[2]->window, nullptr, FALSE);
        InvalidateRect(panel_, nullptr, FALSE);
    }

    void WelcomeView::Command(int id) {
        if (id == 3003) {
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING | (gender_ == 'M' ? MF_CHECKED : 0), 100, L"男");
            AppendMenuW(menu, MF_STRING | (gender_ == 'F' ? MF_CHECKED : 0), 101, L"女");
            RECT position{};
            GetWindowRect(controls_[1]->window, &position);
            const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, position.left, position.bottom, 0,
                                                panel_, nullptr);
            DestroyMenu(menu);
            if (command) {
                SetGender(command == 101 ? 'F' : 'M');
                log_("WELCOME gender changed");
            }
        } else if (id == 1) action_(Action::Lobby);
        else if (id == 3001) action_(Action::Register);
        else if (id == 3004) action_(Action::Settings);
        else if (id == 2) action_(Action::Exit);
        else if (id == 3006) {
            MessageBoxW(panel_, L"本客户端连接本机麻将服务。\n\n"
                        L"使用注册账号和密码登录，或以 NoName 访客登录。\n\n"
                        L"大厅战等待 10 秒后补齐人机；只有全注册真人对局计入排位。",
                        L"使用说明", MB_OK | MB_ICONINFORMATION);
        }
    }

    void WelcomeView::PaintView(HDC dc) {
        RECT bounds{};
        GetClientRect(viewport_, &bounds);
        Fill(dc, bounds, RGB(30, 40, 50));
        if (!background_.bgra.empty()) ImageAt(dc, background_, (bounds.right - background_.width) / 2,
                                               (bounds.bottom - background_.height) / 2);
    }

    void WelcomeView::PaintPanel(HDC dc) {
        Fill(dc, {0, 0, 728, 330}, RGB(255, 255, 255));
        ImageAt(dc, banner_, 0, 0);
        const auto old_font = SelectObject(dc, font_);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(90, 90, 90));
        RECT announcement{30, 30, 698, 105};
        if (error_.empty()) {
            DrawTextW(dc, L"本机账户\n\n填写账号和密码登录；注册时另填昵称。\n使用 NoName 可作为访客，不保存战绩和牌谱。", -1, &announcement,
                      DT_CENTER | DT_WORDBREAK);
        } else {
            SetTextColor(dc, RGB(175, 30, 30));
            const std::wstring message(error_.begin(), error_.end());
            DrawTextW(dc, message.c_str(), -1, &announcement, DT_LEFT | DT_WORDBREAK);
        }
        RECT nick_label{150, 132, 250, 154}, password_label{150, 164, 250, 186};
        DrawTextW(dc, L"昵称（注册）", -1, &nick_label, DT_RIGHT | DT_SINGLELINE);
        DrawTextW(dc, L"密码", -1, &password_label, DT_RIGHT | DT_SINGLELINE);
        RECT status{60, 195, 668, 230};
        DrawTextW(dc, login_status_.c_str(), -1, &status, DT_CENTER | DT_WORDBREAK);
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(204, 204, 204));
        const auto old_pen = SelectObject(dc, pen);
        MoveToEx(dc, 60, 235, nullptr);
        LineTo(dc, 668, 235);
        SelectObject(dc, old_pen);
        DeleteObject(pen);
        RECT edit{};
        GetWindowRect(edit_, &edit);
        MapWindowPoints(nullptr, panel_, reinterpret_cast<POINT *>(&edit), 2);
        InflateRect(&edit, 4, 4);
        FrameRect(dc, &edit, reinterpret_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH)));
        SetTextColor(dc, RGB(68, 68, 68));
        RECT hint{0, edit.bottom + 8, 728, edit.bottom + 30};
        DrawTextW(dc, L"账号：3～32 位字母、数字、下划线或短横线；访客：NoName", -1, &hint, DT_CENTER | DT_SINGLELINE);
        SelectObject(dc, old_font);
    }

    void WelcomeView::PaintButton(Control &control, HDC dc) {
        RECT bounds{0, 0, control.width, control.height};
        const bool enabled = IsWindowEnabled(control.window) != FALSE;
        if (control.anchor) Fill(dc, bounds, RGB(255, 255, 255));
        else if (!enabled) Fill(dc, bounds, RGB(204, 204, 204));
        else {
            Fill(dc, {0, 0, control.width, control.height / 2}, RGB(255, 255, 255));
            const int half = control.height - control.height / 2;
            for (int row = 0; row < half; ++row) {
                const int color = 221 + 34 * row / std::max(1, half - 1);
                Fill(dc, {0, control.height / 2 + row, control.width, control.height / 2 + row + 1},
                     RGB(color, color, color));
            }
        }
        if (!control.anchor) {
            HBRUSH border = CreateSolidBrush(
                control.hovered || control.pressed ? RGB(51, 153, 255) : RGB(136, 136, 136));
            FrameRect(dc, &bounds, border);
            DeleteObject(border);
            InflateRect(&bounds, -8, 0);
            if (control.dropdown) bounds.right -= 6;
        }
        const auto old = SelectObject(dc, font_);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, control.anchor
                             ? (control.pressed ? RGB(255, 0, 0) : RGB(0, 0, 255))
                             : (enabled ? RGB(34, 34, 34) : RGB(102, 102, 102)));
        if (control.pressed) OffsetRect(&bounds, 1, 1);
        DrawTextW(dc, control.text.c_str(), -1, &bounds, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (control.dropdown) {
            POINT points[3]{
                {control.width - 9, control.height / 2 + 2}, {control.width - 3, control.height / 2 + 2},
                {control.width - 6, control.height / 2 - 1}
            };
            const auto pen = SelectObject(dc, GetStockObject(BLACK_PEN));
            const auto brush = SelectObject(dc, GetStockObject(BLACK_BRUSH));
            Polygon(dc, points, 3);
            SelectObject(dc, brush);
            SelectObject(dc, pen);
        }
        if (control.anchor) {
            HPEN pen = CreatePen(PS_SOLID, 1, control.pressed ? RGB(255, 0, 0) : RGB(0, 0, 255));
            const auto old_pen = SelectObject(dc, pen);
            MoveToEx(dc, 0, control.height - 1, nullptr);
            LineTo(dc, control.width, control.height - 1);
            SelectObject(dc, old_pen);
            DeleteObject(pen);
        }
        if (GetFocus() == control.window && enabled) {
            RECT focus{2, 2, control.width - 2, control.height - 2};
            DrawFocusRect(dc, &focus);
        }
        SelectObject(dc, old);
    }

    void WelcomeView::Capture(HDC dc) {
        PaintView(dc);
        POINT root{0, 0};
        ClientToScreen(viewport_, &root);
        RECT panel{};
        GetWindowRect(panel_, &panel);
        const int saved = SaveDC(dc);
        SetViewportOrgEx(dc, panel.left - root.x, panel.top - root.y, nullptr);
        PaintPanel(dc);
        POINT panel_origin{0, 0};
        ClientToScreen(panel_, &panel_origin);
        for (const auto &control: controls_) {
            RECT position{};
            GetWindowRect(control->window, &position);
            const int previous = SaveDC(dc);
            SetViewportOrgEx(dc, position.left - root.x, position.top - root.y, nullptr);
            PaintButton(*control, dc);
            RestoreDC(dc, previous);
        }
        RECT edit{};
        GetWindowRect(edit_, &edit);
        SetViewportOrgEx(dc, edit.left - root.x, edit.top - root.y, nullptr);
        SendMessageW(edit_, WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT | PRF_NONCLIENT | PRF_ERASEBKGND);
        RestoreDC(dc, saved);
    }

LRESULT CALLBACK WelcomeView::ViewProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        auto *view = reinterpret_cast<WelcomeView *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            view = static_cast<WelcomeView *>(reinterpret_cast<CREATESTRUCTW *>(lparam)->lpCreateParams);
            view->viewport_ = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
        }
        if (view && message == WM_SIZE) {
            view->Arrange();
            return 0;
        }
        if (view && message == WM_PAINT) {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(window, &paint);
            try { view->PaintView(dc); } catch (const std::exception &error) {
                EndPaint(window, &paint);
                SendMessageW(view->parent_, WM_APP + 1, 0, reinterpret_cast<LPARAM>(error.what()));
                return 0;
            }
            EndPaint(window, &paint);
            return 0;
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }

LRESULT CALLBACK WelcomeView::PanelProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        auto *view = reinterpret_cast<WelcomeView *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            view = static_cast<WelcomeView *>(reinterpret_cast<CREATESTRUCTW *>(lparam)->lpCreateParams);
            view->panel_ = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
        }
        if (view && message == WM_COMMAND) {
            try { view->Command(LOWORD(wparam)); } catch (const std::exception &error) {
                view->SetLoginState(false, NetworkWide(error.what()));
                view->log_(std::string("LOGIN ERROR ") + error.what());
            }
            return 0;
        }
        if (view && message == WM_PAINT && view->edit_) {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(window, &paint);
            try { view->PaintPanel(dc); } catch (const std::exception &error) {
                EndPaint(window, &paint);
                SendMessageW(view->parent_, WM_APP + 1, 0, reinterpret_cast<LPARAM>(error.what()));
                return 0;
            }
            EndPaint(window, &paint);
            return 0;
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }

LRESULT CALLBACK WelcomeView::ButtonProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        auto *control = reinterpret_cast<Control *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            control = static_cast<Control *>(reinterpret_cast<CREATESTRUCTW *>(lparam)->lpCreateParams);
            control->window = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(control));
        }
        if (control) {
            if (message == WM_PAINT) {
                PAINTSTRUCT paint{};
                HDC dc = BeginPaint(window, &paint);
                control->owner->PaintButton(*control, dc);
                EndPaint(window, &paint);
                return 0;
            }
            if (message == WM_LBUTTONDOWN && IsWindowEnabled(window)) {
                control->pressed = true;
                SetFocus(window);
                SetCapture(window);
                InvalidateRect(window, nullptr, FALSE);
                return 0;
            }
            if (message == WM_LBUTTONUP) {
                const bool pressed = control->pressed;
                control->pressed = false;
                ReleaseCapture();
                InvalidateRect(window, nullptr, FALSE);
                RECT bounds{};
                GetClientRect(window, &bounds);
                POINT point{static_cast<short>(LOWORD(lparam)), static_cast<short>(HIWORD(lparam))};
                if (pressed && PtInRect(&bounds, point)) SendMessageW(GetParent(window), WM_COMMAND, control->id,
                                                                      reinterpret_cast<LPARAM>(window));
                return 0;
            }
            if ((message == WM_KEYUP && (wparam == VK_SPACE || wparam == VK_RETURN)) || message == BM_CLICK) {
                if (IsWindowEnabled(window)) SendMessageW(GetParent(window), WM_COMMAND, control->id,
                                                          reinterpret_cast<LPARAM>(window));
                return 0;
            }
            if (message == WM_MOUSEMOVE) {
                control->hovered = true;
                TRACKMOUSEEVENT tracking{sizeof(TRACKMOUSEEVENT), TME_LEAVE, window, 0};
                TrackMouseEvent(&tracking);
                InvalidateRect(window, nullptr, FALSE);
                return 0;
            }
            if (message == WM_MOUSELEAVE || message == WM_CAPTURECHANGED) {
                control->hovered = false;
                control->pressed = false;
                InvalidateRect(window, nullptr, FALSE);
                return 0;
            }
            if (message == WM_SETFOCUS || message == WM_KILLFOCUS || message == WM_ENABLE) InvalidateRect(
                window, nullptr, FALSE);
            if (message == WM_GETDLGCODE) return DLGC_BUTTON;
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }
} // namespace tenhou
