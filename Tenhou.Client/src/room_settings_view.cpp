#include "room_settings_view.hpp"
#include "network_protocol.hpp"
#include <algorithm>
#include <charconv>
#include <cstring>

namespace tenhou {
    namespace {
        struct RoomDialog {
            const std::map<std::string, ServerRule> &rules;
            std::vector<const ServerRule *> available;
            std::map<std::string, std::string> values;
            std::map<std::size_t, HWND> controls;
            std::vector<HWND> fields;
            HFONT font = nullptr;
            HWND window = nullptr;
            std::size_t selected = 0, page = 0;
            static constexpr std::size_t page_size = 8;
            HWND add(int id, const wchar_t *cls, const std::wstring &text, DWORD style, int x, int y, int width, int height) {
                RECT r{x, y, x + width, y + height}; MapDialogRect(window, &r);
                HWND control = CreateWindowExW(0, cls, text.c_str(), WS_CHILD | WS_VISIBLE | style,
                    r.left, r.top, r.right - r.left, r.bottom - r.top, window,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
                SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE); return control;
            }
            void render() {
                for (HWND field: fields) DestroyWindow(field);
                fields.clear(); controls.clear();
                const auto &schema = available.at(selected)->fields;
                for (std::size_t i = page * page_size; i < std::min(schema.size(), (page + 1) * page_size); ++i) {
                    const auto &field = schema[i]; const int row = 48 + static_cast<int>(i % page_size) * 26;
                    fields.push_back(add(0, L"STATIC", NetworkWide(field.label), 0, 12, row + 2, 156, 23));
                    const bool choice = !field.choices.empty();
                    HWND control = add(100 + static_cast<int>(i), choice ? L"COMBOBOX" : L"EDIT", L"",
                        WS_TABSTOP | (choice ? CBS_DROPDOWNLIST | WS_VSCROLL : WS_BORDER | ES_AUTOHSCROLL),
                        174, row, 169, choice ? 160 : 20);
                    fields.push_back(control); controls[i] = control;
                    const auto current = values.find(field.key);
                    const auto text = current == values.end() ? field.initial : current->second;
                    if (choice) {
                        int pick = 0;
                        for (std::size_t j = 0; j < field.choices.size(); ++j) {
                            const auto label = NetworkWide(field.choices[j].label);
                            SendMessageW(control, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
                            if (field.choices[j].value == text) pick = static_cast<int>(j);
                        }
                        SendMessageW(control, CB_SETCURSEL, pick, 0);
                    } else { SetWindowTextW(control, NetworkWide(text).c_str()); SendMessageW(control, EM_LIMITTEXT, 32, 0); }
                }
                EnableWindow(GetDlgItem(window, 400), page > 0);
                EnableWindow(GetDlgItem(window, 401), (page + 1) * page_size < schema.size());
            }
            bool capture() {
                const auto &schema = available.at(selected)->fields;
                for (const auto &entry: controls) {
                    const auto &field = schema[entry.first]; std::string value;
                    if (!field.choices.empty()) {
                        const auto index = SendMessageW(entry.second, CB_GETCURSEL, 0, 0);
                        if (index < 0 || static_cast<std::size_t>(index) >= field.choices.size()) return false;
                        value = field.choices[static_cast<std::size_t>(index)].value;
                    } else {
                        wchar_t text[64]{}; GetWindowTextW(entry.second, text, 64); value = NetworkUtf8(text);
                        if (!field.minimum.empty() || !field.maximum.empty()) {
                            int number = 0; const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
                            const int low = field.minimum.empty() ? 0 : std::stoi(field.minimum);
                            const int high = field.maximum.empty() ? 100000000 : std::stoi(field.maximum);
                            if (parsed.ec != std::errc() || parsed.ptr != value.data() + value.size() || number < low || number > high ||
                                (field.step && (number - low) % static_cast<int>(field.step))) {
                                SetWindowTextW(GetDlgItem(window, 402), (NetworkWide(field.label) + L"：请输入服务器允许的数值").c_str()); return false;
                            }
                        }
                    }
                    values[field.key] = std::move(value);
                }
                return true;
            }
            void init(HWND target) {
                window = target;
                font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
                add(0, L"STATIC", L"麻将规则", 0, 12, 16, 150, 20);
                HWND selector = add(403, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 174, 12, 169, 160);
                for (const auto &entry: rules) {
                    if (entry.second.presentation != "riichi-v1" && entry.second.presentation != "mahjong-v1") continue;
                    available.push_back(&entry.second); const auto title = NetworkWide(entry.second.name);
                    SendMessageW(selector, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(title.c_str()));
                    if (values.count("rule") && values.at("rule") == entry.first) selected = available.size() - 1;
                }
                if (available.empty()) { EndDialog(window, IDCANCEL); return; }
                const auto &schema = available[selected]->fields;
                for (auto item = values.begin(); item != values.end();) {
                    if (item->first != "rule" && std::none_of(schema.begin(), schema.end(), [&](const RuleField &f) { return f.key == item->first; }))
                        item = values.erase(item);
                    else ++item;
                }
                values["rule"] = available[selected]->id; SendMessageW(selector, CB_SETCURSEL, selected, 0);
                add(400, L"BUTTON", L"上一页", WS_TABSTOP, 12, 262, 65, 20);
                add(401, L"BUTTON", L"下一页", WS_TABSTOP, 82, 262, 65, 20);
                add(402, L"STATIC", L"设置由服务器提供，服务器校验后生效。", 0, 12, 290, 331, 38);
                add(IDOK, L"BUTTON", L"保存房间配置", WS_TABSTOP | BS_DEFPUSHBUTTON, 180, 334, 96, 22);
                add(IDCANCEL, L"BUTTON", L"取消", WS_TABSTOP, 282, 334, 61, 22); render();
            }
            static INT_PTR CALLBACK procedure(HWND window, UINT message, WPARAM wp, LPARAM lp) {
                auto *self = reinterpret_cast<RoomDialog *>(GetWindowLongPtrW(window, DWLP_USER));
                if (message == WM_INITDIALOG) { self = reinterpret_cast<RoomDialog *>(lp); SetWindowLongPtrW(window, DWLP_USER, lp); self->init(window); return TRUE; }
                if (!self) return FALSE;
                if (message == WM_COMMAND) {
                    const int id = LOWORD(wp);
                    if (id == IDCANCEL) { EndDialog(window, IDCANCEL); return TRUE; }
                    if (id == IDOK && self->capture()) { EndDialog(window, IDOK); return TRUE; }
                    if ((id == 400 || id == 401) && self->capture()) { if (id == 400) --self->page; else ++self->page; self->render(); return TRUE; }
                    if (id == 403 && HIWORD(wp) == CBN_SELCHANGE) {
                        self->selected = static_cast<std::size_t>(SendDlgItemMessageW(window, 403, CB_GETCURSEL, 0, 0));
                        self->values = {{"rule", self->available.at(self->selected)->id}}; self->page = 0; self->render(); return TRUE;
                    }
                }
                if (message == WM_DESTROY && self->font) { DeleteObject(self->font); self->font = nullptr; }
                return FALSE;
            }
        };
    }
    std::optional<std::map<std::string, std::string> > EditRoomSettings(
        HWND owner, const std::map<std::string, std::string> &settings, const std::map<std::string, ServerRule> &rules) {
        RoomDialog dialog{rules, {}, settings, {}, {}, nullptr, nullptr, 0, 0};
        DLGTEMPLATE header{}; header.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME; header.cx = 356; header.cy = 370;
        const std::wstring title = L"好友麻将 · 房间规则";
        std::vector<WORD> buffer((sizeof(header) + 1) / 2 + 2 + title.size() + 1, 0);
        std::memcpy(buffer.data(), &header, sizeof(header));
        std::memcpy(buffer.data() + (sizeof(header) + 1) / 2 + 2, title.c_str(), (title.size() + 1) * sizeof(wchar_t));
        if (DialogBoxIndirectParamW(GetModuleHandleW(nullptr), reinterpret_cast<const DLGTEMPLATE *>(buffer.data()),
            owner, RoomDialog::procedure, reinterpret_cast<LPARAM>(&dialog)) == IDOK) return dialog.values;
        return std::nullopt;
    }
}
