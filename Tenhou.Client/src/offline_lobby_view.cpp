#include "offline_lobby_view.hpp"
#include "original_button.hpp"
#include "background.hpp"
#include "network_protocol.hpp"
#include "window_layout.hpp"
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <stdexcept>

namespace tenhou {
    namespace {
        using Label = TextLabelKey;
        constexpr wchar_t kViewport[] = L"TENHOU_REBUILD_OFFLINE_VIEW";
        constexpr wchar_t kPanel[] = L"TENHOU_REBUILD_OFFLINE_LOBBY";
        constexpr wchar_t kCertificate[] = L"TENHOU_REBUILD_CERTIFICATION";
        constexpr UINT_PTR kCertificateTimer = 0x69;
        constexpr int kPanelWidth = 590, kPanelHeight = 430;
        constexpr unsigned kOuter = 0xFA4, kTier = 0xC00, kQueue = 0xFAF;
        constexpr unsigned kContestReserve = 0xFBB, kContestRegister = 0xFBC;
        constexpr unsigned kPractice = 0xFBD, kMenu = 0xFBE, kCancel = IDCANCEL;
        constexpr unsigned kLifetime = 0xFC0, kMonthly = 0xFC1, kProfileExport = 0xE104, kHistory = 200;
        constexpr UINT_PTR kClockTimer = 0x65, kRulesTimer = 0x66, kMoveTimer = 0x68;
        constexpr COLORREF kInk = RGB(32, 32, 32), kMuted = RGB(95, 95, 95), kAccent = RGB(28, 77, 124);
        constexpr COLORREF kProfileInk = RGB(0, 0, 0), kProfileDisabled = RGB(170, 170, 170),
            kProfileSubheader = RGB(136, 136, 136), kProfileLine = RGB(204, 204, 204);
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
        constexpr const wchar_t *kPrivateProfileKeys[]{L"PFPV4", L"PFPV3"};

        const wchar_t *Text(Label label) { return kTextLabels[static_cast<std::size_t>(label)].label.data(); }

        void Draw(HDC dc, std::wstring_view text, RECT rect, UINT flags = DT_LEFT, COLORREF color = kInk) {
            SetTextColor(dc, color);
            DrawTextW(dc, text.data(), static_cast<int>(text.size()), &rect,
                      DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS | flags);
        }

        void Line(HDC dc, RECT rect) { FillRect(dc, &rect, GetSysColorBrush(COLOR_3DLIGHT)); }

        void ProfileLine(HDC dc, int x1, int y1, int x2, int y2, COLORREF color = kProfileLine) {
            const HPEN pen = CreatePen(PS_SOLID, 1, color);
            const auto old = SelectObject(dc, pen);
            MoveToEx(dc, x1, y1, nullptr);
            LineTo(dc, x2, y2);
            SelectObject(dc, old);
            DeleteObject(pen);
        }

        bool FindPngEncoder(CLSID &clsid) {
            UINT count = 0, size = 0;
            if (Gdiplus::GetImageEncodersSize(&count, &size) != Gdiplus::Ok || !size) return false;
            std::vector<BYTE> storage(size);
            auto *encoders = reinterpret_cast<Gdiplus::ImageCodecInfo *>(storage.data());
            if (Gdiplus::GetImageEncoders(count, size, encoders) != Gdiplus::Ok) return false;
            for (UINT i = 0; i < count; ++i) {
                if (encoders[i].MimeType && std::wcscmp(encoders[i].MimeType, L"image/png") == 0) {
                    clsid = encoders[i].Clsid;
                    return true;
                }
            }
            return false;
        }

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
                const std::wstring title = Text(Label::lobby_enter_number), font = kOriginalUiFace;
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
            for (const auto *name: {kViewport, kPanel, kCertificate}) {
                WNDCLASSW wc{};
                wc.hInstance = instance;
                wc.lpfnWndProc = Procedure;
                wc.lpszClassName = name;
                wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
                if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
                    throw std::runtime_error("Cannot register offline lobby window");
            }
            font_ = CreateOriginalUiFont();
            viewport_ = CreateWindowExW(WS_EX_CONTROLPARENT, kViewport, L"", WS_CHILD | WS_CLIPCHILDREN,
                                        0, 0, 0, 0, parent_, nullptr, instance, this);
            if (!viewport_ || !font_) throw std::runtime_error("Cannot create offline lobby viewport");
            panel_ = CreateWindowExW(WS_EX_CONTROLPARENT, kPanel, Text(submit_ ? Label::lobby_service : Label::lobby_offline),
                WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE, 0, 0, kPanelWidth, kPanelHeight,
                viewport_, nullptr, instance, this);
            if (!panel_) throw std::runtime_error("Cannot create offline lobby panel");
            certificate_ = CreateWindowExW(WS_EX_CONTROLPARENT, kCertificate, L"认定",
                WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 0, 0, viewport_,
                reinterpret_cast<HMENU>(0xbc0), instance, this);
            if (!certificate_) throw std::runtime_error("Cannot create certification window");
            certificate_ok_ = CreateWindowExW(0, L"BUTTON", L"确定", WS_CHILD | WS_TABSTOP | BS_OWNERDRAW,
                0, 0, 0, 0, certificate_, reinterpret_cast<HMENU>(IDOK), instance, nullptr);
            if (!certificate_ok_) throw std::runtime_error("Cannot create certification button");
            SendMessageW(certificate_ok_, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
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
            // Exact WC_LOBBY_PROF 0042a736..0042a79c & WC_ITEM_BTN 113020..113041: 17x10 at (320+235-17, 30).
            AddButton(kProfileExport, {538, 30, 555, 40}, Label::lobby_profile_copy_text);
            SetWindowTextW(buttons_.at(kProfileExport), L"");
            LoadProfilePreferences();
            toutenkou_ = std::filesystem::is_regular_file(config_.parent_path() / L"toutenkou.test");
            Refresh();
        } catch (...) {
            if (viewport_) DestroyWindow(viewport_);
            if (font_) DeleteObject(font_);
            throw;
        }
    }

