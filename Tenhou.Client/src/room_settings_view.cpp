#include "room_settings_view.hpp"
#include "room_settings_view_model.hpp"
#include "win32_utf.hpp"
#include <algorithm>
#include <cstring>

namespace tenhou {
    namespace {
        struct RoomDialog {
            RoomSettingsViewModel model;
            const std::map<std::string, ServerRule> &rules;
            std::map<std::string, std::string> initial_values;
            std::map<std::size_t, HWND> controls;
            std::vector<HWND> fields;
            HFONT font = nullptr;
            HWND window = nullptr;

            HWND add(const int id, const wchar_t *cls, const std::wstring &text, const DWORD style, const int x, const int y, const int width, const int height) {
                RECT r{x, y, x + width, y + height}; MapDialogRect(window, &r);
                HWND control = CreateWindowExW(0, cls, text.c_str(), WS_CHILD | WS_VISIBLE | style,
                    r.left, r.top, r.right - r.left, r.bottom - r.top, window,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
                SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE); return control;
            }
            void render() {
                for (HWND field: fields) DestroyWindow(field);
                fields.clear(); controls.clear();
                const auto &schema = model.CurrentRule().fields;
                for (std::size_t i = model.PageBegin(); i < model.PageEnd(); ++i) {
                    const auto &field = schema[i];
                    const int row = 48 + static_cast<int>(i % RoomSettingsViewModel::kPageSize) * 26;
                    fields.push_back(add(0, L"STATIC", Utf8ToWide(field.label), 0, 12, row + 2, 156, 23));
                    const bool choice = !field.choices.empty();
                    HWND control = add(100 + static_cast<int>(i), choice ? L"COMBOBOX" : L"EDIT", L"",
                        WS_TABSTOP | (choice ? CBS_DROPDOWNLIST | WS_VSCROLL : WS_BORDER | ES_AUTOHSCROLL),
                        174, row, 169, choice ? 160 : 20);
                    fields.push_back(control); controls[i] = control;
                    const auto text = model.CurrentFieldValue(i);
                    if (choice) {
                        int pick = 0;
                        for (std::size_t j = 0; j < field.choices.size(); ++j) {
                            const auto label = Utf8ToWide(field.choices[j].label);
                            SendMessageW(control, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
                            if (field.choices[j].value == text) pick = static_cast<int>(j);
                        }
                        SendMessageW(control, CB_SETCURSEL, pick, 0);
                    } else { SetWindowTextW(control, Utf8ToWide(text).c_str()); SendMessageW(control, EM_LIMITTEXT, 32, 0); }
                }
                EnableWindow(GetDlgItem(window, 400), model.CanPrevPage());
                EnableWindow(GetDlgItem(window, 401), model.CanNextPage());
            }
            bool capture() {
                const auto &schema = model.CurrentRule().fields;
                for (const auto &entry: controls) {
                    const auto &field = schema[entry.first];
                    if (!field.choices.empty()) {
                        const auto index = SendMessageW(entry.second, CB_GETCURSEL, 0, 0);
                        if (index < 0 || !model.SetChoiceField(entry.first, static_cast<std::size_t>(index))) {
                            SetWindowTextW(GetDlgItem(window, 402), Utf8ToWide(model.error()).c_str());
                            return false;
                        }
                    } else {
                        wchar_t text[64]{}; GetWindowTextW(entry.second, text, 64);
                        if (!model.SetTextField(entry.first, WideToUtf8(text))) {
                            SetWindowTextW(GetDlgItem(window, 402), Utf8ToWide(model.error()).c_str());
                            return false;
                        }
                    }
                }
                return true;
            }
            void init(const HWND target) {
                window = target;
                font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
                add(0, L"STATIC", L"麻将规则", 0, 12, 16, 150, 20);
                HWND selector = add(403, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 174, 12, 169, 160);
                if (!model.Initialize(rules, initial_values)) { EndDialog(window, IDCANCEL); return; }
                for (const auto *rule: model.AvailableRules()) {
                    const auto title = Utf8ToWide(rule->name);
                    SendMessageW(selector, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(title.c_str()));
                }
                SendMessageW(selector, CB_SETCURSEL, model.selected_rule(), 0);
                add(400, L"BUTTON", L"上一页", WS_TABSTOP, 12, 262, 65, 20);
                add(401, L"BUTTON", L"下一页", WS_TABSTOP, 82, 262, 65, 20);
                add(402, L"STATIC", L"设置由服务器提供，服务器校验后生效。", 0, 12, 290, 331, 38);
                add(IDOK, L"BUTTON", L"保存房间配置", WS_TABSTOP | BS_DEFPUSHBUTTON, 180, 334, 96, 22);
                add(IDCANCEL, L"BUTTON", L"取消", WS_TABSTOP, 282, 334, 61, 22); render();
            }
            static INT_PTR CALLBACK procedure(const HWND window, const UINT message, const WPARAM wp, const LPARAM lp) {
                auto *self = reinterpret_cast<RoomDialog *>(GetWindowLongPtrW(window, DWLP_USER));
                if (message == WM_INITDIALOG) { self = reinterpret_cast<RoomDialog *>(lp); SetWindowLongPtrW(window, DWLP_USER, lp); self->init(window); return TRUE; }
                if (!self) return FALSE;
                if (message == WM_COMMAND) {
                    const int id = LOWORD(wp);
                    if (id == IDCANCEL) { EndDialog(window, IDCANCEL); return TRUE; }
                    if (id == IDOK && self->capture()) { EndDialog(window, IDOK); return TRUE; }
                    if ((id == 400 || id == 401) && self->capture()) {
                        if (id == 400) self->model.PrevPage();
                        else self->model.NextPage();
                        self->render();
                        return TRUE;
                    }
                    if (id == 403 && HIWORD(wp) == CBN_SELCHANGE) {
                        const auto sel = static_cast<std::size_t>(SendDlgItemMessageW(window, 403, CB_GETCURSEL, 0, 0));
                        if (self->model.SelectRule(sel)) self->render();
                        return TRUE;
                    }
                }
                if (message == WM_DESTROY && self->font) { DeleteObject(self->font); self->font = nullptr; }
                return FALSE;
            }
        };
    }
    std::optional<std::map<std::string, std::string> > EditRoomSettings(
        const HWND owner, const std::map<std::string, std::string> &settings, const std::map<std::string, ServerRule> &rules) {
        RoomDialog dialog{{}, rules, settings, {}, {}, nullptr, nullptr};
        DLGTEMPLATE header{}; header.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME; header.cx = 356; header.cy = 370;
        const std::wstring title = L"好友麻将 · 房间规则";
        std::vector<WORD> buffer((sizeof(header) + 1) / 2 + 2 + title.size() + 1, 0);
        std::memcpy(buffer.data(), &header, sizeof(header));
        std::memcpy(buffer.data() + (sizeof(header) + 1) / 2 + 2, title.c_str(), (title.size() + 1) * sizeof(wchar_t));
        if (DialogBoxIndirectParamW(GetModuleHandleW(nullptr), reinterpret_cast<const DLGTEMPLATE *>(buffer.data()),
            owner, RoomDialog::procedure, reinterpret_cast<LPARAM>(&dialog)) == IDOK) return dialog.model.values();
        return std::nullopt;
    }
}
