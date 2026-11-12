#include "common.hpp"
#include <charconv>
#include <numeric>
#include <sstream>
namespace authority {
int number(const std::string& s,int low,int high) {
    int n=0; const auto p=std::from_chars(s.data(),s.data()+s.size(),n);
    if(s.empty() || p.ec!=std::errc() || p.ptr!=s.data()+s.size() || n<low || n>high) throw std::runtime_error("Invalid number"); return n;
}
std::vector<int> integers(const std::string& s) { std::vector<int> v; if(s.empty()) return v; std::size_t p=0; for(;;) { auto e=s.find(',',p); v.push_back(number(s.substr(p,e==s.npos?e:e-p),-100000000)); if(e==s.npos) return v; p=e+1; } }
std::string list(const std::vector<int>& v) { std::string s; for(auto x:v) { if(!s.empty()) s+=','; s+=std::to_string(x); } return s; }
std::string escape(const std::string& s) { std::string r; for(auto c:s) { if(c=='&') r+="&amp;"; else if(c=='\"') r+="&quot;"; else if(c=='<') r+="&lt;"; else if(c=='>') r+="&gt;"; else if(static_cast<unsigned char>(c)<32) throw std::runtime_error("Control character"); else r+=c; } return r; }
std::string xml(const std::string& tag,const Attrs& a) { std::string r="<"+tag; for(const auto& e:a) r+=" "+e.first+"=\""+escape(e.second)+"\""; return r+"/>"; }
Message parse(const std::string& s) {
    if(s.size()>4096 || s.size()<4 || s[0]!='<') throw std::runtime_error("Invalid frame");
    std::size_t p=1; auto word=[&]() { auto b=p; while(p<s.size() && (isalnum(static_cast<unsigned char>(s[p])) || s[p]=='_')) ++p; if(b==p) throw std::runtime_error("Invalid name"); return s.substr(b,p-b); };
    auto space=[&]() { while(p<s.size() && (s[p]==' ' || s[p]=='\t' || s[p]=='\r' || s[p]=='\n')) ++p; };
    Message m; m.tag=word();
    while(true) {
        auto before=p; space(); if(s.compare(p,2,"/>")==0) { p+=2; break; }
        if(before==p || m.attrs.size()>=32) throw std::runtime_error("Invalid attributes");
        auto k=word(); space(); if(p>=s.size() || s[p++]!='=') throw std::runtime_error("Expected equals"); space();
        if(p>=s.size() || (s[p]!='\"' && s[p]!='\'')) throw std::runtime_error("Expected quote"); auto q=s[p++]; std::string v;
        while(p<s.size() && s[p]!=q) {
            char c=s[p++]; if(c=='<' || static_cast<unsigned char>(c)<32) throw std::runtime_error("Invalid attribute");
            if(c=='&') { auto e=s.find(';',p); if(e==s.npos) throw std::runtime_error("Invalid entity"); auto entity=s.substr(p,e-p); p=e+1;
                if(entity=="amp") c='&'; else if(entity=="quot") c='\"'; else if(entity=="apos") c='\''; else if(entity=="lt") c='<'; else if(entity=="gt") c='>'; else throw std::runtime_error("Unsupported entity"); }
            v+=c;
        }
        if(p>=s.size() || !m.attrs.emplace(k,v).second) throw std::runtime_error("Duplicate or incomplete attribute"); ++p;
    }
    if(p!=s.size()) throw std::runtime_error("Trailing data"); return m;
}
Counts counts(const std::vector<int>& hand) { Counts c{}; for(auto t:hand) { if(t<0 || t>=136) throw std::runtime_error("Invalid tile"); ++c[t/4]; } return c; }
std::vector<int> Config::red_tiles() const { if(!red_count) return {}; std::vector<int> r=n()==3?std::vector<int>{52,88}:std::vector<int>{16,52,88}; if(red_count>static_cast<int>(r.size())) r.push_back(53); return r; }
Attrs Config::wire() const { return {{"match_type",family},{"length",length},{"time",time},{"minimum",minimum},{"fast",std::to_string(fast)},{"advanced",std::to_string(advanced)},
    {"starting_points",std::to_string(starting)},{"target_points",std::to_string(target)},{"red_count",std::to_string(red_count)},{"bankruptcy",std::to_string(bankruptcy)},
    {"ancient",std::to_string(ancient)},{"open_tanyao",std::to_string(open_tanyao)},{"open_hands",std::to_string(open_hands)}}; }
Config Config::from(Attrs a) {
    Config c; const auto allowed=c.wire(); for(const auto& e:a) if(!allowed.count(e.first)) throw std::runtime_error("Unknown setting");
    auto get=[&](const char* key,std::string fallback) { auto i=a.find(key); return i==a.end()?fallback:i->second; };
    auto boolean=[&](const char* k,bool d) { return number(get(k,d?"1":"0"),0,1)!=0; };
    c.family=get("match_type",c.family); if(c.family!="standard3" && c.family!="standard4") throw std::runtime_error("Unsupported family");
    c.length=get("length",c.length); if(c.length!="single" && c.length!="east" && c.length!="south") throw std::runtime_error("Unsupported length");
    c.fast=boolean("fast",false); c.advanced=boolean("advanced",false); c.time=get("time",c.time); if(c.time=="3+5") c.fast=true; if(c.fast) c.time="3+5";
    if(c.time!="3+5" && c.time!="5+10" && c.time!="5+20" && c.time!="60+0" && c.time!="300+0") throw std::runtime_error("Invalid thinking time");
    if(c.advanced) { c.starting=number(get("starting_points",c.n()==3?"35000":"25000"),0,200000); c.target=number(get("target_points",c.n()==3?"40000":"30000"),c.starting); c.red_count=number(get("red_count",c.n()==3?"2":"3"),0,4); }
    c.bankruptcy=boolean("bankruptcy",true); c.ancient=boolean("ancient",false);
    c.open_tanyao=boolean("open_tanyao",true); c.open_hands=boolean("open_hands",false); c.minimum=get("minimum","1");
    if(!std::set<std::string>{"1","2","4","mangan","haneman","baiman","sanbaiman","yakuman"}.count(c.minimum)) throw std::runtime_error("Invalid minimum");
    if(!c.advanced) { c.starting=c.n()==3?35000:25000; c.target=c.n()==3?40000:30000; c.red_count=c.n()==3?2:3; c.bankruptcy=true; c.ancient=false; c.open_tanyao=true; c.open_hands=false; c.minimum="1"; }
    if(c.starting%100 || c.target%100 || !(c.n()==3?std::set<int>{0,2,3}:std::set<int>{0,3,4}).count(c.red_count)) throw std::runtime_error("Invalid point/red settings"); return c;
}
Config Config::general(int t) {
    if(!std::set<int>{1,9,65,73,17,25,81,89,7,71,23,87}.count(t)) throw std::runtime_error("Unsupported general rule");
    Config c=from({{"match_type",t&16?"standard3":"standard4"},{"length",t&8?"south":"east"},{"fast",t&64?"1":"0"}});
    c.red_count=t&2?0:c.red_count; c.open_tanyao=!(t&4); return c;
}
int Meld::encode() const {
    auto ts=tiles; std::sort(ts.begin(),ts.end()); int relative=(source-who+4)%4;
    if(kind=="chi") { int low=ts[0]/4, packed=(low/9*7+low%9)*3+static_cast<int>(std::find(ts.begin(),ts.end(),called)-ts.begin()); return (packed<<10)|((ts[0]%4)<<3)|((ts[1]%4)<<5)|((ts[2]%4)<<7)|4|relative; }
    if(kind=="pon" || kind=="added_kan") { int base=ts[0]/4*4, excluded=added; if(kind=="pon") for(int t=base;t<base+4;++t) if(!has(ts,t)) excluded=t;
        std::vector<int> pon; for(int t=base;t<base+4;++t) if(t!=excluded) pon.push_back(t);
        int packed=base/4*3+static_cast<int>(std::find(pon.begin(),pon.end(),called)-pon.begin()); return (packed<<9)|((excluded%4)<<5)|(kind=="pon"?8:16)|relative; }
    return ((opened()?called:ts[0])<<8)|relative;
}
}