    OfflineLobbyView::~OfflineLobbyView() {
        if (IsWindow(viewport_)) DestroyWindow(viewport_);
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

    void OfflineLobbyView::ConfigureArtwork(const std::filesystem::path &resources, int canvas_width) {
        certificate_scale_ = canvas_width == 1440 ? 1.4f : 1.f;
        certificate_width_ = static_cast<int>(580 * certificate_scale_);
        certificate_height_ = static_cast<int>(470 * certificate_scale_);
        certificate_image_.reset();
        RECT client{};
        GetClientRect(viewport_, &client);
        Resize(client.right, client.bottom);
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
        SetWindowPos(certificate_, HWND_TOP, (width - certificate_width_) / 2, (height - certificate_height_) / 2,
                     certificate_width_, certificate_height_, SWP_NOACTIVATE);
        HDC dc = GetDC(certificate_ok_);
        auto old = SelectObject(dc, font_);
        SIZE size{};
        GetTextExtentPoint32W(dc, L"确定", 2, &size);
        SelectObject(dc, old);
        ReleaseDC(certificate_ok_, dc);
        const int button_width = size.cx + 16 + static_cast<int>(10 * certificate_scale_);
        MoveWindow(certificate_ok_, (certificate_width_ - (size.cx + 16)) / 2,
                   certificate_height_ - static_cast<int>(60 * certificate_scale_), button_width,
                   size.cy + 8 + static_cast<int>(2 * certificate_scale_), TRUE);
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
            if (!certificate_body_.empty()) SetCertificationVisible(true);
            else SetFocus(buttons_.at(kOuter + outer_));
        } else {
            SetCertificationVisible(false);
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
        if (visible_ && IsWindowVisible(certificate_)) {
            if (key == VK_RETURN || key == VK_SPACE) ConfirmCertification();
            return true;
        }
        if (!visible_ || key != VK_ESCAPE || !state_.reservations() || !IsChild(panel_, GetFocus())) return false;
        Command(kCancel);
        return true;
    }

    void OfflineLobbyView::SetServiceSnapshot(const LobbySnapshot &snapshot) {
        if (certificate_receipt_ != snapshot.certification_receipt) {
            SetCertificationVisible(false);
            certificate_receipt_ = snapshot.certification_receipt;
            certificate_body_ = snapshot.certification;
            certificate_image_.reset();
            if (visible_ && !certificate_body_.empty()) SetCertificationVisible(true);
        }
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
            if (timing_.hovered() == static_cast<int>(i) && (!normal || pending || slot.reserved)) Hover(-1);
        }
        const auto contest_action = state_.ContestAction(seconds_);
        const bool contest_page = state_.contest() && outer_ == 0;
        control(kContestRegister, contest_page && contest_action == ContestStatus::Register, !pending);
        control(kContestReserve, contest_page && (contest_action == ContestStatus::Open ||
                contest_action == ContestStatus::Reserved), !pending && contest_action == ContestStatus::Open);
        control(kMenu, true, !busy);
        control(kPractice, true, !busy);
        control(kCancel, true, state_.reservations() != 0 && !pending);
        control(kLifetime, timing_.rule_slot() < 0, true);
        control(kMonthly, timing_.rule_slot() < 0, true);
        control(kProfileExport, timing_.rule_slot() < 0, true);
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
        timing_.BeginMove(GetTickCount64());
        SetTimer(panel_, kMoveTimer, OfflineLobbyTiming::kMoveDelay, nullptr);
        Refresh();
    }

    void OfflineLobbyView::CompleteMove() {
        if (!state_.pending() || !timing_.MoveReady(GetTickCount64())) return;
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
        if (timing_.hovered() == slot) return;
        KillTimer(panel_, kRulesTimer);
        switch (timing_.Hover(slot, GetTickCount64())) {
            case OfflineLobbyTiming::HoverEffect::Schedule:
                SetTimer(panel_, kRulesTimer, OfflineLobbyTiming::kRulesDelay, nullptr);
                break;
            case OfflineLobbyTiming::HoverEffect::Redraw:
                InvalidateRect(panel_, nullptr, FALSE);
                break;
            case OfflineLobbyTiming::HoverEffect::None:
                break;
        }
    }

    void OfflineLobbyView::ClearRules() {
        KillTimer(panel_, kRulesTimer);
        timing_.ClearRules();
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
                case kProfileExport: ShowProfileExportMenu(); break;
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

    void OfflineLobbyView::ShowProfileExportMenu() {
        constexpr UINT kCopyText = 0xFC2, kSaveImage = 0xFC3;
        const auto menu = CreatePopupMenu();
        MenuItem(menu, kCopyText, Label::lobby_profile_copy_text);
        MenuItem(menu, kSaveImage, Label::lobby_profile_save_image);
        const UINT pick = Popup(menu, buttons_.at(kProfileExport), panel_);
        if (pick == kCopyText) CopyProfileText();
        else if (pick == kSaveImage) SaveProfileImage();
    }

    void OfflineLobbyView::CopyProfileText() {
        SYSTEMTIME now{};
        GetLocalTime(&now);
        wchar_t timestamp[64]{};
        std::swprintf(timestamp, std::size(timestamp), L"%04u/%02u/%02u %02u:%02u:%02u",
                      now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);
        const auto text = FormatProfileClipboardText(state_.profile, private_lifetime_, lifetime_, monthly_,
                                                     official_, timestamp);
        if (!OpenClipboard(panel_)) return;
        EmptyClipboard();
        const std::size_t bytes = (text.size() + 1) * sizeof(wchar_t);
        if (const HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes)) {
            if (auto *dest = static_cast<wchar_t *>(GlobalLock(mem))) {
                std::memcpy(dest, text.c_str(), bytes);
                GlobalUnlock(mem);
                if (!SetClipboardData(CF_UNICODETEXT, mem)) GlobalFree(mem);
            } else GlobalFree(mem);
        }
        CloseClipboard();
    }

