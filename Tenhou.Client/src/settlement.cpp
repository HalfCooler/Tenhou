#include "settlement.hpp"
#include "replay.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>

namespace tenhou {
namespace {
using Attributes = std::map<std::string, std::string>;
[[noreturn]] void Fail(const std::string& message) { throw std::runtime_error("Settlement: " + message); }
int Integer(const std::string& text, int low, int high) {
    int value = 0; const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (text.empty() || parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() || value < low || value > high)
        Fail("invalid integer or range");
    return value;
}
std::vector<int> List(const Attributes& a, const std::string& key, int low, int high, std::size_t limit, bool required = false) {
    const auto found = a.find(key);
    if (found == a.end()) { if (required) Fail("missing " + key); return {}; }
    if (found->second.size() > 4096) Fail(key + " exceeds limit");
    std::vector<int> values; std::size_t start = 0;
    for (;;) {
        const auto end = found->second.find(',', start);
        values.push_back(Integer(found->second.substr(start, end == std::string::npos ? end : end - start), low, high));
        if (values.size() > limit) Fail("too many " + key + " values");
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return values;
}
int Number(const Attributes& a, const std::string& key, int high) {
    const auto value = List(a, key, 0, high, 1, true); return value.front();
}
void Unique(const std::vector<int>& values) {
    if (std::set<int>(values.begin(), values.end()).size() != values.size()) Fail("duplicate physical tile");
}
std::string Quote(const std::string& value) {
    std::string out = "\""; const char* hex = "0123456789abcdef";
    for (unsigned char c : value) {
        if (c == '\\' || c == '"') { out += '\\'; out += static_cast<char>(c); }
        else if (c < 32) { out += "\\u00"; out += hex[c >> 4]; out += hex[c & 15]; }
        else out += static_cast<char>(c);
    }
    return out + '"';
}
template<class T> void Array(std::ostream& out, const T& values) {
    out << '['; bool comma = false;
    for (const auto& value : values) { if (comma) out << ','; comma = true; out << value; }
    out << ']';
}
std::string Tiles(const std::vector<int>& values, bool red) {
    std::string result;
    for (int tile : values) {
        auto label = TileLabel(tile); if (!red && label.back() == '*') label.pop_back();
        if (!result.empty()) result += ' ';
        result += label;
    }
    return result;
}
std::string Seat(int seat, const std::array<std::string, 4>& names) {
    return "Seat " + std::to_string(seat) + (names[seat].empty() ? "" : "  " + names[seat]);
}
}
void AppendSettlement(SettlementState& state, const std::string& tag, const Attributes& a, unsigned seat_mask) {
    // Validate everything before committing, including final display values.
    auto next = state; SettlementReceipt r; r.tag = tag; r.seat_mask=seat_mask;
    if (SeatCount(seat_mask)!=3 && seat_mask!=15) Fail("invalid active seats");
    if (!next.receipts.empty() && next.seat_mask!=seat_mask) Fail("active seats changed within a result");
    next.seat_mask=seat_mask;
    if (tag != "AGARI" && tag != "RYUUKYOKU") Fail("unsupported receipt");
    if (next.receipts.size() >= 4) Fail("too many round receipts");
    const auto scores = List(a, "sc", -1000000, 1000000, 8, true);
    if (scores.size() != 8) Fail("sc needs four base/delta pairs");
    for (int i = 0; i < 4; ++i) { r.base[i] = scores[i * 2] * 100; r.delta[i] = scores[i * 2 + 1] * 100; }
    for (int i=0;i<4;++i) if (!SeatPresent(seat_mask,i) && (r.base[i] || r.delta[i])) Fail("absent seat has settlement scores");
    r.ba = List(a, "ba", 0, 1000000, 2);
    if (!r.ba.empty() && r.ba.size() != 2) Fail("ba needs two values");
    if (tag == "AGARI") {
        r.who = Number(a, "who", 3); r.from = Number(a, "fromWho", 3); r.winning_tile = Number(a, "machi", 135);
        if (!SeatPresent(seat_mask,r.who) || !SeatPresent(seat_mask,r.from)) Fail("absent winner/source");
        for (const auto& old : next.receipts) {
            if (old.tag != tag || old.who == r.who) Fail("duplicate or incompatible winner");
            if (old.who == old.from || r.who == r.from || old.from != r.from || old.winning_tile != r.winning_tile)
                Fail("additional ron differs from the shared discard");
        }
        r.has_hand = a.count("hai") != 0; r.hand = List(a, "hai", 0, 135, 14);
        r.has_melds = a.count("m") != 0;
        if (r.has_melds && !a.at("m").empty()) r.melds = List(a, "m", 0, 65535, seat_mask==15?4:8);
        r.has_yaku = a.count("yaku") != 0; const auto yaku = List(a, "yaku", 0, 65535, 128);
        if (yaku.size() % 2) Fail("yaku needs id/han pairs");
        std::set<int> ids;
        for (std::size_t i = 0; i < yaku.size(); i += 2) {
            if (!ids.insert(yaku[i]).second || yaku[i + 1] > 1000) Fail("invalid yaku pair");
            r.yaku.push_back({yaku[i], yaku[i + 1]});
        }
        r.has_yakuman = a.count("yakuman") != 0; r.yakuman = List(a, "yakuman", 0, 65535, 64);
        if (std::set<int>(r.yakuman.begin(), r.yakuman.end()).size() != r.yakuman.size()) Fail("duplicate yakuman id");
        if (r.has_yaku && r.has_yakuman) Fail("both yaku and yakuman supplied");
        r.has_ten = a.count("ten") != 0; r.ten = List(a, "ten", 0, 100000000, 3);
        if (r.has_ten && r.ten.size() != 3) Fail("AGARI ten needs fu/points/limit");
        r.dora = List(a, "doraHai", 0, 135, 5); r.ura = List(a, "doraHaiUra", 0, 135, 5);
        auto physical = r.hand;
        std::size_t groups=0, extracted=0;
        for (int code : r.melds) {
            const auto meld = DecodeReplayMeld(static_cast<unsigned>(code), r.who);
            if (meld.kind==MeldKind::Nuki) {
                if (seat_mask==15 || meld.tiles.front()/4!=30 || ++extracted>4) Fail("invalid settlement north extraction");
            } else {
                if (++groups>4 || !SeatPresent(seat_mask,meld.from)) Fail("invalid settlement meld source/count");
            }
            physical.insert(physical.end(), meld.tiles.begin(), meld.tiles.end());
        }
        if (r.has_hand) {
            if (std::find(r.hand.begin(), r.hand.end(), r.winning_tile) == r.hand.end()) Fail("machi missing from hai");
            if (r.has_melds && r.hand.size() != 14 - groups * 3) Fail("hai/meld count differs");
            if (r.hand.size() % 3 != 2) Fail("invalid winning hand count");
        }
        physical.insert(physical.end(), r.dora.begin(), r.dora.end());
        physical.insert(physical.end(), r.ura.begin(), r.ura.end()); Unique(physical);
        for (const auto& old : next.receipts) for (int tile : old.hand)
            if (tile != r.winning_tile && std::find(physical.begin(), physical.end(), tile) != physical.end())
                Fail("winning hands share a concealed tile");
    } else {
        if (!next.receipts.empty()) Fail("duplicate draw receipt");
        const auto reason = a.find("type");
        if (reason != a.end()) { if (reason->second.size() > 64) Fail("draw reason exceeds limit"); r.reason = reason->second; }
        std::vector<int> physical;
        for (int i = 0; i < 4; ++i) {
            const auto key = "hai" + std::to_string(i); r.has_revealed[i] = a.count(key) != 0;
            r.revealed[i] = List(a, key, 0, 135, 14);
            if (!SeatPresent(seat_mask,i) && !r.revealed[i].empty()) Fail("absent seat discloses a hand");
            // Abortive draws may disclose fourteen tiles; no tenpai inference.
            physical.insert(physical.end(), r.revealed[i].begin(), r.revealed[i].end());
        }
        Unique(physical);
    }
    if (a.count("owari")) {
        const auto& text = a.at("owari"); if (text.empty() || text.size() > 256) Fail("invalid owari");
        std::istringstream input(text); input.imbue(std::locale::classic()); std::string token; std::vector<double> values;
        while (std::getline(input, token, ',')) {
            std::istringstream number(token); number.imbue(std::locale::classic()); double value = 0;
            if (token.empty() || !(number >> std::noskipws >> value) || !number.eof() || !std::isfinite(value) || std::abs(value) > 1000000)
                Fail("invalid owari number");
            values.push_back(value);
        }
        if (values.size() != 8 || text.back() == ',') Fail("owari needs four score/point pairs");
        if (next.has_final) for (int i = 0; i < 4; ++i)
            if (next.final_scores[i] != values[i * 2] * 100 || next.final_points[i] != values[i * 2 + 1]) Fail("conflicting owari");
        for (int i = 0; i < 4; ++i) { next.final_scores[i] = values[i * 2] * 100; next.final_points[i] = values[i * 2 + 1]; }
        for (int i=0;i<4;++i) if (!SeatPresent(seat_mask,i) && (next.final_scores[i] || next.final_points[i])) Fail("absent final score");
        next.has_final = true;
    }
    next.receipts.push_back(std::move(r)); state = std::move(next);
}
std::string SettlementYakuName(int id) {
    // Original _res.dat image IDs 0x1e1+id, independently inspected and hashed.
    static const char* names[]{"門前清自摸和", "立直", "一発", "槍槓", "嶺上開花", "海底摸月", "河底撈魚", "平和", "断么九", "一盃口",
        "自風 東", "自風 南", "自風 西", "自風 北", "場風 東", "場風 南", "場風 西", "場風 北", "役牌 白", "役牌 發", "役牌 中",
        "両立直", "七対子", "混全帯么九", "一気通貫", "三色同順", "三色同刻", "三槓子", "対々和", "三暗刻", "小三元", "混老頭", "二盃口",
        "純全帯么九", "混一色", "清一色", "人和", "天和", "地和", "大三元", "四暗刻", "四暗刻単騎", "字一色", "緑一色", "清老頭", "九蓮宝燈",
        "純正九蓮宝燈", "国士無双", "国士無双１３面", "大四喜", "小四喜", "四槓子", "ドラ", "裏ドラ", "赤ドラ"};
    static const char* ancient[]{"燕返","杠振","十二落抬","五门齐","三连刻","一色三同顺","一筒摸月","九筒摸鱼","人和","大车轮","大竹林","大数邻","石上三年","大七星","拔北宝牌"};
    if(id>=100 && id<=114) return ancient[id-100];
    return id >= 0 && id < 55 ? names[id] : "Unknown yaku #" + std::to_string(id);
}
std::vector<SettlementYaku> SettlementDisplayYaku(const SettlementReceipt& r) {
    if (!r.yakuman.empty()) return r.yaku;
    std::vector<SettlementYaku> yaku;
    for (auto item : r.yaku) {
        // Older local replays used ID 114 for extracted-north bonus.
        // Preserve the receipt; share one dora row and reveal step in the UI.
        if (item.id==114) item.id=52;
        const auto dora=std::find_if(yaku.begin(),yaku.end(),[](const SettlementYaku& y) { return y.id==52; });
        if (item.id==52 && dora!=yaku.end()) dora->han+=item.han;
        else yaku.push_back(item);
    }
    const bool riichi = std::any_of(yaku.begin(),yaku.end(),[](const SettlementYaku& y) { return y.id==1 || y.id==21; });
    if (riichi && std::none_of(yaku.begin(),yaku.end(),[](const SettlementYaku& y) { return y.id==53; })) yaku.push_back({53,0});
    const auto rank=[](int id) {
        if (id==1 || id==21) return 0; // Riichi / double riichi.
        if (id>=2 && id<=6) return 1; // Ippatsu, chankan, rinshan, haitei, houtei.
        return id==53 ? 3 : 2;
    };
    std::stable_sort(yaku.begin(),yaku.end(),[&](const SettlementYaku& a,const SettlementYaku& b) { return rank(a.id)<rank(b.id); });
    return yaku;
}
void ValidateSettlementDisclosure(const SettlementReceipt& r, const ReplayState& table) {
    const auto validate = [&](const std::vector<int>& hand, int seat, int winning_tile) {
        auto expected = table.players[seat].hand;
        const bool hidden = std::find(expected.begin(), expected.end(), -1) != expected.end();
        if (winning_tile >= 0 && r.who != r.from) expected.push_back(winning_tile);
        if (expected.size() != hand.size()) Fail("disclosed hand count differs from table");
        if (!hidden) {
            auto actual = hand; std::sort(actual.begin(), actual.end()); std::sort(expected.begin(), expected.end());
            if (actual != expected) Fail("disclosed hand differs from known hand");
        }
        for (int tile : hand) {
            if (tile == winning_tile && r.who != r.from) continue; // Shared ron tile also remains in the source river/hand.
            if (std::find(table.dora_indicators.begin(), table.dora_indicators.end(), tile) != table.dora_indicators.end())
                Fail("disclosed hand overlaps an indicator");
            for (int i = 0; i < 4; ++i) {
                const auto& p = table.players[i];
                if (i != seat && std::find(p.hand.begin(), p.hand.end(), tile) != p.hand.end()) Fail("disclosed hand overlaps another known hand");
                for (const auto& d : p.discards) if (d.tile == tile) Fail("disclosed hand overlaps a river");
                for (const auto& m : p.melds) if (std::find(m.tiles.begin(), m.tiles.end(), tile) != m.tiles.end()) Fail("disclosed hand overlaps a meld");
                if (std::find(p.nuki.begin(),p.nuki.end(),tile)!=p.nuki.end()) Fail("disclosed hand overlaps extracted north");
            }
        }
    };
    if (r.has_hand) validate(r.hand, r.who, r.winning_tile);
    for (int i = 0; i < 4; ++i) if (r.has_revealed[i]) validate(r.revealed[i], i, -1);
}
std::string SettlementLabel(const SettlementReceipt& r) {
    return r.tag == "AGARI" ? (r.who == r.from ? "Tsumo" : "Ron") + std::string(" | Seat ") + std::to_string(r.who) :
        "Draw" + (r.reason.empty() ? std::string() : " | " + r.reason);
}
std::string SettlementText(const SettlementState& s, std::size_t selection, const std::array<std::string, 4>& names, bool red, bool totals) {
    std::ostringstream out;
    if (selection == s.receipts.size() && s.has_final) {
        out << "MATCH RESULTS | supplied scores / points\r\n";
        for (int i = 0; i < 4; ++i) if (SeatPresent(s.seat_mask,i)) out << Seat(i, names) << " | Score " << s.final_scores[i] << " | Points " << std::showpos << s.final_points[i] << std::noshowpos << "\r\n";
        return out.str();
    }
    if (selection >= s.receipts.size()) return "No settlement received.";
    const auto& r = s.receipts[selection]; out << SettlementLabel(r) << "\r\n";
    if (r.who >= 0) {
        out << Seat(r.who, names) << (r.who == r.from ? " | Self draw" : " | From " + Seat(r.from, names)) << "\r\n";
        out << "Winning tile: " << Tiles({r.winning_tile}, red) << "\r\n";
        out << "Revealed hand: " << (r.has_hand ? Tiles(r.hand, red) : "not supplied") << "\r\nMelds: ";
        if (!r.has_melds) out << "not supplied";
        for (int code : r.melds) { const auto m = DecodeReplayMeld(static_cast<unsigned>(code), r.who); out << MeldKindName(m.kind) << " [" << Tiles(m.tiles, red) << "] from seat " << m.from << "  "; }
        out << "\r\nDora indicators: " << (r.dora.empty() ? "not supplied" : Tiles(r.dora, red))
            << "\r\nUra indicators: " << (r.ura.empty() ? "not supplied" : Tiles(r.ura, red)) << "\r\n";
        if (totals) {
            if (r.has_ten) out << "Fu: " << r.ten[0] << " | Hand points: " << r.ten[1] << " | Limit code: " << r.ten[2] << "\r\n";
            else out << "Fu / hand points: not supplied\r\n";
        }
        if (r.has_yaku) {
            int han = 0; for (const auto& y : r.yaku) han += y.han;
            if (totals) out << "Han (sum of supplied values): " << han << "\r\n";
            for (const auto& y : r.yaku) out << "  " << SettlementYakuName(y.id) << " [" << y.id << "]  " << y.han << " han\r\n";
        } else if (r.has_yakuman) {
            out << "Yakuman (supplied IDs; no multiplier inferred):\r\n";
            for (int id : r.yakuman) out << "  " << SettlementYakuName(id) << " [" << id << "]\r\n";
        } else out << "Yaku: not supplied\r\n";
    } else {
        out << "Reason code: " << (r.reason.empty() ? "ordinary exhaustive draw" : r.reason) << "\r\n";
        for (int i = 0; i < 4; ++i) if (SeatPresent(r.seat_mask,i)) out << Seat(i, names) << " revealed: " << (r.has_revealed[i] ? Tiles(r.revealed[i], red) : "not supplied (hidden)") << "\r\n";
    }
    if (!r.ba.empty()) out << "Honba / sticks supplied: " << r.ba[0] << " / " << r.ba[1] << "\r\n";
    if (!totals) return out.str();
    out << "\r\nSCORE CHANGES (this receipt)\r\n";
    for (int i = 0; i < 4; ++i) if (SeatPresent(r.seat_mask,i)) out << Seat(i, names) << ": " << r.base[i] << "  " << std::showpos << r.delta[i] << std::noshowpos << "  = " << r.base[i] + r.delta[i] << "\r\n";
    out << "\r\nSupplied results only; no local scoring or hidden-hand inference.";
    return out.str();
}
std::string SettlementJson(const SettlementState& s) {
    std::ostringstream out; out << std::setprecision(15) << "{\"receipts\":[";
    for (std::size_t i = 0; i < s.receipts.size(); ++i) {
        if (i) out << ','; const auto& r = s.receipts[i];
        out << "{\"tag\":" << Quote(r.tag) << ",\"reason\":" << Quote(r.reason) << ",\"who\":" << r.who << ",\"from\":" << r.from << ",\"winning_tile\":" << r.winning_tile;
        out << ",\"has_hand\":" << (r.has_hand ? "true" : "false") << ",\"has_melds\":" << (r.has_melds ? "true" : "false")
            << ",\"has_yaku\":" << (r.has_yaku ? "true" : "false") << ",\"has_yakuman\":" << (r.has_yakuman ? "true" : "false") << ",\"has_ten\":" << (r.has_ten ? "true" : "false");
        const auto field = [&](const char* name, const auto& value) { out << ",\"" << name << "\":"; Array(out, value); };
        field("hand", r.hand); field("melds", r.melds); field("dora", r.dora); field("ura", r.ura); field("yakuman", r.yakuman); field("ten", r.ten); field("ba", r.ba); field("base", r.base); field("delta", r.delta);
        out << ",\"has_revealed\":["; for (int j = 0; j < 4; ++j) { if (j) out << ','; out << (r.has_revealed[j] ? "true" : "false"); }
        out << "],\"revealed\":["; for (int j = 0; j < 4; ++j) { if (j) out << ','; Array(out, r.revealed[j]); }
        out << "],\"yaku\":["; for (std::size_t j = 0; j < r.yaku.size(); ++j) { if (j) out << ','; out << '[' << r.yaku[j].id << ',' << r.yaku[j].han << ']'; } out << "]}";
    }
    out << "],\"has_final\":" << (s.has_final ? "true" : "false") << ",\"final_scores\":"; Array(out, s.final_scores);
    out << ",\"final_points\":"; Array(out, s.final_points); out << '}'; return out.str();
}
} // namespace tenhou
