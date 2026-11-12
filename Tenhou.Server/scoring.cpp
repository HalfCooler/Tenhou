// Native decomposition/scoring migration of server/rules.py and ancient.py.
// The pinned MIT MahjongRepository implementation remains an independent oracle.
#include "common.hpp"
#include <functional>
#include <numeric>
#include <unordered_map>
namespace authority {
namespace {
struct Block { int kind=0,size=3; bool sequence=false,opened=false,declared=false; };
using Shape=std::vector<Block>;
void decompose(Counts& c,Shape& path,std::vector<Shape>& out) {
    int k=0; while(k<34 && !c[k]) ++k;
    if(k==34) { out.push_back(path); return; }
    if(c[k]>=3) { c[k]-=3; path.push_back({k,3}); decompose(c,path,out); path.pop_back(); c[k]+=3; }
    if(k<27 && k%9<7 && c[k+1] && c[k+2]) { --c[k]; --c[k+1]; --c[k+2]; path.push_back({k,3,true}); decompose(c,path,out); path.pop_back(); ++c[k]; ++c[k+1]; ++c[k+2]; }
}
std::vector<Shape> shapes(Counts c) {
    std::vector<Shape> out; for(int k=0;k<34;++k) if(c[k]>=2) { c[k]-=2; Shape path{{k,2}}; decompose(c,path,out); c[k]+=2; } return out;
}
bool kokushi(const Counts& c) { int pair=0; for(int k=0;k<34;++k) { if(terminal(k)) { if(!c[k]) return false; if(c[k]>=2) ++pair; } else if(c[k]) return false; } return pair==1; }
bool seven(const Counts& c) { return std::count(c.begin(),c.end(),2)==7; }
bool includes(const Block& b,int k) { return b.sequence?k>=b.kind && k<b.kind+3:k==b.kind; }
int next_dora(int k,int n) { if(k<27) return n==3 && k==0?8:(k/9*9+(k%9+1)%9); if(k<31) return 27+(k-27+1)%4; return 31+(k-31+1)%3; }
}
bool complete(const Counts& c) {
    int total=std::accumulate(c.begin(),c.end(),0); if(total%3!=2) return false;
    return (total==14 && (kokushi(c)||seven(c))) || !shapes(c).empty();
}
int shanten(const Counts& input) {
    std::string key; for(auto k:input) key+=static_cast<char>(k);
    static std::unordered_map<std::string,int> cache; auto found=cache.find(key); if(found!=cache.end()) return found->second;
    int total=std::accumulate(input.begin(),input.end(),0),best=8; Counts c=input;
    std::function<void(int,int,int,int)> search=[&](int at,int meld,int partial,int pair) {
        while(at<34 && !c[at]) ++at;
        if(at==34) { best=std::min(best,8-2*meld-std::min(partial,4-meld)-pair); return; }
        if(c[at]>=3) { c[at]-=3; search(at,meld+1,partial,pair); c[at]+=3; }
        if(at<27 && at%9<7 && c[at+1] && c[at+2]) { --c[at]; --c[at+1]; --c[at+2]; search(at,meld+1,partial,pair); ++c[at]; ++c[at+1]; ++c[at+2]; }
        if(c[at]>=2) { c[at]-=2; if(!pair) search(at,meld,partial,1); search(at,meld,partial+1,pair); c[at]+=2; }
        if(at<27 && at%9<8 && c[at+1]) { --c[at]; --c[at+1]; search(at,meld,partial+1,pair); ++c[at]; ++c[at+1]; }
        if(at<27 && at%9<7 && c[at+2]) { --c[at]; --c[at+2]; search(at,meld,partial+1,pair); ++c[at]; ++c[at+2]; }
        --c[at]; search(at,meld,partial,pair); ++c[at];
    };
    search(0,(14-total)/3,0,0);
    if(total>=13) { int pairs=0,distinct=0,orphans=0,orphan_pair=0; for(int k=0;k<34;++k) { pairs+=input[k]>=2; distinct+=input[k]>0; if(terminal(k)) { orphans+=input[k]>0; orphan_pair|=input[k]>=2; } }
        best=std::min({best,6-pairs+std::max(0,7-distinct),13-orphans-orphan_pair}); }
    if(cache.size()>=100000) cache.clear(); cache.emplace(std::move(key),best); return best;
}
std::set<int> waits(const std::vector<int>& hand,int n) {
    if(hand.size()%3!=1) return {}; auto c=counts(hand);
    std::string key; for(int v:c) key+=static_cast<char>(v); key+=static_cast<char>(n);
    static std::unordered_map<std::string,std::set<int>> cache;
    auto i=cache.find(key); if(i!=cache.end()) return i->second;
    std::set<int> r; for(int k=0;k<34;++k) { if(n==3 && k>=1 && k<=7) continue; ++c[k]; if(complete(c)) r.insert(k); --c[k]; }
    if(cache.size()>=100000) cache.clear(); cache.emplace(std::move(key),r); return r;
}
Value payments(int han,int fu,bool dealer,bool tsumo,bool natural) {
    Value v; v.han=han; v.fu=fu; if(han<=0) return v;
    int base=0;
    if(han>=13) { base=8000*(natural?han/13:1); v.level=5; }
    else if(han>=11) { base=6000; v.level=4; } else if(han>=8) { base=4000; v.level=3; }
    else if(han>=6) { base=3000; v.level=2; } else { base=han>=5?2000:fu*(1<<(han+2)); if(base>2000) base=2000; if(base==2000) v.level=1; }
    auto round=[](int x) { return (x+99)/100*100; };
    v.main=round(base*(tsumo?2:dealer?6:4)); v.additional=tsumo?round(base*(dealer?2:1)):0; return v;
}
Value evaluate(const std::vector<int>& hand,const std::vector<Meld>& melds,int tile,const Config& cfg,const Context& x) {
    const auto c=counts(hand); if(!complete(c)) return {};
    const bool closed=std::none_of(melds.begin(),melds.end(),[](const Meld& m) { return m.opened(); });
    std::vector<int> tiles=hand; for(const auto& m:melds) tiles.insert(tiles.end(),m.tiles.begin(),m.tiles.end());
    const auto all=counts(tiles); const bool chiitoi=melds.empty() && seven(c), orphan=melds.empty() && kokushi(c);
    auto decompositions=shapes(c);
    if(chiitoi) { Shape s; for(int k=0;k<34;++k) if(c[k]==2) s.push_back({k,2}); decompositions.push_back(s); }
    if(orphan) decompositions.push_back({Block{tile/4,2}});
    Value best;
    for(auto shape:decompositions) {
        const auto concealed=shape.size();
        const bool pairs=shape.size()==7;
        const bool is_orphan=orphan && shape.size()==1;
        for(const auto& m:melds) shape.push_back({m.tiles[0]/4,static_cast<int>(m.tiles.size()),m.kind=="chi",m.opened(),true});
        for(std::size_t win=0;win<concealed;++win) {
            if(!includes(shape[win],tile/4)) continue;
            std::vector<std::pair<int,int>> ys, yak;
            auto add=[&](int id,int han) { if(han) ys.emplace_back(id,han); };
            auto natural=[&](int id,int units=1) { yak.emplace_back(id,units); };
            auto erase=[&](int id) { ys.erase(std::remove_if(ys.begin(),ys.end(),[&](auto y) { return y.first==id; }),ys.end()); };
            int pair=-1,sequences=0,triplets=0,kans=0,concealed_sets=0,extra_fu=0;
            std::set<int> trips,seqs; std::map<int,int> seq_count;
            bool all_terminal=true,has_honor=false,has_terminal=false,has_simple=false;
            std::set<int> suits;
            for(int k=0;k<34;++k) if(all[k]) { if(k>=27) has_honor=true; else suits.insert(k/9); if(terminal(k)) has_terminal=true; else has_simple=true; }
            for(std::size_t j=0;j<shape.size();++j) {
                const auto& b=shape[j];
                if(b.size==2) { pair=b.kind; if(!terminal(b.kind)) all_terminal=false; continue; }
                if(b.sequence) { ++sequences; seqs.insert(b.kind); ++seq_count[b.kind]; if(b.kind%9!=0 && b.kind%9!=6) all_terminal=false; continue; }
                ++triplets; trips.insert(b.kind); if(b.size==4) ++kans;
                bool open=b.opened || (!x.tsumo && j==win); if(!open) ++concealed_sets;
                extra_fu+=(open?2:4)*(terminal(b.kind)?2:1)*(b.size==4?4:1);
                if(!terminal(b.kind)) all_terminal=false;
            }
            if(x.tenhou) natural(37); if(x.chiihou) natural(38);
            if(is_orphan) { auto before=c; --before[tile/4]; bool thirteen=true; for(int k=0;k<34;++k) if(terminal(k) && before[k]!=1) thirteen=false; natural(thirteen?48:47,thirteen?2:1); }
            else {
                if(trips.count(31) && trips.count(32) && trips.count(33)) natural(39);
                if(concealed_sets==4) natural(shape[win].size==2?41:40,shape[win].size==2?2:1);
                if(trips.count(27) && trips.count(28) && trips.count(29) && trips.count(30)) natural(49,2);
                else if(pair>=27 && pair<=30 && std::count_if(trips.begin(),trips.end(),[](int k) { return k>=27 && k<=30; })==3) natural(50);
                if(suits.empty()) natural(42);
                if(!has_honor && !has_simple) natural(44);
                if(std::all_of(tiles.begin(),tiles.end(),[](int t) { return std::set<int>{19,20,21,23,25,32}.count(t/4)!=0; })) natural(43);
                if(kans==4) natural(51);
                if(closed && melds.empty() && suits.size()==1 && !has_honor && tiles.size()==14) {
                    const int offset=*suits.begin()*9; bool gates=all[offset]>=3 && all[offset+8]>=3;
                    for(int k=1;k<8;++k) gates=gates && all[offset+k]>=1;
                    if(gates) { auto before=all; --before[tile/4]; bool pure=before[offset]==3 && before[offset+8]==3; for(int k=1;k<8;++k) pure=pure && before[offset+k]==1; natural(pure?46:45,pure?2:1); }
                }
            }
            if(!is_orphan) {
                if(x.tsumo && closed) add(0,1);
                if(x.double_reach) add(21,2); else if(x.riichi) add(1,1);
                if(x.ippatsu && (x.riichi||x.double_reach)) add(2,1);
                if(x.chankan) add(3,1); if(x.rinshan) add(4,1); if(x.haitei) add(5,1); if(x.houtei) add(6,1);
                if(!has_terminal && (closed || cfg.open_tanyao)) add(8,1);
                if(pairs) add(22,2);
                if(!pairs) {
                    for(int k:trips) { if(k>=31) add(18+k-31,1); if(k==x.seat_wind) add(10+k-27,1); if(k==x.round_wind) add(14+k-27,1); }
                    if(triplets==4) add(28,2); if(concealed_sets==3) add(29,2); if(kans==3) add(27,2);
                    if(pair>=31 && trips.count(31)+trips.count(32)+trips.count(33)==2) add(30,2);
                    int doubled=0; if(closed) for(auto s:seq_count) doubled+=s.second/2;
                    if(doubled>=2) add(32,3); else if(doubled) add(9,1);
                    for(int k=0;k<9;++k) if(trips.count(k)&&trips.count(k+9)&&trips.count(k+18)) { add(26,2); break; }
                    for(int k=0;k<7;++k) if(seqs.count(k)&&seqs.count(k+9)&&seqs.count(k+18)) { add(25,closed?2:1); break; }
                    for(int k=0;k<27;k+=9) if(seqs.count(k)&&seqs.count(k+3)&&seqs.count(k+6)) { add(24,closed?2:1); break; }
                    if(all_terminal && sequences) add(has_honor?23:33,has_honor?(closed?2:1):(closed?3:2));
                }
                if(!has_simple) add(31,2);
                if(suits.size()==1) add(has_honor?34:35,has_honor?(closed?3:2):(closed?6:5));
            }
            bool bad_wait=shape[win].size==2;
            const auto& w=shape[win]; if(w.sequence) bad_wait=tile/4==w.kind+1 || (w.kind%9==0 && tile/4==w.kind+2) || (w.kind%9==6 && tile/4==w.kind);
            const int pair_fu=pair<0?0:(pair>=31?2:0)+(pair==x.seat_wind?2:0)+(pair==x.round_wind?2:0);
            const bool pinfu=!pairs && !is_orphan && sequences==4 && !pair_fu && !bad_wait;
            if(pinfu && closed) add(7,1);
            int fu=pairs?25:20+extra_fu+pair_fu+(bad_wait?2:0);
            if(!pairs) { if(x.tsumo && !(pinfu&&closed)) fu+=2; if(!closed && fu==20) fu+=2; if(closed && !x.tsumo) fu+=10; fu=(fu+9)/10*10; }
            if(is_orphan) fu=0;
            if(cfg.ancient) {
                if(x.swallow) add(100,1); if(x.kan_discard) add(101,1);
                if(melds.size()==4 && std::all_of(melds.begin(),melds.end(),[](const Meld& m) { return m.opened(); })) add(102,1);
                bool winds=false,dragons=false; for(int k=27;k<31;++k) winds=winds||all[k]>0; for(int k=31;k<34;++k) dragons=dragons||all[k]>0;
                if(suits.size()==3 && winds && dragons) add(103,2);
                for(auto k:trips) if(k<27 && k%9<=6 && trips.count(k+1) && trips.count(k+2)) { add(104,2); break; }
                for(auto s:seq_count) if(s.second>=3) { erase(9); add(105,closed?3:2); break; }
                if(x.haitei && tile/4==9) { erase(5); add(106,5); } if(x.houtei && tile/4==17) { erase(6); add(107,5); }
                if(x.renhou) natural(36);
                if(chiitoi) {
                    for(int suit=0;suit<3;++suit) { bool wheel=true; for(int k=0;k<34;++k) if(all[k]!=(k>=suit*9+1 && k<=suit*9+7?2:0)) wheel=false; if(wheel) natural(suit==0?111:suit==1?109:110); }
                    if(suits.empty()) { yak.erase(std::remove_if(yak.begin(),yak.end(),[](auto y) { return y.first==42; }),yak.end()); natural(113,2); }
                }
                if(x.double_reach && (x.haitei || x.houtei)) natural(112);
            }
            int base_han=0; for(auto y:ys) base_han+=y.second;
            if(yak.empty() && !base_han) continue;
            int han=0; if(!yak.empty()) for(auto y:yak) han+=13*y.second;
            else {
                han=base_han;
                int dora=0,ura=0,red=0;
                for(auto d:x.dora) dora+=all[next_dora(d/4,cfg.n())];
                if(x.riichi || x.double_reach) for(auto d:x.ura) ura+=all[next_dora(d/4,cfg.n())];
                dora+=static_cast<int>(x.nuki.size())*(1+static_cast<int>(std::count_if(x.dora.begin(),x.dora.end(),[](int t) { return t/4==29; }))+((x.riichi || x.double_reach)?static_cast<int>(std::count_if(x.ura.begin(),x.ura.end(),[](int t) { return t/4==29; })):0));
                for(auto r:cfg.red_tiles()) red+=static_cast<int>(std::count(tiles.begin(),tiles.end(),r));
                add(52,dora); add(53,ura); add(54,red); han+=dora+ura+red;
            }
            auto v=payments(han,fu,x.dealer,x.tsumo,!yak.empty());
            if(yak.empty()) {
                if((cfg.minimum=="1" || cfg.minimum=="2" || cfg.minimum=="4") && base_han<number(cfg.minimum)) continue;
                const std::vector<std::string> limits{"mangan","haneman","baiman","sanbaiman","yakuman"}; auto limit=std::find(limits.begin(),limits.end(),cfg.minimum);
                if(limit!=limits.end() && v.level<static_cast<int>(limit-limits.begin())+1) continue;
            }
            if(yak.empty()) for(auto y:ys) { v.yaku.push_back(y.first); v.yaku.push_back(y.second); } for(auto y:yak) v.yakuman.push_back(y.first);
            auto key=[](const Value& a) { return std::make_tuple(a.main+2*a.additional,a.han,a.fu); };
            if(key(v)>key(best)) best=std::move(v);
        }
    }
    return best;
}
}