    void OfflineLobbyView::SaveProfileImage() {
        SYSTEMTIME now{};
        GetLocalTime(&now);
        wchar_t path[MAX_PATH]{};
        std::swprintf(path, std::size(path), L"tenhou_prof_%04u%02u%02u.png", now.wYear, now.wMonth, now.wDay);
        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = parent_;
        dialog.lpstrFilter = L"PNG 图像 (*.png)\0*.png\0所有文件 (*.*)\0*.*\0\0";
        dialog.lpstrFile = path;
        dialog.nMaxFile = static_cast<DWORD>(std::size(path));
        dialog.lpstrDefExt = L"png";
        dialog.lpstrTitle = Text(Label::lobby_profile_save_image);
        dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        if (!GetSaveFileNameW(&dialog)) return;

        CLSID png{};
        if (!FindPngEncoder(png)) return;
        constexpr int kProfX = 320, kProfY = 30, kProfW = 240, kProfH = 370;
        Gdiplus::Bitmap image(kProfW, kProfH, PixelFormat32bppRGB);
        {
            Gdiplus::Graphics graphics(&image);
            HDC dc = graphics.GetHDC();
            const int saved = SaveDC(dc);
            const RECT full{0, 0, kProfW, kProfH};
            FillRect(dc, &full, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
            OffsetViewportOrgEx(dc, -kProfX, -kProfY, nullptr);
            SetBkMode(dc, TRANSPARENT);
            SelectObject(dc, font_);
            PaintProfile(dc);
            for (unsigned id : {kLifetime, kMonthly}) {
                const HWND button = buttons_.at(id);
                if (!IsWindowVisible(button)) continue;
                RECT rect{};
                GetWindowRect(button, &rect);
                MapWindowPoints(nullptr, panel_, reinterpret_cast<POINT *>(&rect), 2);
                PaintButton({ODT_BUTTON, id, 0, ODA_DRAWENTIRE, 0, button, dc, rect, 0});
            }
            RestoreDC(dc, saved);
            graphics.ReleaseHDC(dc);
        }
        image.Save(path, &png, nullptr);
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
            SetWindowTextW(button, LifetimeProfileCaption(choice).c_str());
            WritePrivateProfileStringW(L"REBUILD", L"ProfileLifetime", std::to_wstring(choice).c_str(), config_.c_str());
        }
    }

