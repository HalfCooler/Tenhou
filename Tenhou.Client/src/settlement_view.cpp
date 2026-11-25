#include "settlement_view.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <stdexcept>

namespace tenhou {
    namespace {
        std::wstring Wide(const std::string &value) {
            const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
            std::wstring text(size, L' ');
            if (size) MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), text.data(), size);
            return text;
        }
    }

    SettlementView::SettlementView(HWND parent) {
        WNDCLASSEXW wc{sizeof(wc)};
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpfnWndProc = Procedure;
        wc.lpszClassName = L"TENHOU_REBUILD_SETTLEMENT";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            throw std::runtime_error("Cannot register settlement view");
        font_ = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"MS UI Gothic");
        window_ = CreateWindowExW(WS_EX_CONTROLPARENT, wc.lpszClassName, L"Settlement", WS_CHILD | WS_CLIPCHILDREN,
                                  0, 0, 1, 1, parent, reinterpret_cast<HMENU>(5400), wc.hInstance, this);
        if (!font_ || !window_) throw std::runtime_error("Cannot create settlement view");
        choices_ = CreateWindowExW(0, L"COMBOBOX", L"",
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                                   0, 0, 1, 1, window_, reinterpret_cast<HMENU>(5401), wc.hInstance, nullptr);
        details_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
                                   0, 0, 1, 1, window_, reinterpret_cast<HMENU>(5402), wc.hInstance, nullptr);
        if (!choices_ || !details_) throw std::runtime_error("Cannot create settlement controls");
        for (HWND control: {choices_, details_})
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
        SendMessageW(details_, EM_LIMITTEXT, 65536, 0);
        ok_ = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                              0, 0, 1, 1, window_, reinterpret_cast<HMENU>(5404), wc.hInstance, nullptr);
        if (!ok_) throw std::runtime_error("Cannot create settlement confirmation");
        SendMessageW(ok_, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
        previous_patterns_ = CreateWindowExW(0, L"BUTTON", L"上一页", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                             0, 0, 1, 1, window_, reinterpret_cast<HMENU>(5405), wc.hInstance, nullptr);
        next_patterns_ = CreateWindowExW(0, L"BUTTON", L"下一页", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                         0, 0, 1, 1, window_, reinterpret_cast<HMENU>(5406), wc.hInstance, nullptr);
        if (!previous_patterns_ || !next_patterns_) throw std::runtime_error("Cannot create pattern paging controls");
        for (HWND control: {previous_patterns_, next_patterns_})
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
    }

    SettlementView::~SettlementView() {
        InvalidateBackdrop(true);
        if (IsWindow(window_)) DestroyWindow(window_);
        if (font_) DeleteObject(font_);
    }

    void SettlementView::InvalidateBackdrop(bool release) {
        backdrop_valid_ = false;
        if (!release) return;
        if (backdrop_old_ && backdrop_dc_) SelectObject(backdrop_dc_, backdrop_old_);
        if (backdrop_bitmap_) DeleteObject(backdrop_bitmap_);
        if (backdrop_dc_) DeleteDC(backdrop_dc_);
        backdrop_dc_ = nullptr;
        backdrop_bitmap_ = nullptr;
        backdrop_old_ = nullptr;
        backdrop_w_ = backdrop_h_ = 0;
    }

    void SettlementView::ApplyEffects(const SettlementEffects &effects) {
        if (effects.stop_sound && stop_sound_) stop_sound_();
        for (unsigned id: effects.sounds)
            if (play_sound_) play_sound_(id);
        if (effects.presentation_timer_ms.has_value()) {
            KillTimer(window_, 1);
            if (*effects.presentation_timer_ms > 0 &&
                !SetTimer(window_, 1, *effects.presentation_timer_ms, nullptr)) {
                ApplyEffects(sequencer_.CancelPresentation(GetTickCount64()));
                return;
            }
        }
        if (effects.confirm_timer_ms.has_value()) {
            KillTimer(window_, 2);
            if (*effects.confirm_timer_ms > 0) SetTimer(window_, 2, *effects.confirm_timer_ms, nullptr);
        }
        if (choices_ && sequencer_.ChoiceCount() > 0) {
            const auto current = SendMessageW(choices_, CB_GETCURSEL, 0, 0);
            if (current != static_cast<LRESULT>(sequencer_.selected()))
                SendMessageW(choices_, CB_SETCURSEL, sequencer_.selected(), 0);
        }
        if (effects.dirty) {
            if (details_) {
                UpdateText();
                Arrange();
            }
            InvalidateRect(window_, nullptr, FALSE);
        }
        if (effects.notify_bgm_changed) PostMessageW(GetParent(window_), WM_APP + 18, 0, 0);
        if (effects.notify_presentation_done) PostMessageW(GetParent(window_), WM_APP + 16, 0, 0);
        if (effects.confirm_finished && confirm_handler_) confirm_handler_();
    }

    void SettlementView::SetSoundEnabled(bool enabled) {
        ApplyEffects(sequencer_.SetSoundEnabled(enabled, GetTickCount64()));
    }

    void SettlementView::SetAnimationEnabled(bool enabled) {
        ApplyEffects(sequencer_.SetAnimationEnabled(enabled, GetTickCount64()));
    }

    void SettlementView::SetConfirmEnabled(bool enabled) {
        ApplyEffects(sequencer_.SetConfirmEnabled(enabled, GetTickCount64()));
    }

    void SettlementView::SetContinuation(bool available) {
        ApplyEffects(sequencer_.SetContinuation(available, GetTickCount64()));
    }

    void SettlementView::Configure(const std::filesystem::path &directory, int width) {
        display_artwork_.Configure(directory, width);
        directory_ = directory;
        canvas_ = width;
        artwork_ = {};
        resource_error_.clear();
        artwork_.SetColor(tile_color_);
        InvalidateBackdrop();
        if (choices_) Select();
        InvalidateRect(window_, nullptr, FALSE);
    }

    void SettlementView::Show(bool visible) {
        const bool opening = visible && !sequencer_.visible();
        if (!visible && peeking_) {
            peeking_ = false;
            if (GetCapture() == window_) ReleaseCapture();
        }
        InvalidateBackdrop(!visible);
        ShowWindow(window_, visible ? SW_SHOW : SW_HIDE);
        const bool host_visible = visible && IsWindowVisible(window_) != FALSE;
        ApplyEffects(sequencer_.SetVisible(visible, GetTickCount64(), host_visible));
        if (opening && host_visible) SetFocus(ok_);
    }

    void SettlementView::Move(RECT r) {
        MoveWindow(window_, r.left, r.top, std::max(1L, r.right - r.left), std::max(1L, r.bottom - r.top), TRUE);
    }

    void SettlementView::Capture(HDC dc) {
        if (!IsWindowVisible(window_)) return;
        POINT origin{};
        ClientToScreen(GetParent(window_), &origin);
        RECT r{};
        GetWindowRect(window_, &r);
        const int saved = SaveDC(dc);
        SetViewportOrgEx(dc, r.left - origin.x, r.top - origin.y, nullptr);
        SendMessageW(window_, WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT | PRF_CHILDREN | PRF_ERASEBKGND);
        RestoreDC(dc, saved);
    }

    void SettlementView::Set(const SettlementState &s, const std::array<std::string, 4> &names, bool red) {
        SettlementEffects effects;
        if (!sequencer_.SetState(s, names, red, GetTickCount64(), effects)) return;
        InvalidateBackdrop();
        artwork_.SetRedTiles(s.red_tiles);
        SendMessageW(choices_, CB_RESETCONTENT, 0, 0);
        for (const auto &r: s.receipts) {
            const auto text = Wide(SettlementLabel(r));
            SendMessageW(choices_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
        }
        if (s.has_final)
            SendMessageW(choices_, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(L"Match results | supplied scores / points"));
        SendMessageW(choices_, CB_SETCURSEL, sequencer_.selected(), 0);
        ApplyEffects(effects);
    }

    void SettlementView::SetRoundFinished(bool finished) {
        ApplyEffects(sequencer_.SetRoundFinished(finished, GetTickCount64()));
    }

    void SettlementView::Select() {
        const auto selected = SendMessageW(choices_, CB_GETCURSEL, 0, 0);
        const std::size_t index = selected >= 0 ? static_cast<std::size_t>(selected) : sequencer_.selected();
        ApplyEffects(sequencer_.SelectReceipt(index, GetTickCount64()));
    }

    void SettlementView::UpdateText() {
        const auto text = sequencer_.FormatDetailsText(resource_error_.empty());
        SetWindowTextW(details_, Wide(text).c_str());
    }

    void SettlementView::CancelPresentation() {
        ApplyEffects(sequencer_.CancelPresentation(GetTickCount64()));
    }

    void SettlementView::Present() {
        ApplyEffects(sequencer_.Present(GetTickCount64(), IsWindowVisible(window_) != FALSE));
    }

    void SettlementView::PresentationTick() {
        ApplyEffects(sequencer_.PresentationTick(GetTickCount64(), IsWindowVisible(window_) != FALSE));
    }

    void SettlementView::SetContext(int dealer, int honba, int sticks) {
        bool changed = false;
        sequencer_.SetContext(dealer, honba, sticks, changed);
        if (changed) {
            InvalidateBackdrop();
            InvalidateRect(window_, nullptr, FALSE);
        }
    }

    SettlementLayout SettlementView::Layout() {
        const auto params = sequencer_.SceneParameters();
        SettlementLayout layout;
        layout.width = canvas_;
        layout.dealer = params.dealer;
        layout.honba = params.honba;
        layout.sticks = params.sticks;
        layout.pattern_page = params.pattern_page;
        layout.yaku_entrance = params.yaku_entrance;
        layout.tile_bounds = [this](const TableTile &tile) {
            const auto &item = artwork_.Get(tile, sequencer_.red());
            return RECT{item.x, item.y, item.x + item.image.width, item.y + item.image.height};
        };
        layout.text_width = DisplayArtwork::MeasureText;
        layout.text_extent = DisplayArtwork::MeasureTextExtent;
        return layout;
    }

    RECT SettlementView::Target() const {
        RECT r{};
        GetClientRect(window_, &r);
        if (!resource_error_.empty()) r.bottom = std::max(100L, r.bottom / 2);
        return FitNativeCanvas(r, canvas_);
    }

    void SettlementView::Arrange() {
        if (!ok_) return;
        if (peeking_) {
            for (HWND child: {choices_, details_, ok_, previous_patterns_, next_patterns_}) ShowWindow(child, SW_HIDE);
            return;
        }
        ShowWindow(ok_, SW_SHOW);
        RECT r{};
        GetClientRect(window_, &r);
        const bool details = !resource_error_.empty();
        const bool multiple = sequencer_.ChoiceCount() > 1;
        MoveWindow(choices_, 8, 4, std::max(50L, r.right - 160), 220, TRUE);
        ShowWindow(choices_, multiple || details ? SW_SHOW : SW_HIDE);
        const auto target = Target();
        const double unit = canvas_ == 1440 ? 1.4 : 1.0, fit = double(target.right - target.left) / canvas_;
        const int center = target.left + (target.right - target.left) / 2;
        const auto confirm = SettlementConfirmRect(sequencer_.state(), sequencer_.selected(), canvas_);
        const int x = target.left + static_cast<int>(confirm.left * fit),
                  y = target.top + static_cast<int>(confirm.top * fit);
        const int w = std::max(38, static_cast<int>(56 * unit * fit)),
                  h = std::max(18, static_cast<int>(21 * unit * fit));
        const int bottom = std::min(static_cast<int>(r.bottom) - h - 3, y);
        const auto pages = sequencer_.CurrentPatternPages();
        const int page_y = target.top + static_cast<int>((canvas_ * 3 / 8.0 + 45 * unit) * fit);
        for (HWND child: {previous_patterns_, next_patterns_}) ShowWindow(child, pages > 1 ? SW_SHOW : SW_HIDE);
        MoveWindow(previous_patterns_, center - static_cast<int>(160 * unit * fit), page_y,
                   std::max(50, static_cast<int>(80 * unit * fit)), h, TRUE);
        MoveWindow(next_patterns_, center + static_cast<int>(80 * unit * fit), page_y,
                   std::max(50, static_cast<int>(80 * unit * fit)), h, TRUE);
        EnableWindow(previous_patterns_, !Animating() && sequencer_.pattern_page() > 0);
        EnableWindow(next_patterns_, !Animating() && sequencer_.pattern_page() + 1 < pages);
        MoveWindow(ok_, x, bottom, w, h, TRUE);
        EnableWindow(ok_, sequencer_.confirm_enabled() && !Animating());
        SetWindowTextW(ok_, sequencer_.EnterFinalLabel() ? L"END" : L"OK");
        const int text_top = std::max(65L, r.bottom / 2);
        MoveWindow(details_, 8, text_top, std::max(50L, r.right - 16), std::max(30L, r.bottom - text_top - 8), TRUE);
        ShowWindow(details_, details ? SW_SHOW : SW_HIDE);
    }

    void SettlementView::Confirm() {
        if (!sequencer_.visible() || Animating() || !sequencer_.confirm_enabled() || !IsWindowEnabled(ok_)) return;
        const auto before = sequencer_.selected();
        ApplyEffects(sequencer_.Confirm(GetTickCount64(), IsWindowVisible(window_) != FALSE));
        if (sequencer_.selected() != before && IsWindowVisible(window_)) SetFocus(ok_);
    }

    void SettlementView::ConfirmTick() {
        ApplyEffects(sequencer_.ConfirmTick(GetTickCount64()));
    }

    bool SettlementView::HandleKey(UINT key) {
        if (!sequencer_.visible() || !IsWindowVisible(window_)) return false;
        if (key == VK_NEXT || key == VK_PRIOR) {
            ChangePatternPage(key == VK_NEXT ? 1 : -1);
            return true;
        }
        if (key == VK_RETURN && (GetFocus() == ok_ || GetFocus() == window_)) {
            Confirm();
            return true;
        }
        return false;
    }

    void SettlementView::ChangePatternPage(int step) {
        ApplyEffects(sequencer_.ChangePatternPage(step, GetTickCount64()));
    }

    void SettlementView::Menu(POINT point) {
        const auto menu = CreatePopupMenu();
        const auto &state = sequencer_.state();
        for (std::size_t i = 0; i < sequencer_.ChoiceCount(); ++i) {
            const auto label = i < state.receipts.size() ? Wide(SettlementLabel(state.receipts[i])) : L"最终成绩";
            AppendMenuW(menu, MF_STRING | (i == sequencer_.selected() ? MF_CHECKED : 0), static_cast<UINT_PTR>(10 + i),
                        label.c_str());
        }
        if (point.x == -1 && point.y == -1) {
            point = {20, 20};
            ClientToScreen(window_, &point);
        }
        const auto command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, 0, window_, nullptr);
        DestroyMenu(menu);
        if (command >= 10) {
            SendMessageW(choices_, CB_SETCURSEL, command - 10, 0);
            Select();
            Present();
        }
    }

    void SettlementView::Paint(HDC dc) {
        RECT bounds{};
        GetClientRect(window_, &bounds);
        const int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
        // Render a fresh table when the backdrop is invalidated. Never sample an already dimmed frame.
        try {
            if (background_painter_ && width > 0 && height > 0) {
                if (!backdrop_valid_ || backdrop_w_ != width || backdrop_h_ != height || !backdrop_dc_) {
                    if (backdrop_w_ != width || backdrop_h_ != height || !backdrop_dc_) {
                        InvalidateBackdrop(true);
                        BITMAPINFO info{};
                        info.bmiHeader = {sizeof(BITMAPINFOHEADER), width, -height, 1, 32, BI_RGB, 0, 0, 0, 0, 0};
                        backdrop_dc_ = CreateCompatibleDC(dc);
                        void *pixels = nullptr;
                        backdrop_bitmap_ = backdrop_dc_
                                               ? CreateDIBSection(backdrop_dc_, &info, DIB_RGB_COLORS, &pixels, nullptr, 0)
                                               : nullptr;
                        if (backdrop_dc_ && backdrop_bitmap_) backdrop_old_ = SelectObject(backdrop_dc_, backdrop_bitmap_);
                        if (!backdrop_old_ || backdrop_old_ == HGDI_ERROR) {
                            InvalidateBackdrop(true);
                        } else {
                            backdrop_w_ = width;
                            backdrop_h_ = height;
                        }
                    }
                    if (backdrop_dc_) {
                        const auto brush = CreateSolidBrush(RGB(15, 24, 32));
                        FillRect(backdrop_dc_, &bounds, brush);
                        DeleteObject(brush);
                        background_painter_(backdrop_dc_, bounds);
                        backdrop_valid_ = true;
                    }
                }
                if (backdrop_dc_ && backdrop_valid_) {
                    BitBlt(dc, bounds.left, bounds.top, width, height, backdrop_dc_, 0, 0, SRCCOPY);
                } else {
                    const auto brush = CreateSolidBrush(RGB(15, 24, 32));
                    FillRect(dc, &bounds, brush);
                    DeleteObject(brush);
                    background_painter_(dc, bounds);
                }
            } else {
                const auto brush = CreateSolidBrush(RGB(15, 24, 32));
                FillRect(dc, &bounds, brush);
                DeleteObject(brush);
            }
            if (peeking_) return;
            if (!artwork_.width()) artwork_.Load(directory_, canvas_);
            const auto &state = sequencer_.state();
            artwork_.SetRedTiles(state.red_tiles);
            const auto params = sequencer_.SceneParameters();
            auto layout = Layout();
            const bool win = params.selected < state.receipts.size();
            const auto scene = BuildSettlementScene(state, params.selected, sequencer_.names(), params.shown_yaku,
                                                    params.shown_tiles, params.show_ura, params.show_totals, layout);
            const auto target = Target();
            const float scale = static_cast<float>(target.right - target.left) / scene.width;
            Gdiplus::Graphics graphics(dc);
            graphics.TranslateTransform(static_cast<float>(target.left), static_cast<float>(target.top));
            graphics.ScaleTransform(scale, scale);
            const auto &p = scene.panel;
            Gdiplus::SolidBrush shade(Gdiplus::Color(0xdd000000));
            graphics.FillRectangle(&shade, p.left, p.top, p.right - p.left, p.bottom - p.top);
            const unsigned colors[]{0xffcccccc, 0xffaaaaaa, 0xff888888, 0xff666666};
            for (int side = 0; side < 4; ++side) {
                Gdiplus::Pen edge(Gdiplus::Color(colors[side]), 1);
                if (side == 0) graphics.DrawLine(&edge, p.left, p.top, p.right - 1, p.top);
                if (side == 1) graphics.DrawLine(&edge, p.left, p.top, p.left, p.bottom - 1);
                if (side == 2) graphics.DrawLine(&edge, p.right - 1, p.top, p.right - 1, p.bottom - 1);
                if (side == 3) graphics.DrawLine(&edge, p.left, p.bottom - 1, p.right - 1, p.bottom - 1);
            }
            for (const auto &panel: scene.score_panels) {
                Gdiplus::LinearGradientBrush strip(Gdiplus::Point(panel.left, panel.top),
                                                   Gdiplus::Point(panel.left, panel.bottom),
                                                   Gdiplus::Color(0xff222222),
                                                   Gdiplus::Color(win ? 0x88111111 : 0xff111111));
                graphics.FillRectangle(&strip, panel.left, panel.top, panel.right - panel.left,
                                       panel.bottom - panel.top);
            }
            for (const auto index: SettlementPaintOrder(scene)) {
                const auto &item = scene.tiles[index];
                auto command = item.tile;
                if (command.tile < 0 && command.family == 'k') command.family = 'n';
                const auto &im = artwork_.Get(command, sequencer_.red()).image;
                Gdiplus::Bitmap bitmap(im.width, im.height, im.width * 4, PixelFormat32bppARGB,
                                       const_cast<BYTE *>(im.bgra.data()));
                const auto saved = graphics.Save();
                Gdiplus::Status status;
                if (item.sideways && !item.native_orientation) {
                    graphics.TranslateTransform(static_cast<float>(item.rect.left),
                                                static_cast<float>(item.rect.bottom));
                    graphics.RotateTransform(-90);
                    status = graphics.DrawImage(&bitmap, 0, 0, item.rect.bottom - item.rect.top,
                                                item.rect.right - item.rect.left);
                } else {
                    status = graphics.DrawImage(&bitmap, item.rect.left, item.rect.top,
                                                item.rect.right - item.rect.left, item.rect.bottom - item.rect.top);
                }
                graphics.Restore(saved);
                if (status != Gdiplus::Ok) throw std::runtime_error("Cannot paint settlement tile");
            }
            for (const auto &item: scene.images) {
                const auto &im = display_artwork_.Get(item.id);
                Gdiplus::Bitmap bitmap(im.width, im.height, im.width * 4, PixelFormat32bppARGB,
                                       const_cast<BYTE *>(im.bgra.data()));
                if (graphics.DrawImage(&bitmap, item.x, item.y,
                                       static_cast<int>(im.width * item.scale),
                                       static_cast<int>(im.height * item.scale)) != Gdiplus::Ok)
                    throw std::runtime_error("Cannot paint settlement image");
            }
            for (const auto &item: scene.text) {
                if (!item.visible || !item.spans.empty() || item.entrance <= 0) continue;
                Gdiplus::FontFamily family(item.role == "unknown_yaku" || item.role.rfind("variant_", 0) == 0
                                               ? L"KaiTi"
                                               : item.original_font
                                                     ? L"System"
                                                     : L"Arial");
                Gdiplus::Font font(family.IsAvailable() ? &family : Gdiplus::FontFamily::GenericSansSerif(), item.size,
                                   Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
                const auto alpha = static_cast<unsigned>((item.color >> 24) * item.entrance);
                Gdiplus::SolidBrush ink(Gdiplus::Color((alpha << 24) | (item.color & 0xffffff)));
                const auto value = Wide(item.text);
                Gdiplus::StringFormat format;
                format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
                format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
                format.SetAlignment(item.align == 1
                                        ? Gdiplus::StringAlignmentCenter
                                        : item.align == 2
                                              ? Gdiplus::StringAlignmentFar
                                              : Gdiplus::StringAlignmentNear);
                Gdiplus::RectF rect(static_cast<float>(item.rect.left), static_cast<float>(item.rect.top),
                                    static_cast<float>(item.rect.right - item.rect.left),
                                    static_cast<float>(item.rect.bottom - item.rect.top));
                Gdiplus::Status status;
                const auto saved = graphics.Save();
                if (item.entrance < 1) {
                    const auto zoom = static_cast<float>(2.5 - 1.5 * item.entrance);
                    const auto center_x = rect.X + rect.Width / 2, center_y = rect.Y + rect.Height / 2;
                    graphics.TranslateTransform(center_x, center_y);
                    graphics.ScaleTransform(zoom, zoom);
                    graphics.TranslateTransform(-center_x, -center_y);
                }
                if (item.original_font) {
                    format.SetTrimming(Gdiplus::StringTrimmingNone);
                    format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsNoClip);
                    const float x = item.align == 1
                                        ? rect.X + rect.Width / 2
                                        : item.align == 2
                                              ? rect.X + rect.Width
                                              : rect.X;
                    status = graphics.DrawString(value.c_str(), -1, &font, Gdiplus::PointF(x, rect.Y), &format, &ink);
                } else {
                    status = graphics.DrawString(value.c_str(), -1, &font, rect, &format, &ink);
                }
                graphics.Restore(saved);
                if (status != Gdiplus::Ok) throw std::runtime_error("Cannot paint settlement text");
            }
            graphics.ResetTransform();
            graphics.SetPageUnit(Gdiplus::UnitPixel);
            const bool sd = scene.width == 1024;
            const auto context = TextRenderContext::Viewport(
                scene.width, scene.height,
                Gdiplus::RectF(static_cast<float>(target.left), static_cast<float>(target.top),
                               static_cast<float>(target.right - target.left),
                               static_cast<float>(target.bottom - target.top)),
                sd, GetDpiForWindow(window_));
            for (const auto &item: scene.text) {
                if (!item.visible || item.spans.empty()) continue;
                auto &renderer = display_artwork_.Text();
                for (const auto &run: LayoutSettlementText(renderer, item, sd)) renderer.Draw(graphics, context, run);
            }
            if (sequencer_.confirm_deadline() && sequencer_.confirm_seconds() > 0) {
                RECT button{};
                GetWindowRect(ok_, &button);
                MapWindowPoints(nullptr, window_, reinterpret_cast<POINT *>(&button), 2);
                Gdiplus::Font font(L"Arial", 9, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
                Gdiplus::SolidBrush ink(Gdiplus::Color(255, 255, 255, 255));
                const auto value = std::to_wstring(sequencer_.confirm_seconds());
                graphics.DrawString(value.c_str(), -1, &font,
                                    Gdiplus::PointF(static_cast<float>(button.right + 3),
                                                    static_cast<float>(button.top)), &ink);
            }
            if (!resource_error_.empty()) {
                resource_error_.clear();
                UpdateText();
                Arrange();
            }
        } catch (const std::exception &e) {
            if (resource_error_ != e.what()) {
                resource_error_ = e.what();
                CancelPresentation();
                UpdateText();
                Arrange();
                PostMessageW(GetParent(window_), WM_APP + 16, 0, 0);
            }
            const auto old = SelectObject(dc, font_);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(255, 210, 160));
            RECT hint{8, 35, bounds.right - 8, 90};
            DrawTextW(dc, L"牌桌资源不可用，请修复资源后按 F5 重试。", -1, &hint, DT_LEFT | DT_WORDBREAK);
            SelectObject(dc, old);
        }
    }

    void SettlementView::SetPeekEnabled(bool enabled) {
        peek_enabled_ = enabled;
        if (!enabled) Peek(false);
    }

    void SettlementView::Peek(bool down) {
        if (down && !peek_enabled_) return;
        if (peeking_ == down || !sequencer_.visible()) return;
        peeking_ = down;
        if (down) {
            peek_focus_ = GetFocus();
            SetCapture(window_);
        } else if (GetCapture() == window_) ReleaseCapture();
        Arrange();
        if (!down && peek_focus_ && IsWindowVisible(peek_focus_) && IsChild(window_, peek_focus_))
            SetFocus(peek_focus_);
        InvalidateRect(window_, nullptr, FALSE);
    }

    LRESULT CALLBACK SettlementView::Procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        auto *self = reinterpret_cast<SettlementView *>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = static_cast<SettlementView *>(reinterpret_cast<CREATESTRUCTW *>(lparam)->lpCreateParams);
            self->window_ = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self) {
            if (message == WM_LBUTTONDOWN) {
                self->Peek(true);
                return 0;
            }
            if (message == WM_LBUTTONUP || message == WM_CAPTURECHANGED || message == WM_CANCELMODE) {
                self->Peek(false);
                return 0;
            }
            if (message == WM_ERASEBKGND) return 1;
            if (message == WM_SIZE) {
                self->InvalidateBackdrop();
                self->Arrange();
                return 0;
            }
            if (message == WM_KEYDOWN && self->HandleKey(static_cast<UINT>(wparam))) return 0;
            if (message == WM_CONTEXTMENU) {
                self->Menu({static_cast<short>(LOWORD(lparam)), static_cast<short>(HIWORD(lparam))});
                return 0;
            }
            if (message == WM_COMMAND && LOWORD(wparam) == 5404) {
                self->Confirm();
                return 0;
            }
            if (message == WM_COMMAND && (LOWORD(wparam) == 5405 || LOWORD(wparam) == 5406)) {
                self->ChangePatternPage(LOWORD(wparam) == 5405 ? -1 : 1);
                return 0;
            }
            if (message == WM_COMMAND && LOWORD(wparam) == 5401 && HIWORD(wparam) == CBN_SELCHANGE) {
                self->Select();
                self->Present();
                return 0;
            }
            if (message == WM_TIMER && self->sequencer_.paused()) return 0;
            if (message == WM_TIMER && wparam == 1) {
                self->PresentationTick();
                return 0;
            }
            if (message == WM_TIMER && wparam == 2) {
                self->ConfirmTick();
                return 0;
            }
            if (message == WM_PAINT) {
                try {
                    self->paint_buffer_.Paint(window, [&](HDC dc) { self->Paint(dc); });
                } catch (const std::exception &) {
                }
                return 0;
            }
            if (message == WM_PRINTCLIENT) {
                self->Paint(reinterpret_cast<HDC>(wparam));
                return 0;
            }
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }
} // namespace tenhou
