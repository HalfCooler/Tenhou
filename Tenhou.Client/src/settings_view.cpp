#define UNICODE
#define _UNICODE
#include "settings_view.hpp"
#include "settings_schema.hpp"
#include "replay_effects.hpp"
#include "background.hpp"
#include "music_preferences.hpp"
#include "window_layout.hpp"
#include "win32_utf.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <set>
#include <stdexcept>
#include <algorithm>

namespace tenhou {
    namespace {
        std::wstring Wide(const std::string &text) {
            const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
            std::wstring result(size, L' ');
            MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), size);
            return result;
        }

        void Disabled(const HWND page, const int id, const wchar_t *text = nullptr) {
            const auto control = GetDlgItem(page, id);
            EnableWindow(control, FALSE);
            if (text) SetWindowTextW(control, text);
        }
    }

    SettingsView::SettingsView(const HWND parent, std::filesystem::path config, std::filesystem::path resources,
                               std::function<void(PreferenceChange)> changed,
                               std::function<void(const std::string &)> log)
        : parent_(parent), config_(std::move(config)), resources_(std::move(resources)),
          changed_(std::move(changed)), log_(std::move(log)), audio_(log_) {
        const AssetId assets[]{AssetId::Dialog104, AssetId::Dialog105, AssetId::Dialog106,
                               AssetId::Dialog109, AssetId::Dialog110, AssetId::Dialog114, AssetId::Dialog107};
        for (unsigned i = 0; i < pages_.size(); ++i)
            pages_[i] = {this, static_cast<int>(ResourceManager::Info(assets[i]).embedded_id), assets[i]};
    }

    SettingsView::~SettingsView() { if (IsWindow(window_)) DestroyWindow(window_); }

