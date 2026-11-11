#include "room_settings_view.hpp"
#include "network_protocol.hpp"
#include <array>
#include <algorithm>
#include <charconv>
#include <cstring>
#include <vector>

namespace tenhou {
namespace {
struct RoomDialog {
    std::map<std::string,std::string> values;
    std::map<int,HWND> controls;
    HFONT font=nullptr;
    static constexpr const char* keys[]{"match_type","length","time","fast","advanced","starting_points","target_points","bankruptcy","ancient","red_count","open_tanyao","minimum","open_hands"};
    const std::vector<std::vector<std::string>> options{
        {"standard4","standard3"},{"single","east","south"},{"5+10","5+20","60+0","300+0"},
        {},{}, {},{}, {"1","0"},{"0","1"},{},{"1","0"},{"1","2","4","mangan","haneman","baiman","sanbaiman","yakuman"},{"0","1"}};
    std::string value(int i) const { const auto it=values.find(keys[i]); return it==values.end()?"":it->second; }
    void select(int i,const std::string& text) {
        const auto& choices=options[static_cast<std::size_t>(i)];
        const auto found=std::find(choices.begin(),choices.end(),text);
        SendMessageW(controls.at(i),CB_SETCURSEL,found==choices.end()?0:found-choices.begin(),0);
    }
    bool checked(int i) const { return SendMessageW(controls.at(i),BM_GETCHECK,0,0)==BST_CHECKED; }
    void thinking_time() {
        const bool fast=checked(3);
        EnableWindow(controls.at(2),!fast);
        ShowWindow(controls.at(2),fast?SW_HIDE:SW_SHOW);
        ShowWindow(controls.at(302),fast?SW_SHOW:SW_HIDE);
    }
    void red_options(bool defaults) {
        const bool three=SendMessageW(controls.at(0),CB_GETCURSEL,0,0)==1;
        auto red=controls.at(9); SendMessageW(red,CB_RESETCONTENT,0,0);
        for(const auto* text : {L"无赤",three?L"2赤（默认）":L"3赤（默认）",three?L"3赤":L"4赤"})
            SendMessageW(red,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
        const auto current=value(9);
        SendMessageW(red,CB_SETCURSEL,defaults?1:current=="0"?0:current==(three?"3":"4")?2:1,0);
        if(defaults) {
            SetWindowTextW(controls.at(5),three?L"35000":L"25000");
            SetWindowTextW(controls.at(6),three?L"40000":L"30000");
        }
    }
    void advanced() {
        const bool enabled=checked(4);
        for(int i=5;i<=12;++i) EnableWindow(controls.at(i),enabled);
    }
    void init(HWND window) {
        font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei");
        const auto add=[&](int id,const wchar_t* cls,const wchar_t* title,DWORD style,int x,int y,int w,int h) {
            RECT rect{x,y,x+w,y+h}; MapDialogRect(window,&rect);
            HWND control=CreateWindowExW(0,cls,title,WS_CHILD|WS_VISIBLE|style,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
            SendMessageW(control,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
            return control;
        };
        const wchar_t* labels[]{L"对局",L"局数",L"思考时间",L"速模式（3+5）",L"开启高级设置",L"起始点数（0～200000）",L"一位终局的必要点数",L"击飞",L"古役",L"赤宝牌",L"食断",L"翻缚",L"是否明牌"};
        for(int i=0;i<13;++i) {
            const int y=12+i*20;
            if(i==3 || i==4) {
                controls[i]=add(100+i,L"BUTTON",labels[i],WS_TABSTOP|BS_AUTOCHECKBOX,14,y,300,18);
                SendMessageW(controls[i],BM_SETCHECK,value(i)=="1"?BST_CHECKED:BST_UNCHECKED,0);
            } else {
                add(200+i,L"STATIC",labels[i],0,14,y+2,151,18);
                const bool edit=i==5 || i==6;
                controls[i]=add(100+i,edit?L"EDIT":L"COMBOBOX",L"",WS_TABSTOP|(edit?WS_BORDER|ES_NUMBER|ES_AUTOHSCROLL:CBS_DROPDOWNLIST|WS_VSCROLL),168,y,169,edit?18:170);
                if(edit) { SetWindowTextW(controls[i],NetworkWide(value(i)).c_str());SendMessageW(controls[i],EM_LIMITTEXT,9,0); }
            }
        }
        const std::vector<std::vector<const wchar_t*>> titles{
            {L"标准四人",L"标准三人"},{L"一局",L"东风",L"半庄"},{L"5+10",L"5+20（默认）",L"60+0",L"300+0"},
            {},{},{},{},{L"开（默认）",L"关"},{L"关（默认）",L"开"},{},{L"开（默认）",L"关"},
            {L"1翻缚（默认）",L"2翻缚",L"4翻缚",L"满贯缚",L"跳满缚",L"倍满缚",L"三倍满缚",L"役满缚"},{L"关（默认）",L"开"}};
        for(int i=0;i<13;++i) if(!titles[static_cast<std::size_t>(i)].empty()) {
            for(const auto* text:titles[static_cast<std::size_t>(i)]) SendMessageW(controls.at(i),CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
            select(i,value(i));
        }
        controls[302]=add(302,L"STATIC",L"3+5（速模式）",0,168,54,169,18);
        if(value(2)=="3+5") select(2,"5+20");
        red_options(false); advanced(); thinking_time();
        add(300,L"STATIC",L"点数以100为单位。翻缚的1/2/4翻不计宝牌；满贯以上按得点等级。",0,14,276,323,27);
        controls[301]=add(301,L"STATIC",L"",0,14,306,323,28);
        add(IDOK,L"BUTTON",L"保存房间配置",WS_TABSTOP|BS_DEFPUSHBUTTON,180,340,96,22);
        add(IDCANCEL,L"BUTTON",L"取消",WS_TABSTOP|BS_PUSHBUTTON,282,340,55,22);
        RECT owner{};GetWindowRect(GetParent(window),&owner);RECT r{};GetWindowRect(window,&r);
        SetWindowPos(window,nullptr,owner.left+((owner.right-owner.left)-(r.right-r.left))/2,std::max(0L,owner.top+((owner.bottom-owner.top)-(r.bottom-r.top))/2),0,0,SWP_NOSIZE|SWP_NOZORDER);
    }
    bool save() {
        for(int i=0;i<13;++i) {
            if(i==3 || i==4) values[keys[i]]=checked(i)?"1":"0";
            else if(i==5 || i==6) {
                wchar_t text[16]{};GetWindowTextW(controls.at(i),text,16);values[keys[i]]=NetworkUtf8(text);
            } else {
                const auto selected=SendMessageW(controls.at(i),CB_GETCURSEL,0,0);
                if(selected<0) return false;
                if(i==9) values[keys[i]]=selected==0?"0":selected==1?(value(0)=="standard3"?"2":"3"):(value(0)=="standard3"?"3":"4");
                else values[keys[i]]=options[static_cast<std::size_t>(i)].at(static_cast<std::size_t>(selected));
            }
        }
        if(checked(3)) values["time"]="3+5";
        if(checked(4)) {
            const auto number=[&](int i) { int n=-1;const auto& s=values[keys[i]];const auto p=std::from_chars(s.data(),s.data()+s.size(),n);return p.ec==std::errc() && p.ptr==s.data()+s.size()?n:-1; };
            const int start=number(5),target=number(6);
            if(start<0 || start>200000 || start%100 || target<start || target>100000000 || target%100) {
                SetWindowTextW(controls.at(301),L"起始点数须为0～200000，必要点数不得低于起始点数，均为100的倍数。");return false;
            }
        }
        return true;
    }
    static INT_PTR CALLBACK procedure(HWND window,UINT message,WPARAM wp,LPARAM lp) {
        auto* self=reinterpret_cast<RoomDialog*>(GetWindowLongPtrW(window,DWLP_USER));
        if(message==WM_INITDIALOG) { self=reinterpret_cast<RoomDialog*>(lp);SetWindowLongPtrW(window,DWLP_USER,lp);self->init(window);return TRUE; }
        if(!self) return FALSE;
        if(message==WM_COMMAND) {
            const int id=LOWORD(wp);
            if(id==IDOK) {if(self->save()) EndDialog(window,IDOK);return TRUE;}
            if(id==IDCANCEL) {EndDialog(window,IDCANCEL);return TRUE;}
            if(id==100 && HIWORD(wp)==CBN_SELCHANGE) self->red_options(true);
            if(id==103) self->thinking_time();
            if(id==104) self->advanced();
        }
        if(message==WM_DESTROY && self->font) {DeleteObject(self->font);self->font=nullptr;}
        return FALSE;
    }
};
}
std::optional<std::map<std::string,std::string>> EditRoomSettings(HWND owner,const std::map<std::string,std::string>& settings) {
    RoomDialog dialog;dialog.values=settings;
    DLGTEMPLATE header{};header.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;header.cx=352;header.cy=374;
    const std::wstring title=L"自定义对局房间";
    std::vector<WORD> buffer((sizeof(header)+1)/2+2+title.size()+1,0);
    std::memcpy(buffer.data(),&header,sizeof(header));
    std::memcpy(buffer.data()+(sizeof(header)+1)/2+2,title.c_str(),(title.size()+1)*sizeof(wchar_t));
    if(DialogBoxIndirectParamW(GetModuleHandleW(nullptr),reinterpret_cast<const DLGTEMPLATE*>(buffer.data()),owner,RoomDialog::procedure,reinterpret_cast<LPARAM>(&dialog))==IDOK) return dialog.values;
    return std::nullopt;
}
}
