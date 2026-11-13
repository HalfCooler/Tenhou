#include "variant_table_view.hpp"
#include <algorithm>
#include <sstream>

namespace tenhou {
    namespace {
        std::wstring TileText(int tile) {
            static const wchar_t *honors[]{L"东", L"南", L"西", L"北", L"白", L"发", L"中"};
            static const wchar_t *flowers[]{L"梅", L"兰", L"竹", L"菊", L"春", L"夏", L"秋", L"冬"};
            if (tile >= 136 && tile < 144) return flowers[tile - 136];
            if (tile < 0 || tile >= 136) return L"■";
            const int kind = tile / 4;
            return kind >= 27 ? honors[kind - 27] : std::to_wstring(kind % 9 + 1) + std::wstring(1, L"万筒索"[kind / 9]);
        }
        std::wstring Tiles(const std::string &value) {
            std::istringstream input(value); std::string field; std::wstring text;
            while (std::getline(input, field, ',')) {
                if (field.empty()) continue;
                if (!text.empty()) text += L" ";
                text += field == "?" ? L"暗牌" : TileText(std::stoi(field));
            }
            return text;
        }
        std::wstring Melds(const std::string &value) {
            std::istringstream input(value); std::string field; std::wstring text;
            while (std::getline(input, field, ';')) {
                const auto colon = field.find(':');
                if (colon != std::string::npos) text += L" [" + Tiles(field.substr(colon + 1)) + L"]";
            }
            return text;
        }
    }
    VariantTableView::VariantTableView(HWND parent, Submit submit, std::function<void()> next, std::function<void()> exit)
        : on_submit_(std::move(submit)), on_return_(std::move(next)), on_exit_(std::move(exit)) {
        WNDCLASSEXW wc{sizeof(wc)}; wc.hInstance = GetModuleHandleW(nullptr); wc.lpfnWndProc = Procedure;
        wc.lpszClassName = L"TENHOU_VARIANT_TABLE"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassExW(&wc);
        window_ = CreateWindowExW(0, wc.lpszClassName, L"", WS_CHILD | WS_CLIPCHILDREN, 0, 0, 1024, 768,
            parent, nullptr, wc.hInstance, this);
        font_ = CreateFontW(-19, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
        choices_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY,
            0, 0, 100, 100, window_, reinterpret_cast<HMENU>(1), wc.hInstance, nullptr);
        const auto button = [&](int id, const wchar_t *title) { return CreateWindowExW(0, L"BUTTON", title,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 100, 30, window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), wc.hInstance, nullptr); };
        submit_ = button(2, L"执行所选操作"); return_ = button(3, L"返回大厅"); exit_ = button(4, L"退出本桌");
        for (HWND control: {choices_, submit_, return_, exit_}) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    }
    VariantTableView::~VariantTableView() { if (window_) DestroyWindow(window_); if (font_) DeleteObject(font_); }
    void VariantTableView::Move(RECT bounds) {
        const int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
        MoveWindow(window_, bounds.left, bounds.top, width, height, TRUE);
        MoveWindow(choices_, std::max(20, width - 300), 100, 280, std::max(50, height - 220), TRUE);
        MoveWindow(submit_, std::max(20, width - 300), height - 105, 280, 36, TRUE);
        MoveWindow(return_, width - 300, height - 55, 130, 34, TRUE);
        MoveWindow(exit_, width - 150, height - 55, 130, 34, TRUE);
    }
    void VariantTableView::Show(bool visible) {
        ShowWindow(window_, visible ? SW_SHOW : SW_HIDE);
        if (visible) SetWindowPos(window_, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    void VariantTableView::Set(const LobbySnapshot &snapshot) {
        const bool changed = snapshot.actions.revision != snapshot_.actions.revision;
        snapshot_ = snapshot;
        if (changed) {
            SendMessageW(choices_, LB_RESETCONTENT, 0, 0);
            for (const auto &action: snapshot.actions.choices) {
                const auto blank = action.label.find(' ');
                std::wstring label = NetworkWide(action.label.substr(0, blank));
                if (blank != std::string::npos) {
                    const auto tiles = action.label.substr(blank + 1);
                    label += L"  " + (action.kind == "dingque" ? std::wstring(1, L"万筒索"[std::stoi(tiles)]) : Tiles(tiles));
                }
                SendMessageW(choices_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
            }
            SendMessageW(choices_, LB_SETCURSEL, 0, 0);
        }
        EnableWindow(submit_, snapshot.phase == LobbyPhase::Game && !snapshot.table_paused && !snapshot.recovering
            && snapshot.actions.pending.empty() && !snapshot.actions.choices.empty());
        const bool disconnected = snapshot.phase == LobbyPhase::Error || snapshot.phase == LobbyPhase::Disconnected;
        SetWindowTextW(return_, disconnected ? L"返回登录" : L"返回大厅");
        EnableWindow(return_, snapshot.phase == LobbyPhase::MatchComplete || disconnected);
        EnableWindow(exit_, snapshot.exit_allowed && snapshot.phase == LobbyPhase::Game && !snapshot.exit_pending);
        InvalidateRect(window_, nullptr, FALSE);
    }
    void VariantTableView::Act() {
        if (!IsWindowEnabled(submit_)) return;
        const auto index = SendMessageW(choices_, LB_GETCURSEL, 0, 0);
        if (index >= 0 && static_cast<std::size_t>(index) < snapshot_.actions.choices.size())
            on_submit_(snapshot_.actions.revision, static_cast<std::size_t>(index));
    }
    void VariantTableView::Capture(HDC dc) {
        Paint(dc);
        for (HWND control: {choices_, submit_, return_, exit_}) {
            RECT r{}; GetWindowRect(control, &r); MapWindowPoints(nullptr, window_, reinterpret_cast<POINT *>(&r), 2);
            const int saved = SaveDC(dc); OffsetViewportOrgEx(dc, r.left, r.top, nullptr);
            SendMessageW(control, WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT | PRF_NONCLIENT);
            RestoreDC(dc, saved);
        }
    }
    void VariantTableView::Paint(HDC dc) {
        RECT r{}; GetClientRect(window_, &r); HBRUSH bg = CreateSolidBrush(RGB(18, 70, 62)); FillRect(dc, &r, bg); DeleteObject(bg);
        const auto old = SelectObject(dc, font_); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(245, 241, 218));
        const auto value = [&](const std::string &key) { auto i = snapshot_.variant_state.find(key); return i == snapshot_.variant_state.end() ? std::string() : i->second; };
        const auto rule = snapshot_.rules.find(value("rule"));
        std::wstring title = rule == snapshot_.rules.end() ? L"好友麻将" : NetworkWide(rule->second.name);
        title += L"   第 " + NetworkWide(value("hand_number")) + L" / " + NetworkWide(value("total_hands")) + L" 盘   余牌 " + NetworkWide(value("wall"));
        RECT heading{24, 20, r.right - 20, 55}; DrawTextW(dc, title.c_str(), -1, &heading, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
        const auto stage = value("phase");
        std::wstring instruction = stage == "exchange" ? L"选择同一花色的三张牌交换" : stage == "missing" ? L"选择定缺花色" : stage == "claims" ? L"响应其他玩家的出牌" : stage == "ended" ? L"本盘结束，请确认下一盘" : stage == "complete" ? L"本场结束" : L"选择右侧合法操作";
        if (stage == "exchange") {
            const auto direction = value("exchange_direction");
            instruction += direction == "clockwise" ? L"（顺时针）" : direction == "counterclockwise" ? L"（逆时针）" : direction == "opposite" ? L"（对侧）" : L"";
        }
        if ((stage == "turn" || stage == "claims") && value("retired" + value("seat")) == "1")
            instruction = L"你已和牌退场，等待本局结束";
        if (snapshot_.table_paused) instruction = L"牌桌暂停，等待重连或退出投票";
        else if (stage == "ended" && snapshot_.actions.choices.empty()) instruction = L"已准备，等待其他玩家进入下一盘";
        else if (!snapshot_.actions.pending.empty()) instruction = L"已提交，等待其他玩家与服务器";
        RECT prompt{24, 60, r.right - 20, 90}; DrawTextW(dc, instruction.c_str(), -1, &prompt, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
        const auto scores = value("scores"); std::istringstream score_input(scores); std::string score;
        const int panel_height = std::max(100, static_cast<int>((r.bottom - 230) / 4));
        for (int seat = 0; seat < 4; ++seat) {
            std::getline(score_input, score, ','); const std::string key = std::to_string(seat);
            std::wstring text = NetworkWide(value("n" + key)) + L"  " + NetworkWide(score) + L"分";
            if (value("seat") == key) text += L"（你）";
            if (value("dealer") == key) text += L"  庄";
            if (value("turn") == key && value("retired" + key) != "1") text += L"  ←";
            if (value("missing" + key) != "-1" && !value("missing" + key).empty()) text += L"  缺" + std::wstring(1, L"万筒索"[std::stoi(value("missing" + key))]);
            text += L"\n手牌：" + (value("hand" + key).empty() ? NetworkWide(value("hidden" + key)) + L"张暗牌" : Tiles(value("hand" + key)));
            text += L"\n副露：" + Melds(value("melds" + key));
            if (!value("flowers" + key).empty()) text += L"  花：" + Tiles(value("flowers" + key));
            text += L"\n牌河：" + Tiles(value("river" + key));
            if (value("retired" + key) == "1") text += L"  已和退场";
            else if (!value("wins" + key).empty() && value("wins" + key) != "0") text += L"  已和 " + NetworkWide(value("wins" + key)) + L" 次";
            if (!value("winning" + key).empty()) text += L"  和牌：" + Tiles(value("winning" + key));
            RECT panel{24, 105 + seat * panel_height, r.right - 325, 105 + (seat + 1) * panel_height};
            DrawTextW(dc, text.c_str(), -1, &panel, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX);
        }
        std::wstring history = NetworkWide(value("result") + "\n" + value("history"));
        RECT footer{24, r.bottom - 125, r.right - 325, r.bottom - 15}; DrawTextW(dc, history.c_str(), -1, &footer, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX);
        SelectObject(dc, old);
    }
    LRESULT CALLBACK VariantTableView::Procedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
        auto *self = reinterpret_cast<VariantTableView *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) { self = static_cast<VariantTableView *>(reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams); SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self)); }
        if (self) {
            if (message == WM_COMMAND) {
                if (LOWORD(wp) == 2 || (LOWORD(wp) == 1 && HIWORD(wp) == LBN_DBLCLK)) self->Act();
                if (LOWORD(wp) == 3 && IsWindowEnabled(self->return_)) self->on_return_();
                if (LOWORD(wp) == 4 && IsWindowEnabled(self->exit_)) self->on_exit_();
                return 0;
            }
            if (message == WM_PAINT) { PAINTSTRUCT p{}; HDC dc = BeginPaint(window, &p); self->Paint(dc); EndPaint(window, &p); return 0; }
        }
        return DefWindowProcW(window, message, wp, lp);
    }
}