    void OfflineLobbyView::LoadProfilePreferences() {
        const auto saved_lifetime = GetPrivateProfileIntW(L"REBUILD", L"ProfileLifetime", lifetime_, config_.c_str());
        if (!LifetimeProfileLabel(saved_lifetime).empty()) lifetime_ = saved_lifetime;
        const auto saved_monthly = GetPrivateProfileIntW(L"REBUILD", L"ProfileMonthly", monthly_, config_.c_str());
        if (ValidMonthlyProfile(saved_monthly)) monthly_ = saved_monthly;
        SetWindowTextW(buttons_.at(kLifetime), LifetimeProfileCaption(lifetime_).c_str());
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

    void OfflineLobbyView::SetCertificationVisible(bool show) {
        KillTimer(certificate_, kCertificateTimer);
        if (timing_.certificate_closing()) {
            certificate_body_.clear();
            certificate_image_.reset();
        }
        timing_.ResetCertificate(GetTickCount64());
        ShowWindow(certificate_ok_, SW_HIDE);
        EnableWindow(panel_, !show);
        ShowWindow(certificate_, show ? SW_SHOW : SW_HIDE);
        if (!show) return;
        SetFocus(certificate_);
        SetTimer(certificate_, kCertificateTimer, OfflineLobbyTiming::kCertificateTick, nullptr);
        InvalidateRect(certificate_, nullptr, FALSE);
    }

    void OfflineLobbyView::ConfirmCertification() {
        if (!IsWindowVisible(certificate_ok_) || !timing_.CloseCertificate(GetTickCount64())) return;
        ShowWindow(certificate_ok_, SW_HIDE);
        SetTimer(certificate_, kCertificateTimer, OfflineLobbyTiming::kCertificateTick, nullptr);
    }

    void OfflineLobbyView::PaintCertification(HDC dc) {
        POINT origin{};
        MapWindowPoints(certificate_, viewport_, &origin, 1);
        const int saved = SaveDC(dc);
        OffsetViewportOrgEx(dc, -origin.x, -origin.y, nullptr);
        PaintLobby(dc);
        RestoreDC(dc, saved);

        using namespace Gdiplus;
        const int w = certificate_width_, h = certificate_height_;
        if (!certificate_image_) {
            auto image = std::make_unique<Bitmap>(w, h, PixelFormat32bppPARGB);
            Graphics g(image.get());
            g.Clear(Color(0xdd000000));
            const auto edge = [&](ARGB color, int x, int y, int right, int bottom) {
                Pen pen(Color(color), 1);
                g.DrawLine(&pen, x, y, right, bottom);
            };
            edge(0xffcccccc, 0, 0, w - 1, 0);
            edge(0xffaaaaaa, 0, 0, 0, h - 1);
            edge(0xff888888, w - 1, 0, w - 1, h - 1);
            edge(0xff666666, 0, h - 1, w - 1, h - 1);

            const float scale = certificate_scale_;
            // DisplayArtwork holds the HD lobby pack even with the SD canvas.
            auto title = artwork_.TextRun(TextKey::title_certification, 580 * .5f * 1.4f, 75 * 1.4f, true);
            TextRenderContext context;
            context.native_layout_scale = scale / 1.4;
            artwork_.Text().Draw(g, context, title);
            FontFamily system(L"SYSTEM");
            Font font(system.IsAvailable() ? &system : FontFamily::GenericSansSerif(),
                      scale * (44.f / 3), FontStyleRegular, UnitPixel);
            RectF extent;
            g.MeasureString(certificate_body_.c_str(), -1, &font, PointF(), &extent);
            const float margin = 10 * scale;
            const float x = std::max(margin, (w - extent.Width) * .5f);
            const RectF block(x, 180 * scale, w - 2 * x + 1, (470 - 180 - 60) * scale);
            SolidBrush white(Color(0xffffffff));
            g.SetTextRenderingHint(TextRenderingHintAntiAlias);
            g.DrawString(certificate_body_.c_str(), -1, &font, block, nullptr, &white);
            // The original entrance/exit bitmap includes the child button.
            RECT button{};
            GetClientRect(certificate_ok_, &button);
            Bitmap button_image(button.right, button.bottom, PixelFormat32bppRGB);
            {
                Graphics button_graphics(&button_image);
                HDC button_dc = button_graphics.GetHDC();
                DRAWITEMSTRUCT item{ODT_BUTTON, IDOK, 0, ODA_DRAWENTIRE, 0, certificate_ok_, button_dc, button};
                PaintOriginalButton(item, font_, L"确定");
                button_graphics.ReleaseHDC(button_dc);
            }
            POINT pos{};
            MapWindowPoints(certificate_ok_, certificate_, &pos, 1);
            g.DrawImage(&button_image, pos.x, pos.y, button.right, button.bottom);
            certificate_image_ = std::move(image);
        }
        const auto [zoom, alpha] = timing_.CertificateFrame(GetTickCount64());
        ColorMatrix matrix{};
        for (int i = 0; i < 5; ++i) matrix.m[i][i] = 1.f;
        matrix.m[3][3] = alpha;
        ImageAttributes attributes;
        attributes.SetColorMatrix(&matrix);
        Graphics g(dc);
        RectF dest(w * (1 - zoom) / 2, h * (1 - zoom) / 2, w * zoom, h * zoom);
        g.DrawImage(certificate_image_.get(), dest, 0.f, 0.f, static_cast<float>(w), static_cast<float>(h),
                    UnitPixel, &attributes);
    }

    void OfflineLobbyView::PaintViewport(HDC dc) {
        RECT client{};
        GetClientRect(viewport_, &client);
        FillRect(dc, &client, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        ImageAt(dc, background_, (client.right - background_.width) / 2, (client.bottom - background_.height) / 2);
    }

    void OfflineLobbyView::Capture(HDC dc) {
        if (!IsWindowVisible(viewport_)) return;
        PaintLobby(dc);
        if (!IsWindowVisible(certificate_)) return;
        POINT origin{};
        MapWindowPoints(certificate_, viewport_, &origin, 1);
        const int saved = SaveDC(dc);
        OffsetViewportOrgEx(dc, origin.x, origin.y, nullptr);
        PaintCertification(dc);
        if (IsWindowVisible(certificate_ok_)) {
            POINT button{};
            MapWindowPoints(certificate_ok_, certificate_, &button, 1);
            OffsetViewportOrgEx(dc, button.x, button.y, nullptr);
            SendMessageW(certificate_ok_, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT);
        }
        RestoreDC(dc, saved);
    }

    void OfflineLobbyView::PaintLobby(HDC dc) {
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
        SelectObject(dc, font_);
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
        if (timing_.rule_slot() < 0) PaintProfile(dc);
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
        SIZE unit{};
        GetTextExtentPoint32W(dc, L"a", 1, &unit);
        const int cy = std::max<int>(12, unit.cy);

        // Exact WC_LOBBY_PROF (0042a8e3..0042aa60): vertical border at x=320, centered 2-line name + expiry.
        ProfileLine(dc, 320, 30, 320, 400);
        const std::wstring header_block = profile.name + L"\r\n" +
            std::wstring(Text(Label::lobby_membership_expiry)) + L" : " + FormatExpiry(profile.membership_expiry);
        RECT header_rect{335, 30, 555, 60};
        SetTextColor(dc, kProfileInk);
        DrawTextW(dc, header_block.c_str(), static_cast<int>(header_block.size()), &header_rect,
                  DT_CENTER | DT_NOPREFIX);

        const bool ranked = lifetime_ == kRankedFourProfile || lifetime_ == kRankedThreeProfile;
        const bool private_room = lifetime_ == kPrivateFourProfile || lifetime_ == kPrivateThreeProfile;
        const bool three = lifetime_ == kRankedThreeProfile || lifetime_ == kPrivateThreeProfile ||
                           lifetime_ == kAmberLifetimeProfiles[6] || lifetime_ == kAmberLifetimeProfiles[7];
        const auto amber_it = std::find(kAmberLifetimeProfiles.begin(), kAmberLifetimeProfiles.end(), lifetime_);
        const bool amber_chips = amber_it != kAmberLifetimeProfiles.end() &&
                                 (lifetime_ % 10 != 1); // *02, *03, *12, *13, *23, *33 have chip bit 0x200.

        const LobbyStatistics empty{};
        const auto extra = profile.other_lifetime.find(lifetime_);
        const auto &all = ranked ? profile.lifetime[three] : private_room ? private_lifetime_[three] :
                          extra != profile.other_lifetime.end() ? extra->second : empty;

        if (lifetime_ == kAmberRatingProfile || lifetime_ == kAmberGProfile) {
            // Exact 0042c395..0042c810: 8-row Amber summary table.
            int y = 84;
            for (unsigned i = 0; i < kAmberLifetimeProfiles.size(); ++i) {
                const auto it = profile.other_lifetime.find(kAmberLifetimeProfiles[i]);
                std::wstring value = L"-";
                if (it != profile.other_lifetime.end()) {
                    const auto &stats = it->second;
                    if (lifetime_ == kAmberRatingProfile && stats.rating)
                        value = L"R" + std::to_wstring(stats.rating);
                    if (lifetime_ == kAmberGProfile && stats.average_g_hundredths)
                        value = L"G" + Decimal(*stats.average_g_hundredths, 2);
                }
                RECT left_r{335, y, 485, y + cy};
                RECT right_r{485, y, 555, y + cy};
                SetTextColor(dc, kProfileInk);
                const auto label = AmberProfileLabel(i);
                DrawTextW(dc, label.c_str(), static_cast<int>(label.size()), &left_r, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
                DrawTextW(dc, value.c_str(), static_cast<int>(value.size()), &right_r, DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX);
                if (i == 2 || i == 5) ProfileLine(dc, 335, y + cy + 1, 555, y + cy + 1);
                y += 13;
            }
        } else {
            // Exact 0042b1ed..0042b3dc: rank/R/G header line + full underline from 335 to 555.
            std::wstring summary = L"-";
            if (ranked) {
                if (profile.ranked_allowed) {
                    const auto &rank = three ? profile.three : profile.four;
                    if (rank.rating > 0 || all.games > 0 || all.points != 0 || rank.code > 0) {
                        const auto target = official_ && !all.next_points ? std::wstring{} : L"/" + std::to_wstring(all.next_points);
                        summary = RankGradeName(rank.code) + L" " + std::to_wstring(all.points) + target +
                                  L"pt R" + std::to_wstring(rank.rating);
                    } else {
                        summary = L"新人";
                    }
                }
            } else if (!private_room && (all.rating || all.average_g_hundredths)) {
                summary.clear();
                if (all.rating) summary = L"R" + std::to_wstring(all.rating);
                if (all.average_g_hundredths) {
                    if (!summary.empty()) summary += L"  ";
                    summary += L"G" + Decimal(*all.average_g_hundredths, 2);
                }
            }
            const int summary_y = 85;
            RECT summary_rect{335, summary_y, 555, summary_y + cy};
            SetTextColor(dc, kProfileInk);
            DrawTextW(dc, summary.c_str(), static_cast<int>(summary.size()), &summary_rect, DT_CENTER | DT_NOPREFIX);
            ProfileLine(dc, 335, summary_y + cy, 555, summary_y + cy);

            // Exact 0042c139..0042c2be: 14 lifetime items in 3 columns with 10px gaps.
            // Col 0: x=335 w=57 (335..392), Col 1: x=402 w=81 (402..483), Col 2: x=493 w=62 (493..555).
            const bool have_games = all.games > 0;
            const bool have_game_rates = have_games && (!submit_ || official_ || private_room);
            const bool have_hand_rates = all.hands > 0 || (!submit_ && have_games);
            const int raw_places_sum = std::accumulate(all.place_counts.begin(), all.place_counts.end(), 0);
            const bool use_raw_places = have_games && raw_places_sum == all.games;
            const auto place_rate = [&](int idx) {
                return use_raw_places ? static_cast<double>(all.place_counts[idx]) / static_cast<double>(all.games)
                                      : static_cast<double>(all.place_per_mille[idx]) / 1000.0;
            };
            const auto hand_rate = [&](int raw_count, int permille) {
                return all.hands > 0 && (raw_count > 0 || all.win_count > 0 || all.deal_in_count > 0 ||
                                         all.call_count > 0 || all.reach_count > 0)
                    ? static_cast<double>(raw_count) / static_cast<double>(all.hands)
                    : static_cast<double>(permille) / 1000.0;
            };
            const double bust_rate = use_raw_places && (all.bust_count > 0 || all.bust_per_mille == 0)
                ? static_cast<double>(all.bust_count) / static_cast<double>(all.games)
                : static_cast<double>(all.bust_per_mille) / 1000.0;
            const double avg_score = use_raw_places && (all.total_score != 0.0 || all.average_score_hundredths == 0)
                ? all.total_score / static_cast<double>(all.games)
                : static_cast<double>(all.average_score_hundredths) / 100.0;
            const double avg_place = use_raw_places
                ? static_cast<double>(all.place_counts[0] + all.place_counts[1] * 2 +
                                      all.place_counts[2] * 3 + all.place_counts[3] * 4) / static_cast<double>(all.games)
                : static_cast<double>(all.average_place_hundredths) / 100.0;

            struct LifetimeRow {
                Label label;
                std::wstring value;
                bool grayed;
            };
            const std::array<LifetimeRow, 14> items{{
                {Label::lobby_place_1, have_games ? FormatRate(place_rate(0), 3) : L"-", false},
                {Label::lobby_place_2, have_games ? FormatRate(place_rate(1), 3) : L"-", false},
                {Label::lobby_place_3, have_games ? FormatRate(place_rate(2), 3) : L"-", false},
                {Label::lobby_place_4, !three && have_games ? FormatRate(place_rate(3), 3) : L"-", three},
                {Label::lobby_bust, have_game_rates ? FormatRate(bust_rate, 3) : L"-", false},
                {Label::lobby_games, have_games || (submit_ && !official_ && ranked) ? std::to_wstring(all.games) : L"-", false},
                {Label::lobby_average_score, have_game_rates ? FormatSignedAdaptive(avg_score, 2) : L"-", false},
                {Label::lobby_average_place, have_games ? FormatFixedTrunc(avg_place, 2, false, nullptr) : L"-", false},
                {Label::lobby_average_income, amber_chips && all.average_income ? FormatSignedAdaptive(*all.average_income, 2) : L"-", !amber_chips},
                {Label::lobby_average_chips, amber_chips && all.average_chips ? FormatSignedAdaptive(*all.average_chips, 2) : L"-", !amber_chips},
                {Label::lobby_wins, have_hand_rates ? FormatRate(hand_rate(all.win_count, all.win_per_mille), 3) : L"-", false},
                {Label::lobby_deal_in, have_hand_rates ? FormatRate(hand_rate(all.deal_in_count, all.deal_in_per_mille), 3) : L"-", false},
                {Label::lobby_call, have_hand_rates ? FormatRate(hand_rate(all.call_count, all.call_per_mille), 3) : L"-", false},
                {Label::lobby_reach, have_hand_rates ? FormatRate(hand_rate(all.reach_count, all.reach_per_mille), 3) : L"-", false},
            }};

            constexpr int col_x[]{335, 402, 493};
            constexpr int col_w[]{57, 81, 62};
            const int grid_top = summary_y + cy + 9;
            const int row_step = cy + 5;
            for (int i = 0; i < 14; ++i) {
                const int col = i / 5;
                const int r = i % 5;
                const int x = col_x[col];
                const int w = col_w[col];
                const int y = grid_top + r * row_step;
                ProfileLine(dc, x, y + cy, x + w, y + cy);
                SetTextColor(dc, items[i].grayed ? kProfileDisabled : kProfileInk);
                RECT cell{x, y, x + w, y + cy};
                const auto *label = Text(items[i].label);
                DrawTextW(dc, label, -1, &cell, DT_LEFT | DT_NOPREFIX);
                DrawTextW(dc, items[i].value.c_str(), static_cast<int>(items[i].value.size()), &cell,
                          DT_RIGHT | DT_NOPREFIX);
            }
        }

        // Exact WC_LOBBY_PROF monthly section (0042cd56..0042db30 + FUN_00433e00).
        const auto month_it = profile.monthly.find(monthly_);
        const auto &ms = month_it == profile.monthly.end() ? empty : month_it->second;
        const bool m_three = (monthly_ & 0x10) != 0;
        const bool m_amber_chips = (monthly_ & 0x200) != 0;
        const bool m_games = ms.games > 0;
        const auto &mp = ms.place_counts;
        const int mp_sum = std::accumulate(mp.begin(), mp.end(), 0);
        const bool m_raw = m_games && mp_sum == ms.games;

        const auto rank_text = [&](std::size_t idx, bool enabled = true) -> std::wstring {
            if (!enabled || idx >= ms.monthly_ranks.size() || ms.monthly_ranks[idx] <= 0) return L"-";
            return std::to_wstring(ms.monthly_ranks[idx]) + Text(Label::lobby_rank_unit);
        };

        const auto draw_monthly_cell = [&](int x_off, int y, int width, int mid_right,
                                           const wchar_t *label, const std::wstring &right_text,
                                           const std::wstring &mid_text, bool grayed, bool dot) {
            const int x = 335 + x_off;
            ProfileLine(dc, x, y + cy, x + width, y + cy);
            SetTextColor(dc, grayed ? kProfileDisabled : kProfileInk);
            RECT cell{x, y, x + width, y + cy};
            if (label && *label) DrawTextW(dc, label, -1, &cell, DT_LEFT | DT_NOPREFIX);
            if (!right_text.empty())
                DrawTextW(dc, right_text.c_str(), static_cast<int>(right_text.size()), &cell, DT_RIGHT | DT_NOPREFIX);
            if (!mid_text.empty() && mid_right > 0) {
                RECT mid_rect{x, y, x + mid_right, y + cy};
                DrawTextW(dc, mid_text.c_str(), static_cast<int>(mid_text.size()), &mid_rect, DT_RIGHT | DT_NOPREFIX);
            }
            if (dot) {
                // Exact 0042d970..0042d99d: 2x2 black square at (320 + x_off + 10, y + cy/2 - 2).
                const RECT square{320 + x_off + 10, y + cy / 2 - 2, 320 + x_off + 12, y + cy / 2};
                FillRect(dc, &square, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            }
        };

        // Row 0: Games breakdown (left) + Rate & Rate rank (right).
        const int row0_y = 220;
        std::wstring games_summary = L"-";
        if (m_raw) {
            const bool compact_games = mp[0] >= 100 || mp[1] >= 100 || mp[2] >= 100 || (!m_three && mp[3] >= 100);
            if (m_three) {
                games_summary = std::to_wstring(mp[0]) + L"+" + std::to_wstring(mp[1]) + L"+" + std::to_wstring(mp[2]);
            } else {
                games_summary = std::to_wstring(mp[0]) + L"+" + std::to_wstring(mp[1]) + L"+" +
                                std::to_wstring(mp[2]) + L"+" + std::to_wstring(mp[3]);
            }
            games_summary += compact_games ? (L"=" + std::to_wstring(ms.games))
                                           : (L" = " + std::to_wstring(ms.games) + Text(Label::lobby_games_unit));
        } else if (m_games) {
            games_summary = std::to_wstring(ms.games) + Text(Label::lobby_games_unit);
        }
        std::wstring rate_mid;
        const bool has_rate_rank = ms.monthly_ranks.size() >= 9 && ms.monthly_ranks[8] > 0;
        const int corr_rating = MonthlyCorrespondingRating(profile, monthly_);
        if (has_rate_rank && corr_rating > 0) rate_mid = L"R" + std::to_wstring(corr_rating);
        else if (!official_ && ms.rating > 0) rate_mid = L"R" + std::to_wstring(ms.rating);

        draw_monthly_cell(0, row0_y, 120, 0, nullptr, games_summary, {}, false, false);
        draw_monthly_cell(130, row0_y, 90, 40, nullptr, rank_text(8), rate_mid, false, false);

        // Column sub-headers: 通算 & 平均 (0042d9ac..0042daae).
        const int sub_y = row0_y + cy + 9;
        SetTextColor(dc, kProfileSubheader);
        const UINT old_align = SetTextAlign(dc, TA_RIGHT);
        const auto *total_lbl = Text(Label::lobby_monthly_total);
        const auto *avg_lbl = Text(Label::lobby_monthly_average);
        TextOutW(dc, 335 + 5 + 65, sub_y, total_lbl, static_cast<int>(std::wcslen(total_lbl)));
        TextOutW(dc, 335 + 130 + 40, sub_y, avg_lbl, static_cast<int>(std::wcslen(avg_lbl)));
        SetTextAlign(dc, old_align);
        ProfileLine(dc, 335, sub_y + cy, 335 + 5 + 115, sub_y + cy);
        ProfileLine(dc, 335 + 130, sub_y + cy, 335 + 130 + 90, sub_y + cy);

        // Rows 2..9: 得点, 顺位, 收支, 祝仪 (通算 left, 平均 right).
        const int table_y = sub_y + cy + 4;
        const int m_step = cy + 4;
        const double total_score = ms.has_monthly_raw
            ? ms.total_score
            : (static_cast<double>(ms.average_score_hundredths) * ms.games / 100.0);
        const double avg_m_score = m_games ? total_score / static_cast<double>(ms.games) : 0.0;
        const double place_uma = m_raw
            ? (m_three ? (mp[0] * 30.0 - mp[2] * 30.0) : (mp[0] * 30.0 + mp[1] * 10.0 - mp[2] * 10.0 - mp[3] * 30.0))
            : 0.0;
        const double avg_m_place = m_raw
            ? static_cast<double>(mp[0] + mp[1] * 2 + mp[2] * 3 + mp[3] * 4) / static_cast<double>(ms.games)
            : static_cast<double>(ms.average_place_hundredths) / 100.0;

        draw_monthly_cell(5, table_y, 115, 65, Text(Label::lobby_monthly_score),
                          rank_text(1), m_games ? FormatSignedAdaptive(total_score, 1) : L"-", false, !m_amber_chips);
        draw_monthly_cell(130, table_y, 90, 40, nullptr,
                          rank_text(2), m_games ? FormatSignedAdaptive(avg_m_score, 2) : L"-", false, false);

        draw_monthly_cell(5, table_y + m_step, 115, 65, Text(Label::lobby_monthly_place),
                          rank_text(3), m_raw ? FormatFixedTrunc(place_uma, 0, false, L"+") : L"-", false, !m_amber_chips);
        draw_monthly_cell(130, table_y + m_step, 90, 40, nullptr,
                          rank_text(4), m_games ? FormatFixedTrunc(avg_m_place, 2, false, nullptr) : L"-", false, false);

        const bool have_amber_data = m_amber_chips && m_games && ms.has_amber_monthly;
        draw_monthly_cell(5, table_y + m_step * 2, 115, 65, Text(Label::lobby_monthly_income),
                          rank_text(9, m_amber_chips), have_amber_data ? FormatSignedAdaptive(ms.total_income, 1) : L"-",
                          !m_amber_chips, m_amber_chips);
        draw_monthly_cell(130, table_y + m_step * 2, 90, 40, nullptr,
                          rank_text(10, m_amber_chips), have_amber_data ? FormatSignedAdaptive(ms.total_income / ms.games, 2) : L"-",
                          !m_amber_chips, false);

        draw_monthly_cell(5, table_y + m_step * 3, 115, 65, Text(Label::lobby_monthly_chips),
                          rank_text(11, m_amber_chips), have_amber_data ? FormatSignedAdaptive(ms.total_chips, 0) : L"-",
                          !m_amber_chips, m_amber_chips);
        draw_monthly_cell(130, table_y + m_step * 3, 90, 40, nullptr,
                          rank_text(12, m_amber_chips), have_amber_data ? FormatSignedAdaptive(static_cast<double>(ms.total_chips) / ms.games, 2) : L"-",
                          !m_amber_chips, false);

        // Row 10: 综合 (0042d3a0..0042d48e).
        const int overall_y = table_y + m_step * 4;
        std::wstring overall_mid = L"-";
        if (ms.monthly_ranks.size() == 9 && ms.monthly_ranks[2] > 0) {
            overall_mid = std::to_wstring(ms.monthly_ranks[1] + ms.monthly_ranks[2] +
                                          ms.monthly_ranks[3] + ms.monthly_ranks[4]);
        } else if (ms.monthly_ranks.size() == 13 && ms.monthly_ranks[10] > 0) {
            overall_mid = std::to_wstring(ms.monthly_ranks[9] + ms.monthly_ranks[10] +
                                          ms.monthly_ranks[11] + ms.monthly_ranks[12]);
        }
        draw_monthly_cell(90, overall_y, 130, 80, Text(Label::lobby_monthly_overall),
                          rank_text(0), overall_mid, false, true);

        // Rows 11..13: 一位率 / 连对率 / 末位率 (0042d5bc..0042d683).
        const int rates_y = overall_y + cy + 9;
        const double top_rate = m_raw ? static_cast<double>(mp[0]) / ms.games
                                      : static_cast<double>(ms.place_per_mille[0]) / 1000.0;
        const double top2_rate = m_raw ? static_cast<double>(mp[0] + mp[1]) / ms.games
                                       : static_cast<double>(ms.place_per_mille[0] + ms.place_per_mille[1]) / 1000.0;
        const double last_rate = m_raw ? static_cast<double>(m_three ? mp[2] : mp[3]) / ms.games
                                       : static_cast<double>(ms.place_per_mille[m_three ? 2 : 3]) / 1000.0;
        draw_monthly_cell(0, rates_y, 105, 59, Text(Label::lobby_monthly_top_rate),
                          rank_text(5), m_games ? FormatRate(top_rate, 3) : L"-", false, false);
        draw_monthly_cell(115, rates_y, 105, 59, Text(Label::lobby_monthly_top2_rate),
                          rank_text(6, !m_three), !m_three && m_games ? FormatRate(top2_rate, 3) : L"-", m_three, false);
        draw_monthly_cell(0, rates_y + m_step, 105, 59, Text(Label::lobby_monthly_last_rate),
                          rank_text(7), m_games ? FormatRate(last_rate, 3) : L"-", false, false);
    }

    void OfflineLobbyView::PaintRules(HDC dc) {
        const unsigned index = static_cast<unsigned>(timing_.rule_slot());
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
        const bool profile_btn = id == kLifetime || id == kMonthly || id == kProfileExport;
        if (profile_btn && !disabled) {
            // Exact WC_ITEM_BTN gradient (tenhou.exe.c:112696-112722): top half white, bottom half 221..255.
            const int height = rect.bottom - rect.top;
            const RECT top_half{rect.left, rect.top, rect.right, rect.top + height / 2};
            FillRect(item.hDC, &top_half, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
            const int half = height - height / 2;
            const auto brush = reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH));
            for (int row = 0; row < half; ++row) {
                const int shade = 221 + 34 * row / std::max(1, half - 1);
                SetDCBrushColor(item.hDC, RGB(shade, shade, shade));
                const RECT line{rect.left, rect.top + height / 2 + row, rect.right, rect.top + height / 2 + row + 1};
                FillRect(item.hDC, &line, brush);
            }
            SetDCBrushColor(item.hDC, pressed ? RGB(51, 153, 255) : RGB(136, 136, 136));
            FrameRect(item.hDC, &rect, brush);
        } else {
            FillRect(item.hDC, &rect, GetSysColorBrush(disabled ? COLOR_3DFACE :
                                                     selected ? COLOR_WINDOW : COLOR_BTNFACE));
            FrameRect(item.hDC, &rect, GetSysColorBrush(selected || pressed ? COLOR_HIGHLIGHT : COLOR_3DSHADOW));
        }
        const bool dropdown = id == kMenu || id == kPractice || id == kLifetime || id == kMonthly || id == kProfileExport;
        auto text_bounds = rect;
        InflateRect(&text_bounds, id == kLifetime || id == kMonthly ? -6 : -3, -1);
        SelectObject(item.hDC, font_);
        SetBkMode(item.hDC, TRANSPARENT);
        if (dropdown) {
            const int cx = id == kProfileExport ? (rect.left + rect.right) / 2 : rect.right - 9;
            const int cy = (rect.top + rect.bottom) / 2;
            POINT triangle[3]{{cx - 3, cy - 1}, {cx + 3, cy - 1}, {cx, cy + 2}};
            const auto pen = SelectObject(item.hDC, GetStockObject(BLACK_PEN));
            const auto brush = SelectObject(item.hDC, GetStockObject(BLACK_BRUSH));
            Polygon(item.hDC, triangle, 3);
            SelectObject(item.hDC, brush);
            SelectObject(item.hDC, pen);
            text_bounds.right -= 14;
        }
        if (id != kProfileExport) {
            wchar_t text[96]{};
            GetWindowTextW(item.hwndItem, text, static_cast<int>(std::size(text)));
            Draw(item.hDC, text, text_bounds, id == kLifetime || id == kMonthly ? DT_LEFT : DT_CENTER,
                 disabled ? kMuted : selected ? kAccent : profile_btn ? kProfileInk : kInk);
        }
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
            } else if ((msg == WM_MOUSELEAVE || msg == WM_CAPTURECHANGED) && self.timing_.hovered() == slot)
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
            if (window == self->certificate_) {
                switch (msg) {
                    case WM_ERASEBKGND: return 1;
                    case WM_PAINT:
                        self->certificate_buffer_.Paint(window, [&](HDC dc) { self->PaintCertification(dc); });
                        return 0;
                    case WM_DRAWITEM:
                        PaintOriginalButton(*reinterpret_cast<DRAWITEMSTRUCT *>(lparam), self->font_, L"确定");
                        return TRUE;
                    case WM_PRINTCLIENT:
                        self->PaintCertification(reinterpret_cast<HDC>(wparam));
                        return 0;
                    case WM_COMMAND:
                        if (LOWORD(wparam) == IDOK) self->ConfirmCertification();
                        return 0;
                    case WM_TIMER:
                        if (wparam != kCertificateTimer) break;
                        if (const auto phase = self->timing_.CertificateStatus(GetTickCount64());
                            phase == OfflineLobbyTiming::CertificatePhase::Finished) {
                            self->SetCertificationVisible(false);
                            SetFocus(self->buttons_.at(kOuter + self->outer_));
                        } else if (phase == OfflineLobbyTiming::CertificatePhase::Ready) {
                            KillTimer(window, kCertificateTimer);
                            ShowWindow(self->certificate_ok_, SW_SHOW);
                            SetFocus(self->certificate_ok_);
                        }
                        InvalidateRect(window, nullptr, FALSE);
                        return 0;
                }
                return DefWindowProcW(window, msg, wparam, lparam);
            }
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
                    if (wparam == kRulesTimer && self->timing_.AdvanceRules(GetTickCount64())) {
                        KillTimer(window, kRulesTimer);
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
