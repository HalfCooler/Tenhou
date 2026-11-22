#include "offline_lobby_view.hpp"
#include "background.hpp"
#include "network_protocol.hpp"
#include "window_layout.hpp"
#include <commctrl.h>
#include <shellapi.h>
#include <cstring>
#include <stdexcept>

namespace tenhou {
    namespace {
        using Label = TextLabelKey;
        constexpr wchar_t kViewport[] = L"TENHOU_REBUILD_OFFLINE_VIEW";
        constexpr wchar_t kPanel[] = L"TENHOU_REBUILD_OFFLINE_LOBBY";
        constexpr int kPanelWidth = 590, kPanelHeight = 430;
        constexpr unsigned kOuter = 0xFA4, kTier = 0xC00, kQueue = 0xFAF;
        constexpr unsigned kContestReserve = 0xFBB, kContestRegister = 0xFBC;
        constexpr unsigned kPractice = 0xFBD, kMenu = 0xFBE, kCancel = IDCANCEL;
        constexpr unsigned kLifetime = 0xFC0, kMonthly = 0xFC1, kHistory = 200;
        constexpr UINT_PTR kClockTimer = 0x65, kRulesTimer = 0x66, kMoveTimer = 0x68;
        constexpr UINT kRulesDelay = 500, kMoveDelay = 250;
        constexpr COLORREF kInk = RGB(32, 32, 32), kMuted = RGB(95, 95, 95), kAccent = RGB(28, 77, 124);
        constexpr Label kTabs[]{Label::lobby_ranked, Label::lobby_parlor, Label::lobby_observe,
                                 Label::lobby_replays, Label::lobby_help, Label::lobby_toutenkou};
        constexpr int kTabWidths[]{52, 55, 46, 46, 46, 45};
        constexpr Label kTiers[]{Label::lobby_general, Label::lobby_advanced, Label::lobby_special, Label::lobby_phoenix};
        constexpr Label kRequirements[]{Label::lobby_general_limit, Label::lobby_advanced_limit,
                                        Label::lobby_special_limit, Label::lobby_phoenix_limit};
        constexpr Label kRows[]{Label::lobby_no_tanyao, Label::lobby_red, Label::lobby_fast,
                                Label::lobby_light, Label::lobby_three_red, Label::lobby_three_fast};
        constexpr Label kParlorRows[]{Label::lobby_parlor_0, Label::lobby_parlor_2, Label::lobby_parlor_5,
                                      Label::lobby_parlor_three};
        constexpr Label kEvents[]{Label::lobby_event_one, Label::lobby_event_two, Label::lobby_event_three};
        struct ProfileChoice {
            unsigned type;
            Label label;
        };
        constexpr ProfileChoice kLifetimeChoices[]{
            {kRankedFourProfile, Label::lobby_lifetime_four}, {kRankedThreeProfile, Label::lobby_lifetime_three},
            {kPrivateFourProfile, Label::lobby_private_four}, {kPrivateThreeProfile, Label::lobby_private_three},
            {kAmberRatingProfile, Label::lobby_amber_rating}, {kAmberGProfile, Label::lobby_amber_g}
        };
        constexpr Label kRankedMonthlyRules[]{
            Label::lobby_no_tanyao, Label::lobby_four_tanyao, Label::lobby_red, Label::lobby_three_red
        };
        constexpr const wchar_t *kPrivateProfileKeys[]{L"PFPV4", L"PFPV3"};

        const wchar_t *Text(Label label) { return kTextLabels[static_cast<std::size_t>(label)].label.data(); }

        std::wstring ProfileCaption(Label period, const std::wstring &category) {
            return std::wstring(Text(period)) + L"／" + category;
        }

        std::wstring AmberProfileLabel(std::size_t index) {
            const auto length = (index < 3 || index == 6) ? Label::lobby_east : Label::lobby_south;
            return std::wstring(Text(length)) + L" " + Text(kParlorRows[index < 6 ? index % 3 : 3]);
        }

        std::wstring LifetimeProfileLabel(unsigned type) {
            for (const auto &choice: kLifetimeChoices)
                if (type == choice.type) return Text(choice.label);
            const auto it = std::find(kAmberLifetimeProfiles.begin(), kAmberLifetimeProfiles.end(), type);
            if (it != kAmberLifetimeProfiles.end())
                return std::wstring(Text(Label::lobby_amber)) + L" " + AmberProfileLabel(it - kAmberLifetimeProfiles.begin());
            return {};
        }

        std::wstring RankedMonthlyLabel(unsigned type) {
            const auto length = (type & 8) ? Label::lobby_south : Label::lobby_east;
            const auto rule = (type & 0x10) ? 3 : (type & 4) ? 0 : (type & 2) ? 1 : 2;
            return std::wstring(Text(length)) + L" " + Text(kRankedMonthlyRules[rule]);
        }

        std::wstring MonthlyProfileCaption(unsigned type) {
            const auto amber = std::find(kAmberMonthlyProfiles.begin(), kAmberMonthlyProfiles.end(), type);
            if (amber != kAmberMonthlyProfiles.end())
                return ProfileCaption(Label::lobby_month, std::wstring(Text(Label::lobby_amber)) + L" " +
                                      AmberProfileLabel(amber - kAmberMonthlyProfiles.begin()));
            const unsigned tier = ((type & 0x80) ? 1 : 0) + ((type & 0x20) ? 2 : 0);
            return ProfileCaption(Label::lobby_month, std::wstring(Text(kTiers[tier])) + L"桌 " + RankedMonthlyLabel(type));
        }

        void Draw(HDC dc, std::wstring_view text, RECT rect, UINT flags = DT_LEFT, COLORREF color = kInk) {
            SetTextColor(dc, color);
            DrawTextW(dc, text.data(), static_cast<int>(text.size()), &rect,
                      DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS | flags);
        }

        void Line(HDC dc, RECT rect) { FillRect(dc, &rect, GetSysColorBrush(COLOR_3DLIGHT)); }

        void ImageAt(HDC dc, const Image &image, int x, int y) {
            if (image.bgra.empty()) return;
            Gdiplus::Graphics graphics(dc);
            Gdiplus::Bitmap bitmap(image.width, image.height, image.width * 4, PixelFormat32bppARGB,
                                   const_cast<BYTE *>(image.bgra.data()));
            if (graphics.DrawImage(&bitmap, x, y, image.width, image.height) != Gdiplus::Ok)
                throw std::runtime_error("Cannot draw offline lobby artwork");
        }

        RECT QueueBounds(unsigned slot) {
            constexpr int row_y[]{52, 79, 106, 133, 165, 192};
            const int x = slot % 2 == 0 ? 130 : 220;
            const int y = 120 + row_y[slot / 2];
            return {x, y, x + 40, y + 20};
        }

        std::wstring Decimal(int value, std::size_t decimals) {
            auto digits = std::to_wstring(std::abs(value));
            if (digits.size() <= decimals) digits.insert(0, decimals + 1 - digits.size(), L'0');
            digits.insert(digits.size() - decimals, 1, L'.');
            return value < 0 ? L"-" + digits : digits;
        }

        void MenuItem(HMENU menu, UINT id, Label label, UINT flags = MF_ENABLED) {
            AppendMenuW(menu, MF_STRING | flags, id, Text(label));
        }

