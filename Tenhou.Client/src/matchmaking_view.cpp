#include "matchmaking_view.hpp"
#include "network_protocol.hpp"
#include "room_settings_view.hpp"
#include <commctrl.h>
#include <algorithm>
#include <stdexcept>

namespace tenhou {
    namespace {
        constexpr wchar_t kClass[] = L"TENHOU_MATCHMAKING";

        void Label(HDC dc, const wchar_t *text, RECT r, UINT flags = DT_LEFT | DT_SINGLELINE) {
            DrawTextW(dc, text, -1, &r, flags);
        }

LRESULT CALLBACK Hover(HWND window, UINT message, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR) {
            if (message == WM_MOUSEMOVE) {
                TRACKMOUSEEVENT t{sizeof(t), TME_LEAVE, window, 0};
                TrackMouseEvent(&t);
                InvalidateRect(window, nullptr, FALSE);
            }
            if (message == WM_MOUSELEAVE) InvalidateRect(window, nullptr, FALSE);
            return DefSubclassProc(window, message, wp, lp);
        }
    }

    MatchmakingView::MatchmakingView(HWND parent, std::function<void(const std::string &)> log) : log_(std::move(log)) {
        const auto instance = GetModuleHandleW(nullptr);
        WNDCLASSEXW wc{sizeof(wc)};
        wc.hInstance = instance;
        wc.lpfnWndProc = Procedure;
        wc.lpszClassName = kClass;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) throw std::runtime_error(
            "Cannot register matching panel");
        font_ = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"MS UI Gothic");
        window_ = CreateWindowExW(WS_EX_CONTROLPARENT, kClass, L"", WS_CHILD | WS_CLIPCHILDREN, 0, 0, 590, 430, parent,
                                  nullptr, instance, this);
        if (!font_ || !window_) throw std::runtime_error("Cannot create matching panel");
        const auto add = [&](int id, const wchar_t *text, int x, int y, int w, int h, const wchar_t *cls = L"BUTTON") {
            const bool edit = wcscmp(cls, L"EDIT") == 0;
            auto control = CreateWindowExW(0, cls, text,
                                           WS_CHILD | WS_TABSTOP | (
                                               edit ? WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER : BS_OWNERDRAW), x, y, w,
                                           h, window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance,
                                           nullptr);
            if (!control) throw std::runtime_error("Cannot create matching control");
            controls_[id] = control;
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
            if (!edit) SetWindowSubclass(control, Hover, 1, 0);
        };
        add(5200, L"人机场", 30, 95, 145, 25);
        add(5201, L"大厅战", 175, 95, 145, 25);
        // Original 00428a20: rule button x=100/190 and y=52+27*n inside (30,120).
        // Rows reduced to the user's supported subset. Labels and no-kuitan sanma are authorized adaptations.
        for (int i = 0; i < 12; ++i) add(5210 + i, (i % 2) ? L"南" : L"东", 130 + (i % 2) * 90,
                                         172 + (i / 2) * 27 + (i >= 8 ? 27 : 0), 40, 22);
        add(5230, L"创建房间", 50, 150, 112, 25);
        add(5231, L"加入房间", 178, 150, 112, 25);
        add(5232, L"", 50, 196, 112, 23, L"EDIT");
        SendMessageW(controls_[5232], EM_LIMITTEXT, 4, 0);
        add(5233, L"对局设置", 50, 244, 112, 25);
        add(5234, L"离开房间", 178, 244, 112, 25);
        add(5235, L"开始匹配", 98, 286, 144, 25);
        add(5240, L"取消", 230, 365, 60, 25);
        add(5241, L"返回登录", 30, 365, 86, 25);
        add(5242, L"关闭房间", 128, 365, 86, 25);
        Update();
    }

    MatchmakingView::~MatchmakingView() {
        if (window_) DestroyWindow(window_);
        if (font_) DeleteObject(font_);
    }

    void MatchmakingView::Move(RECT bounds) {
        MoveWindow(window_, (bounds.left + bounds.right - 590) / 2, (bounds.top + bounds.bottom - 430) / 2, 590, 430,
                   TRUE);
    }

    void MatchmakingView::Show(bool visible) { ShowWindow(window_, visible ? SW_SHOW : SW_HIDE); }

    void MatchmakingView::Reset() {
        draft_.Reset();
        custom_ = false;
        status_.clear();
        if (window_) {
            SetWindowTextW(controls_.at(5232), L"");
            Update();
        }
    }

    void MatchmakingView::SetSnapshot(const LobbySnapshot &s) {
        snapshot_ = s;
        unsigned room = 0;
        if (s.lobby.size() == 5 && s.lobby[0] == 'L' && std::all_of(s.lobby.begin() + 1, s.lobby.end(),
                                                                    [](char c) {
                                                                        return c >= '0' && c <= '9';
                                                                    })) room = static_cast<unsigned>(std::stoul(
                                                                            s.lobby.substr(1)));
        if (room && room != draft_.room() && !s.room_settings.empty()) log_(
            "MATCH ROOM CONFIRMED number=" + std::to_string(room));
        if (room && !s.room_settings.empty()) draft_.Confirm(
            room, s.room_settings.count("owner") && s.room_settings.at("owner") == "1", s.room_settings);
        else if (!room) draft_.Confirm(0, false, {});
        if (!s.native_error.empty()) {
            const std::map<std::string, std::wstring> errors{
                {"NOT_FOUND", L"房间不存在"}, {"CLOSED", L"房间已关闭"}, {"IN_PROGRESS", L"房间正在对局"}, {"FULL", L"房间已满"},
                {"OWNER_REQUIRED", L"只有房主可以修改设置"}, {"REVISION_CONFLICT", L"房间设置已更新，请重新打开"}, {"BUSY", L"排队或对局期间不能修改"},
                {"ROOM_CAPACITY", L"房间号已用完"}, {"DRAINING", L"服务正在停止，暂停接受匹配"}
            };
            auto i = errors.find(s.native_error);
            status_ = i == errors.end() ? NetworkWide(s.native_error) : i->second;
        } else if (s.phase == LobbyPhase::Error) status_ = L"连接失败：" + NetworkWide(s.error);
        else status_.clear();
        Update();
    }

    void MatchmakingView::Update() {
        const bool pending = snapshot_.phase == LobbyPhase::Matching || snapshot_.phase == LobbyPhase::Cancelling || !
                             snapshot_.native_pending.empty(), room = draft_.room() != 0;
        const bool ready = snapshot_.native_service && snapshot_.phase == LobbyPhase::Lobby;
        for (const auto &item: controls_) {
            const int id = item.first;
            bool visible = true, enabled = !pending && ready;
            if (id >= 5210 && id < 5222) visible = !custom_;
            if (id >= 5230 && id <= 5235) visible = custom_;
            if (id == 5230 || id == 5231 || id == 5232) enabled = enabled && !room;
            if (id == 5233) enabled = enabled && draft_.intent() == RoomIntent::Create;
            if (id == 5234 || id == 5235) enabled = enabled && room;
            if (id == 5240) {
                visible = snapshot_.phase == LobbyPhase::Matching || snapshot_.phase == LobbyPhase::Cancelling;
                enabled = snapshot_.phase == LobbyPhase::Matching;
            }
            if (id == 5241) enabled = true;
            if (id == 5242) {
                visible = custom_ && room && draft_.intent() == RoomIntent::Create;
                enabled = enabled && visible;
            }
            ShowWindow(item.second, visible ? SW_SHOW : SW_HIDE);
            EnableWindow(item.second, enabled);
            InvalidateRect(item.second, nullptr, FALSE);
        }
        InvalidateRect(window_, nullptr, FALSE);
    }

    void MatchmakingView::Command(int id) {
        if (editing_) return;
        if (id == 5241) {
            Reset();
            PostMessageW(GetParent(window_), WM_APP + 25, 0, 0);
            return;
        }
        if (id == 5240) {
            request_("cancel", 0, {});
            return;
        }
        if (!snapshot_.native_pending.empty() || snapshot_.phase != LobbyPhase::Lobby || !request_) return;
        status_.clear();
        if (id == 5200 || id == 5201) {
            custom_ = id == 5201;
            SetWindowTextW(controls_.at(5232), L"");
        }
        if (id >= 5210 && id < 5222) {
            // Display order is row pairs: normal4, fast4, normal3, fast3, no-kuitan4/3.
            if (draft_.room()) {
                status_ = L"请先离开大厅战房间";
                Update();
                return;
            }
            request_("match", GeneralMatchRules()[id - 5210].type, {});
        }
        if (id == 5230) {
            draft_.Reset();
            draft_.CreateRoom();
            request_("create", 0, draft_.settings());
        }
        if (id == 5231) {
            wchar_t number[64]{};
            GetWindowTextW(controls_.at(5232), number, 64);
            MatchmakingDraft target;
            if (!target.JoinRoom(NetworkUtf8(number))) status_ = L"请输入 1000～9999 的四位房间号";
            else request_("join", target.room(), {});
        }
        if (id == 5233) {
            editing_ = true;
            EnableWindow(controls_.at(5235), FALSE);
            auto settings = EditRoomSettings(window_, draft_.settings());
            editing_ = false;
            if (settings) {
                request_("settings", 0, *settings);
            }
        }
        if (id == 5234) request_("leave", 0, {});
        if (id == 5242) request_("close", 0, {});
        if (id == 5235 && !snapshot_.room_settings.empty()) {
            const auto &s = snapshot_.room_settings;
            unsigned type = 1u | (s.at("match_type") == "standard3" ? 16u : 0u) | (s.at("length") == "south" ? 8u : 0u)
                            | (s.at("fast") == "1" ? 64u : 0u) | (s.at("red_count") == "0" ? 2u : 0u) | (
                                s.at("open_tanyao") == "0" ? 4u : 0u);
            request_("match", type, {});
        }
        Update();
    }

    void MatchmakingView::Paint(HDC dc) {
        RECT r{0, 0, 590, 430};
        FillRect(dc, &r, reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
        FrameRect(dc, &r, reinterpret_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH)));
        const auto old = SelectObject(dc, font_);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(0, 0, 0));
        Label(dc, L"天鳳 / 对局大厅", {30, 24, 320, 44});
        Label(dc, L"请选择对局", {30, 55, 320, 75});
        if (!custom_) {
            Label(dc, L"东风          东南", {133, 145, 310, 165});
            const wchar_t *labels[]{L"四人", L"速 四人", L"三人", L"速 三人", L"无食断 四人", L"无食断 三人"};
            for (int i = 0; i < 6; ++i) Label(dc, labels[i], {
                                                  50, 176 + i * 27 + (i >= 4 ? 27 : 0), 130,
                                                  196 + i * 27 + (i >= 4 ? 27 : 0)
                                              });
            // Last two rows are East normal / East fast, not East / South.
            Label(dc, L"常规          速", {133, 286, 310, 304});
        } else {
            Label(dc, L"房间号", {50, 179, 200, 194});
            if (draft_.room()) {
                const auto number = std::to_wstring(draft_.room());
                Label(dc, number.c_str(), {178, 199, 300, 220});
                Label(dc, draft_.intent() == RoomIntent::Create ? L"已确认 · 房主" : L"已确认 · 加入者", {50, 224, 310, 242});
            }
        }
        const HPEN pen = CreatePen(PS_SOLID, 1, RGB(204, 204, 204));
        const auto old_pen = SelectObject(dc, pen);
        MoveToEx(dc, 320, 30, nullptr);
        LineTo(dc, 320, 348);
        SelectObject(dc, old_pen);
        DeleteObject(pen);
        Label(dc, L"对局规则", {340, 32, 560, 52});
        if (snapshot_.phase == LobbyPhase::Matching || snapshot_.phase == LobbyPhase::Cancelling) {
            std::wstring name = L"大厅战";
            if (!draft_.room()) for (const auto &rule: GeneralMatchRules()) if (rule.type == snapshot_.match_type)
                name = rule.name;
            Label(dc, name.c_str(), {340, 65, 560, 125}, DT_LEFT | DT_WORDBREAK);
            Label(dc, snapshot_.phase == LobbyPhase::Cancelling
                          ? L"等待取消确认"
                          : snapshot_.queue_confirmed
                                ? (draft_.room() ? L"已排队，等待真人\n10 秒后人机补席" : L"正在准备人机对局")
                                : L"等待排队确认", {340, 137, 560, 194}, DT_LEFT | DT_WORDBREAK);
        } else {
            Label(dc, custom_ ? L"创建或加入房间后\n选择匹配。" : L"常规 / 速\n四人 / 三人\n东风 / 东南\n\n无食断对局仅东风", {340, 65, 560, 214},
                  DT_LEFT | DT_WORDBREAK);
            if (custom_ &&draft_.intent() == RoomIntent::Create)
            {
                const auto &settings = draft_.settings();
                const std::wstring summary = (settings.at("match_type") == "standard3" ? L"三人 / " : L"四人 / ") +
                                             std::wstring(settings.at("length") == "south"
                                                              ? L"南"
                                                              : settings.at("length") == "single"
                                                                    ? L"一局"
                                                                    : L"东") +
                                             (settings.at("fast") == "1" ? L" / 速" : L" / 常规");
                Label(dc, summary.c_str(), {340, 166, 560, 210}, DT_LEFT | DT_WORDBREAK);
            }
        }
        SetTextColor(dc, RGB(102, 102, 102));
        Label(dc, !snapshot_.native_pending.empty() ? L"等待服务确认" : snapshot_.native_service ? L"本机服务 · 已连接" : L"匹配服务未连接",
              {340, 330, 560, 350});
        if (!snapshot_.profile.empty()) {
            auto value = [&](const std::string &key) {
                auto i = snapshot_.profile.find(key);
                return i == snapshot_.profile.end() ? std::wstring() : NetworkWide(i->second);
            };
            auto text = value("nickname");
            if (value("account").empty()) text += L"\n访客，不保存战绩和牌谱";
            else {
                for (const auto &n: {"4", "3"}) {
                    auto rating = value(std::string("rating") + n);
                    const auto dot = rating.find(L'.');
                    if (dot != rating.npos) rating.resize(dot);
                    text += std::wstring(n[0] == '4' ? L"\n四人：" : L"\n三人：") + value(std::string("grade") + n) + L" " +
                            value(std::string("points") + n) + L"/" + value(std::string("target") + n) + L"pt  R" +
                            rating;
                }
            }
            Label(dc, text.c_str(), {340, 222, 580, 287}, DT_LEFT | DT_WORDBREAK);
        }
        Label(dc, custom_ ? L"全注册真人计排位\n含访客或人机不计排位" : L"人机对局不计排位", {340, 288, 580, 325}, DT_LEFT | DT_WORDBREAK);
        if (!status_.empty()) {
            SetTextColor(dc, RGB(180, 32, 32));
            Label(dc, status_.c_str(), {30, 398, 565, 420});
        } else if (!snapshot_.replay_status.empty()) {
            auto text = NetworkWide(snapshot_.replay_status);
            Label(dc, text.c_str(), {30, 395, 580, 427}, DT_LEFT | DT_WORDBREAK);
        }
        SelectObject(dc, old);
    }

    void MatchmakingView::PaintButton(const DRAWITEMSTRUCT &item) {
        const bool enabled = IsWindowEnabled(item.hwndItem) != FALSE;
        POINT point{};
        GetCursorPos(&point);
        ScreenToClient(item.hwndItem, &point);
        const bool hot = enabled && PtInRect(&item.rcItem, point);
        auto r = item.rcItem;
        FillRect(item.hDC, &r, reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
        // Same white / lower-half grey gradient and 888 border used by WC_ITEM_BTN.
        const int half = r.bottom / 2;
        for (int y = half; y < r.bottom; ++y) {
            const int value = enabled
                                  ? 221 + 34 * (y - half) / std::max(1, static_cast<int>(r.bottom) - half - 1)
                                  : 238;
            SetDCBrushColor(item.hDC, RGB(value, value, value));
            RECT row{0, y, r.right, y + 1};
            FillRect(item.hDC, &row, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        }
        const bool selected = (item.CtlID == 5200 && !custom_) || (item.CtlID == 5201 && custom_);
        SetDCBrushColor(item.hDC, hot || selected ? RGB(51, 153, 255) : RGB(136, 136, 136));
        FrameRect(item.hDC, &r, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        const auto old = SelectObject(item.hDC, font_);
        SetBkMode(item.hDC, TRANSPARENT);
        SetTextColor(item.hDC, enabled ? RGB(34, 34, 34) : RGB(153, 153, 153));
        wchar_t text[80]{};
        GetWindowTextW(item.hwndItem, text, 80);
        if (item.CtlID >= 5218 && item.CtlID <= 5221) wcscpy_s(text, item.CtlID % 2 ? L"速 东" : L"东");
        if (item.itemState & ODS_SELECTED) OffsetRect(&r, 1, 1);
        Label(item.hDC, text, r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (item.itemState & ODS_FOCUS) {
            InflateRect(&r, -3, -3);
            DrawFocusRect(item.hDC, &r);
        }
        SelectObject(item.hDC, old);
    }

    void MatchmakingView::Capture(HDC dc) {
        if (!IsWindowVisible(window_)) return;
        RECT r{};
        GetWindowRect(window_, &r);
        MapWindowPoints(nullptr, GetParent(window_), reinterpret_cast<POINT *>(&r), 2);
        const int saved = SaveDC(dc);
        SetViewportOrgEx(dc, r.left, r.top, nullptr);
        Paint(dc);
        for (const auto &item: controls_)
            if (IsWindowVisible(item.second)) {
                RECT child{};
                GetWindowRect(item.second, &child);
                MapWindowPoints(nullptr, window_, reinterpret_cast<POINT *>(&child), 2);
                const int s = SaveDC(dc);
                OffsetViewportOrgEx(dc, child.left, child.top, nullptr);
                SendMessageW(item.second, WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT | PRF_NONCLIENT);
                RestoreDC(dc, s);
            }
        RestoreDC(dc, saved);
    }

LRESULT CALLBACK MatchmakingView::Procedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
        auto *self = reinterpret_cast<MatchmakingView *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = static_cast<MatchmakingView *>(reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self) {
            try {
                if (message == WM_ERASEBKGND) return 1;
                if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLOREDIT) {
                    SetBkColor(reinterpret_cast<HDC>(wp), RGB(255, 255, 255));
                    return reinterpret_cast<LRESULT>(GetStockObject(WHITE_BRUSH));
                }
                if (message == WM_COMMAND && HIWORD(wp) == BN_CLICKED) {
                    self->Command(LOWORD(wp));
                    return 0;
                }
                if (message == WM_DRAWITEM) {
                    self->PaintButton(*reinterpret_cast<DRAWITEMSTRUCT *>(lp));
                    return TRUE;
                }
                if (message == WM_PRINTCLIENT) {
                    self->Paint(reinterpret_cast<HDC>(wp));
                    return 0;
                }
                if (message == WM_PAINT) {
                    PAINTSTRUCT p{};
                    const auto dc = BeginPaint(window, &p);
                    self->Paint(dc);
                    EndPaint(window, &p);
                    return 0;
                }
            } catch (const std::exception &error) {
                self->editing_ = false;
                self->log_(std::string("MATCH DRAFT ERROR ") + error.what());
                self->status_ = L"操作失败，请重试";
                self->Update();
                return 0;
            }
        }
        return DefWindowProcW(window, message, wp, lp);
    }
}