LRESULT CALLBACK SettingsView::SheetProcedure(const HWND window, const UINT message, const WPARAM wparam, const LPARAM lparam, const UINT_PTR id,
                                              const DWORD_PTR data) {
        auto *self = reinterpret_cast<SettingsView *>(data);
        const auto result = DefSubclassProc(window, message, wparam, lparam);
        if (message == WM_NCDESTROY) {
            RemoveWindowSubclass(window, SheetProcedure, id);
            self->window_ = nullptr;
            if (self->owner_disabled_ && IsWindow(self->parent_)) {
                self->owner_disabled_ = false;
                EnableWindow(self->parent_, TRUE);
                SetActiveWindow(self->parent_);
                if (IsWindow(self->owner_focus_) && IsWindowVisible(self->owner_focus_)) SetFocus(self->owner_focus_);
            }
        } else if ((message == WM_CLOSE || (message == WM_COMMAND && LOWORD(wparam) == IDCANCEL)) && IsWindow(window)) {
            // The four restored pages have no cancellation veto/rollback. A native
            // modeless sheet can retain its current page after WM_CLOSE; waiting
            // for another message leaves its owner disabled. Finish the original
            // Close lifetime here, after the common-control notifications.
            DestroyWindow(window);
        }
        return result;
    }

    int CALLBACK SettingsView::SheetCallback(const HWND window, const UINT message, LPARAM) {
        if (message == PSCB_INITIALIZED) {
            // Original common callback 00416ee9..f4 sends WM_CLOSE to IDOK.
            // Removing it also leaves Enter to the active page's UPDATE button.
            SendMessageW(GetDlgItem(window, IDOK), WM_CLOSE, 0, 0);
            SetDlgItemTextW(window, IDCANCEL, L"&Close");
        }
        return 0;
    }

    void SettingsView::Open() {
        if (IsWindow(window_) && !PropSheet_GetCurrentPageHwnd(window_)) {
            DestroyWindow(window_);
            window_ = nullptr;
        }
        if (IsWindow(window_)) {
            ShowWindow(window_, SW_SHOW);
            SetForegroundWindow(window_);
            return;
        }
        owner_focus_ = GetFocus();
        INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_WIN95_CLASSES};
        InitCommonControlsEx(&controls);
        std::array<PROPSHEETPAGEW, 7> pages{};
        for (unsigned i = 0; i < pages.size(); ++i) {
            auto &p = pages[i];
            p.dwSize = sizeof(p);
            p.hInstance = GetModuleHandleW(nullptr);
            const auto resource = ResourceManager::Embedded(pages_[i].asset);
            // comctl32 PropertySheetW with PSP_DLGINDIRECT mutates DLGTEMPLATE::style in place,
            // so copy read-only static embedded bytes into a writable member buffer.
            page_templates_[i].assign(resource.data, resource.data + resource.size);
            p.dwFlags = PSP_DLGINDIRECT;
            p.pResource = reinterpret_cast<const DLGTEMPLATE *>(page_templates_[i].data());
            p.pfnDlgProc = Procedure;
            p.lParam = reinterpret_cast<LPARAM>(&pages_[i]);
        }
        PROPSHEETHEADERW sheet{};
        sheet.dwSize = sizeof(sheet);
        sheet.dwFlags = PSH_PROPSHEETPAGE | PSH_MODELESS | PSH_NOAPPLYNOW | PSH_USECALLBACK;
        sheet.hwndParent = parent_;
        sheet.hInstance = GetModuleHandleW(nullptr);
        sheet.pszCaption = L"设置";
        sheet.nPages = static_cast<UINT>(pages.size());
        sheet.ppsp = pages.data();
        sheet.pfnCallback = SheetCallback;
        const auto result = PropertySheetW(&sheet);
        if (result == -1 || !IsWindow(reinterpret_cast<HWND>(result))) throw std::runtime_error(
            "Cannot create settings property sheet");
        window_ = reinterpret_cast<HWND>(result);
        if (!SetWindowSubclass(window_, SheetProcedure, 1, reinterpret_cast<DWORD_PTR>(this))) {
            DestroyWindow(window_);
            window_ = nullptr;
            throw std::runtime_error("Cannot manage settings window lifetime");
        }
        // Original header dwFlags=0x88 is modal. Keep the existing MotionTimer and
        // network message pump running, while enforcing its disabled-owner input.
        owner_disabled_ = IsWindowEnabled(parent_) != FALSE;
        if (owner_disabled_) EnableWindow(parent_, FALSE);
        log_("SETTINGS OPEN pages=104,105,106,109,110,114,107");
    }

    void SettingsView::InitMusic(const HWND page) {
        const auto preset = GetDlgItem(page, 0x408);
        SetWindowLongPtrW(preset, GWL_STYLE, (GetWindowLongPtrW(preset, GWL_STYLE) & ~BS_TYPEMASK) | BS_PUSHBUTTON);
        const auto list = GetDlgItem(page, 0x40d);
        ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        LVCOLUMNW col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH;
        col.cx = 110;
        col.pszText = const_cast<wchar_t *>(L"场景");
        ListView_InsertColumn(list, 0, &col);
        col.cx = 150;
        col.pszText = const_cast<wchar_t *>(L"文件");
        ListView_InsertColumn(list, 1, &col);
        col.cx = 40;
        col.pszText = const_cast<wchar_t *>(L"音量");
        ListView_InsertColumn(list, 2, &col);
        SendDlgItemMessageW(page, 0x3fa, TBM_SETRANGE, 0, MAKELPARAM(0, 200));
        SendDlgItemMessageW(page, 0x3fa, TBM_SETPOS, TRUE,
                            PreferenceInt(config_, "RESOURCE", "BGM_VOL", 100));
        SendDlgItemMessageW(page, 0x3f9, TBM_SETRANGE, 0, MAKELPARAM(0, 200));
        MusicList(page);
        for (int id: {0x408, 0x419, 0x416, 0x3f9}) EnableWindow(GetDlgItem(page, id), FALSE);
        const auto label = std::to_wstring(PreferenceInt(config_, "RESOURCE", "BGM_VOL", 100));
        SetDlgItemTextW(page, 0x428, label.c_str());
    }

    void SettingsView::MusicList(HWND page) const
    {
        const wchar_t *names[]{L"人机场大厅", L"C 类大厅", L"东场", L"南场", L"最终局", L"延长局", L"一家立直", L"多家立直", L"最终成绩"};
        const auto list = GetDlgItem(page, 0x40d);
        const int selected = ListView_GetNextItem(list, -1, LVNI_SELECTED);
        ListView_DeleteAllItems(list);
        for (int i = 0; i < 9; ++i) {
            LVITEMW item{};
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = i;
            item.pszText = const_cast<wchar_t *>(names[i]);
            item.lParam = i + 1;
            ListView_InsertItem(list, &item);
            const auto raw_file = PreferenceString(config_, "RESOURCE", MusicKeys()[i]);
            std::wstring file;
            if (raw_file.empty() || raw_file == "default") file = L"（标准）";
            else if (raw_file == "none") file = L"无";
            else file = PathFromUtf8(raw_file).filename().wstring();
            ListView_SetItemText(list, i, 1, file.data());
            auto level = std::to_wstring(
                PreferenceInt(config_, "RESOURCE", MusicOption(MusicKeys()[i]), 60));
            ListView_SetItemText(list, i, 2, level.data());
        }
        if (selected >= 0) ListView_SetItemState(list, selected, LVIS_SELECTED | LVIS_FOCUSED,
                                                 LVIS_SELECTED | LVIS_FOCUSED);
    }

    void SettingsView::MusicPreview(const HWND page) {
        const int row = ListView_GetNextItem(GetDlgItem(page, 0x40d), -1, LVNI_SELECTED);
        audio_.Stop();
        if (row < 0) return;
        if (!music_previewing_) {
            music_previewing_ = true;
            changed_(PreferenceChange::MusicPreviewBegin);
        }
        const auto path = MusicPath(resources_, config_, MusicKeys()[row]);
        if (path.empty()) return;
        audio_.SetVolumePercent(PreferenceInt(config_, "RESOURCE", "BGM_VOL", 100));
        const auto level = PreferenceInt(config_, "RESOURCE", MusicOption(MusicKeys()[row]), 60);
        audio_.Play(0, std::make_shared<SoundClip>(DecodeMusic(ReadFile(path))), true, level * .01);
        SetTimer(page, 101, 25, nullptr);
    }

    void SettingsView::MusicCommand(const HWND page, const int command) {
        const int row = ListView_GetNextItem(GetDlgItem(page, 0x40d), -1, LVNI_SELECTED);
        if (row < 0) return;
        const auto key = MusicKeys()[row];
        std::string value;
        if (command == 0x419) {
            wchar_t file[32768]{};
            OPENFILENAMEW dialog{};
            dialog.lStructSize = sizeof(dialog);
            dialog.hwndOwner = page;
            dialog.lpstrFilter = L"Wave (*.wav)\0*.wav\0\0";
            dialog.lpstrFile = file;
            dialog.nMaxFile = 32768;
            dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
            if (!GetOpenFileNameW(&dialog)) {
                if (CommDlgExtendedError()) throw std::runtime_error("无法选择音乐文件");
                return;
            }
            DecodeMusic(ReadFile(file));
            value = WideToUtf8(file);
        } else {
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, 100, L"标准");
            AppendMenuW(menu, MF_STRING, 200, L"无");
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(menu, MF_STRING, 300, L"默认 BGM 0");
            AppendMenuW(menu, MF_STRING, 400, L"默认 BGM 1");
            RECT r{};
            GetWindowRect(GetDlgItem(page, 0x408), &r);
            auto chosen = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, r.right, r.top, 0, page, nullptr);
            DestroyMenu(menu);
            if (!chosen) return;
            if (chosen == 200) value = "none";
            else if (chosen == 300 || chosen == 400)
                value = PathToUtf8(ResourceManager(resources_).Path(chosen == 300 ? AssetId::LobbyMusic : AssetId::LobbyMusicAlternate));
        }
        WritePreference(config_, "RESOURCE", key, value.empty() ? nullptr : value.c_str());
        initializing_ = true;
        MusicList(page);
        initializing_ = false;
        changed_(PreferenceChange::Sound);
        MusicPreview(page);
    }

    void SettingsView::MusicVolume(const HWND page, const int control) {
        const int value = static_cast<int>(SendDlgItemMessageW(page, control, TBM_GETPOS, 0, 0));
        if (control == 0x3fa) {
            WritePreferenceInt(config_, "RESOURCE", "BGM_VOL", value);
            const auto label = std::to_wstring(value);
            SetDlgItemTextW(page, 0x428, label.c_str());
        } else {
            const auto row = ListView_GetNextItem(GetDlgItem(page, 0x40d), -1, LVNI_SELECTED);
            if (row < 0) return;
            WritePreferenceInt(config_, "RESOURCE", MusicOption(MusicKeys()[row]), value);
            initializing_ = true;
            MusicList(page);
            initializing_ = false;
        }
        changed_(PreferenceChange::Sound);
        MusicPreview(page);
    }

    void SettingsView::InitReplay(const HWND page) {
        const auto path = PreferenceString(config_, "LOG", "PATH");
        CheckDlgButton(page, 0x420, path.empty() ? BST_UNCHECKED : BST_CHECKED);
        if (!path.empty()) SetDlgItemTextW(page, 0x419, Utf8ToWide(path).c_str());
        else ReplayCommand(page, 0x408);
        for (int id: {0x419, 0x41a, 6, 0x408}) EnableWindow(GetDlgItem(page, id), !path.empty());
    }

    void SettingsView::SaveReplaySettings(const HWND page) const
    {
        wchar_t text[32768]{};
        GetDlgItemTextW(page, 0x419, text, 32768);
        if (IsDlgButtonChecked(page, 0x420) == BST_CHECKED) {
            if (!text[0]) throw std::runtime_error("请选择牌谱保存目录");
            auto path = std::filesystem::path(text);
            if (path.is_relative()) path = config_.parent_path() / path;
            std::filesystem::create_directories(path);
            if (!std::filesystem::is_directory(path)) throw std::runtime_error("牌谱路径不是文件夹");
            WritePreference(config_, "LOG", "PATH", PathToUtf8(path).c_str());
        } else WritePreference(config_, "LOG", "PATH", nullptr);
    }

    void SettingsView::ReplayCommand(const HWND page, const int command) {
        if (command == 0x408) {
            wchar_t docs[MAX_PATH]{};
            if (!SHGetSpecialFolderPathW(page, docs, CSIDL_PERSONAL, TRUE)) throw std::runtime_error("无法读取文档目录");
            auto path = (std::filesystem::path(docs) / L"My Tenhou").wstring();
            SetDlgItemTextW(page, 0x419, path.c_str());
        } else if (command == 6) {
            BROWSEINFOW info{};
            info.hwndOwner = page;
            info.lpszTitle = L"选择牌谱保存文件夹";
            info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            auto *item = SHBrowseForFolderW(&info);
            if (!item) return;
            wchar_t folder[MAX_PATH]{};
            const bool ok = SHGetPathFromIDListW(item, folder) != FALSE;
            CoTaskMemFree(item);
            if (!ok) throw std::runtime_error("保存目录无效");
            SetDlgItemTextW(page, 0x419, folder);
        }
        const bool enabled = IsDlgButtonChecked(page, 0x420) == BST_CHECKED;
        for (int id: {0x419, 0x41a, 6, 0x408}) EnableWindow(GetDlgItem(page, id), enabled);
        if (!initializing_) SaveReplaySettings(page);
    }

    void SettingsView::InitScreen(const HWND page) const
    {
        // Original Dialog 114 (FUN_0041d...): 0x426 = ScreenSize, 0x429 = FullscreenSize.
        const auto &presets = SettingsScreenSizePresets();
        const int configured = ParseScreenSizeSetting(SettingsStore(config_).LoadScreenSize());
        int selected = 0;
        for (std::size_t i = 0; i < presets.size(); ++i) {
            const auto label = Utf8ToWide(presets[i].label);
            SendDlgItemMessageW(page, 0x426, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
            if (presets[i].width == configured) selected = static_cast<int>(i);
        }
        SendDlgItemMessageW(page, 0x426, CB_SETCURSEL, selected, 0);

        const auto fs_combo = GetDlgItem(page, 0x429);
        const auto auto_idx = SendMessageW(fs_combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"自动"));
        if (auto_idx >= 0) SendMessageW(fs_combo, CB_SETITEMDATA, auto_idx, 0);
        int want_w = 0, want_h = 0;
        const auto saved_fs = SettingsStore(config_).LoadFullscreenSize();
        if (const auto comma = saved_fs.find(','); comma != std::string::npos) {
            try {
                want_w = std::stoi(saved_fs.substr(0, comma));
                want_h = std::stoi(saved_fs.substr(comma + 1));
            } catch (...) {
                want_w = want_h = 0;
            }
        }
        std::set<std::pair<int, int>> modes;
        DEVMODEW dm{};
        dm.dmSize = sizeof(dm);
        for (DWORD i = 0; EnumDisplaySettingsW(nullptr, i, &dm); ++i) {
            if (dm.dmBitsPerPel == 32 && dm.dmPelsWidth >= 1024 && dm.dmPelsHeight >= 768)
                modes.emplace(static_cast<int>(dm.dmPelsWidth), static_cast<int>(dm.dmPelsHeight));
        }
        int fs_selected = 0;
        for (const auto &[mw, mh]: modes) {
            const auto text = std::to_wstring(mw) + L" x " + std::to_wstring(mh);
            const auto idx = SendMessageW(fs_combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
            if (idx >= 0) {
                SendMessageW(fs_combo, CB_SETITEMDATA, idx, MAKELPARAM(mw, mh));
                if (mw == want_w && mh == want_h) fs_selected = static_cast<int>(idx);
            }
        }
        SendMessageW(fs_combo, CB_SETCURSEL, fs_selected, 0);
    }

    void SettingsView::ScreenCommand(const HWND page, const int control) const
    {
        SettingsStore store(config_);
        if (control == 0x426) {
            const auto sel = static_cast<int>(SendDlgItemMessageW(page, 0x426, CB_GETCURSEL, 0, 0));
            const auto &presets = SettingsScreenSizePresets();
            const char *chosen = (sel >= 0 && static_cast<std::size_t>(sel) < presets.size())
                                     ? presets[static_cast<std::size_t>(sel)].value
                                     : "default";
            if (store.SaveScreenSize(chosen)) {
                log_(std::string("SETTINGS SCREENSIZE=") + chosen);
                changed_(PreferenceChange::ScreenSize);
            }
        } else if (control == 0x429) {
            const auto sel = SendDlgItemMessageW(page, 0x429, CB_GETCURSEL, 0, 0);
            const auto data = sel >= 0 ? SendDlgItemMessageW(page, 0x429, CB_GETITEMDATA, sel, 0) : 0;
            if (!data || data == CB_ERR) {
                store.SaveFullscreenSize(std::nullopt);
            } else {
                store.SaveFullscreenSize(std::pair<int, int>{LOWORD(data), HIWORD(data)});
            }
        }
    }

    bool SettingsView::HandleMessage(MSG &message) {
        if (!IsWindow(window_)) {
            window_ = nullptr;
            return false;
        }
        if (!PropSheet_GetCurrentPageHwnd(window_)) {
            DestroyWindow(window_);
            window_ = nullptr;
            return false;
        }
        return PropSheet_IsDialogMessage(window_, &message) != FALSE;
    }

    void SettingsView::InitOther(const HWND page) const
    {
        const auto prefs = ClientPreferences::Load(config_);
        CheckDlgButton(page, 0x420, prefs.hidden_id ? BST_CHECKED : BST_UNCHECKED);
        for (const auto &flag: kPreferenceFlags) CheckDlgButton(page, flag.control,
                                                                prefs.Checked(flag.control)
                                                                    ? BST_CHECKED
                                                                    : BST_UNCHECKED);
        // Keep original template text and visibly disable consumers not yet restored.
        Disabled(page, 0x421); // chat backend absent
    }

    void SettingsView::InitSound(const HWND page) {
        for (const auto *text: {L"标准", L"以效果音替代语音", L"分别指定各席语音"})
            SendDlgItemMessageW(page, 0x426, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text));
        const int previous = PreferenceInt(config_, "REBUILD", "SoundEffectsOnly", 0) ? 1 : 0;
        const int mode = std::clamp(PreferenceInt(config_, "RESOURCE", "SE_NOVOICE", previous), 0, 2);
        SendDlgItemMessageW(page, 0x426, CB_SETCURSEL, mode, 0);
        const auto list = GetDlgItem(page, 0x40d);
        ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.cx = 100;
        column.pszText = const_cast<wchar_t *>(L"Name");
        ListView_InsertColumn(list, 0, &column);
        column.cx = 175;
        column.pszText = const_cast<wchar_t *>(L"File");
        ListView_InsertColumn(list, 1, &column);
        SoundList(page);
        SendDlgItemMessageW(page, 0x3fa, TBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendDlgItemMessageW(page, 0x3fa, TBM_SETTICFREQ, 10, 0);
        const auto prefs = ClientPreferences::Load(config_);
        SendDlgItemMessageW(page, 0x3fa, TBM_SETPOS, TRUE, prefs.se_volume);
        const auto label = std::to_wstring(prefs.se_volume) + (prefs.se_volume == 100 ? L"(标准)" : L"");
        SetDlgItemTextW(page, 0x428, label.c_str());
        audio_.SetVolumePercent(prefs.se_volume);
    }

    void SettingsView::SoundList(const HWND page) const
    {
        const auto list = GetDlgItem(page, 0x40d);
        ListView_DeleteAllItems(list);
        const int mode = static_cast<int>(SendDlgItemMessageW(page, 0x426, CB_GETCURSEL, 0, 0));
        // FUN_00418a30: simple voice aliases, no voice aliases, or expanded voices,
        // followed by the common SE list. The list carries the exact resource ID.
        std::vector<unsigned> ids;
        if (mode == 0) ids = {119, 123, 128, 132, 137, 141, 146, 150, 155, 159, 164, 168};
        else if (mode == 2)
            for (unsigned i = 119; i <= 171; ++i)
                if (i != 127 && i != 136 && i != 145 && i != 154 && i != 163) ids.push_back(i);
        const unsigned common[]{
            101, 102, 103, 104, 105, 106, 107, 108, 109, 111, 112, 118, 136, 127, 145, 154, 163, 113, 114, 115, 116, 117
        };
        ids.insert(ids.end(), std::begin(common), std::end(common));
        int row = 0;
        for (unsigned id: ids) {
            const auto found = SoundKeys().find(id);
            if (found == SoundKeys().end()) continue;
            auto name = Wide(found->second);
            LVITEMW item{};
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = row;
            item.pszText = name.data();
            item.lParam = id;
            ListView_InsertItem(list, &item);
            const auto raw_path = PreferenceString(config_, "RESOURCE", found->second);
            std::wstring path;
            if (raw_path.empty()) path = L"(标准)";
            else if (AsciiCaseEqual(raw_path, "none")) path = L"无";
            else path = PathFromUtf8(raw_path).filename().wstring();
            ListView_SetItemText(list, row, 1, path.data());
            ++row;
        }
        for (int id: {0x408, 0x419, 0x427}) EnableWindow(GetDlgItem(page, id), FALSE);
    }

    unsigned SettingsView::SelectedSound(const HWND page) const {
        const auto list = GetDlgItem(page, 0x40d);
        const int row = ListView_GetNextItem(list, -1, LVNI_SELECTED);
        if (row < 0) return 0;
        LVITEMW item{};
        item.mask = LVIF_PARAM;
        item.iItem = row;
        return ListView_GetItem(list, &item) ? static_cast<unsigned>(item.lParam) : 0;
    }

    void SettingsView::Preview(const HWND page) {
        const unsigned id = SelectedSound(page);
        if (!id) return;
        sounds_.Load(resources_, 1440, config_);
        audio_.SetVolumePercent(ClientPreferences::Load(config_).se_volume);
        audio_.Play(id, sounds_.Get(id));
        SetTimer(page, 101, 25, nullptr);
    }

    void SettingsView::SoundOverride(const HWND page, const int command) {
        const unsigned id = SelectedSound(page);
        if (!id) return;
        const auto &key = SoundKeys().at(id);
        std::string path;
        if (command == 0x419) {
            wchar_t file[32768]{};
            OPENFILENAMEW dialog{};
            dialog.lStructSize = sizeof(dialog);
            dialog.hwndOwner = page;
            dialog.lpstrFilter = L"PCM Wave (*.wav)\0*.wav\0\0";
            dialog.lpstrFile = file;
            dialog.nMaxFile = 32768;
            dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
            if (!GetOpenFileNameW(&dialog)) {
                if (CommDlgExtendedError()) throw std::runtime_error("Cannot open SE file dialog");
                return;
            }
            DecodeSound(ReadFile(file)); // Reject unsupported files before altering a working preference.
            path = WideToUtf8(file);
        } else if (command == 0x427) path = "none";
        WritePreference(config_, "RESOURCE", key, command == 0x408 ? nullptr : path.c_str());
        // Validation above belongs to the selected file. A different broken
        // override must not prevent repairing this alias one key at a time.
        changed_(PreferenceChange::Sound);
        const int row = ListView_GetNextItem(GetDlgItem(page, 0x40d), -1, LVNI_SELECTED);
        auto label = command == 0x408
                         ? std::wstring(L"(标准)")
                         : command == 0x427
                               ? std::wstring(L"无")
                               : PathFromUtf8(path).filename().wstring();
        ListView_SetItemText(GetDlgItem(page, 0x40d), row, 1, label.data());
        Preview(page);
    }

    void SettingsView::Volume(const HWND page) {
        const int volume = static_cast<int>(SendDlgItemMessageW(page, 0x3fa, TBM_GETPOS, 0, 0));
        if (volume == 100) WritePreference(config_, "RESOURCE", "SE_VOL", nullptr);
        else WritePreferenceInt(config_, "RESOURCE", "SE_VOL", volume);
        audio_.SetVolumePercent(volume);
        changed_(PreferenceChange::Sound);
        const auto label = std::to_wstring(volume) + (volume == 100 ? L"(标准)" : L"");
        SetDlgItemTextW(page, 0x428, label.c_str());
        sounds_.Load(resources_, 1440, config_);
        audio_.Play(106, sounds_.Get(106));
        SetTimer(page, 101, 25, nullptr);
    }

    void SettingsView::InitAppearance(const HWND page, const int id) {
        appearance_ = ClientPreferences::Load(config_);
        // Native original buttons use their own owner draw handler. Use a standard
        // push button for the same command/menu in the reconstruction.
        const auto preset = GetDlgItem(page, 0x408);
        SetWindowLongPtrW(preset, GWL_STYLE, (GetWindowLongPtrW(preset, GWL_STYLE) & ~BS_TYPEMASK) | BS_PUSHBUTTON);
        if (id == 104) {
            const int sliders[]{0x400, 0x3fe, 0x3ff}, edits[]{0x403, 0x404, 0x405};
            for (unsigned i = 0; i < 3; ++i) {
                SendDlgItemMessageW(page, sliders[i], TBM_SETRANGE, 0, MAKELPARAM(0, 255));
                SendDlgItemMessageW(page, sliders[i], TBM_SETTICFREQ, 8, 0);
                SetDlgItemInt(page, edits[i], appearance_.tile_rgb[i], FALSE);
                SendDlgItemMessageW(page, sliders[i], TBM_SETPOS, TRUE, appearance_.tile_rgb[i]);
            }
            SetDlgItemInt(page, 0x3ee, appearance_.tile_shadow, FALSE);
            SendDlgItemMessageW(page, 0x3f8, TBM_SETRANGE, 0, MAKELPARAM(0, 10));
            SendDlgItemMessageW(page, 0x3f8, TBM_SETPOS, TRUE, GetDlgItemInt(page, 0x3ee, nullptr, FALSE));
        } else {
            for (const auto *label: {L"背景", L"叠加层一", L"叠加层二"})
                SendDlgItemMessageW(page, 0x426, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
            SendDlgItemMessageW(page, 0x426, CB_SETCURSEL, 0, 0);
            background_layer_ = 0;
            const int sliders[]{0x3f7, 0x3f8, 0x3f9, 0x3fa};
            for (unsigned i = 0; i < 4; ++i) {
                SendDlgItemMessageW(page, sliders[i], TBM_SETRANGE, 0, MAKELPARAM(0, i == 0 ? 359 : i == 3 ? 10 : 255));
                SendDlgItemMessageW(page, sliders[i], TBM_SETTICFREQ, i == 0 ? 10 : i == 3 ? 1 : 8, 0);
            }
            BackgroundLayer(page, false);
            SetDlgItemTextW(page, 0x3f4, Utf8ToWide(appearance_.background_lhsv[0]).c_str());
            SendDlgItemMessageW(page, 0x3fa, TBM_SETPOS, TRUE, GetDlgItemInt(page, 0x3f4, nullptr, FALSE));
            CheckDlgButton(page, 0x420, (appearance_.flags & 0x2000) ? BST_CHECKED : BST_UNCHECKED);
        }
    }

    void SettingsView::BackgroundLayer(const HWND page, const bool save) {
        const int edits[]{0x3ed, 0x3ee, 0x3ef}, sliders[]{0x3f7, 0x3f8, 0x3f9};
        for (unsigned i = 0; i < 3; ++i) {
            auto &text = appearance_.background_lhsv[1 + background_layer_ * 3 + i];
            if (save) {
                if (IsWindowEnabled(GetDlgItem(page, edits[i]))) {
                    wchar_t value[80]{};
                    GetDlgItemTextW(page, edits[i], value, 80);
                    text = WideToUtf8(value);
                }
            } else {
                EnableWindow(GetDlgItem(page, edits[i]), !text.empty());
                EnableWindow(GetDlgItem(page, sliders[i]), !text.empty());
                SetDlgItemTextW(page, edits[i], Utf8ToWide(text).c_str());
                SendDlgItemMessageW(page, sliders[i], TBM_SETPOS, TRUE, GetDlgItemInt(page, edits[i], nullptr, FALSE));
            }
        }
    }

    void SettingsView::Appearance(const HWND page, const int id, const int command) {
        SettingsStore store(config_);
        if (id == 104) {
            if (command == 0x408) {
                HMENU menu = CreatePopupMenu();
                for (const auto &preset: SettingsTilePresets())
                    AppendMenuW(menu, MF_STRING, preset.menu_command, Utf8ToWide(preset.name).c_str());
                RECT bounds{};
                GetWindowRect(GetDlgItem(page, command), &bounds);
                const auto color = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, bounds.right, bounds.top, 0,
                                                  page, nullptr);
                DestroyMenu(menu);
                if (!color) return;
                for (const auto &preset: SettingsTilePresets()) {
                    if (preset.menu_command == static_cast<std::uint32_t>(color)) {
                        SetDlgItemInt(page, 0x403, static_cast<UINT>(preset.rgb[0]), FALSE);
                        SetDlgItemInt(page, 0x404, static_cast<UINT>(preset.rgb[1]), FALSE);
                        SetDlgItemInt(page, 0x405, static_cast<UINT>(preset.rgb[2]), FALSE);
                        SetDlgItemInt(page, 0x3ee, static_cast<UINT>(preset.shadow), FALSE);
                        break;
                    }
                }
            }
            store.SaveTileAppearance(
                {static_cast<int>(GetDlgItemInt(page, 0x403, nullptr, FALSE)),
                 static_cast<int>(GetDlgItemInt(page, 0x404, nullptr, FALSE)),
                 static_cast<int>(GetDlgItemInt(page, 0x405, nullptr, FALSE))},
                static_cast<int>(GetDlgItemInt(page, 0x3ee, nullptr, FALSE)));
            changed_(PreferenceChange::Artwork);
        } else if (command == 0x420) {
            store.SetCenterPanelHidden(IsDlgButtonChecked(page, 0x420) == BST_CHECKED);
            changed_(PreferenceChange::Artwork);
        } else {
            BackgroundLayer(page, true);
            appearance_.background_lhsv[0] = std::to_string(GetDlgItemInt(page, 0x3f4, nullptr, FALSE));
            if (command == 0x408) {
                HMENU menu = CreatePopupMenu();
                for (const auto &preset: SettingsBackgroundPresets())
                    AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(preset.menu_command),
                                Utf8ToWide(preset.name).c_str());
                AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(menu, MF_STRING, 400, L"指定文件...");
                RECT bounds{};
                GetWindowRect(GetDlgItem(page, command), &bounds);
                const auto selected = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, bounds.right, bounds.top, 0,
                                                     page, nullptr);
                DestroyMenu(menu);
                if (!selected) return;
                if (selected == 400) {
                    wchar_t file[32768]{};
                    OPENFILENAMEW open{};
                    open.lStructSize = sizeof(open);
                    open.hwndOwner = page;
                    open.lpstrFilter = L"Image (*.bmp;*.jpg;*.png;*.gif)\0*.bmp;*.jpg;*.png;*.gif\0\0";
                    open.lpstrFile = file;
                    open.nMaxFile = 32768;
                    open.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
                    if (!GetOpenFileNameW(&open)) {
                        if (CommDlgExtendedError()) throw std::runtime_error("Cannot open background file dialog");
                        return;
                    }
                    appearance_.background_source = WideToUtf8(file);
                    // Image dimensions determine the original three-layer default.
                    Gdiplus::Bitmap input(file);
                    if (input.GetLastStatus() != Gdiplus::Ok) throw std::runtime_error("Cannot load background image");
                    appearance_.background_lhsv = {"2", "0", "255", "255", "", "", "", "", "", ""};
                    if (input.GetWidth() == 1024 && input.GetHeight() == 3840)
                        appearance_.background_lhsv = {
                            "2", "0", "255", "255", "0", "255", "255", "0", "255", "255"
                        };
                } else {
                    for (const auto &preset: SettingsBackgroundPresets()) {
                        if (preset.menu_command == selected) {
                            appearance_.background_source = preset.source;
                            for (std::size_t i = 0; i < 10; ++i) appearance_.background_lhsv[i] = preset.lhsv[i];
                            break;
                        }
                    }
                }
            }
            BuildPreferredBackground(resources_, config_, 1440, 1080, appearance_);
            // Validate before persistence/cache notification.
            store.SaveBackground(appearance_.background_source, appearance_.background_lhsv);
            changed_(PreferenceChange::Artwork);
            if (command == 0x408) {
                initializing_ = true;
                BackgroundLayer(page, false);
                SetDlgItemTextW(page, 0x3f4, Utf8ToWide(appearance_.background_lhsv[0]).c_str());
                SendDlgItemMessageW(page, 0x3fa, TBM_SETPOS, TRUE, GetDlgItemInt(page, 0x3f4, nullptr, FALSE));
                initializing_ = false;
            }
        }
    }