        UINT Popup(HMENU menu, HWND button, HWND owner) {
            RECT anchor{};
            GetWindowRect(button, &anchor);
            const auto command = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN,
                                                  anchor.left, anchor.bottom, owner, nullptr);
            DestroyMenu(menu);
            return command;
        }

        struct RoomNumberDialog {
            std::wstring result;

            static INT_PTR CALLBACK Procedure(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
                constexpr int kEdit = 100, kPrompt = 101;
                if (message == WM_INITDIALOG) {
                    SetWindowLongPtrW(dialog, DWLP_USER, lparam);
                    const auto add = [&](const wchar_t *type, Label label, int id, DWORD style, RECT bounds) {
                        MapDialogRect(dialog, &bounds);
                        const HWND child = CreateWindowExW(0, type, id == kEdit ? L"" : Text(label),
                            WS_CHILD | WS_VISIBLE | style, bounds.left, bounds.top,
                            bounds.right - bounds.left, bounds.bottom - bounds.top, dialog,
                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
                        SendMessageW(child, WM_SETFONT, SendMessageW(dialog, WM_GETFONT, 0, 0), 0);
                    };
                    add(L"STATIC", Label::lobby_number_prompt, kPrompt, 0, {12, 12, 212, 31});
                    add(L"EDIT", Label::lobby_enter_number, kEdit, WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
                        {12, 34, 212, 51});
                    add(L"BUTTON", Label::lobby_confirm, IDOK, WS_TABSTOP | BS_DEFPUSHBUTTON, {95, 61, 151, 79});
                    add(L"BUTTON", Label::lobby_cancel, IDCANCEL, WS_TABSTOP, {156, 61, 212, 79});
                    SendDlgItemMessageW(dialog, kEdit, EM_LIMITTEXT, 32, 0);
                    RECT owner{}, bounds{};
                    GetWindowRect(GetWindow(dialog, GW_OWNER), &owner);
                    GetWindowRect(dialog, &bounds);
                    SetWindowPos(dialog, nullptr, (owner.left + owner.right - (bounds.right - bounds.left)) / 2,
                                 (owner.top + owner.bottom - (bounds.bottom - bounds.top)) / 2,
                                 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
                    PostMessageW(dialog, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(GetDlgItem(dialog, kEdit)), TRUE);
                    return FALSE;
                }
                if (message != WM_COMMAND) return FALSE;
                if (LOWORD(wparam) == IDCANCEL) { EndDialog(dialog, IDCANCEL); return TRUE; }
                if (LOWORD(wparam) != IDOK) return FALSE;
                wchar_t value[33]{};
                GetDlgItemTextW(dialog, kEdit, value, static_cast<int>(std::size(value)));
                if (auto room = LobbyNumber(value)) {
                    reinterpret_cast<RoomNumberDialog *>(GetWindowLongPtrW(dialog, DWLP_USER))->result = std::move(*room);
                    EndDialog(dialog, IDOK);
                } else {
                    SetDlgItemTextW(dialog, kPrompt, Text(Label::lobby_number_invalid));
                    SetFocus(GetDlgItem(dialog, kEdit));
                    SendDlgItemMessageW(dialog, kEdit, EM_SETSEL, 0, -1);
                }
                return TRUE;
            }

            static std::wstring Show(HWND owner) {
                RoomNumberDialog state;
                const std::wstring title = Text(Label::lobby_enter_number), font = L"Microsoft YaHei UI";
                DLGTEMPLATE header{};
                header.style = WS_POPUP | WS_SYSMENU | WS_CAPTION | DS_MODALFRAME | DS_SETFONT;
                header.cx = 224;
                header.cy = 89;
                constexpr auto header_words = (sizeof(header) + sizeof(WORD) - 1) / sizeof(WORD);
                std::vector<WORD> buffer(header_words + 2 + title.size() + 1 + 1 + font.size() + 1, 0);
                std::memcpy(buffer.data(), &header, sizeof(header));
                auto *cursor = buffer.data() + header_words + 2;
                std::memcpy(cursor, title.c_str(), (title.size() + 1) * sizeof(wchar_t));
                cursor += title.size() + 1;
                *cursor++ = 9;
                std::memcpy(cursor, font.c_str(), (font.size() + 1) * sizeof(wchar_t));
                if (DialogBoxIndirectParamW(GetModuleHandleW(nullptr), reinterpret_cast<const DLGTEMPLATE *>(buffer.data()),
                    owner, Procedure, reinterpret_cast<LPARAM>(&state)) == -1)
                    throw std::runtime_error("Cannot create room number dialog");
                return state.result;
            }
        };
    }

