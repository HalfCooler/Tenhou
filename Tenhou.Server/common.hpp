#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <stdexcept>

namespace authority {
using Attrs=std::map<std::string,std::string>;
struct Message { std::string tag; Attrs attrs; };
std::string xml(const std::string& tag,const Attrs& attrs={});
Message parse(const std::string& text);
int number(const std::string& text,int low=0,int high=100000000);
std::vector<int> integers(const std::string& text);
std::string list(const std::vector<int>& values);
template<std::size_t N> std::string list(const std::array<int,N>& values) { return list(std::vector<int>(values.begin(),values.end())); }
inline bool has(const std::vector<int>& a,int x) { return std::find(a.begin(),a.end(),x)!=a.end(); }
inline void remove(std::vector<int>& a,int x) { auto i=std::find(a.begin(),a.end(),x); if(i==a.end()) throw std::runtime_error("Missing physical tile"); a.erase(i); }
inline bool terminal(int k) { return k>=27 || k%9==0 || k%9==8; }
using Counts=std::array<int,34>;
Counts counts(const std::vector<int>& hand);
struct Config {
    std::string family="standard4",length="east",time="5+20",minimum="1";
    bool fast=false,advanced=false,bankruptcy=true,ancient=false,open_tanyao=true,open_hands=false;
    int starting=25000,target=30000,red_count=3;
    int n() const { return family=="standard3"?3:4; }
    int replacement_limit() const { return n()==3?8:4; }
    int type() const { return 1|(n()==3?16:0)|(length=="south"?8:0)|(fast?64:0)|(!open_tanyao?4:0)|(!red_count?2:0); }
    std::vector<int> red_tiles() const;
    Attrs wire() const;
    static Config from(Attrs values);
    static Config general(int type);
};
struct Meld {
    std::string kind;
    std::vector<int> tiles;
    int who=0,source=0,called=-1,added=-1;
    bool opened() const { return kind!="closed_kan"; }
    int encode() const;
};
struct Value { int han=0,fu=0,main=0,additional=0,level=0; std::vector<int> yaku,yakuman; };
struct Context {
    bool tsumo=false,dealer=false,riichi=false,double_reach=false,ippatsu=false,rinshan=false,chankan=false,
        haitei=false,houtei=false,tenhou=false,chiihou=false,renhou=false,swallow=false,kan_discard=false;
    int seat_wind=28,round_wind=27;
    std::vector<int> dora,ura,nuki;
};
std::set<int> waits(const std::vector<int>& hand,int n=4);
bool complete(const Counts& c);
int shanten(const Counts& c);
Value payments(int han,int fu,bool dealer,bool tsumo,bool natural=false);
Value evaluate(const std::vector<int>& hand,const std::vector<Meld>& melds,int tile,const Config& cfg,const Context& ctx);
}
