#include "original_display.hpp"
#include "table_scene.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tenhou {
bool HudInteraction::Handle(HWND window, RECT board, int width, UINT message, LPARAM point) {
    if (message == WM_TIMER) {
        KillTimer(window,9);
        if (inside_) return false;
        const bool changed=relative_; relative_=false; return changed;
    }
    bool inside=false;
    if (message != WM_MOUSELEAVE) {
        const auto target=FitNativeCanvas(board,width);
        const double fit=double(target.right-target.left)/width;
        const auto hud=BuildOriginalHud({},width);
        const double x=(static_cast<short>(LOWORD(point))-target.left)/fit;
        const double y=(static_cast<short>(HIWORD(point))-target.top)/fit;
        inside=x>hud.panel[0]+7 && x<hud.panel[0]+hud.panel[2]-7 &&
            y>hud.panel[1] && y<hud.panel[1]+hud.panel[3];
    }
    const bool changed=inside && !relative_;
    if (inside) { KillTimer(window,9); relative_=true; }
    else if (inside_) SetTimer(window,9,750,nullptr);
    inside_=inside; return changed;
}
RECT OriginalAutoButtonRect(int width, int index) {
    // 0047d657..0047d77a: right offsets 2.15,3.15,4.15,5.15.
    // Native controls are 80x22 (1024) and 110x37 (1440).
    const int bw = width == 1440 ? 110 : 80, bh = width == 1440 ? 37 : 22;
    const int stride = width == 1440 ? 123 : 88; // trunc(88 * (1.4 or 1.0)), 0047d5f7..0047d60c.
    const int x = static_cast<int>(width - stride * (2.15 + index));
    const int y = width*3/4 - (width == 1440 ? 57 : 32);
    return {x, y, x+bw, y+bh};
}
namespace {
using Affine = std::array<float, 6>;
Affine Multiply(const Affine& a, const Affine& b) {
    return {a[0]*b[0]+a[1]*b[2], a[0]*b[1]+a[1]*b[3],
            a[2]*b[0]+a[3]*b[2], a[2]*b[1]+a[3]*b[3],
            a[4]*b[0]+a[5]*b[2]+b[4], a[4]*b[1]+a[5]*b[3]+b[5]};
}
Affine Rotation(float degrees) {
    const double angle = degrees * 3.14159265358979323846 / 180;
    return {static_cast<float>(std::cos(angle)), static_cast<float>(std::sin(angle)),
        static_cast<float>(-std::sin(angle)), static_cast<float>(std::cos(angle)), 0, 0};
}
void Transform(Gdiplus::Graphics& g, const Affine& a) {
    Gdiplus::Matrix matrix(a[0], a[1], a[2], a[3], a[4], a[5]);
    if (g.SetTransform(&matrix) != Gdiplus::Ok) throw std::runtime_error("Cannot set original display transform");
}
std::wstring Wide(const std::string& value) {
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring out(size, L' ');
    if (size) MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), out.data(), size);
    return out;
}
}
void DisplayArtwork::Caption(Gdiplus::Graphics& g, const std::wstring& text, float x, float y,
             float em, const OriginalHudLayout& hud, float width) {
    const auto key = std::make_tuple(text,x,y,em,width);
    auto found = captions_.find(key);
    if (found == captions_.end()) {
        Gdiplus::FontFamily family(L"System");
        const auto* selected = family.IsAvailable() ? &family : Gdiplus::FontFamily::GenericSansSerif();
        auto path = std::make_shared<Gdiplus::GraphicsPath>();
        Gdiplus::StringFormat format;
        format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
        format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
        const Gdiplus::RectF bounds(x, y, width, em * 1.7f);
        if (width > 0) path->AddString(text.c_str(), -1, selected, Gdiplus::FontStyleRegular, em, bounds, &format);
        else path->AddString(text.c_str(), -1, selected, Gdiplus::FontStyleRegular, em, Gdiplus::PointF(x, y), &format);
        if (captions_.size() >= 128) captions_.clear();
        found = captions_.emplace(key,std::move(path)).first;
    }
    Gdiplus::Pen outline(Gdiplus::Color(hud.outline_color), hud.outline_width);
    outline.SetLineJoin(Gdiplus::LineJoinRound);
    Gdiplus::SolidBrush fill(Gdiplus::Color(hud.text_color));
    g.DrawPath(&outline, found->second.get()); g.FillPath(&fill, found->second.get());
}
OriginalHudLayout BuildOriginalHud(const ReplayState& state, int width) {
    if (width != 1024 && width != 1440) throw std::runtime_error("Unsupported native display canvas");
    OriginalHudLayout out;
    // Original HUD uses exactly 1.4, not the tile canvas ratio 1440/1024.
    out.scale = width == 1440 ? 1.4f : 1;
    out.outline_width = 2 * out.scale;
    out.center = {width / 2.0f, width * 3 / 8.0f - (width == 1440 ? 75.0f : 52.0f) - 1};
    const int panel_width = width == 1440 ? 244 : 174, panel_height = width == 1440 ? 172 : 123;
    out.panel = {(width - panel_width - 18) / 2 + 9,
        (width * 3 / 4 - panel_height - 20) / 2 - (width == 1440 ? 75 : 52) + 10,
        panel_width, panel_height};
    const Affine base{1, 0, 0, 1, out.center[0], out.center[1]};
    out.seats[0] = base;
    out.seats[1] = Multiply({1, 0, 0, 1, 0, 23*out.scale},
        Multiply(Rotation(-91.6f), Multiply({1, .03f, 0, 1, 0, 0}, base)));
    out.seats[2] = Multiply(Rotation(-180), base);
    out.seats[3] = Multiply({1, 0, 0, 1, 0, 23*out.scale},
        Multiply(Rotation(-268.4f), Multiply({1, -.03f, 0, 1, 0, 0}, base)));
    out.sticks = out.seats;
    out.sticks[1] = Multiply({.9f, 0, 0, 1, 0, 0}, Multiply(Rotation(-.6f), out.seats[1]));
    out.sticks[3] = Multiply({.9f, 0, 0, 1, 0, 0}, Multiply(Rotation(.6f), out.seats[3]));
    const auto scaled = [&](float n) { return static_cast<int>(n * out.scale); };
    out.sprites.push_back({407u + static_cast<unsigned>((state.round / 4) % 4), scaled(-30), scaled(-17)});
    out.sprites.push_back({403u + static_cast<unsigned>(state.round % 4), scaled(-10), scaled(-17)});
    out.sprites.push_back({411, scaled(10), scaled(-17)});
    for (int i = 0; i < 4; ++i) {
        if (!SeatPresent(state.seat_mask,i)) continue;
        const unsigned wind = static_cast<unsigned>(SeatWind(state.seat_mask,state.dealer,i));
        out.sprites.push_back({(state.turn == i ? 416u : 420u) + wind,
            static_cast<int>(6 - 58 * out.scale), scaled(28), i});
        if (state.players[i].reach == 2) out.sprites.push_back({466, 0, scaled(57), i, true});
    }
    return out;
}
RECT FitNativeCanvas(RECT bounds, int width) {
    const int aw = std::max(1L, bounds.right - bounds.left), ah = std::max(1L, bounds.bottom - bounds.top);
    const double scale = std::min(double(aw) / width, double(ah) / (width * 3 / 4));
    const int w = std::max(1, static_cast<int>(width * scale)), h = std::max(1, static_cast<int>(width * 3 / 4 * scale));
    const int x = bounds.left + (aw - w) / 2, y = bounds.top + (ah - h) / 2;
    return {x, y, x + w, y + h};
}
void DisplayArtwork::Configure(const std::filesystem::path& directory, int width) {
    directory_ = directory; width_ = width; pack_ = {}; images_.clear(); captions_.clear();
}
const Image& DisplayArtwork::Get(unsigned id) {
    auto cached = images_.find(id);
    if (cached != images_.end()) return cached->second;
    if (pack_.entries.empty()) {
        pack_ = LoadResourcePack(directory_ / "_res2.dat");
        if (pack_.signature != (width_ == 1440 ? "res2" : "res0")) {
            pack_ = {};
            throw std::runtime_error("Original display pack has the wrong canvas signature");
        }
    }
    const auto found = std::find_if(pack_.entries.begin(), pack_.entries.end(), [&](const Resource& r) { return r.id == id; });
    if (found == pack_.entries.end() || !found->metadata) throw std::runtime_error("Missing original display sprite " + std::to_string(id));
    return images_.emplace(id, DecodeImage(*found)).first->second;
}
float DisplayArtwork::MeasureText(const std::string& text, float em) {
    Gdiplus::Bitmap bitmap(1,1,PixelFormat32bppARGB); Gdiplus::Graphics graphics(&bitmap);
    Gdiplus::FontFamily family(L"System");
    Gdiplus::Font font(family.IsAvailable()?&family:Gdiplus::FontFamily::GenericSansSerif(),em,Gdiplus::FontStyleRegular,Gdiplus::UnitPixel);
    Gdiplus::RectF rect; const auto value=Wide(text);
    graphics.MeasureString(value.c_str(),-1,&font,Gdiplus::PointF(0,0),&rect); return rect.Width;
}
void DisplayArtwork::Sprite(Gdiplus::Graphics& graphics, unsigned id, int x, int y, bool cx, bool cy) {
    const auto& im = Get(id);
    Gdiplus::Bitmap bitmap(im.width, im.height, im.width*4, PixelFormat32bppARGB, const_cast<BYTE*>(im.bgra.data()));
    if (graphics.DrawImage(&bitmap, x - (cx ? im.width/2 : 0), y - (cy ? im.height/2 : 0)) != Gdiplus::Ok)
        throw std::runtime_error("Cannot draw original display sprite");
}
void DisplayArtwork::PaintHud(Gdiplus::Graphics& g, const ReplayState& state) {
    const auto hud = BuildOriginalHud(state, width_); const float s = hud.scale;
    const auto saved = g.Save();
    try {
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        // 00482fe1 pushes 0x24d. 00482f30 draws that rounded plate at
        // (9,10) inside a centered (image+18,image+20) background capture.
        g.ResetTransform(); if (panel_enabled_) Sprite(g, 589, hud.panel[0], hud.panel[1]);
        for (const auto& item : hud.sprites) {
            const Affine base{1, 0, 0, 1, hud.center[0], hud.center[1]};
            Transform(g, item.seat < 0 ? base : item.id == 466 ? hud.sticks[item.seat] : hud.seats[item.seat]);
            Sprite(g, item.id, item.x, item.y, item.center_x, item.center_y);
        }
        Transform(g, {1, 0, 0, 1, hud.center[0], hud.center[1]});
        const auto sticks=std::to_wstring(state.sticks), honba=std::to_wstring(state.honba);
        const float sw=MeasureText(std::to_string(state.sticks),12*s), hw=MeasureText(std::to_string(state.honba),12*s);
        float cursor=-(sw+7+hw+4*s+7)*.5f;
        Sprite(g,401,static_cast<int>(cursor),static_cast<int>(6*s),false,true);
        cursor+=7; Caption(g,sticks,cursor,6*s,12*s,hud);
        cursor+=sw+4*s; Sprite(g,402,static_cast<int>(cursor),static_cast<int>(6*s),false,true);
        Caption(g,honba,cursor+7,6*s,12*s,hud);
        for (int i = 0; i < 4; ++i) {
            if (!SeatPresent(state.seat_mask,i)) continue;
            Transform(g, hud.seats[i]);
            const int value=state.relative_scores && i ? state.players[i].score-state.players[0].score : state.players[i].score;
            const auto score=(state.relative_scores && i && value>0 ? L"+" : L"")+std::to_wstring(value);
            Caption(g,score,-26*s,28*s,16*s,hud);
            if (!state.relative_scores) {
                auto name_hud=hud; if (!state.players[i].connected) name_hud.text_color=0xffcc0000;
                Caption(g,Wide(state.players[i].name),-25*s,46*s,(32.0f/3)*s,name_hud,70*s);
            }
        }
        g.Restore(saved);
    } catch (...) { g.Restore(saved); throw; }
}
void DisplayArtwork::PaintChrome(Gdiplus::Graphics& g, unsigned type, const std::array<bool,4>& automatic, int hovered) {
    const auto saved = g.Save(); g.ResetTransform();
    try {
        // FUN_0049c900, normal four-player branches (sanma resources remain distinct).
        const unsigned level = (type & 0x800) ? 588u : 566 + (((type >> 3 & 0x10) | (type & 0x20)) >> 4);
        const unsigned rules[]{level, 570 + ((type >> 6) & 1) + ((type >> 2) & 2), 574 + ((type >> 2) & 1), 576 + ((type >> 1) & 1)};
        int y = 0;
        for (unsigned id : rules) { const auto& im = Get(id); Sprite(g, id, width_-im.width, y); y += im.height; }
        // WC_ITEM_CHECK: checked captions 424..427, unchecked 429..432
        // at 70% alpha, vertical 0 -> 0x22000000 gradient and original border.
        for (int i = 0; i < 4; ++i) {
            const RECT r = OriginalAutoButtonRect(width_, i);
            Gdiplus::LinearGradientBrush shade(Gdiplus::Point(r.left,r.top), Gdiplus::Point(r.left,r.bottom),
                Gdiplus::Color(0,0,0,0), Gdiplus::Color(0x22,0,0,0));
            g.FillRectangle(&shade, r.left,r.top,r.right-r.left,r.bottom-r.top);
            const auto& im = Get((automatic[i] ? 424u : 429u) + i);
            Gdiplus::Bitmap bitmap(im.width,im.height,im.width*4,PixelFormat32bppARGB,const_cast<BYTE*>(im.bgra.data()));
            Gdiplus::ColorMatrix opacity{{{1,0,0,0,0},{0,1,0,0,0},{0,0,1,0,0},{0,0,0,automatic[i] ? 1.0f : .7f,0},{0,0,0,0,1}}};
            Gdiplus::ImageAttributes attrs; attrs.SetColorMatrix(&opacity);
            g.DrawImage(&bitmap,Gdiplus::Rect(r.left+(r.right-r.left-im.width)/2,r.top+(r.bottom-r.top-im.height)/2,im.width,im.height),
                0,0,im.width,im.height,Gdiplus::UnitPixel,&attrs);
            Gdiplus::Pen border(Gdiplus::Color(hovered == i ? 0xffff9933u : 0x11000000u),1);
            g.DrawRectangle(&border,r.left,r.top,r.right-r.left-1,r.bottom-r.top-1);
        }
        g.Restore(saved);
    } catch (...) { g.Restore(saved); throw; }
}
void DisplayArtwork::PaintStatus(Gdiplus::Graphics& g, const ReplayState& state) {
    const auto saved = g.Save(); g.ResetTransform();
    try {
        // FUN_0049c900: append 578 when round >= the game's last scheduled hand.
        if (state.initialized && state.round >= static_cast<int>(((state.type & 8 | 6) >> 1) - (state.type >> 4 & 1))) {
            const auto& im = Get(578);
            int y = 0;
            const unsigned ids[]{(state.type&0x800) ? 588u : 566+(((state.type>>3&0x10)|(state.type&0x20))>>4),
                570+((state.type>>6)&1)+((state.type>>2)&2),574+((state.type>>2)&1),576+((state.type>>1)&1)};
            for (auto id : ids) y += Get(id).height;
            Sprite(g,578,width_-im.width,y);
        }
        // 0047d77c..0047d7b3: fa6 at trunc(300 * scale), shared control baseline.
        if (state.initialized && !state.ended && state.furiten)
            Sprite(g,590,static_cast<int>(300*(width_==1440?1.4:1.0)),OriginalAutoButtonRect(width_,0).top);
        g.Restore(saved);
    } catch (...) { g.Restore(saved); throw; }
}
std::array<float,2> DisplayArtwork::MeasureTextExtent(const std::string& text,float em) {
    Gdiplus::Bitmap bitmap(1,1,PixelFormat32bppARGB); Gdiplus::Graphics g(&bitmap);
    Gdiplus::FontFamily family(L"System");
    Gdiplus::Font font(family.IsAvailable()?&family:Gdiplus::FontFamily::GenericSansSerif(),em,Gdiplus::FontStyleRegular,Gdiplus::UnitPixel);
    const auto value=Wide(text); Gdiplus::RectF bounds;
    if(g.MeasureString(value.c_str(),-1,&font,Gdiplus::PointF(0,0),&bounds)!=Gdiplus::Ok)
        throw std::runtime_error("Cannot measure north-count text");
    return {bounds.Width,bounds.Height};
}
NukiCountLayout OriginalNukiCountLayout(RECT a,int seat,int text_width,int text_height) {
    const int w=(seat&1)?text_height:text_width,h=(seat&1)?text_width:text_height;
    RECT r{};
    switch(seat) {
    case 0: r={a.right-2,a.bottom-h,a.right-2+w,a.bottom}; break;
    case 1: r={a.right-w-3,a.top-3-h,a.right-3,a.top-3}; break;
    case 2: r={a.left-w,a.top+w-h,a.left,a.top+w}; break;
    case 3: r={a.left,a.bottom,a.left+w,a.bottom+h}; break;
    default: throw std::runtime_error("Invalid north-count direction");
    }
    --r.left; --r.top; ++r.right; ++r.bottom;
    POINT p{1,1};
    if(seat==1) p.x=1-(r.bottom-r.top);
    else if(seat==2) { p.x=1-(r.right-r.left); p.y=1-(r.bottom-r.top); }
    else if(seat==3) p.y=1-(r.right-r.left);
    return {r,p};
}
void DisplayArtwork::PaintNukiCounts(Gdiplus::Graphics& g, const ReplayState& state, TableArtwork& tiles) {
    if(!(state.type&16) || std::all_of(state.players.begin(),state.players.end(),[](const ReplayPlayer& p) { return p.nuki.empty(); })) return;
    // 0049c200 / 004aa250: one n/0 sprite plus a rotated xN count,
    // System bold 12*1.4, white fill and four one-pixel black offsets.
    const auto saved=g.Save();
    try {
        Gdiplus::FontFamily family(L"System");
        Gdiplus::Font font(family.IsAvailable()?&family:Gdiplus::FontFamily::GenericSansSerif(),
            12*(width_==1440?1.4f:1.f),Gdiplus::FontStyleBold,Gdiplus::UnitPixel);
        Gdiplus::SolidBrush black(Gdiplus::Color(0xff000000)),white(Gdiplus::Color(0xffffffff));
        for(int seat=0;seat<4;++seat) {
            if(!SeatPresent(state.seat_mask,seat) || state.players[seat].nuki.empty()) continue;
            // 00480930 maps the original entity sentinel 254 to variant 37.
            const auto& tile=tiles.Get({'n',0,-1,seat});
            const auto text=L"\nx"+std::to_wstring(state.players[seat].nuki.size());
            Gdiplus::RectF measure;
            // WM_CREATE measures the fixed two-line PE string, not the
            // current count. The standard mode leaves the first line blank.
            g.MeasureString(L"99+\nx99",-1,&font,Gdiplus::PointF(0,0),&measure);
            const auto layout=OriginalNukiCountLayout({tile.x,tile.y,tile.x+tile.image.width,tile.y+tile.image.height},
                seat,static_cast<int>(measure.Width),static_cast<int>(measure.Height));
            g.ResetTransform(); g.TranslateTransform(static_cast<float>(layout.bounds.left),static_cast<float>(layout.bounds.top));
            g.RotateTransform(static_cast<float>(seat*270));
            const float x=static_cast<float>(layout.text_origin.x),y=static_cast<float>(layout.text_origin.y);
            for(const auto& p:std::array<Gdiplus::PointF,4>{{{1,0},{-1,0},{0,1},{0,-1}}})
                g.DrawString(text.c_str(),-1,&font,Gdiplus::PointF(x+p.X,y+p.Y),&black);
            g.DrawString(text.c_str(),-1,&font,Gdiplus::PointF(x,y),&white);
        }
        g.Restore(saved);
    } catch(...) { g.Restore(saved); throw; }
}
} // namespace tenhou
