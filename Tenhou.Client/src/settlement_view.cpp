#include "settlement_view.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <stdexcept>

namespace tenhou {
namespace {
std::wstring Wide(const std::string& value) {
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring text(size, L' ');
    if (size) MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), text.data(), size);
    return text;
}
}
SettlementView::SettlementView(HWND parent) {
    WNDCLASSEXW wc{sizeof(wc)}; wc.hInstance = GetModuleHandleW(nullptr); wc.lpfnWndProc = Procedure;
    wc.lpszClassName = L"TENHOU_REBUILD_SETTLEMENT"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) throw std::runtime_error("Cannot register settlement view");
    font_ = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"MS UI Gothic");
    window_ = CreateWindowExW(WS_EX_CONTROLPARENT, wc.lpszClassName, L"Settlement", WS_CHILD | WS_CLIPCHILDREN,
        0, 0, 1, 1, parent, reinterpret_cast<HMENU>(5400), wc.hInstance, this);
    if (!font_ || !window_) throw std::runtime_error("Cannot create settlement view");
    choices_ = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        0, 0, 1, 1, window_, reinterpret_cast<HMENU>(5401), wc.hInstance, nullptr);
    details_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE |
        ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL, 0, 0, 1, 1, window_, reinterpret_cast<HMENU>(5402), wc.hInstance, nullptr);
    if (!choices_ || !details_) throw std::runtime_error("Cannot create settlement controls");
    for (HWND control : {choices_, details_}) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font_), FALSE);
    SendMessageW(details_, EM_LIMITTEXT, 65536, 0);
    ok_ = CreateWindowExW(0,L"BUTTON",L"OK",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
        0,0,1,1,window_,reinterpret_cast<HMENU>(5404),wc.hInstance,nullptr);
    if (!ok_) throw std::runtime_error("Cannot create settlement confirmation");
    SendMessageW(ok_,WM_SETFONT,reinterpret_cast<WPARAM>(font_),FALSE);
}
SettlementView::~SettlementView() { if (IsWindow(window_)) DestroyWindow(window_); if (font_) DeleteObject(font_); }
void SettlementView::SetSoundEnabled(bool enabled) {
    sound_enabled_ = enabled;
    if (!enabled && !animation_enabled_) CancelPresentation();
}
void SettlementView::SetAnimationEnabled(bool enabled) {
    animation_enabled_ = enabled;
    if (!enabled && !sound_enabled_) CancelPresentation();
    UpdateText(); Arrange(); InvalidateRect(window_,nullptr,FALSE);
}
void SettlementView::Configure(const std::filesystem::path& directory, int width) {
    display_artwork_.Configure(directory, width);
    directory_ = directory; canvas_ = width; artwork_ = {}; resource_error_.clear();
    artwork_.SetColor(tile_color_);
    if (choices_) Select();
    InvalidateRect(window_, nullptr, FALSE);
}
void SettlementView::Show(bool visible) {
    const bool opening = visible && !visible_, changed=visible!=visible_; visible_ = visible;
    if (!visible && peeking_) { peeking_=false; if (GetCapture()==window_) ReleaseCapture(); }
    ShowWindow(window_, visible ? SW_SHOW : SW_HIDE);
    if (!visible) CancelPresentation(); else if (opening) { Present(); SetFocus(ok_); }
    if (changed) Arrange();
}
void SettlementView::Move(RECT r) { MoveWindow(window_, r.left, r.top, std::max(1L, r.right - r.left), std::max(1L, r.bottom - r.top), TRUE); }
void SettlementView::Capture(HDC dc) {
    if (!IsWindowVisible(window_)) return;
    POINT origin{}; ClientToScreen(GetParent(window_), &origin); RECT r{}; GetWindowRect(window_, &r);
    const int saved = SaveDC(dc); SetViewportOrgEx(dc, r.left - origin.x, r.top - origin.y, nullptr);
    SendMessageW(window_, WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT | PRF_CHILDREN | PRF_ERASEBKGND);
    RestoreDC(dc, saved);
}
void SettlementView::Set(const SettlementState& s, const std::array<std::string, 4>& names, bool red) {
    if (SettlementJson(state_) == SettlementJson(s) && state_.red_tiles==s.red_tiles && names_ == names && red_ == red) return;
    const bool was_final = state_.has_final && selected_ == state_.receipts.size();
    const bool new_round = s.receipts.size() < state_.receipts.size() || (!s.receipts.empty() && !state_.receipts.empty() &&
        (s.receipts[0].tag != state_.receipts[0].tag || s.receipts[0].base != state_.receipts[0].base || s.receipts[0].hand != state_.receipts[0].hand));
    state_ = s; names_ = names; red_ = red;
    artwork_.SetRedTiles(s.red_tiles);
    if (new_round) selected_ = 0;
    if (was_final && s.has_final) selected_ = s.receipts.size();
    SendMessageW(choices_, CB_RESETCONTENT, 0, 0);
    for (const auto& r : s.receipts) {
        const auto text = Wide(SettlementLabel(r)); SendMessageW(choices_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
    }
    if (s.has_final) SendMessageW(choices_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Match results | supplied scores / points"));
    const auto count = s.receipts.size() + (s.has_final ? 1 : 0);
    selected_ = count ? std::min(selected_, count - 1) : 0;
    SendMessageW(choices_, CB_SETCURSEL, selected_, 0); Select();
}
void SettlementView::Select() {
    CancelPresentation();
    const auto selected = SendMessageW(choices_, CB_GETCURSEL, 0, 0);
    if (selected >= 0) selected_ = static_cast<std::size_t>(selected);
    UpdateText(); Arrange(); InvalidateRect(window_, nullptr, FALSE);
    PostMessageW(GetParent(window_),WM_APP+18,0,0);
}
void SettlementView::UpdateText() {
    auto displayed = state_;
    for (auto& receipt : displayed.receipts) receipt.yaku = SettlementDisplayYaku(receipt);
    const bool animate = animation_enabled_ && resource_error_.empty();
    if (animate && selected_ < displayed.receipts.size()) {
        auto& receipt = displayed.receipts[selected_];
        if (shown_yaku_ < receipt.yaku.size()) receipt.yaku.resize(shown_yaku_);
        if (shown_yaku_ < receipt.yakuman.size()) receipt.yakuman.resize(shown_yaku_);
        if (UraPending()) receipt.ura.clear();
        if (receipt.who < 0 && shown_tiles_ != static_cast<std::size_t>(-1)) {
            std::size_t visible = shown_tiles_ > PreviewTiles() ? shown_tiles_ - PreviewTiles() : 0;
            for (int i = 0; i < 4; ++i) if (receipt.has_revealed[i]) {
                const auto count = std::min(visible, receipt.revealed[i].size());
                visible -= count; receipt.revealed[i].resize(count);
                if (!count) receipt.has_revealed[i] = false;
            }
        }
    }
    auto text = SettlementText(displayed, selected_, names_, red_, !animate || Complete());
    if (animate && UraPending()) {
        const auto at = text.find("Ura indicators: not supplied");
        if (at != std::string::npos) text.replace(at, 28, "Ura indicators: awaiting reveal");
    }
    if (!resource_error_.empty()) text = "Tile artwork unavailable; text remains complete.\r\n" + text;
    SetWindowTextW(details_, Wide(text).c_str());
}
std::size_t SettlementView::PreviewTiles() const {
    return 0; // Draw hands are resolved on the table before this popup.
}
std::size_t SettlementView::YakuCount() const {
    if (selected_ >= state_.receipts.size()) return 0;
    const auto& r = state_.receipts[selected_];
    return r.yakuman.empty() ? SettlementDisplayYaku(r).size() : r.yakuman.size();
}
bool SettlementView::UraPending() const {
    return !ura_revealed_ && selected_ < state_.receipts.size() && !state_.receipts[selected_].ura.empty();
}
bool SettlementView::Complete() const { return totals_revealed_ && shown_yaku_ >= YakuCount() && shown_tiles_ >= PreviewTiles() * 2 && !UraPending(); }
void SettlementView::ArmPresentationTimer() {
    KillTimer(window_, 1);
    if (Complete() || selected_ >= state_.receipts.size()) return;
    const auto& r = state_.receipts[selected_];
    // Keep native timer CALLBACK counts. A 10ms Win32 timer is commonly
    // delivered every ~15.6ms; replacing 55 callbacks with 550ms speeds it up.
    const unsigned delay = r.who < 0 ? 80 : 10;
    if (!SetTimer(window_, 1, delay, nullptr)) CancelPresentation();
}
void SettlementView::CancelPresentation() {
    KillTimer(window_, 1);
    KillTimer(window_, 2); confirm_deadline_ = 0; confirm_seconds_ = -1; confirm_ready_ = false;
    presentation_frames_ = 0; totals_revealed_ = true; ura_pause_done_ = false;
    if ((shown_yaku_ != static_cast<std::size_t>(-1) || shown_tiles_ != static_cast<std::size_t>(-1)) && stop_sound_) stop_sound_();
    shown_yaku_ = shown_tiles_ = static_cast<std::size_t>(-1);
    ura_revealed_ = true;
    if (details_) { UpdateText();Arrange(); }
}
void SettlementView::Present() {
    CancelPresentation();
    if (!visible_ || !IsWindowVisible(window_)) return;
    confirm_ready_ = true;
    if ((!animation_enabled_ && !sound_enabled_) || selected_ >= state_.receipts.size()) { Arrange(); return; }
    const auto& r = state_.receipts[selected_];
    if (r.who<0) { Arrange(); return; }
    shown_yaku_ = 0;
    presentation_frames_ = 30; totals_revealed_ = r.who < 0;
    ura_revealed_ = r.ura.empty();
    shown_tiles_ = r.who < 0 ? 0 : static_cast<std::size_t>(-1);
    if (sound_enabled_ && r.who >= 0 && r.yakuman.empty() && play_sound_) play_sound_(101);
    UpdateText(); Arrange(); InvalidateRect(window_, nullptr, FALSE);
    ArmPresentationTimer();
}
void SettlementView::PresentationTick() {
    if (!visible_ || !IsWindowVisible(window_) || selected_ >= state_.receipts.size()) { CancelPresentation(); return; }
    const auto& r = state_.receipts[selected_];
    if (r.who>=0 && presentation_frames_) {
        if (--presentation_frames_) return;
    }
    const auto yaku = SettlementDisplayYaku(r);
    const bool next_ura = r.yakuman.empty() && shown_yaku_ && shown_yaku_<yaku.size() && yaku[shown_yaku_].id==53;
    if (next_ura && !ura_pause_done_) {
        // Native frame 54, 004ad739: an additional 1200ms before ura frame 0.
        ura_pause_done_=true; SetTimer(window_,1,1200,nullptr); return;
    }
    if (shown_yaku_ < YakuCount()) {
        if (!r.yakuman.empty()) { shown_yaku_ = r.yakuman.size(); if (sound_enabled_ && play_sound_) play_sound_(111); }
        else {
            if (SettlementDisplayYaku(r)[shown_yaku_].id == 53) ura_revealed_ = true;
            ++shown_yaku_; if (sound_enabled_ && play_sound_) play_sound_(109);
        }
        ura_pause_done_=false;
        if (shown_yaku_>=YakuCount()) presentation_frames_=12;
        else presentation_frames_=r.yakuman.empty() && yaku[shown_yaku_].id==53 ? 54 : 55;
    } else if (UraPending()) {
        // Even zero-hit ura indicators must turn face up after a riichi win.
        ura_revealed_ = true;
        if (sound_enabled_ && play_sound_) play_sound_(104);
    } else totals_revealed_=true;
    if (shown_tiles_ < PreviewTiles() * 2) { ++shown_tiles_; if (sound_enabled_ && play_sound_) play_sound_(104); }
    UpdateText(); Arrange(); InvalidateRect(window_, nullptr, FALSE);
    ArmPresentationTimer();
    if (Complete()) PostMessageW(GetParent(window_),WM_APP+16,0,0);
}
void SettlementView::SetContext(int dealer, int honba, int sticks) {
    if (dealer_==dealer && honba_==honba && sticks_==sticks) return;
    dealer_ = dealer; honba_ = honba; sticks_ = sticks;
    InvalidateRect(window_,nullptr,FALSE);
}
SettlementLayout SettlementView::Layout() {
    SettlementLayout layout; layout.width=canvas_; layout.dealer=dealer_; layout.honba=honba_; layout.sticks=sticks_;
    layout.tile_bounds = [this](const TableTile& tile) {
        const auto& item=artwork_.Get(tile,red_);
        return RECT{item.x,item.y,item.x+item.image.width,item.y+item.image.height};
    };
    layout.image_size = [this](unsigned id) { const auto& im=display_artwork_.Get(id);return SIZE{im.width,im.height}; };
    layout.text_width=DisplayArtwork::MeasureText;
    layout.text_extent=DisplayArtwork::MeasureTextExtent;
    return layout;
}
RECT SettlementView::Target() const {
    RECT r{}; GetClientRect(window_,&r);
    // Details are an explicit inspection mode; normal results use the whole table.
    if (text_details_ || !resource_error_.empty()) r.bottom=std::max(100L,r.bottom/2);
    return FitNativeCanvas(r,canvas_);
}
void SettlementView::Arrange() {
    if (!ok_) return;
    if (peeking_) { for (HWND child : {choices_,details_,ok_}) ShowWindow(child,SW_HIDE); return; }
    ShowWindow(ok_,SW_SHOW);
    RECT r{};GetClientRect(window_,&r);
    const bool details=text_details_ || !resource_error_.empty();
    const bool multiple=state_.receipts.size()+(state_.has_final?1:0)>1;
    MoveWindow(choices_,8,4,std::max(50L,r.right-160),220,TRUE);
    ShowWindow(choices_,multiple || details ? SW_SHOW : SW_HIDE);
    const auto target=Target();
    const bool win=selected_<state_.receipts.size();
    const double unit=canvas_==1440?1.4:1.0, fit=double(target.right-target.left)/canvas_;
    const int center=target.left+(target.right-target.left)/2;
    const int x=center+static_cast<int>((win?-23:-28)*unit*fit), y=target.top+static_cast<int>((canvas_*3/8.0+271*unit)*fit);
    // Same native rectangle as SettlementScene::confirm, including centering.
    const int w=std::max(38,static_cast<int>(56*unit*fit)), h=std::max(18,static_cast<int>(21*unit*fit));
    const int bottom=std::min(static_cast<int>(r.bottom)-h-3,y);
    const int final_top=(canvas_*3/4-static_cast<int>(480*unit))/2;
    MoveWindow(ok_,x,win?bottom:target.top+static_cast<int>((final_top+430*unit)*fit),w,h,TRUE);
    EnableWindow(ok_,confirm_enabled_ && !Animating());
    const bool next = continuation_ && !state_.has_final;
    const bool enter_final = state_.has_final && selected_+1 == state_.receipts.size();
    SetWindowTextW(ok_,enter_final ? L"END" : L"OK");
    if (!next) { KillTimer(window_,2); confirm_deadline_=0; confirm_seconds_=-1; }
    if (next && visible_ && confirm_ready_ && confirm_enabled_ && !Animating() && confirm_seconds_ < 0) {
        confirm_seconds_ = 5; confirm_deadline_ = GetTickCount64()+5000;
        SetTimer(window_,2,100,nullptr);
        if (sound_enabled_ && play_sound_) play_sound_(102);
    }
    const int text_top=std::max(65L,r.bottom/2);
    MoveWindow(details_,8,text_top,std::max(50L,r.right-16),std::max(30L,r.bottom-text_top-8),TRUE);
    ShowWindow(details_,details?SW_SHOW:SW_HIDE);
}
void SettlementView::Confirm() {
    if (!visible_ || Animating() || !confirm_enabled_ || !IsWindowEnabled(ok_)) return;
    KillTimer(window_,2); confirm_deadline_=0;
    InvalidateRect(window_,nullptr,FALSE);
    const auto count=state_.receipts.size()+(state_.has_final?1:0);
    if (selected_+1<count) {
        SendMessageW(choices_,CB_SETCURSEL,++selected_,0);Select();Present();SetFocus(ok_);
    } else if (confirm_handler_) confirm_handler_();
}
void SettlementView::ConfirmTick() {
    if (!visible_ || !confirm_deadline_) return;
    const auto now = GetTickCount64();
    const int seconds = now>=confirm_deadline_ ? 0 : static_cast<int>((confirm_deadline_-now+999)/1000);
    if (!seconds) {
        KillTimer(window_,2); confirm_deadline_=0; Confirm(); return;
    }
    if (seconds!=confirm_seconds_) {
        confirm_seconds_=seconds; InvalidateRect(window_,nullptr,FALSE);
        if (sound_enabled_ && play_sound_) play_sound_(102);
    }
}
bool SettlementView::HandleKey(UINT key) {
    if (!visible_ || !IsWindowVisible(window_)) return false;
    if (key==VK_RETURN && (GetFocus()==ok_ || GetFocus()==window_)) { Confirm();return true; }
    return false;
}
void SettlementView::Menu(POINT point) {
    const auto menu=CreatePopupMenu();
    for (std::size_t i=0;i<state_.receipts.size()+(state_.has_final?1:0);++i) {
        const auto label=i<state_.receipts.size()?Wide(SettlementLabel(state_.receipts[i])):L"最终成绩";
        AppendMenuW(menu,MF_STRING|(i==selected_?MF_CHECKED:0),static_cast<UINT_PTR>(10+i),label.c_str());
    }
    if (point.x==-1 && point.y==-1) { point={20,20};ClientToScreen(window_,&point); }
    const auto command=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,0,window_,nullptr);
    DestroyMenu(menu);
    if (command>=10) { SendMessageW(choices_,CB_SETCURSEL,command-10,0);Select();Present(); }
}
void SettlementView::Paint(HDC dc) {
    RECT bounds{}; GetClientRect(window_, &bounds);
    const auto brush=CreateSolidBrush(RGB(15,24,32));FillRect(dc,&bounds,brush);DeleteObject(brush);
    // Render a fresh table on every frame. Never sample an already dimmed frame.
    try {
        if (background_painter_) background_painter_(dc,bounds);
        if (peeking_) return;
        if (!artwork_.width()) artwork_.Load(directory_,canvas_);
        artwork_.SetRedTiles(state_.red_tiles);
        const auto all=static_cast<std::size_t>(-1);
        auto layout=Layout();
        const bool win=selected_<state_.receipts.size();
        const auto scene=BuildSettlementScene(state_,selected_,names_,animation_enabled_?shown_yaku_:all,
            animation_enabled_?shown_tiles_:all,!animation_enabled_||!UraPending(),!animation_enabled_||Complete(),layout);
        const auto target=Target();
        const float scale=static_cast<float>(target.right-target.left)/scene.width;
        Gdiplus::Graphics graphics(dc);graphics.TranslateTransform(static_cast<float>(target.left),static_cast<float>(target.top));
        graphics.ScaleTransform(scale,scale);
        const auto& p=scene.panel;
        Gdiplus::SolidBrush shade(Gdiplus::Color(0xdd000000));
        graphics.FillRectangle(&shade,p.left,p.top,p.right-p.left,p.bottom-p.top);
        const unsigned colors[]{0xffcccccc,0xffaaaaaa,0xff888888,0xff666666};
        for (int side=0;side<4;++side) {
            Gdiplus::Pen edge(Gdiplus::Color(colors[side]),1);
            if (side==0) graphics.DrawLine(&edge,p.left,p.top,p.right-1,p.top);
            if (side==1) graphics.DrawLine(&edge,p.left,p.top,p.left,p.bottom-1);
            if (side==2) graphics.DrawLine(&edge,p.right-1,p.top,p.right-1,p.bottom-1);
            if (side==3) graphics.DrawLine(&edge,p.left,p.bottom-1,p.right-1,p.bottom-1);
        }
        for (const auto& panel : scene.score_panels) {
            Gdiplus::LinearGradientBrush strip(Gdiplus::Point(panel.left,panel.top),Gdiplus::Point(panel.left,panel.bottom),
                Gdiplus::Color(0xff222222),Gdiplus::Color(win?0x88111111:0xff111111));
            graphics.FillRectangle(&strip,panel.left,panel.top,panel.right-panel.left,panel.bottom-panel.top);
        }
        for (const auto index : SettlementPaintOrder(scene)) {
            const auto& item=scene.tiles[index];
            auto command=item.tile;
            if (command.tile<0 && command.family=='k') command.family='n';
            const auto& im=artwork_.Get(command,red_).image;
            Gdiplus::Bitmap bitmap(im.width,im.height,im.width*4,PixelFormat32bppARGB,const_cast<BYTE*>(im.bgra.data()));
            const auto saved=graphics.Save();
            Gdiplus::Status status;
            if (item.sideways && !item.native_orientation) {
                graphics.TranslateTransform(static_cast<float>(item.rect.left),static_cast<float>(item.rect.bottom));
                graphics.RotateTransform(-90);
                status=graphics.DrawImage(&bitmap,0,0,item.rect.bottom-item.rect.top,item.rect.right-item.rect.left);
            } else status=graphics.DrawImage(&bitmap,item.rect.left,item.rect.top,item.rect.right-item.rect.left,item.rect.bottom-item.rect.top);
            graphics.Restore(saved);
            if (status!=Gdiplus::Ok) throw std::runtime_error("Cannot paint settlement tile");
        }
        const double resource_scale=1;
        for (const auto& item : scene.images) {
            const auto& im=display_artwork_.Get(item.id);
            Gdiplus::Bitmap bitmap(im.width,im.height,im.width*4,PixelFormat32bppARGB,const_cast<BYTE*>(im.bgra.data()));
            if (graphics.DrawImage(&bitmap,item.x,item.y,static_cast<int>(im.width*item.scale/resource_scale),static_cast<int>(im.height*item.scale/resource_scale))!=Gdiplus::Ok)
                throw std::runtime_error("Cannot paint settlement label");
        }
        for (const auto& item : scene.text) {
            if (!item.visible) continue;
            Gdiplus::FontFamily family(item.role=="unknown_yaku"?L"KaiTi":item.original_font?L"System":L"Arial");
            Gdiplus::Font font(family.IsAvailable()?&family:Gdiplus::FontFamily::GenericSansSerif(),item.size,Gdiplus::FontStyleRegular,Gdiplus::UnitPixel);
            Gdiplus::SolidBrush ink(Gdiplus::Color(item.color));
            const auto value=Wide(item.text);
            Gdiplus::StringFormat format;format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
            format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
            format.SetAlignment(item.align==1?Gdiplus::StringAlignmentCenter:item.align==2?Gdiplus::StringAlignmentFar:Gdiplus::StringAlignmentNear);
            Gdiplus::RectF rect(static_cast<float>(item.rect.left),static_cast<float>(item.rect.top),
                static_cast<float>(item.rect.right-item.rect.left),static_cast<float>(item.rect.bottom-item.rect.top));
            Gdiplus::Status status;
            if (item.original_font) {
                // FUN_004b3360 uses zero-width point anchors. A clipped layout
                // rectangle adds GDI+ padding and truncates even measured scores.
                format.SetTrimming(Gdiplus::StringTrimmingNone);
                format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap|Gdiplus::StringFormatFlagsNoClip);
                const float x=item.align==1?rect.X+rect.Width/2:item.align==2?rect.X+rect.Width:rect.X;
                status=graphics.DrawString(value.c_str(),-1,&font,Gdiplus::PointF(x,rect.Y),&format,&ink);
            } else status=graphics.DrawString(value.c_str(),-1,&font,rect,&format,&ink);
            if (status!=Gdiplus::Ok) throw std::runtime_error("Cannot paint settlement text");
        }
        if (confirm_deadline_ && confirm_seconds_>0) {
            // Draw with the parent frame so repeated paints cannot accumulate
            // ClearType edges from a transparent child window.
            graphics.ResetTransform();
            RECT button{};GetWindowRect(ok_,&button);MapWindowPoints(nullptr,window_,reinterpret_cast<POINT*>(&button),2);
            Gdiplus::Font font(L"Arial",9,Gdiplus::FontStyleRegular,Gdiplus::UnitPixel);
            Gdiplus::SolidBrush ink(Gdiplus::Color(255,255,255,255));
            const auto value=std::to_wstring(confirm_seconds_);
            graphics.DrawString(value.c_str(),-1,&font,Gdiplus::PointF(static_cast<float>(button.right+3),static_cast<float>(button.top)),&ink);
        }
        if (!resource_error_.empty()) { resource_error_.clear(); UpdateText(); Arrange(); }
    } catch (const std::exception& e) {
        if (resource_error_!=e.what()) {
            resource_error_=e.what();CancelPresentation();UpdateText();Arrange();
            PostMessageW(GetParent(window_),WM_APP+16,0,0);
        }
        const auto old=SelectObject(dc,font_);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,210,160));
        RECT hint{8,35,bounds.right-8,90};
        DrawTextW(dc,L"牌桌资源不可用，请修复资源后按 F5 重试。",-1,&hint,DT_LEFT|DT_WORDBREAK);
        SelectObject(dc,old);
    }
}
void SettlementView::SetPeekEnabled(bool enabled) {
    peek_enabled_ = enabled;
    if (!enabled) Peek(false);
}
void SettlementView::Peek(bool down) {
    if (down && !peek_enabled_) return;
    if (peeking_==down || !visible_) return;
    peeking_=down;
    if (down) { peek_focus_=GetFocus(); SetCapture(window_); }
    else if (GetCapture()==window_) ReleaseCapture();
    Arrange();
    if (!down && peek_focus_ && IsWindowVisible(peek_focus_) && IsChild(window_,peek_focus_)) SetFocus(peek_focus_);
    InvalidateRect(window_,nullptr,FALSE);
}
LRESULT CALLBACK SettlementView::Procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* self = reinterpret_cast<SettlementView*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) { self = static_cast<SettlementView*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams); self->window_ = window; SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self)); }
    if (self) {
        if (message==WM_LBUTTONDOWN) { self->Peek(true); return 0; }
        if (message==WM_LBUTTONUP || message==WM_CAPTURECHANGED || message==WM_CANCELMODE) { self->Peek(false); return 0; }
        if (message == WM_ERASEBKGND) return 1; // Paint supplies the complete background.
        if (message == WM_SIZE) { self->Arrange(); return 0; }
        if (message == WM_KEYDOWN && self->HandleKey(static_cast<UINT>(wparam))) return 0;
        if (message == WM_CONTEXTMENU) { self->Menu({static_cast<short>(LOWORD(lparam)),static_cast<short>(HIWORD(lparam))});return 0; }
        if (message == WM_COMMAND && LOWORD(wparam) == 5404) { self->Confirm();return 0; }
        if (message == WM_COMMAND && LOWORD(wparam) == 5401 && HIWORD(wparam) == CBN_SELCHANGE) { self->Select(); self->Present(); return 0; }
        if (message == WM_TIMER && self->paused_at_) return 0;
        if (message == WM_TIMER && wparam == 1) { self->PresentationTick(); return 0; }
        if (message == WM_TIMER && wparam == 2) { self->ConfirmTick(); return 0; }
        if (message == WM_PAINT) {
            try { self->paint_buffer_.Paint(window, [&](HDC dc) { self->Paint(dc); }); }
            catch (const std::exception&) { /* Keep the last complete frame if buffering fails. */ }
            return 0;
        }
        if (message == WM_PRINTCLIENT) { self->Paint(reinterpret_cast<HDC>(wparam)); return 0; }
    }
    return DefWindowProcW(window, message, wparam, lparam);
}
} // namespace tenhou
