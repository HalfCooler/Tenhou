#include "welcome_view.hpp"
#include "resource_manager.hpp"
#include "background.hpp"
#include "client_preferences.hpp"
#include "original_display.hpp"
#include "variant_artwork.hpp"
#include "window_layout.hpp"
#include "win32_utf.hpp"
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
        constexpr int kServer = 3009;

        std::wstring EditText(const HWND edit) {
            std::wstring value(static_cast<std::size_t>(GetWindowTextLengthW(edit)) + 1, L'\0');
            value.resize(GetWindowTextW(edit, value.data(), static_cast<int>(value.size())));
            return value;
        }

        void Fill(const HDC dc, const RECT rectangle, const COLORREF color) {
            HBRUSH brush = CreateSolidBrush(color);
            FillRect(dc, &rectangle, brush);
            DeleteObject(brush);
        }

        void ImageAt(const HDC dc, const Image &image, const int x, const int y) {
            if (image.bgra.empty()) return;
            Gdiplus::Graphics graphics(dc);
            Gdiplus::Bitmap bitmap(image.width, image.height, image.width * 4, PixelFormat32bppARGB,
                                   const_cast<BYTE *>(image.bgra.data()));
            if (bitmap.GetLastStatus() != Gdiplus::Ok || graphics.DrawImage(&bitmap, x, y, image.width, image.height) !=
                Gdiplus::Ok)
                throw std::runtime_error("Welcome image drawing failed");
        }
    } // namespace

    WelcomeView::WelcomeView(const HWND parent, std::function<void(Action)> action,
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
        viewport_ = CreateWindowExW(WS_EX_CONTROLPARENT, kView, L"", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 1024, 768,
                                    parent_, reinterpret_cast<HMENU>(200), instance, this);
        panel_ = CreateWindowExW(WS_EX_CONTROLPARENT, kPanel, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 728,
                                 330,
                                 viewport_, reinterpret_cast<HMENU>(201), instance, this);
        if (!viewport_ || !panel_) throw std::runtime_error("Cannot create welcome viewport/panel");
        HDC dc = GetDC(panel_);
        const auto old_font = SelectObject(dc, font_);
        SIZE unit{};
        GetTextExtentPoint32W(dc, L"0", 1, &unit);
        server_ = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
                                  260, 70, 210, 100, panel_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kServer)),
                                  instance, nullptr);
        if (!server_) throw std::runtime_error("Cannot create server selector");
        SendMessageW(server_, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
        SendMessageW(server_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"第三方服务器"));
        SendMessageW(server_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"天凤官方"));
        const auto field = [&](const int id, const int x, const int y, const int width, const unsigned limit, const DWORD style = 0) {
            const HWND window = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | style, x, y, width, 22, panel_,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
            if (!window) throw std::runtime_error("Cannot create login field");
            SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
            SendMessageW(window, EM_LIMITTEXT, limit, 0);
            return window;
        };
        host_ = field(3010, 260, 105, 158, 253);
        port_ = field(3011, 425, 105, 45, 5, ES_NUMBER);
        nickname_ = field(3008, 260, 137, 210, 16);
        password_ = field(3007, 260, 169, 210, 64, ES_PASSWORD);
        official_id_ = field(3012, 260, 112, 210, 64);
        edit_ = CreateWindowExW(0, L"EDIT", L"NoName", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                0, 0, 160, unit.cy, panel_, reinterpret_cast<HMENU>(3002), instance, nullptr);
        if (!edit_) throw std::runtime_error("Cannot create account name field");
        SendMessageW(edit_, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
        SendMessageW(edit_, EM_LIMITTEXT, 32, 0);
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
                                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(definition.id)),
                                              instance, control.get());
            if (!control->window) throw std::runtime_error("Cannot create welcome button");
            controls_.push_back(std::move(control));
        }
        SelectObject(dc, old_font);
        ReleaseDC(panel_, dc);
        SetServer(false);
    }

    WelcomeView::~WelcomeView() {
        if (viewport_ &&IsWindow(viewport_)) DestroyWindow(viewport_);
        if (font_) DeleteObject(font_);
    }

    void WelcomeView::Load(const std::filesystem::path &resources, const std::filesystem::path &config,
                           const int screen_width, const int screen_height) {
        (void) config;
        error_.clear();
        banner_ = {};
        background_ = {};
        try {
            DisplayArtwork artwork;
            artwork.Configure(resources, kDefaultClientWidth);
            banner_ = artwork.Get(591);
            if (banner_.width <= 0 || banner_.height <= 0 || banner_.bgra.empty())
                throw std::runtime_error("Original welcome frame resource 591 is missing");
            background_ = BuildBackground(ResourceManager(resources).Path(AssetId::Background), screen_width, screen_height);
            log_("WELCOME frame=591 size=" + std::to_string(banner_.width) + "x" + std::to_string(banner_.height) +
                 " canvas=" + std::to_string(screen_width) + "x" + std::to_string(screen_height));
        } catch (const std::exception &error) {
            error_ = error.what();
            log_("ERROR " + error_);
        }
        InvalidateRect(viewport_, nullptr, TRUE);
        InvalidateRect(panel_, nullptr, TRUE);
    }

    void WelcomeView::SyncModelFromControls() const {
        if (edit_) vm_.SetIdentity(WideToUtf8(EditText(edit_)));
        if (official_id_) vm_.SetOfficialIdentity(WideToUtf8(EditText(official_id_)));
        if (host_) vm_.SetHost(WideToUtf8(EditText(host_)));
        if (port_) vm_.SetPortText(WideToUtf8(EditText(port_)));
        if (nickname_) vm_.SetNickname(WideToUtf8(EditText(nickname_)));
        if (password_) vm_.SetPassword(WideToUtf8(EditText(password_)));
    }

    void WelcomeView::LoadPreferences(const std::filesystem::path &config) {
        vm_.LoadPreferences(config);
        SetWindowTextW(edit_, Utf8ToWide(vm_.DisplayIdentity()).c_str());
        SetWindowTextW(official_id_, Utf8ToWide(vm_.TrimmedOfficialIdentity()).c_str());
        SetWindowTextW(host_, Utf8ToWide(vm_.TrimmedHost()).c_str());
        SetWindowTextW(port_, Utf8ToWide(vm_.port_text()).c_str());
        SetGender(vm_.gender());
        SetServer(vm_.official_server());
    }

    void WelcomeView::Arrange() const
    {
        if (!panel_ || !edit_ || controls_.size() != 6) return;
        const bool official_server = vm_.official_server();
        RECT viewport{};
        GetClientRect(viewport_, &viewport);
        SetWindowPos(panel_, nullptr, (viewport.right - 728) / 2, (viewport.bottom - 330) / 2, 728, 330, SWP_NOZORDER);
        int total = official_server ? 0 : 160;
        for (unsigned i = official_server ? 2 : 0; i < 5; ++i) total += controls_[i]->width;
        // Exact gaps from 004a44d0: 5,3,3,1,1; original centering uses728-4.
        int x = (724 - total) / 2;
        if (!official_server) {
            SetWindowPos(controls_[0]->window, nullptr, x, 265 - controls_[0]->height / 2, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER);
            x += controls_[0]->width + 5;
            RECT edit{};
            GetClientRect(edit_, &edit);
            SetWindowPos(edit_, nullptr, x, 265 - edit.bottom / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            x += 160 + 3;
        }
        const int row_top = 265 - controls_[2]->height / 2;
        for (unsigned i = official_server ? 2 : 1; i < 5; ++i) {
            SetWindowPos(controls_[i]->window, nullptr, x, row_top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            x += controls_[i]->width + (i < 2 ? 3 : 1);
        }
        SetWindowPos(controls_[5]->window, nullptr, (728 - controls_[5]->width) / 2, 307,
                     0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    void WelcomeView::Resize(const int width, const int height) {
        MoveWindow(viewport_, 0, 0, width, height, TRUE);
        Arrange();
    }

    void WelcomeView::Show(const bool visible) const { ShowWindow(viewport_, visible ? SW_SHOW : SW_HIDE); }

    void WelcomeView::SetGender(const char gender) const
    {
        vm_.SetGender(gender);
        if (controls_.size() < 2) return;
        controls_[1]->text = vm_.gender() == 'F' ? L"女" : L"男";
        InvalidateRect(controls_[1]->window, nullptr, FALSE);
    }

    void WelcomeView::Save(const std::filesystem::path &config) const
    {
        SyncModelFromControls();
        vm_.SavePreferences(config);
    }

    std::wstring WelcomeView::Identity() const {
        SyncModelFromControls();
        return Utf8ToWide(vm_.EffectiveIdentity());
    }

    std::wstring WelcomeView::Password() const {
        SyncModelFromControls();
        return Utf8ToWide(vm_.password());
    }

    std::wstring WelcomeView::Nickname() const {
        SyncModelFromControls();
        return Utf8ToWide(vm_.nickname());
    }

    std::wstring WelcomeView::Host() const {
        SyncModelFromControls();
        return Utf8ToWide(vm_.TrimmedHost());
    }

    unsigned WelcomeView::Port() const {
        SyncModelFromControls();
        return vm_.ParsedPort();
    }

    bool WelcomeView::HasOfficialId() const {
        SyncModelFromControls();
        return vm_.HasValidOfficialId();
    }

    std::wstring WelcomeView::OfficialIdentity() const {
        SyncModelFromControls();
        return Utf8ToWide(vm_.TrimmedOfficialIdentity());
    }

    void WelcomeView::ClearPassword() const
    {
        vm_.ClearPassword();
        SetWindowTextW(password_, L"");
    }

    void WelcomeView::SetHiddenId(const bool hidden) const
    {
        SyncModelFromControls();
        vm_.SetHiddenId(hidden);
        SendMessageW(official_id_, EM_SETPASSWORDCHAR, hidden ? L'*' : 0, 0);
        InvalidateRect(official_id_, nullptr, TRUE);
        SetWindowTextW(edit_, Utf8ToWide(vm_.DisplayIdentity()).c_str());
    }

    void WelcomeView::SetServer(const bool official) {
        vm_.SetServer(official);
        SendMessageW(server_, CB_SETCURSEL, official ? 1 : 0, 0);
        for (auto field: {edit_, host_, port_, nickname_, password_, controls_[0]->window, controls_[1]->window})
            ShowWindow(field, official ? SW_HIDE : SW_SHOW);
        ShowWindow(official_id_, official ? SW_SHOW : SW_HIDE);
        Arrange();
        SetLoginState(false, L"");
    }

    void WelcomeView::SetLoginState(const bool connecting, const std::wstring &status) {
        vm_.SetLoginState(connecting, WideToUtf8(status));
        login_status_ = status;
        for (auto field: {server_, edit_, host_, port_, nickname_, password_, official_id_,
                          controls_[0]->window, controls_[1]->window})
            EnableWindow(field, !connecting);
        controls_[2]->text = connecting ? L"  取消  " : L"  登录  ";
        SetWindowTextW(controls_[2]->window, controls_[2]->text.c_str());
        InvalidateRect(controls_[2]->window, nullptr, FALSE);
        InvalidateRect(panel_, nullptr, FALSE);
    }

    void WelcomeView::Command(const int id) {
        const bool official_server = vm_.official_server();
        if (id == 3003) {
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING | (vm_.gender() == 'M' ? MF_CHECKED : 0), 100, L"男");
            AppendMenuW(menu, MF_STRING | (vm_.gender() == 'F' ? MF_CHECKED : 0), 101, L"女");
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
        else if (id == 3001 && !official_server) action_(Action::Register);
        else if (id == 3004) action_(Action::Settings);
        else if (id == 2) action_(Action::Exit);
        else if (id == 3006) {
            MessageBoxW(panel_, official_server ? L"请输入 19 位天凤官方 ID（IDxxxxxxxx-xxxxxxxx）或 NoName。" :
                        L"填写第三方服务器主机和端口，默认 localhost:10080。\n\n"
                        L"使用注册账号和密码登录，或以 NoName 访客登录。\n\n"
                        L"大厅战等待 10 秒后补齐人机；只有全注册真人对局计入排位。",
                        L"使用说明", MB_OK | MB_ICONINFORMATION);
        }
    }

    void WelcomeView::PaintView(const HDC dc) {
        RECT bounds{};
        GetClientRect(viewport_, &bounds);
        Fill(dc, bounds, RGB(30, 40, 50));
        if (background_.bgra.empty() || bounds.right <= 0 || bounds.bottom <= 0) return;
        if (bounds.right == background_.width && bounds.bottom == background_.height) {
            ImageAt(dc, background_, 0, 0);
        } else {
            Gdiplus::Bitmap bmp(background_.width, background_.height, background_.width * 4,
                                PixelFormat32bppARGB, const_cast<BYTE *>(background_.bgra.data()));
            Gdiplus::Graphics g(dc);
            g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            g.DrawImage(&bmp, Gdiplus::Rect(0, 0, bounds.right, bounds.bottom),
                        0, 0, background_.width, background_.height, Gdiplus::UnitPixel);
        }
    }

    void WelcomeView::PaintPanel(const HDC dc) {
        Fill(dc, {0, 0, 728, 330}, RGB(255, 255, 255));
        if (!banner_.bgra.empty() && (banner_.width != 728 || banner_.height != 330)) {
            Gdiplus::Bitmap bmp(banner_.width, banner_.height, banner_.width * 4,
                                PixelFormat32bppARGB, const_cast<BYTE *>(banner_.bgra.data()));
            Gdiplus::Graphics g(dc);
            g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            g.DrawImage(&bmp, Gdiplus::Rect(0, 0, 728, 330),
                        0, 0, banner_.width, banner_.height, Gdiplus::UnitPixel);
        } else {
            ImageAt(dc, banner_, 0, 0);
        }
        const auto old_font = SelectObject(dc, font_);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(90, 90, 90));
        const bool official_server = vm_.official_server();
        RECT announcement{30, 29, 698, 66};
        if (error_.empty()) {
            DrawTextW(dc, official_server ? L"使用天凤官方 ID 或 NoName 访客登录" : L"使用注册账号登录，或以 NoName 访客登录",
                      -1, &announcement,
                      DT_CENTER | DT_WORDBREAK);
        } else {
            SetTextColor(dc, RGB(175, 30, 30));
            const std::wstring message(error_.begin(), error_.end());
            DrawTextW(dc, message.c_str(), -1, &announcement, DT_LEFT | DT_WORDBREAK);
        }
        const auto label = [&](const wchar_t *text, const int top) {
            RECT rect{115, top, 250, top + 22};
            DrawTextW(dc, text, -1, &rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        };
        label(L"连接方式", 70);
        if (official_server) {
            label(L"官方 ID", 112);
            RECT format{260, 143, 470, 166};
            DrawTextW(dc, L"IDxxxxxxxx-xxxxxxxx（19 位）", -1, &format, DT_SINGLELINE);
        } else {
            label(L"主机 / 端口", 105);
            label(L"昵称（注册）", 137);
            label(L"密码", 169);
        }
        RECT status{60, 201, 668, 232};
        DrawTextW(dc, login_status_.c_str(), -1, &status, DT_CENTER | DT_WORDBREAK);
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(204, 204, 204));
        const auto old_pen = SelectObject(dc, pen);
        MoveToEx(dc, 60, 235, nullptr);
        LineTo(dc, 668, 235);
        SelectObject(dc, old_pen);
        DeleteObject(pen);
        if (!official_server) {
            RECT edit{};
            GetWindowRect(edit_, &edit);
            MapWindowPoints(nullptr, panel_, reinterpret_cast<POINT *>(&edit), 2);
            InflateRect(&edit, 4, 4);
            FrameRect(dc, &edit, reinterpret_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH)));
            SetTextColor(dc, RGB(68, 68, 68));
            RECT hint{0, edit.bottom + 8, 728, edit.bottom + 30};
            DrawTextW(dc, L"账号：3～32 位字母、数字、下划线或短横线；访客：NoName", -1, &hint, DT_CENTER | DT_SINGLELINE);
        }
        SelectObject(dc, old_font);
    }

    void WelcomeView::PaintButton(Control &control, const HDC dc) const
    {
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
            if (!IsWindowVisible(control->window)) continue;
            RECT position{};
            GetWindowRect(control->window, &position);
            const int previous = SaveDC(dc);
            SetViewportOrgEx(dc, position.left - root.x, position.top - root.y, nullptr);
            PaintButton(*control, dc);
            RestoreDC(dc, previous);
        }
        for (auto field: {server_, edit_, host_, port_, nickname_, password_, official_id_}) {
            if (!IsWindowVisible(field)) continue;
            RECT position{};
            GetWindowRect(field, &position);
            SetViewportOrgEx(dc, position.left - root.x, position.top - root.y, nullptr);
            SendMessageW(field, WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT | PRF_NONCLIENT | PRF_ERASEBKGND);
        }
        RestoreDC(dc, saved);
    }

LRESULT CALLBACK WelcomeView::ViewProc(const HWND window, const UINT message, const WPARAM wparam, const LPARAM lparam) {
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

LRESULT CALLBACK WelcomeView::PanelProc(const HWND window, const UINT message, const WPARAM wparam, const LPARAM lparam) {
        auto *view = reinterpret_cast<WelcomeView *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            view = static_cast<WelcomeView *>(reinterpret_cast<CREATESTRUCTW *>(lparam)->lpCreateParams);
            view->panel_ = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
        }
        if (view && message == WM_COMMAND) {
            try {
                if (LOWORD(wparam) == kServer && HIWORD(wparam) == CBN_SELCHANGE && !view->Connecting()) {
                    view->SetServer(SendMessageW(view->server_, CB_GETCURSEL, 0, 0) == 1);
                    view->action_(Action::ServerChanged);
                } else view->Command(LOWORD(wparam));
            } catch (const std::exception &error) {
                view->SetLoginState(false, Utf8ToWide(error.what()));
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

LRESULT CALLBACK WelcomeView::ButtonProc(HWND window, const UINT message, const WPARAM wparam, const LPARAM lparam) {
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
