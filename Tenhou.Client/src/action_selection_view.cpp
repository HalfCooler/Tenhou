#include "action_selection_view.hpp"
#include "network_protocol.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <stdexcept>

namespace tenhou {
ActionSelectionView::ActionSelectionView(HWND parent, std::function<void(std::uint64_t, std::size_t)> submit, std::function<void()> cancel)
    : submit_(std::move(submit)), cancel_(std::move(cancel)) {
    WNDCLASSEXW wc{sizeof(wc)}; wc.hInstance = GetModuleHandleW(nullptr); wc.lpfnWndProc = Procedure;
    wc.lpszClassName = L"TENHOU_REBUILD_ACTION_SELECTION"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) throw std::runtime_error("Cannot register action selection");
    window_ = CreateWindowExW(WS_EX_CONTROLPARENT, wc.lpszClassName, L"", WS_CHILD | WS_CLIPCHILDREN,
        0,0,1,1,parent,reinterpret_cast<HMENU>(5600),wc.hInstance,this);
    font_ = CreateFontW(-14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,DEFAULT_PITCH,L"MS UI Gothic");
    if (!window_ || !font_) throw std::runtime_error("Cannot create action selection");
    for (int i = 0; i < 9; ++i) {
        buttons_[i] = CreateWindowExW(0,L"BUTTON", i == 6 ? L"◀" : i == 7 ? L"▶" : i == 8 ? L"取消 / Esc" : L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | (i < 6 ? BS_OWNERDRAW : BS_PUSHBUTTON),0,0,1,1,window_,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(5601+i)),wc.hInstance,nullptr);
        if (!buttons_[i]) throw std::runtime_error("Cannot create action choice");
        SendMessageW(buttons_[i],WM_SETFONT,reinterpret_cast<WPARAM>(font_),FALSE);
    }
}
ActionSelectionView::~ActionSelectionView() { if (IsWindow(window_)) DestroyWindow(window_); if (font_) DeleteObject(font_); }
void ActionSelectionView::Configure(const std::filesystem::path& directory, int width) { directory_ = directory; width_ = width; artwork_ = {}; InvalidateRect(window_,nullptr,FALSE); }
void ActionSelectionView::Set(const ActionSelection& s, const ActionSnapshot& a, const OnlineState& game, bool visible) {
    if (!visible || s.revision != selection_.revision || s.group != selection_.group) {
        const auto capture = GetCapture();
        if (capture && (capture == window_ || IsChild(window_,capture))) {
            SendMessageW(capture,WM_CANCELMODE,0,0); ReleaseCapture();
        }
        for (auto button : buttons_) SendMessageW(button,BM_SETSTATE,FALSE,0);
        if (s.revision != selection_.revision || s.group != selection_.group) page_ = 0;
    }
    selection_ = s; actions_ = a; game_ = game; red_ = (game.type&2)==0;
    const bool show = visible && s.Active() && s.group != "riichi" && !s.HandSelection();
    ShowWindow(window_,show ? SW_SHOW : SW_HIDE); Arrange(); InvalidateRect(window_,nullptr,FALSE);
}
void ActionSelectionView::Move(RECT r) {
    const int w = std::min(720L,std::max(1L,r.right-r.left-16));
    const int h = std::min(270L,std::max(1L,r.bottom-r.top-16));
    MoveWindow(window_,r.left+(r.right-r.left-w)/2,r.top+(r.bottom-r.top-h)/2,w,h,TRUE);
}
void ActionSelectionView::Focus() { if (IsWindowVisible(window_)) SetFocus(buttons_[0]); }
void ActionSelectionView::Arrange() {
    if (!buttons_[0]) return;
    RECT r{}; GetClientRect(window_,&r);
    const int w = std::max(1L,(r.right-24)/3), h = std::max(1L,(r.bottom-70)/2);
    for (int i = 0; i < 6; ++i) {
        const auto n = page_*6+i; const bool valid = n < selection_.indices.size();
        ShowWindow(buttons_[i],valid ? SW_SHOW : SW_HIDE); EnableWindow(buttons_[i],valid);
        MoveWindow(buttons_[i],8+(i%3)*w,30+(i/3)*h,w-4,h-4,TRUE);
        if (valid) SetWindowTextW(buttons_[i],NetworkWide(actions_.choices.at(selection_.indices[n]).label).c_str());
    }
    MoveWindow(buttons_[6],8,r.bottom-32,60,26,TRUE); MoveWindow(buttons_[7],74,r.bottom-32,60,26,TRUE);
    MoveWindow(buttons_[8],std::max(140L,r.right-128),r.bottom-32,120,26,TRUE);
    EnableWindow(buttons_[6],page_>0); EnableWindow(buttons_[7],(page_+1)*6<selection_.indices.size());
}
void ActionSelectionView::Command(int id) {
    if (id == 5609) { cancel_(); return; }
    if (id == 5607 && page_) --page_;
    else if (id == 5608 && (page_+1)*6 < selection_.indices.size()) ++page_;
    else if (id >= 5601 && id <= 5606) {
        const auto n = page_*6+id-5601;
        if (n < selection_.indices.size()) submit_(selection_.revision,selection_.indices[n]);
        return;
    }
    Arrange(); Focus(); InvalidateRect(window_,nullptr,FALSE);
}
void ActionSelectionView::Paint(HDC dc) {
    RECT r{}; GetClientRect(window_,&r); const auto brush = CreateSolidBrush(RGB(20,40,40)); FillRect(dc,&r,brush); DeleteObject(brush);
    const auto old = SelectObject(dc,font_); SetBkMode(dc,TRANSPARENT); SetTextColor(dc,RGB(255,244,210));
    const auto name = selection_.group == "chi" ? L"吃" : selection_.group == "pon" ? L"碰" : L"杠";
    const auto text = std::wstring(name)+L"  —  "+std::to_wstring(page_+1)+L" / "+std::to_wstring(std::max<std::size_t>(1,(selection_.indices.size()+5)/6));
    r.left += 8; r.top += 5; r.bottom = 28; DrawTextW(dc,text.c_str(),-1,&r,DT_SINGLELINE); SelectObject(dc,old);
}
void ActionSelectionView::PaintChoice(const DRAWITEMSTRUCT& item) {
    const auto n = page_*6+item.CtlID-5601; if (n >= selection_.indices.size()) return;
    const auto& a = actions_.choices.at(selection_.indices[n]); RECT r = item.rcItem;
    const auto brush = CreateSolidBrush(item.itemState & ODS_SELECTED ? RGB(62,100,85) : RGB(38,67,62)); FillRect(item.hDC,&r,brush); DeleteObject(brush);
    const auto old = SelectObject(item.hDC,font_); SetBkMode(item.hDC,TRANSPARENT); SetTextColor(item.hDC,RGB(250,245,220));
    try {
        if (!artwork_.width()) artwork_.Load(directory_,width_);
        Gdiplus::Graphics graphics(item.hDC);
        const auto tiles = BuildCandidateTiles(a,game_);
        const int w = std::min(36L,(r.right-16)/std::max<int>(1,static_cast<int>(tiles.size()))), h = std::min(54L,r.bottom-28);
        for (std::size_t i = 0; i < tiles.size(); ++i) {
            const auto& im = artwork_.Get({'k',0,tiles[i].tile,0},red_).image;
            Gdiplus::Bitmap bitmap(im.width,im.height,im.width*4,PixelFormat32bppARGB,const_cast<BYTE*>(im.bgra.data()));
            graphics.DrawImage(&bitmap,8+static_cast<int>(i)*w,3,w,h);
            if (tiles[i].context) { Gdiplus::Pen gold(Gdiplus::Color(255,255,210,100),2); graphics.DrawRectangle(&gold,8+static_cast<int>(i)*w,3,w,h); }
        }
        std::wstring ids;
        for (int tile : a.tiles) { if (!ids.empty()) ids+=L","; ids+=std::to_wstring(tile); }
        RECT label{6,r.bottom-24,r.right-4,r.bottom}; DrawTextW(item.hDC,ids.c_str(),-1,&label,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
    } catch (const std::exception&) { const auto text = NetworkWide(a.label); DrawTextW(item.hDC,text.c_str(),-1,&r,DT_WORDBREAK); }
    SelectObject(item.hDC,old);
    if (item.itemState & ODS_FOCUS) { InflateRect(&r,-2,-2); DrawFocusRect(item.hDC,&r); }
}
void ActionSelectionView::Capture(HDC dc) {
    if (!IsWindowVisible(window_)) return;
    POINT point{}; MapWindowPoints(window_,GetParent(window_),&point,1);
    const int saved = SaveDC(dc); SetViewportOrgEx(dc,point.x,point.y,nullptr);
    SendMessageW(window_,WM_PRINT,reinterpret_cast<WPARAM>(dc),PRF_CLIENT|PRF_CHILDREN); RestoreDC(dc,saved);
}
LRESULT CALLBACK ActionSelectionView::Procedure(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    auto* self = reinterpret_cast<ActionSelectionView*>(GetWindowLongPtrW(w,GWLP_USERDATA));
    if (m == WM_NCCREATE) { self = static_cast<ActionSelectionView*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams); self->window_=w; SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self)); }
    if (self) {
        if (m == WM_ERASEBKGND) return 1;
        if (m == WM_SIZE) { self->Arrange(); return 0; }
        if (m == WM_COMMAND) { self->Command(LOWORD(wp)); return 0; }
        if (m == WM_DRAWITEM) { self->PaintChoice(*reinterpret_cast<DRAWITEMSTRUCT*>(lp)); return TRUE; }
        if (m == WM_PAINT) { self->buffer_.Paint(w,[&](HDC dc) { self->Paint(dc); }); return 0; }
        if (m == WM_PRINTCLIENT) { self->Paint(reinterpret_cast<HDC>(wp)); return 0; }
    }
    return DefWindowProcW(w,m,wp,lp);
}
} // namespace tenhou