    OfflineLobbyView::OfflineLobbyView(HWND parent, std::filesystem::path config, std::function<void(Action)> action,
                                       std::function<void(const std::string &)> log, Submit submit)
        : config_(std::move(config)), action_(std::move(action)), log_(std::move(log)),
          submit_(std::move(submit)), parent_(parent) {
        const auto instance = GetModuleHandleW(nullptr);
        try {
            for (const auto *name: {kViewport, kPanel}) {
                WNDCLASSW wc{};
                wc.hInstance = instance;
                wc.lpfnWndProc = Procedure;
                wc.lpszClassName = name;
                wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
                if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
                    throw std::runtime_error("Cannot register offline lobby window");
            }
            font_ = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                0, 0, CLEARTYPE_QUALITY, 0, L"Microsoft YaHei UI");
            title_font_ = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                      0, 0, CLEARTYPE_QUALITY, 0, L"Microsoft YaHei UI");
            viewport_ = CreateWindowExW(WS_EX_CONTROLPARENT, kViewport, L"", WS_CHILD | WS_CLIPCHILDREN,
                                        0, 0, 0, 0, parent_, nullptr, instance, this);
            if (!viewport_ || !font_ || !title_font_) throw std::runtime_error("Cannot create offline lobby viewport");
            panel_ = CreateWindowExW(WS_EX_CONTROLPARENT, kPanel, Text(submit_ ? Label::lobby_service : Label::lobby_offline),
                WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE, 0, 0, kPanelWidth, kPanelHeight,
                viewport_, nullptr, instance, this);
            if (!panel_) throw std::runtime_error("Cannot create offline lobby panel");
            int x = 30;
            for (unsigned i = 0; i < std::size(kTabs); ++i) {
                AddButton(kOuter + i, {x, 98, x + kTabWidths[i], 120}, kTabs[i]);
                x += kTabWidths[i];
            }
            for (unsigned i = 0; i < std::size(kTiers); ++i) {
                const LONG left = 40 + static_cast<LONG>(i) * 36;
                AddButton(kTier + i, {left, 124, left + 36, 145}, kTiers[i]);
            }
            for (unsigned i = 0; i < kRankedSlots; ++i) AddButton(kQueue + i, QueueBounds(i), Label::lobby_reserve);
            AddButton(kContestRegister, {210, 306, 286, 326}, Label::lobby_register);
            AddButton(kContestReserve, {210, 306, 286, 326}, Label::lobby_reserve);
            AddButton(kMenu, {90, 365, 148, 385}, Label::lobby_menu);
            AddButton(kPractice, {149, 365, 207, 385}, Label::lobby_practice);
            AddButton(kCancel, {208, 365, 255, 385}, Label::lobby_cancel);
            AddButton(kLifetime, {335, 60, 555, 80}, Label::lobby_lifetime);
            AddButton(kMonthly, {335, 195, 555, 215}, Label::lobby_month);
            LoadProfilePreferences();
            toutenkou_ = std::filesystem::is_regular_file(config_.parent_path() / L"toutenkou.test");
            Refresh();
        } catch (...) {
            if (viewport_) DestroyWindow(viewport_);
            if (font_) DeleteObject(font_);
            if (title_font_) DeleteObject(title_font_);
            throw;
        }
    }

    OfflineLobbyView::~OfflineLobbyView() {
        if (IsWindow(viewport_)) DestroyWindow(viewport_);
        DeleteObject(title_font_);
        DeleteObject(font_);
    }

    void OfflineLobbyView::AddButton(unsigned id, RECT bounds, Label label) {
        const HWND button = CreateWindowExW(0, L"BUTTON", Text(label), WS_CHILD | WS_TABSTOP | BS_OWNERDRAW,
            bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top, panel_,
            reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
        if (!button || !SetWindowSubclass(button, ButtonProcedure, 1, reinterpret_cast<DWORD_PTR>(this)))
            throw std::runtime_error("Cannot create offline lobby control");
        SendMessageW(button, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
        buttons_.emplace(id, button);
    }

    void OfflineLobbyView::ConfigureArtwork(const std::filesystem::path &resources) {
        artwork_.Configure(resources, kDefaultClientWidth);
        const auto &frame = artwork_.Get(592);
        if (frame.width != kPanelWidth || frame.height != kPanelHeight)
            throw std::runtime_error("Original lobby frame dimensions changed");
        background_ = BuildPreferredBackground(resources, config_, kDefaultClientWidth, kDefaultClientHeight,
                                               ClientPreferences::Load(config_));
        log_("OFFLINE LOBBY frame=592 size=590x430");
        InvalidateRect(viewport_, nullptr, FALSE);
        InvalidateRect(panel_, nullptr, FALSE);
    }

    void OfflineLobbyView::Resize(int width, int height) {
        MoveWindow(viewport_, 0, 0, width, height, TRUE);
        MoveWindow(panel_, (width - kPanelWidth) / 2, (height - kPanelHeight) / 2, kPanelWidth, kPanelHeight, TRUE);
    }

    void OfflineLobbyView::Show(bool visible) {
        if (visible_ == visible) return;
        visible_ = visible;
        if (visible) {
            LoadProfilePreferences();
            ShowWindow(viewport_, SW_SHOW);
            Tick();
            SetTimer(panel_, kClockTimer, 1000, nullptr);
            SetFocus(buttons_.at(kOuter + outer_));
        } else {
            KillTimer(panel_, kClockTimer);
            KillTimer(panel_, kMoveTimer);
            ClearRules();
            state_.Cancel();
            state_.DeclineMove();
            move_declined_ = false;
            ShowWindow(viewport_, SW_HIDE);
        }
    }

    bool OfflineLobbyView::HandleKey(UINT key) {
        if (!visible_ || key != VK_ESCAPE || !state_.reservations() || !IsChild(panel_, GetFocus())) return false;
        Command(kCancel);
        return true;
    }

    void OfflineLobbyView::SetServiceSnapshot(const LobbySnapshot &snapshot) {
        if (!snapshot.origin_lobby) return;
        official_ = snapshot.official;
        service_ready_ = !snapshot.recovering &&
                         (snapshot.phase == LobbyPhase::Lobby || snapshot.phase == LobbyPhase::Matching);
        const bool moved = state_.room() != snapshot.origin_lobby->room();
        state_ = *snapshot.origin_lobby;
        service_pending_ = snapshot.origin_pending;
        service_error_ = snapshot.origin_error;
        service_clock_ = snapshot.origin_clock;
        service_clock_at_ = snapshot.origin_clock_at;
        if (moved) RoomChanged();
        else if (visible_) Tick();
    }

    void OfflineLobbyView::Refresh() {
        const HWND old_focus = GetFocus();
        const bool pending = state_.pending().has_value() || !service_pending_.empty() || (submit_ && !service_ready_);
        const bool busy = pending || state_.reservations() != 0;
        const auto control = [&](unsigned id, bool show, bool enable) {
            const HWND button = buttons_.at(id);
            if ((IsWindowEnabled(button) != FALSE) != enable) EnableWindow(button, enable);
            if (((GetWindowLongPtrW(button, GWL_STYLE) & WS_VISIBLE) != 0) != show)
                ShowWindow(button, show ? SW_SHOWNA : SW_HIDE);
        };
        for (unsigned i = 0; i < std::size(kTabs); ++i) {
            const bool show = (i != 1 || state_.ranked()) && (i != 5 || (state_.ranked() && toutenkou_));
            control(kOuter + i, show, true);
        }
        for (unsigned i = 0; i < std::size(kTiers); ++i) control(kTier + i, state_.ranked() && outer_ == 0, true);
        for (unsigned i = 0; i < kRankedSlots; ++i) {
            const auto slot = state_.Candidate(tier_, i);
            const bool normal = outer_ == 0 && slot.visible;
            const bool parlor = outer_ == 1 && i < kParlorSlots;
            const bool toutenkou = outer_ == 5 && i == 0;
            control(kQueue + i, normal || parlor || toutenkou,
                    (normal && !pending && !slot.reserved) || (official_ && toutenkou));
            if (hovered_ == static_cast<int>(i) && (!normal || pending || slot.reserved)) Hover(-1);
        }
        const auto contest_action = state_.ContestAction(seconds_);
        const bool contest_page = state_.contest() && outer_ == 0;
        control(kContestRegister, contest_page && contest_action == ContestStatus::Register, !pending);
        control(kContestReserve, contest_page && (contest_action == ContestStatus::Open ||
                contest_action == ContestStatus::Reserved), !pending && contest_action == ContestStatus::Open);
        control(kMenu, true, !busy);
        control(kPractice, true, !busy);
        control(kCancel, true, state_.reservations() != 0 && !pending);
        control(kLifetime, rule_slot_ < 0, true);
        control(kMonthly, rule_slot_ < 0, true);
        if (IsChild(panel_, old_focus) && (!IsWindowVisible(old_focus) || !IsWindowEnabled(old_focus))) {
            const auto preferred = contest_page && contest_action == ContestStatus::Open && !pending ? kContestReserve :
                state_.reservations() && !pending ? kCancel : kOuter + outer_;
            SetFocus(buttons_.at(preferred));
        }
        RedrawWindow(panel_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
    }

    void OfflineLobbyView::RequestMove(const std::wstring &room) {
        if (submit_) { submit_("change", room); return; }
        if (!state_.RequestMove(room)) return;
        move_declined_ = false;
        ClearRules();
        move_at_ = GetTickCount64() + kMoveDelay;
        SetTimer(panel_, kMoveTimer, kMoveDelay, nullptr);
        Refresh();
    }

    void OfflineLobbyView::CompleteMove() {
        if (!state_.pending() || GetTickCount64() < move_at_) return;
        KillTimer(panel_, kMoveTimer);
        const auto room = *state_.pending();
        if (room.front() == L'C') {
            const auto event = std::find(std::begin(kEventLobbies), std::end(kEventLobbies), room);
            if (event == std::end(kEventLobbies)) {
                state_.DeclineMove();
                move_declined_ = true;
                Refresh();
                return;
            }
            const auto index = static_cast<unsigned>(event - std::begin(kEventLobbies));
            if (!state_.Contest(room)) {
                auto contest = FixtureContest(index, GetTickCount64() / 1000);
                contest.title = Text(kEvents[index]);
                state_.SetContest(room, std::move(contest));
            }
        }
        if (!state_.ConfirmMove(room)) return;
        RoomChanged();
    }

    void OfflineLobbyView::RoomChanged() {
        const auto &room = state_.room();
        ClearRules();
        history_.erase(std::remove(history_.begin(), history_.end(), room), history_.end());
        if (!state_.ranked() && !state_.contest() && room != kBroadcastLobby) history_.insert(history_.begin(), room);
        if (history_.size() > 8) history_.pop_back();
        outer_ = 0;
        tier_ = RankedTier::General;
        SetWindowTextW(buttons_.at(kOuter), Text(state_.ranked() ? Label::lobby_ranked : Label::lobby_match));
        int x = 30;
        for (unsigned i = 0; i < std::size(kTabs); ++i) {
            if (!state_.ranked() && i == 1) continue;
            SetWindowPos(buttons_.at(kOuter + i), nullptr, x, 98, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            x += kTabWidths[i];
        }
        log_(std::string(submit_ ? "SERVICE LOBBY confirmed " : "OFFLINE LOBBY confirmed ") + NetworkUtf8(room));
        action_(Action::RoomConfirmed);
        if (visible_) Tick();
    }

    void OfflineLobbyView::Tick() {
        if (submit_) seconds_ = service_clock_at_ ? service_clock_ + (GetTickCount64() - service_clock_at_) / 1000 : 0;
        else {
            seconds_ = GetTickCount64() / 1000;
            state_.SetCounts(state_.room(), FixtureCounts(state_.room(), seconds_));
        }
        Refresh();
    }

    void OfflineLobbyView::Hover(int slot) {
        if (hovered_ == slot) return;
        hovered_ = slot;
        KillTimer(panel_, kRulesTimer);
        if (slot >= 0 && rule_slot_ >= 0) {
            rule_slot_ = slot;
            InvalidateRect(panel_, nullptr, FALSE);
        } else if (slot >= 0 || rule_slot_ >= 0) {
            rules_at_ = GetTickCount64() + kRulesDelay;
            SetTimer(panel_, kRulesTimer, kRulesDelay, nullptr);
        }
    }

    void OfflineLobbyView::ClearRules() {
        KillTimer(panel_, kRulesTimer);
        hovered_ = rule_slot_ = -1;
    }

    void OfflineLobbyView::Command(unsigned id) {
        const auto found = buttons_.find(id);
        if (found == buttons_.end() || !IsWindowEnabled(found->second) || !IsWindowVisible(found->second)) return;
        if (id >= kOuter && id < kOuter + std::size(kTabs)) {
            ClearRules();
            outer_ = id - kOuter;
        } else if (id >= kTier && id < kTier + std::size(kTiers)) {
            ClearRules();
            tier_ = static_cast<RankedTier>(id - kTier);
            SetMonthlyProfile(RankedMonthlyProfile(monthly_, tier_));
        } else if (id >= kQueue && id < kQueue + kRankedSlots) {
            if (outer_ == 0) {
                const auto type = state_.Candidate(tier_, id - kQueue).type;
                if (submit_) submit_("join", std::to_wstring(type));
                else if (state_.Reserve(tier_, id - kQueue))
                    log_("OFFLINE LOBBY reserved type=" + std::to_string(type));
            }
        } else {
            const auto now = GetTickCount64() / 1000;
            switch (id) {
                case kMenu: ShowMenu(); break;
                case kPractice: ShowPractice(); break;
                case kCancel:
                    if (submit_) submit_("cancel", {});
                    else state_.Cancel();
                    break;
                case kContestRegister:
                    if (submit_) submit_("register", {});
                    else if (state_.RegisterContest(now)) log_("OFFLINE LOBBY fixture registration");
                    break;
                case kContestReserve:
                    if (submit_) {
                        if (const auto *contest = state_.Contest()) submit_("join", std::to_wstring(contest->rule));
                    } else if (state_.ReserveContest(now)) log_("OFFLINE LOBBY fixture contest reservation");
                    break;
                case kLifetime: ChooseProfile(false); break;
                case kMonthly: ChooseProfile(true); break;
            }
        }
        Tick();
    }

    void OfflineLobbyView::ShowMenu() {
        constexpr UINT ranked = 101, event = 102, broadcast = 105, number = 106, replay = 107,
            minimize = 108, settings = 109, logout = 110;
        const auto menu = CreatePopupMenu(), rooms = CreatePopupMenu(), display = CreatePopupMenu();
        MenuItem(rooms, ranked, Label::lobby_title, state_.ranked() ? MF_CHECKED : 0);
        for (unsigned i = 0; i < std::size(kEvents); ++i)
            MenuItem(rooms, event + i, kEvents[i], state_.room() == kEventLobbies[i] ? MF_CHECKED : 0);
        MenuItem(rooms, broadcast, Label::lobby_broadcast, state_.room() == kBroadcastLobby ? MF_CHECKED : 0);
        if (!history_.empty()) AppendMenuW(rooms, MF_SEPARATOR, 0, nullptr);
        // Keep menu commands bound to the list the popup actually displayed.
        const auto history = history_;
        for (unsigned i = 0; i < history.size(); ++i)
            AppendMenuW(rooms, MF_STRING | (state_.room() == history[i] ? MF_CHECKED : 0),
                        kHistory + i, history[i].c_str());
        AppendMenuW(rooms, MF_SEPARATOR, 0, nullptr);
        MenuItem(rooms, 0, Label::lobby_edit_history, MF_GRAYED);
        MenuItem(rooms, number, Label::lobby_enter_number);
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(rooms), Text(Label::lobby_move));
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        MenuItem(menu, 0, Label::lobby_analysis_script, MF_GRAYED);
        MenuItem(menu, 0, Label::lobby_open_url, MF_GRAYED);
        MenuItem(menu, replay, Label::lobby_open_replay);
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        MenuItem(display, 0, Label::lobby_fullscreen, MF_GRAYED);
        MenuItem(display, minimize, Label::lobby_minimize);
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(display), Text(Label::lobby_display));
        MenuItem(menu, settings, Label::lobby_settings);
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        MenuItem(menu, 0, Label::lobby_payment, MF_GRAYED);
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        MenuItem(menu, logout, Label::lobby_logout);
        const UINT pick = Popup(menu, buttons_.at(kMenu), panel_);
        if (pick == ranked) RequestMove(kRankedLobby);
        else if (pick >= event && pick < event + kEventLobbies.size()) RequestMove(kEventLobbies[pick - event]);
        else if (pick == broadcast) RequestMove(kBroadcastLobby);
        else if (pick >= kHistory && pick - kHistory < history.size()) RequestMove(history[pick - kHistory]);
        else if (pick == number) {
            const auto room = RoomNumberDialog::Show(parent_);
            if (!room.empty()) RequestMove(room);
        } else if (pick == replay) action_(Action::OpenReplay);
        else if (pick == minimize) ShowWindow(GetAncestor(parent_, GA_ROOT), SW_MINIMIZE);
        else if (pick == settings) action_(Action::Settings);
        else if (pick == logout) action_(Action::Logout);
    }

    void OfflineLobbyView::ShowPractice() {
        const auto menu = CreatePopupMenu();
        for (auto label: {Label::lobby_practice_riichi_fast, Label::lobby_practice_riichi, Label::lobby_practice_riichi_three})
            MenuItem(menu, 0, label, MF_GRAYED);
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        const UINT parlor_state = official_ && submit_ && state_.ranked() ? MF_ENABLED : MF_GRAYED;
        MenuItem(menu, 1024, Label::lobby_practice_parlor_0, parlor_state);
        MenuItem(menu, 0, Label::lobby_practice_parlor_2, MF_GRAYED);
        MenuItem(menu, 1552, Label::lobby_practice_parlor_three, parlor_state);
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        MenuItem(menu, 1, Label::lobby_practice_toutenkou, official_ ? MF_ENABLED : MF_GRAYED);
        const UINT pick = Popup(menu, buttons_.at(kPractice), panel_);
        if (pick) log_("SERVICE LOBBY practice pick=" + std::to_string(pick) +
                       " ready=" + std::to_string(service_ready_));
        if ((pick == 1024 || pick == 1552) && official_ && submit_)
            submit_("join", std::to_wstring(pick));
    }

    void OfflineLobbyView::ChooseProfile(bool monthly) {
        constexpr UINT help = 3000;
        const auto menu = CreatePopupMenu();
        const auto append = [&](HMENU target, unsigned type, const std::wstring &label) {
            const bool selected = type == (monthly ? monthly_ : lifetime_);
            AppendMenuW(target, MF_STRING | (selected ? MF_CHECKED : 0), type, label.c_str());
        };
        const auto separator = [](HMENU target) { AppendMenuW(target, MF_SEPARATOR, 0, nullptr); };
        const auto submenu = [&](HMENU child, const std::wstring &label, bool selected) {
            AppendMenuW(menu, MF_POPUP | (selected ? MF_CHECKED : 0),
                        reinterpret_cast<UINT_PTR>(child), label.c_str());
        };
        const auto amber = CreatePopupMenu();
        if (monthly) {
            for (unsigned tier = 0; tier < std::size(kTiers); ++tier) {
                const auto ranked = CreatePopupMenu();
                const auto ranked_tier = static_cast<RankedTier>(tier);
                for (unsigned i = 0; i < kRankedMonthlyProfiles.size(); ++i) {
                    if (i == 3 || i == 6) separator(ranked);
                    const auto type = RankedMonthlyProfile(kRankedMonthlyProfiles[i], ranked_tier);
                    append(ranked, type, RankedMonthlyLabel(type));
                }
                submenu(ranked, std::wstring(Text(Label::lobby_ranked)) + L" " + Text(kTiers[tier]),
                        RankedMonthlyProfile(monthly_, ranked_tier) == monthly_);
            }
            separator(menu);
            for (unsigned i = 0; i < kAmberMonthlyProfiles.size(); ++i) {
                if (i == 3 || i == 6) separator(amber);
                append(amber, kAmberMonthlyProfiles[i], AmberProfileLabel(i));
            }
            submenu(amber, Text(Label::lobby_amber), (monthly_ & 0x600) != 0);
            separator(menu);
            MenuItem(menu, help, Label::lobby_public_rankings);
        } else {
            append(menu, kRankedFourProfile, LifetimeProfileLabel(kRankedFourProfile));
            append(menu, kRankedThreeProfile, LifetimeProfileLabel(kRankedThreeProfile));
            separator(menu);
            append(amber, kAmberRatingProfile, LifetimeProfileLabel(kAmberRatingProfile));
            append(amber, kAmberGProfile, LifetimeProfileLabel(kAmberGProfile));
            for (unsigned i = 0; i < kAmberLifetimeProfiles.size(); ++i) {
                if (i == 0 || i == 3 || i == 6) separator(amber);
                append(amber, kAmberLifetimeProfiles[i], AmberProfileLabel(i));
            }
            submenu(amber, Text(Label::lobby_amber), lifetime_ >= 600);
            separator(menu);
            append(menu, kPrivateFourProfile, LifetimeProfileLabel(kPrivateFourProfile));
            append(menu, kPrivateThreeProfile, LifetimeProfileLabel(kPrivateThreeProfile));
            separator(menu);
            MenuItem(menu, help, Label::lobby_private_reset);
        }

        const auto button = buttons_.at(monthly ? kMonthly : kLifetime);
        const auto choice = Popup(menu, button, panel_);
        if (choice == help) {
            if (monthly) {
                ShellExecuteW(panel_, L"open", L"https://tenhou.net/ranking.html", nullptr, nullptr, SW_SHOWNORMAL);
            } else if (MessageBoxW(panel_, Text(Label::lobby_private_reset_prompt), Text(Label::lobby_private_reset),
                                   MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
                for (unsigned i = 0; i < private_lifetime_.size(); ++i) {
                    if (WritePrivateProfileStringW(L"STATUS", kPrivateProfileKeys[i], nullptr, config_.c_str()))
                        private_lifetime_[i] = {};
                }
            }
        } else if (monthly && ValidMonthlyProfile(choice)) {
            SetMonthlyProfile(choice);
        } else if (!monthly && choice) {
            lifetime_ = choice;
            SetWindowTextW(button, ProfileCaption(Label::lobby_lifetime, LifetimeProfileLabel(choice)).c_str());
            WritePrivateProfileStringW(L"REBUILD", L"ProfileLifetime", std::to_wstring(choice).c_str(), config_.c_str());
        }
    }

    void OfflineLobbyView::LoadProfilePreferences() {
        const auto saved_lifetime = GetPrivateProfileIntW(L"REBUILD", L"ProfileLifetime", lifetime_, config_.c_str());
        if (!LifetimeProfileLabel(saved_lifetime).empty()) lifetime_ = saved_lifetime;
        const auto saved_monthly = GetPrivateProfileIntW(L"REBUILD", L"ProfileMonthly", monthly_, config_.c_str());
        if (ValidMonthlyProfile(saved_monthly)) monthly_ = saved_monthly;
        SetWindowTextW(buttons_.at(kLifetime), ProfileCaption(Label::lobby_lifetime, LifetimeProfileLabel(lifetime_)).c_str());
        SetWindowTextW(buttons_.at(kMonthly), MonthlyProfileCaption(monthly_).c_str());
        for (unsigned i = 0; i < private_lifetime_.size(); ++i)
            private_lifetime_[i] = ReadPrivateLobbyStatistics(NetworkUtf8(PreferenceString(config_, L"STATUS", kPrivateProfileKeys[i])));
    }

    void OfflineLobbyView::SetMonthlyProfile(unsigned type) {
        if (monthly_ != type) {
            monthly_ = type;
            SetWindowTextW(buttons_.at(kMonthly), MonthlyProfileCaption(type).c_str());
            WritePrivateProfileStringW(L"REBUILD", L"ProfileMonthly", std::to_wstring(type).c_str(), config_.c_str());
        }
        if (official_ && submit_) submit_("monthly-profile", std::to_wstring(type));
    }

    void OfflineLobbyView::PaintViewport(HDC dc) {
        RECT client{};
        GetClientRect(viewport_, &client);
        FillRect(dc, &client, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        ImageAt(dc, background_, (client.right - background_.width) / 2, (client.bottom - background_.height) / 2);
    }

    void OfflineLobbyView::Capture(HDC dc) {
        if (!IsWindowVisible(viewport_)) return;
        PaintViewport(dc);
        RECT bounds{};
        GetWindowRect(panel_, &bounds);
        MapWindowPoints(nullptr, viewport_, reinterpret_cast<POINT *>(&bounds), 2);
        const int saved = SaveDC(dc);
        OffsetViewportOrgEx(dc, bounds.left, bounds.top, nullptr);
        PaintPanel(dc);
        for (const auto &[id, button] : buttons_) {
            if (!IsWindowVisible(button)) continue;
            RECT rect{};
            GetWindowRect(button, &rect);
            MapWindowPoints(nullptr, panel_, reinterpret_cast<POINT *>(&rect), 2);
            const UINT state = (IsWindowEnabled(button) ? 0 : ODS_DISABLED) | (button == GetFocus() ? ODS_FOCUS : 0);
            PaintButton({ODT_BUTTON, id, 0, ODA_DRAWENTIRE, state, button, dc, rect, 0});
        }
        RestoreDC(dc, saved);
    }

    void OfflineLobbyView::PaintPanel(HDC dc) {
        const RECT panel{0, 0, kPanelWidth, kPanelHeight};
        FillRect(dc, &panel, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
        ImageAt(dc, artwork_.Get(592), 0, 0);
        SetBkMode(dc, TRANSPARENT);
        SelectObject(dc, title_font_);
        std::wstring title = Text(Label::lobby_title);
        if (state_.contest()) title = state_.Contest() ? state_.Contest()->title : Text(Label::lobby_contest);
        else if (state_.room() == kBroadcastLobby) title = Text(Label::lobby_broadcast);
        else if (!state_.ranked()) title = Text(Label::lobby_private_room);
        title += L"  " + state_.room();
        Draw(dc, title, {30, 44, 320, 64}, DT_CENTER);
        Line(dc, {50, 67, 300, 68});
        Line(dc, {320, 30, 321, 400});
        Line(dc, {30, 119, 320, 120});
        Line(dc, {30, 349, 320, 350});
        SelectObject(dc, font_);
        std::wstring status = official_ ? L"天凤官方" : Text(submit_ ? Label::lobby_service : Label::lobby_offline);
        if (state_.pending()) status = std::wstring(Text(Label::lobby_switching)) + L"  " + *state_.pending();
        else if (!service_pending_.empty()) status = Text(Label::lobby_waiting_receipt);
        else if (!service_error_.empty()) {
            const std::map<std::string, Label> errors{
                {"LOBBY_NOT_FOUND", Label::lobby_service_not_found}, {"STALE_LOBBY", Label::lobby_service_stale},
                {"ACCOUNT_REQUIRED", Label::lobby_service_account}, {"INELIGIBLE", Label::lobby_contest_blocked},
                {"REGISTER_FIRST", Label::lobby_contest_register}, {"BEFORE_START", Label::lobby_contest_before},
                {"CLOSED", Label::lobby_contest_closed}, {"DRAINING", Label::lobby_service_draining}
            };
            const auto reason = errors.find(service_error_);
            status = reason == errors.end() ? NetworkWide(service_error_) : Text(reason->second);
        }
        else if (move_declined_) status = Text(Label::lobby_unknown_contest);
        else if (!state_.contest()) {
            const auto &n = state_.counts().totals;
            status += L"  " + std::wstring(Text(Label::lobby_connected)) + L" " + std::to_wstring(n[0]);
            status += L"  " + std::wstring(Text(Label::lobby_waiting)) + L" " + std::to_wstring(n[2]);
            if (!submit_) status += L"  " + std::wstring(Text(Label::lobby_final_round)) + L" " + std::to_wstring(n[3]);
        }
        Draw(dc, status, {30, 72, 320, 94}, DT_CENTER, kAccent);
        if (outer_ <= 1 || outer_ == 5) {
            if (state_.contest()) PaintContest(dc);
            else PaintCandidates(dc);
        } else {
            RECT detail{50, 146, 300, 306};
            const auto text = Text(outer_ == 4 ? Label::lobby_offline_help :
                submit_ ? Label::lobby_service_entry : Label::lobby_entry_only);
            SetTextColor(dc, kMuted);
            DrawTextW(dc, text, -1, &detail, DT_WORDBREAK | DT_NOPREFIX);
        }
        if (state_.reservations()) {
            const auto count = Text(Label::lobby_reserved_count) + (L" " + std::to_wstring(state_.reservations()));
            Draw(dc, count, {254, 365, 319, 385}, DT_RIGHT, kAccent);
        }
        if (rule_slot_ < 0) PaintProfile(dc);
        else PaintRules(dc);
    }

    void OfflineLobbyView::PaintCandidates(HDC dc) {
        if (outer_ == 0) {
            const bool guest = state_.ranked() && !state_.profile.ranked_allowed;
            const auto requirement = guest ? Label::lobby_service_account :
                state_.ranked() ? kRequirements[static_cast<unsigned>(tier_)] : Label::lobby_all_players;
            Draw(dc, Text(requirement), {state_.ranked() && !guest ? 187L : 50L, 124, 311, 145}, DT_RIGHT, kMuted);
            Line(dc, {40, 145, 310, 146});
        }
        if (outer_ != 5) {
            Draw(dc, Text(Label::lobby_east), {125, 149, 175, 168}, DT_CENTER);
            Draw(dc, Text(Label::lobby_south), {215, 149, 265, 168}, DT_CENTER);
        }
        const unsigned slots = outer_ == 0 ? kRankedSlots : outer_ == 1 ? kParlorSlots : 1u;
        for (unsigned i = 0; i < slots; ++i) {
            const auto bounds = QueueBounds(i);
            const auto candidate = state_.Candidate(tier_, i);
            const auto label = outer_ == 0 ? kRows[i / 2] :
                               outer_ == 1 ? kParlorRows[i / 2] : Label::lobby_three_six_red;
            if (!(i % 2)) Draw(dc, Text(label), {49, bounds.top, 126, bounds.bottom});
            const auto index = outer_ == 0 ? candidate.count_slot : outer_ == 1 ? 96 + i : 112;
            const auto &counts = state_.counts();
            const auto number = std::to_wstring(counts.waiting[index]) + L":" + std::to_wstring(counts.playing[index]);
            Draw(dc, number, {bounds.right + 4, bounds.top, bounds.right + 47, bounds.bottom}, DT_LEFT,
                 outer_ == 0 && candidate.visible ? kInk : kMuted);
        }
    }

    void OfflineLobbyView::PaintContest(HDC dc) {
        const auto *contest = state_.Contest();
        if (!contest) {
            Draw(dc, Text(Label::lobby_contest_loading), {50, 158, 308, 180}, DT_CENTER, kMuted);
            return;
        }
        const auto players = contest->players == 3 ? Label::lobby_three_players : Label::lobby_four_players;
        const auto format = (contest->rule & 8) ? Label::lobby_south : Label::lobby_east;
        const auto type = std::wstring(Text(players)) + L" · " + Text(format);
        Draw(dc, type, {50, 155, 306, 176});
        if (contest->fee)
            Draw(dc, Text(Label::lobby_fee) + (L"  " + std::to_wstring(contest->fee)), {50, 183, 306, 203});
        if (contest->premium_only) Draw(dc, Text(Label::lobby_premium), {50, 209, 306, 229});
        Line(dc, {50, 239, 302, 240});
        constexpr Label status[]{Label::lobby_contest_loading, Label::lobby_contest_blocked, Label::lobby_contest_register,
            Label::lobby_contest_unregistered, Label::lobby_contest_before, Label::lobby_contest_open,
            Label::lobby_contest_closed, Label::lobby_contest_reserved};
        const auto action = state_.ContestAction(seconds_);
        const auto text = std::wstring(Text(status[static_cast<unsigned>(action)]));
        std::wstring countdown;
        if (action == ContestStatus::BeforeStart) countdown = L"  " + std::to_wstring(contest->start - seconds_) +
            L" " + Text(Label::lobby_seconds);
        Draw(dc, text + countdown, {50, 250, 306, 272}, DT_LEFT, kAccent);
        if (action == ContestStatus::Open) {
            const auto deadline = std::wstring(Text(Label::lobby_contest_window)) + L"  " +
                std::to_wstring(contest->end - seconds_) + L" " + Text(Label::lobby_seconds);
            Draw(dc, deadline, {50, 275, 306, 298}, DT_LEFT, kMuted);
        }
    }

    void OfflineLobbyView::PaintProfile(HDC dc) {
        const auto &profile = state_.profile;
        Draw(dc, profile.name, {335, 27, 555, 45}, DT_CENTER);
        Draw(dc, std::wstring(Text(Label::lobby_membership_expiry)) + L"  " + profile.membership_expiry,
             {335, 43, 555, 58}, DT_CENTER, kMuted);
        const bool ranked = lifetime_ == kRankedFourProfile || lifetime_ == kRankedThreeProfile;
        const bool private_room = lifetime_ == kPrivateFourProfile || lifetime_ == kPrivateThreeProfile;
        const bool three = lifetime_ == kRankedThreeProfile || lifetime_ == kPrivateThreeProfile ||
                           lifetime_ == kAmberLifetimeProfiles[6] || lifetime_ == kAmberLifetimeProfiles[7];
        const LobbyStatistics empty{};
        const auto extra = profile.other_lifetime.find(lifetime_);
        const auto &all = ranked ? profile.lifetime[three] : private_room ? private_lifetime_[three] :
                          extra != profile.other_lifetime.end() ? extra->second : empty;
        if (ranked) {
            const auto &rank = three ? profile.three : profile.four;
            const auto grade = rank.code == 0 ? L"新人" :
                rank.code < 10 ? std::to_wstring(10 - rank.code) + L"级" : std::to_wstring(rank.code - 9) + L"段";
            const auto target = official_ && !all.next_points ? std::wstring{} : L"/" + std::to_wstring(all.next_points);
            const auto description = grade + L"  " + std::to_wstring(all.points) + target +
                L"pt  R" + std::to_wstring(rank.rating);
            Draw(dc, profile.ranked_allowed ? description : L"—", {335, 81, 555, 98}, DT_CENTER);
        } else if (all.rating || all.average_g_hundredths) {
            auto description = all.rating ? L"R" + std::to_wstring(all.rating) : std::wstring{};
            if (all.average_g_hundredths) description += L"  G" + Decimal(*all.average_g_hundredths, 2);
            Draw(dc, description, {335, 81, 555, 98}, DT_CENTER);
        }
        const auto row = [&](int x, int y, int width, Label label, const std::wstring &value) {
            SIZE value_size{};
            GetTextExtentPoint32W(dc, value.c_str(), static_cast<int>(value.size()), &value_size);
            Draw(dc, Text(label), {x, y, x + width - value_size.cx - 2, y + 16});
            Draw(dc, value, {x, y, x + width, y + 16}, DT_RIGHT);
            Line(dc, {x, y + 16, x + width, y + 17});
        };
        constexpr Label places[]{Label::lobby_place_1, Label::lobby_place_2, Label::lobby_place_3, Label::lobby_place_4};
        const auto percentage = [](int value) { return value == 1000 ? L"100%" : Decimal(value, 1) + L"%"; };
        const auto statistics = [&](const LobbyStatistics &stats, bool three_players, bool monthly, int y) {
            const bool have_games = stats.games > 0;
            const bool have_game_rates = have_games && (!submit_ || official_ || private_room);
            const bool have_hand_rates = stats.hands > 0 || (!submit_ && have_games);
            for (int i = 0; i < (three_players ? 3 : 4); ++i)
                row(335, y + i * 17, 73, places[i], have_games ? percentage(stats.place_per_mille[i]) : L"—");
            if (!official_ || !monthly)
                row(335, y + 68, 73, Label::lobby_bust, have_game_rates ? percentage(stats.bust_per_mille) : L"—");
            row(409, y, 73, Label::lobby_games,
                have_games || (submit_ && !official_ && ranked && !monthly) ? std::to_wstring(stats.games) : L"—");
            row(409, y + 17, 73, Label::lobby_average_score, have_game_rates ? Decimal(stats.average_score_hundredths, 2) : L"—");
            row(409, y + 34, 73, Label::lobby_average_place, have_games ? Decimal(stats.average_place_hundredths, 2) : L"—");
            if (!official_ || !monthly) {
                row(483, y, 73, Label::lobby_wins, have_hand_rates ? percentage(stats.win_per_mille) : L"—");
                row(483, y + 17, 73, Label::lobby_deal_in, have_hand_rates ? percentage(stats.deal_in_per_mille) : L"—");
                row(483, y + 34, 73, Label::lobby_call, have_hand_rates ? percentage(stats.call_per_mille) : L"—");
                row(483, y + 51, 73, Label::lobby_reach, have_hand_rates ? percentage(stats.reach_per_mille) : L"—");
            }
        };
        if (lifetime_ == kAmberRatingProfile || lifetime_ == kAmberGProfile) {
            for (unsigned i = 0; i < kAmberLifetimeProfiles.size(); ++i) {
                const auto it = profile.other_lifetime.find(kAmberLifetimeProfiles[i]);
                std::wstring value = L"—";
                if (it != profile.other_lifetime.end()) {
                    const auto &stats = it->second;
                    if (lifetime_ == kAmberRatingProfile && stats.rating) value = L"R" + std::to_wstring(stats.rating);
                    if (lifetime_ == kAmberGProfile && stats.average_g_hundredths)
                        value = L"G" + Decimal(*stats.average_g_hundredths, 2);
                }
                const int y = 82 + static_cast<int>(i) * 14;
                Draw(dc, AmberProfileLabel(i), {335, y, 480, y + 14});
                Draw(dc, value, {480, y, 555, y + 14}, DT_RIGHT);
            }
        } else {
            statistics(all, three, false, 100);
        }
        const auto month = profile.monthly.find(monthly_);
        if (official_ || month != profile.monthly.end()) {
            const auto &stats = month == profile.monthly.end() ? empty : month->second;
            if (stats.rating) Draw(dc, L"R" + std::to_wstring(stats.rating), {335, 220, 555, 239}, DT_CENTER);
            statistics(stats, (monthly_ & 0x10) != 0, true, 242);
        } else {
            Draw(dc, Text(Label::lobby_no_stats), {335, 221, 555, 241}, DT_CENTER, kMuted);
        }
    }

    void OfflineLobbyView::PaintRules(HDC dc) {
        const unsigned index = static_cast<unsigned>(rule_slot_);
        const auto candidate = state_.Candidate(tier_, index);
        const auto type = candidate.type;
        Draw(dc, Text(Label::lobby_rules), {335, 32, 555, 58}, DT_CENTER, kAccent);
        Line(dc, {335, 59, 555, 60});
        Draw(dc, Text(kRows[index / 2]), {335, 68, 555, 91}, DT_CENTER);
        const Label labels[]{(type & 16) ? Label::lobby_three_players : Label::lobby_four_players,
                             (type & 8) ? Label::lobby_south : Label::lobby_east,
                             (type & 4) ? Label::lobby_tanyao_no : Label::lobby_tanyao_yes,
                             (type & 2) ? Label::lobby_aka_no : Label::lobby_aka_yes};
        int y = 104;
        for (const auto label: labels) {
            Draw(dc, Text(label), {345, y, 545, y + 21});
            y += 23;
        }
        if (type & 0x40) Draw(dc, Text((type & 0x1000) ? Label::lobby_light_mode : Label::lobby_fast_mode),
                             {345, y, 545, y + 21});
        if (state_.ranked()) Draw(dc, Text(kRequirements[static_cast<unsigned>(tier_)]), {345, 245, 545, 266});
    }

    void OfflineLobbyView::PaintButton(const DRAWITEMSTRUCT &item) {
        auto rect = item.rcItem;
        const unsigned id = item.CtlID;
        const bool tab = id >= kOuter && id < kOuter + std::size(kTabs);
        const bool tier = id >= kTier && id < kTier + std::size(kTiers);
        const bool selected = (tab && id == kOuter + outer_) || (tier && id == kTier + static_cast<unsigned>(tier_));
        const bool disabled = (item.itemState & ODS_DISABLED) != 0;
        const bool pressed = (item.itemState & ODS_SELECTED) != 0;
        FillRect(item.hDC, &rect, GetSysColorBrush(disabled ? COLOR_3DFACE :
                                                 selected ? COLOR_WINDOW : COLOR_BTNFACE));
        FrameRect(item.hDC, &rect, GetSysColorBrush(selected || pressed ? COLOR_HIGHLIGHT : COLOR_3DSHADOW));
        const bool dropdown = id == kMenu || id == kPractice || id == kLifetime || id == kMonthly;
        auto text_bounds = rect;
        InflateRect(&text_bounds, -3, -1);
        SelectObject(item.hDC, font_);
        SetBkMode(item.hDC, TRANSPARENT);
        if (dropdown) {
            Draw(item.hDC, L"▾", {rect.right - 15, rect.top, rect.right - 3, rect.bottom}, DT_CENTER);
            text_bounds.right -= 12;
        }
        wchar_t text[96]{};
        GetWindowTextW(item.hwndItem, text, static_cast<int>(std::size(text)));
        Draw(item.hDC, text, text_bounds, id == kLifetime || id == kMonthly ? DT_LEFT : DT_CENTER,
             disabled ? kMuted : selected ? kAccent : kInk);
        if (item.itemState & ODS_FOCUS) {
            InflateRect(&rect, -2, -2);
            DrawFocusRect(item.hDC, &rect);
        }
    }

    LRESULT CALLBACK OfflineLobbyView::ButtonProcedure(HWND window, UINT msg, WPARAM wparam, LPARAM lparam,
                                                        UINT_PTR subclass, DWORD_PTR context) {
        auto &self = *reinterpret_cast<OfflineLobbyView *>(context);
        const auto id = static_cast<unsigned>(GetDlgCtrlID(window));
        const int slot = static_cast<int>(id) - static_cast<int>(kQueue);
        if (slot >= 0 && slot < static_cast<int>(kRankedSlots)) {
            if (msg == WM_MOUSEMOVE && self.outer_ == 0 && self.state_.ranked() &&
                !self.state_.pending() && IsWindowEnabled(window)) {
                TRACKMOUSEEVENT event{sizeof(event), TME_LEAVE, window, 0};
                TrackMouseEvent(&event);
                self.Hover(slot);
            } else if ((msg == WM_MOUSELEAVE || msg == WM_CAPTURECHANGED) && self.hovered_ == slot)
                self.Hover(-1);
        }
        if (msg == WM_NCDESTROY) RemoveWindowSubclass(window, ButtonProcedure, subclass);
        return DefSubclassProc(window, msg, wparam, lparam);
    }

    LRESULT CALLBACK OfflineLobbyView::Procedure(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
        if (msg == WM_NCCREATE) {
            auto *create = reinterpret_cast<CREATESTRUCTW *>(lparam);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        }
        auto *self = reinterpret_cast<OfflineLobbyView *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (!self) return DefWindowProcW(window, msg, wparam, lparam);
        try {
            const bool panel = window == self->panel_;
            switch (msg) {
                case WM_ERASEBKGND: return 1;
                case WM_PAINT:
                    if (panel) self->panel_buffer_.Paint(window, [&](HDC dc) { self->PaintPanel(dc); });
                    else self->viewport_buffer_.Paint(window, [&](HDC dc) { self->PaintViewport(dc); });
                    return 0;
                case WM_PRINTCLIENT:
                    if (panel) self->PaintPanel(reinterpret_cast<HDC>(wparam));
                    else self->PaintViewport(reinterpret_cast<HDC>(wparam));
                    return 0;
                case WM_DRAWITEM:
                    self->PaintButton(*reinterpret_cast<const DRAWITEMSTRUCT *>(lparam));
                    return TRUE;
                case WM_COMMAND:
                    if (panel && HIWORD(wparam) == BN_CLICKED) self->Command(LOWORD(wparam));
                    return 0;
                case WM_TIMER:
                    if (!self->visible_) return 0;
                    if (wparam == kClockTimer) self->Tick();
                    if (wparam == kMoveTimer) self->CompleteMove();
                    if (wparam == kRulesTimer && GetTickCount64() >= self->rules_at_) {
                        KillTimer(window, kRulesTimer);
                        self->rule_slot_ = self->hovered_;
                        self->Refresh();
                    }
                    return 0;
            }
        } catch (const std::exception &e) {
            self->log_(std::string("OFFLINE LOBBY ERROR ") + e.what());
        }
        return DefWindowProcW(window, msg, wparam, lparam);
    }
}