INT_PTR CALLBACK SettingsView::Procedure(const HWND page, const UINT message, const WPARAM wparam, const LPARAM lparam) {
        auto *info = reinterpret_cast<Page *>(GetWindowLongPtrW(page, GWLP_USERDATA));
        if (message == WM_INITDIALOG) {
            info = reinterpret_cast<Page *>(reinterpret_cast<PROPSHEETPAGEW *>(lparam)->lParam);
            SetWindowLongPtrW(page, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(info));
        }
        if (!info) return FALSE;
        try { return info->owner->Message(page, info->id, message, wparam, lparam); } catch (const std::exception &e) {
            info->owner->initializing_ = false;
            info->owner->log_(std::string("SETTINGS ERROR ") + e.what());
            const HWND owner = IsWindow(info->owner->window_) ? info->owner->window_ : page;
            if (!MessageBoxW(owner, Wide(e.what()).c_str(), L"设置", MB_OK | MB_ICONERROR))
                info->owner->log_("SETTINGS ERROR dialog creation failed=" + std::to_string(GetLastError()));
            return TRUE;
        }
    }

    INT_PTR SettingsView::Message(const HWND page, const int id, const UINT message, const WPARAM wparam, const LPARAM lparam) {
        if (message == WM_INITDIALOG) {
            initializing_ = true;
            if (id == 107) InitOther(page);
            else if (id == 106) InitSound(page);
            else if (id == 109) InitMusic(page);
            else if (id == 110) InitReplay(page);
            else if (id == 114) InitScreen(page);
            else InitAppearance(page, id);
            initializing_ = false;
            return TRUE;
        }
        if (message == WM_DESTROY) {
            KillTimer(page, 100);
            KillTimer(page, 101);
            audio_.Stop();
            if (id == 109 && music_previewing_) {
                music_previewing_ = false;
                changed_(PreferenceChange::MusicPreviewEnd);
            }
            return TRUE;
        }
        if (initializing_) return FALSE;
        if (id == 114) {
            if (message == WM_COMMAND && HIWORD(wparam) == CBN_SELCHANGE &&
                (LOWORD(wparam) == 0x426 || LOWORD(wparam) == 0x429)) {
                ScreenCommand(page, LOWORD(wparam));
                return TRUE;
            }
            return FALSE;
        }
        if (id == 110) {
            if (message == WM_COMMAND && (LOWORD(wparam) == 0x408 || LOWORD(wparam) == 6 || LOWORD(wparam) == 0x420)) {
                ReplayCommand(page, LOWORD(wparam));
                return TRUE;
            }
            if (message == WM_COMMAND && LOWORD(wparam) == 0x419 && HIWORD(wparam) == EN_KILLFOCUS) {
                SaveReplaySettings(page);
                return TRUE;
            }
            if (message == WM_NOTIFY && reinterpret_cast<NMHDR *>(lparam)->code == PSN_KILLACTIVE) {
                SaveReplaySettings(page);
                return TRUE;
            }
        }
        if (id == 109) {
            if (message == WM_COMMAND && (LOWORD(wparam) == 0x408 || LOWORD(wparam) == 0x419)) {
                MusicCommand(page, LOWORD(wparam));
                return TRUE;
            }
            if (message == WM_NOTIFY) {
                auto *note = reinterpret_cast<NMHDR *>(lparam);
                if (note->code == PSN_KILLACTIVE) {
                    KillTimer(page, 100);
                    audio_.Stop();
                    if (music_previewing_) {
                        music_previewing_ = false;
                        changed_(PreferenceChange::MusicPreviewEnd);
                    }
                    return TRUE;
                }
                if (note->idFrom == 0x40d && (note->code == LVN_ITEMCHANGED || note->code == NM_DBLCLK)) {
                    const auto row = ListView_GetNextItem(GetDlgItem(page, 0x40d), -1, LVNI_SELECTED);
                    for (int control: {0x408, 0x419, 0x416, 0x3f9}) EnableWindow(GetDlgItem(page, control), row >= 0);
                    if (row >= 0) {
                        SendDlgItemMessageW(page, 0x3f9, TBM_SETPOS, TRUE,
                                            PreferenceInt(config_, "RESOURCE", MusicOption(MusicKeys()[row]), 60));
                        KillTimer(page, 100);
                        SetTimer(page, 100, 200, nullptr);
                    }
                    return TRUE;
                }
                if ((note->idFrom == 0x3fa || note->idFrom == 0x3f9) && note->code == NM_RELEASEDCAPTURE) {
                    MusicVolume(page, static_cast<int>(note->idFrom));
                    return TRUE;
                }
            }
            if (message == WM_TIMER && wparam == 100) {
                KillTimer(page, 100);
                MusicPreview(page);
                return TRUE;
            }
            if (message == WM_TIMER && wparam == 101) {
                audio_.Poll();
                return TRUE;
            }
        }
        if (id == 104 || id == 105) {
            if (message == WM_HSCROLL) {
                const int slider = GetDlgCtrlID(reinterpret_cast<HWND>(lparam));
                int edit = 0;
                if (id == 104) edit = slider == 0x400
                                          ? 0x403
                                          : slider == 0x3fe
                                                ? 0x404
                                                : slider == 0x3ff
                                                      ? 0x405
                                                      : slider == 0x3f8
                                                            ? 0x3ee
                                                            : 0;
                else edit = slider == 0x3f7
                                ? 0x3ed
                                : slider == 0x3f8
                                      ? 0x3ee
                                      : slider == 0x3f9
                                            ? 0x3ef
                                            : slider == 0x3fa
                                                  ? 0x3f4
                                                  : 0;
                if (edit) SetDlgItemInt(
                    page, edit, static_cast<UINT>(SendMessageW(reinterpret_cast<HWND>(lparam), TBM_GETPOS, 0, 0)),
                    FALSE);
                return TRUE;
            }
            if (message == WM_COMMAND) {
                const int control = LOWORD(wparam);
                if (id == 105 && control == 0x426 && HIWORD(wparam) == CBN_SELCHANGE) {
                    BackgroundLayer(page, true);
                    background_layer_ = static_cast<int>(SendDlgItemMessageW(page, control, CB_GETCURSEL, 0, 0));
                    initializing_ = true;
                    BackgroundLayer(page, false);
                    initializing_ = false;
                    return TRUE;
                }
                if (HIWORD(wparam) == EN_CHANGE) {
                    int slider = 0, maximum = 256;
                    if (id == 104) {
                        slider = control == 0x403
                                     ? 0x400
                                     : control == 0x404
                                           ? 0x3fe
                                           : control == 0x405
                                                 ? 0x3ff
                                                 : control == 0x3ee
                                                       ? 0x3f8
                                                       : 0;
                        maximum = control == 0x3ee ? 10 : 256;
                    } else {
                        slider = control == 0x3ed
                                     ? 0x3f7
                                     : control == 0x3ee
                                           ? 0x3f8
                                           : control == 0x3ef
                                                 ? 0x3f9
                                                 : control == 0x3f4
                                                       ? 0x3fa
                                                       : 0;
                        maximum = control == 0x3ed ? 359 : control == 0x3f4 ? 10 : 256;
                    }
                    if (slider && IsWindowEnabled(GetDlgItem(page, control))) {
                        const int raw = static_cast<int>(GetDlgItemInt(page, control, nullptr, FALSE)), value =
                                std::clamp(raw, 0, maximum);
                        if (raw != value) SetDlgItemInt(page, control, value, FALSE);
                        SendDlgItemMessageW(page, slider, TBM_SETPOS, TRUE, value);
                        return TRUE;
                    }
                }
                if (HIWORD(wparam) == BN_CLICKED && (
                        control == IDOK || control == 0x408 || (id == 105 && control == 0x420))) {
                    Appearance(page, id, control);
                    return TRUE;
                }
            }
        }
        if (id == 107 && message == WM_COMMAND) {
            const int control = LOWORD(wparam);
            if (control == 0x419) {
                const auto argument = L"\"" + config_.wstring() + L"\"";
                const auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(
                    page, L"open", L"notepad.exe", argument.c_str(), nullptr, SW_SHOWNORMAL));
                if (result <= 32) throw std::runtime_error("Cannot open client configuration");
                return TRUE;
            }
            if (HIWORD(wparam) == BN_CLICKED && control >= 0x420 && control <= 0x428 && control != 0x421 && control !=
                0x422) {
                const bool checked = IsDlgButtonChecked(page, control) == BST_CHECKED;
                auto prefs = ClientPreferences::Load(config_);
                try {
                    if (control == 0x420) WritePreference(config_, "STATUS", "HIDDENID", checked ? "1" : nullptr);
                    else {
                        prefs.SetChecked(control, checked);
                        WritePreferenceInt(config_, "STATUS", "FLAGS0", static_cast<int>(prefs.flags));
                    }
                } catch (...) {
                    InitOther(page);
                    throw;
                }
                changed_(PreferenceChange::Other);
                return TRUE;
            }
        }
        if (id == 106) {
            if (message == WM_COMMAND) {
                const int control = LOWORD(wparam);
                if (control == 0x426 && HIWORD(wparam) == CBN_SELCHANGE) {
                    WritePreferenceInt(config_, "RESOURCE", "SE_NOVOICE",
                                       static_cast<int>(SendDlgItemMessageW(page, control, CB_GETCURSEL, 0, 0)));
                    SoundList(page);
                    changed_(PreferenceChange::Sound);
                    return TRUE;
                }
                if (control == 0x408 || control == 0x419 || control == 0x427) {
                    SoundOverride(page, control);
                    return TRUE;
                }
            }
            if (message == WM_NOTIFY) {
                const auto *notify = reinterpret_cast<NMHDR *>(lparam);
                if (notify->idFrom == 0x40d && notify->code == LVN_ITEMCHANGED) {
                    const bool selected = SelectedSound(page) != 0;
                    for (int control: {0x408, 0x419, 0x427}) EnableWindow(GetDlgItem(page, control), selected);
                    const auto *change = reinterpret_cast<NMLISTVIEW *>(lparam);
                    if ((change->uChanged & LVIF_STATE) && (change->uNewState & LVIS_SELECTED) && !(
                            change->uOldState & LVIS_SELECTED)) {
                        KillTimer(page, 100);
                        SetTimer(page, 100, 200, nullptr);
                    }
                }
                if (notify->idFrom == 0x40d && notify->code == NM_DBLCLK) {
                    KillTimer(page, 100);
                    SetTimer(page, 100, 200, nullptr);
                }
                if (notify->idFrom == 0x3fa && notify->code == NM_RELEASEDCAPTURE) {
                    Volume(page);
                    return TRUE;
                }
            }
            if (message == WM_TIMER && wparam == 100) {
                KillTimer(page, 100);
                Preview(page);
                return TRUE;
            }
            if (message == WM_TIMER && wparam == 101) {
                audio_.Poll();
                if (!audio_.Active()) KillTimer(page, 101);
                return TRUE;
            }
        }
        return FALSE;
    }
} // namespace tenhou
