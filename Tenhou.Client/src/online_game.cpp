#include "online_game.hpp"
#include "network_protocol.hpp"
#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace tenhou {
namespace {
[[noreturn]] void Fail(const std::string& reason) { throw std::runtime_error("Online game: " + reason); }
std::string Attr(const NetworkMessage& m, const std::string& key, bool required = true) {
    const auto it = m.attributes.find(key);
    if (it == m.attributes.end()) { if (required) Fail("missing " + key); return {}; }
    return it->second;
}
int Number(const std::string& text, int low, int high) {
    int n = 0; const auto p = std::from_chars(text.data(), text.data() + text.size(), n);
    if (text.empty() || p.ec != std::errc() || p.ptr != text.data() + text.size() || n < low || n > high)
        Fail("invalid integer or range");
    return n;
}
int Num(const NetworkMessage& m, const std::string& key, int low, int high) { return Number(Attr(m, key), low, high); }
std::vector<int> List(const std::string& text, int low, int high, std::size_t limit) {
    std::vector<int> result; std::size_t start = 0;
    for (;;) {
        const auto end = text.find(',', start);
        result.push_back(Number(text.substr(start, end == std::string::npos ? end : end - start), low, high));
        if (result.size() > limit) Fail("too many list values");
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return result;
}
int Count(const OnlinePlayer& p) { return static_cast<int>(p.known_hand.size()) + p.hidden_count; }
bool Contains(const std::vector<int>& v, int tile) { return std::find(v.begin(), v.end(), tile) != v.end(); }
void Remove(OnlinePlayer& p, int tile) {
    const auto it = std::find(p.known_hand.begin(), p.known_hand.end(), tile);
    if (it != p.known_hand.end()) p.known_hand.erase(it);
    else if (p.hidden_count) --p.hidden_count;
    else Fail("tile is not in actor's hand");
}
void Scores(OnlineState& s, const NetworkMessage& m) {
    const auto values = List(Attr(m, "sc"), -1000000, 1000000, 8);
    if (values.size() != 8) Fail("sc needs four base/delta pairs");
    for (int i = 0; i < 4; ++i) s.players[i].score = 100 * (values[i * 2] + values[i * 2 + 1]);
}
void Final(OnlineState& s, const NetworkMessage& m) {
    if (!m.attributes.count("owari")) return;
    const auto text = Attr(m, "owari");
    if (text.empty() || text.size() > 256) Fail("invalid final scores");
    // Floating point ranking points are display data, never another payment.
    std::istringstream input(text); std::string item; int count = 0;
    while (std::getline(input, item, ',')) {
        std::istringstream number(item); double value = 0;
        if (!(number >> value) || !number.eof() || !std::isfinite(value) || std::abs(value) > 1000000)
            Fail("invalid final score value");
        ++count;
    }
    if (count != 8 || text.back() == ',') Fail("owari needs eight values");
    s.final_scores = text; s.complete = true;
}
bool TileTag(const std::string& tag) {
    if (tag.empty()) return false;
    const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(tag[0])));
    return std::string("TUVWDEFG").find(c) != std::string::npos &&
        (tag.size() == 1 || (tag[1] >= '0' && tag[1] <= '9'));
}
}
bool OnlineGame::Handles(const std::string& tag) {
    return TileTag(tag) || tag == "GO" || tag == "UN" || tag == "BYE" || tag == "TAIKYOKU" || tag == "INIT" || tag == "REINIT" ||
        tag == "N" || tag == "DORA" || tag == "REACH" || tag == "FURITEN" || tag == "AGARI" || tag == "RYUUKYOKU" || tag == "PROF";
}
void OnlineGame::Receive(const NetworkMessage& message) {
    auto next = *this;
    try {
        if (next.state.events >= 100000) Fail("session event limit exceeded");
        next.Apply(message); next.Validate(); ++next.state.events; next.state.last_event = message.tag;
    } catch (const std::exception& e) { throw std::runtime_error(std::string(e.what()) + " [tag=" + message.tag + "]"); }
    *this = std::move(next);
}
void OnlineGame::Apply(const NetworkMessage& m) {
    auto& s = state; const auto& tag = m.tag;
    if (tag == "GO") {
        if (s.started && !s.complete) Fail("GO during a match");
        const auto type = static_cast<unsigned>(Num(m, "type", 0, 65535));
        if ((type&0x810)==0x810) Fail("special three-player rules are not restored");
        auto players = s.players; const auto events = s.events; s = {}; s.events = events; s.started = true; s.type = type;
        s.open_hands=m.attributes.count("open_hands") && Num(m,"open_hands",0,1)!=0;
        s.local_action_wait_ms=m.attributes.count("local_time")?static_cast<unsigned>(Num(m,"local_time",0,300000)):0;
        if(m.attributes.count("red_tiles")) {
            const auto text=Attr(m,"red_tiles");s.red_tiles=text.empty()?std::vector<int>{}:List(text,0,135,4);
            for(int t:s.red_tiles) if(t/4!=4 && t/4!=13 && t/4!=22) Fail("invalid red tile kind");
        }
        for (int i = 0; i < 4; ++i) { s.players[i].name = players[i].name; s.players[i].gender = players[i].gender; }
        if (type&16) {
            s.seat_mask=0;
            for(int i=0;i<4;++i) if(!s.players[i].name.empty()) s.seat_mask|=1u<<i;
        }
        return;
    }
    if (tag == "UN") {
        for (int i = 0; i < 4; ++i) {
            if (m.attributes.count("sx")) s.players[i].gender = SoundGender(Attr(m, "sx"), i);
            const auto key = "n" + std::to_string(i);
            if (m.attributes.count(key)) {
                const auto name = DecodeNetworkText(Attr(m, key));
                if (name.size() > 1024) Fail("player name exceeds limit");
                s.players[i].name = name; s.players[i].connected = true;
            }
        }
        if (s.type&16) {
            unsigned mask=0;
            for(int i=0;i<4;++i) if(!s.players[i].name.empty()) mask|=1u<<i;
            if (s.initialized && mask!=s.seat_mask) Fail("active seats changed within a hand");
            s.seat_mask=mask;
        }
        return;
    }
    if (!s.started) Fail("event before GO");
    if (tag == "BYE") { s.players[Num(m,"who",0,3)].connected = false; return; }
    if (tag == "FURITEN") {
        if (!s.initialized || s.ended) Fail("FURITEN outside a live hand");
        s.furiten = Num(m, "show", 0, 1) != 0;
        return;
    }
    if (tag == "TAIKYOKU") {
        if (s.initialized) Fail("TAIKYOKU after INIT");
        if (m.attributes.count("oya")) (void)Num(m, "oya", 0, 3);
        // No seat rotation: network actors/oya are already relative to self=0.
        // Replay-log token and absolute-seat hints are not persisted or logged.
        return;
    }
    if (tag == "INIT" || tag == "REINIT") {
        const bool restore = tag == "REINIT";
        if ((s.type&16) && (SeatCount(s.seat_mask)!=3 || !SeatPresent(s.seat_mask,0)))
            Fail("three-player INIT needs UN names identifying the absent seat");
        if (!restore && (s.complete || (s.initialized && !s.ended))) Fail("INIT before round end or after match end");
        if (restore && s.complete) Fail("REINIT after match end");
        const auto seed = List(Attr(m, "seed"), 0, 10000, restore ? 10 : 6);
        const auto scores = List(Attr(m, "ten"), -1000000, 1000000, 4);
        if (seed.size() < 6 || (!restore && seed.size() != 6) || scores.size() != 4 || seed[0] > 15 || seed[5] > 135) Fail("invalid INIT seed/ten");
        for (int i = 0; i < 4; ++i) if (m.attributes.count("hai" + std::to_string(i)))
            Fail("full replay hands are unsupported; expected online hai");
        const auto hand = List(Attr(m, "hai"), 0, 135, restore ? 14 : 13);
        if (!restore && hand.size() != 13) Fail("INIT needs thirteen local tiles");
        std::array<std::string, 4> names;
        std::array<char, 4> genders;
        std::array<bool, 4> connected;
        for (int i = 0; i < 4; ++i) names[i] = s.players[i].name;
        for (int i = 0; i < 4; ++i) genders[i] = s.players[i].gender;
        for (int i = 0; i < 4; ++i) connected[i] = s.players[i].connected;
        const auto events = s.events; const auto type = s.type; const auto seats=s.seat_mask;
        const auto open_hands=s.open_hands;const auto wait=s.local_action_wait_ms;const auto reds=s.red_tiles;
        s = {}; s.started = s.initialized = true; s.events = events; s.type = type;
        s.open_hands=open_hands;s.local_action_wait_ms=wait;s.red_tiles=reds;
        s.seat_mask=seats;
        s.round = seed[0]; s.honba = seed[1]; s.sticks = seed[2];
        s.dealer = Num(m, "oya", 0, 3); s.turn = s.dealer; s.dora_indicators = {seed[5]};
        if (!SeatPresent(seats,s.dealer)) Fail("absent dealer");
        if (!restore) s.wall.Init(s.dealer, seed[3], seed[4],type);
        for (int i = 0; i < 4; ++i) {
            auto& p = s.players[i]; p.name = names[i]; p.score = scores[i] * 100;
            p.gender = genders[i]; p.connected = connected[i];
            if (!SeatPresent(seats,i)) {
                if (scores[i]) Fail("absent seat has a score");
                p.connected=false; continue;
            }
            if (i == 0) { p.deal_hand = hand; p.known_hand = hand; std::sort(p.known_hand.begin(), p.known_hand.end()); }
            else if(s.open_hands) {
                p.deal_hand=List(Attr(m,"open"+std::to_string(i)),0,135,13);
                if(p.deal_hand.size()!=13) Fail("open room needs thirteen opponent tiles");
                p.known_hand=p.deal_hand;std::sort(p.known_hand.begin(),p.known_hand.end());
            } else p.hidden_count = 13;
        }
        if (restore) {
            s.turn = -1; s.awaiting_context = true;
            s.dora_indicators.assign(seed.begin() + 5, seed.end());
            for (int i = 0; i < 4; ++i) {
                auto& p = s.players[i];
                const auto melds = Attr(m, "m" + std::to_string(i), false);
                if (!SeatPresent(seats,i)) {
                    if (!melds.empty() || !Attr(m,"kawa"+std::to_string(i),false).empty()) Fail("absent seat has restored tiles");
                    continue;
                }
                auto restored_melds=melds.empty()?std::vector<int>{}:List(melds,0,65535,seats==15?4:8);
                // Original INIT/REINIT C97613 walks mN from the final item
                // backward before calling 0049a570. Preserve that order for
                // newly restored sanma, including the last visible north.
                if(type&16) std::reverse(restored_melds.begin(),restored_melds.end());
                for (int value : restored_melds) {
                    auto meld = DecodeReplayMeld(static_cast<unsigned>(value), i);
                    if (meld.kind==MeldKind::Nuki) {
                        if (!(type&16) || meld.tiles.front()/4!=30 || p.nuki.size()>=4) Fail("invalid restored north extraction");
                        p.nuki.push_back(meld.tiles.front()); continue;
                    }
                    if (!SeatPresent(seats,meld.from) || ((type&16) && meld.kind==MeldKind::Chi) || p.melds.size()>=4)
                        Fail("invalid restored meld source/count/kind");
                    // Decoder's added-kan called_tile is the added tile. Recover
                    // the historical pon-called entity for drawing/source links.
                    if (meld.kind == MeldKind::AddedKan) {
                        auto pon = DecodeReplayMeld((static_cast<unsigned>(value) & ~16u) | 8u, i);
                        meld.called_tile = pon.called_tile;
                    }
                    p.melds.push_back(meld);
                }
                const int base = 13 - 3 * static_cast<int>(p.melds.size());
                if (i == 0) {
                    if (Count(p) != base && Count(p) != base + 1) Fail("REINIT local hand/meld count differs");
                } else p.hidden_count = base;
                const auto river = Attr(m, "kawa" + std::to_string(i), false);
                bool reach = false, tsumogiri = false;
                if (!river.empty()) for (int value : List(river, 0, 255, 274)) {
                    if (value == 255) {
                        if (p.reach || reach) Fail("duplicate REINIT riichi marker");
                        reach = true; p.reach = 1;
                    } else if (value == 254) {
                        if (tsumogiri) Fail("duplicate REINIT tsumogiri marker");
                        tsumogiri = true;
                    } else {
                        if (value > 135) Fail("invalid REINIT river tile");
                        p.discards.push_back({value, false, reach, tsumogiri});
                        if (reach) p.reach = 2;
                        reach = tsumogiri = false;
                    }
                }
                if (tsumogiri) Fail("dangling REINIT tsumogiri marker");
                // Scores/sticks already include accepted riichi: never deduct again.
            }
            // Some producers include the claimed tile in the source river;
            // others omit it. Do not invent its missing historical position.
            for (int i = 0; i < 4; ++i) for (const auto& meld : s.players[i].melds) {
                if (meld.kind == MeldKind::ClosedKan) continue;
                auto& river = s.players[meld.from].discards;
                for (auto& d : river) if (d.tile == meld.called_tile) {
                    if (d.claimed) Fail("REINIT tile claimed twice");
                    d.claimed = true;
                }
            }
        }
        return;
    }
    if (!s.initialized) Fail("event before INIT");
    if (tag == "PROF") {
        if (!s.ended) Fail("PROF before round end");
        s.complete = true; return;
    }
    if (tag == "AGARI" || tag == "RYUUKYOKU") {
        s.settlement.red_tiles=s.red_tiles;
        AppendSettlement(s.settlement, tag, m.attributes,s.seat_mask);
        ValidateSettlementDisclosure(s.settlement.receipts.back(), OnlineTableState(s));
        if (tag == "AGARI") {
            const int who = Num(m, "who", 0, 3);
            (void)Num(m, "fromWho", 0, 3); (void)Num(m, "machi", 0, 135);
            if ((s.ended && s.result != "AGARI") || Contains(s.winners, who)) Fail("invalid additional winner");
            s.winners.push_back(who); s.result = "AGARI"; s.sticks = 0;
        } else {
            if (s.ended) Fail("duplicate round end");
            s.result = "RYUUKYOKU";
            const auto reason = Attr(m, "type", false);
            if (reason.size() > 64) Fail("draw reason exceeds limit");
            if (!reason.empty()) s.result += ":" + reason;
        }
        Scores(s, m); Final(s, m); s.ended = true; s.turn = -1; s.awaiting_context = false; return;
    }
    if (s.ended || s.complete) Fail("event after round end");
    if (tag == "DORA") {
        if (s.dora_indicators.size() >= 5) Fail("too many dora indicators");
        s.dora_indicators.push_back(Num(m, "hai", 0, 135)); return;
    }
    if (tag == "REACH") {
        const int who = Num(m, "who", 0, 3), step = Num(m, "step", 1, 2); auto& p = s.players[who];
        if (!SeatPresent(s.seat_mask,who)) Fail("absent riichi actor");
        if (step == 1) {
            if (s.awaiting_context) {
                s.turn = who;
                if (who != 0 && Count(p) % 3 == 1) ++p.hidden_count;
            }
            if (s.turn != who || Count(p) % 3 != 2 || p.reach) Fail("invalid riichi declaration");
            p.reach = 1;
        } else {
            if (p.reach != 1 || p.discards.empty() || !p.discards.back().riichi) Fail("invalid riichi acceptance");
            p.reach = 2; p.score -= 1000; ++s.sticks;
            if (m.attributes.count("ten")) {
                const auto scores = List(Attr(m, "ten"), -1000000, 1000000, 4);
                if (scores.size() != 4) Fail("riichi ten needs four scores");
                for (int i = 0; i < 4; ++i) s.players[i].score = scores[i] * 100;
            }
        }
        return;
    }
    if (tag == "N") {
        const int who = Num(m, "who", 0, 3); auto& p = s.players[who];
        if (!SeatPresent(s.seat_mask,who)) Fail("absent meld actor");
        auto meld = DecodeReplayMeld(static_cast<std::uint32_t>(Num(m, "m", 0, 65535)), who);
        if (meld.kind==MeldKind::Nuki) {
            if (!(s.type&16) || meld.tiles.front()/4!=30 || p.nuki.size()>=4) Fail("invalid north extraction");
            if (s.awaiting_context) {
                s.turn=who;
                if (who!=0 && Count(p)%3==1) ++p.hidden_count;
                s.awaiting_context=false;
            }
            if (s.turn!=who || Count(p)%3!=2) Fail("north extraction out of turn/phase");
            Remove(p,meld.tiles.front()); p.nuki.push_back(meld.tiles.front());
            p.drawn_tile=-1; p.draw_pending=false; s.turn=who; s.last_discard_seat=-1; s.wall.Kan(); return;
        }
        if (!SeatPresent(s.seat_mask,meld.from) || ((s.type&16) && meld.kind==MeldKind::Chi)) Fail("invalid three-player meld source/kind");
        if (meld.kind == MeldKind::OpenKan || meld.kind == MeldKind::ClosedKan || meld.kind == MeldKind::AddedKan) s.wall.Kan();
        if (s.awaiting_context) {
            if (meld.kind == MeldKind::ClosedKan || meld.kind == MeldKind::AddedKan) {
                s.turn = who;
                if (who != 0 && Count(p) % 3 == 1) ++p.hidden_count;
            } else s.last_discard_seat = meld.from;
            s.awaiting_context = false;
        }
        if (meld.kind == MeldKind::AddedKan) {
            if (s.turn != who || Count(p) % 3 != 2) Fail("added kan out of turn");
            auto pon = std::find_if(p.melds.begin(), p.melds.end(), [&](const ReplayMeld& old) {
                return old.kind == MeldKind::Pon && old.tiles.front() / 4 == meld.tiles.front() / 4;
            });
            if (pon == p.melds.end() || pon->from != meld.from || Contains(pon->tiles, meld.called_tile)) Fail("added kan lacks matching pon");
            Remove(p, meld.called_tile); pon->kind = MeldKind::AddedKan; pon->tiles = meld.tiles; pon->encoded = meld.encoded;
        } else {
            if (p.melds.size() >= 4) Fail("too many melds");
            if (meld.kind == MeldKind::ClosedKan) {
                if (s.turn != who || Count(p) % 3 != 2) Fail("closed kan out of turn");
            } else {
                if (s.last_discard_seat != meld.from || meld.from == who || Count(p) % 3 != 1) Fail("invalid meld source/phase");
                auto& river = s.players[meld.from].discards;
                if (river.empty() || river.back().claimed || river.back().tile != meld.called_tile) Fail("called tile differs from latest discard");
                river.back().claimed = true;
            }
            for (int tile : meld.tiles) if (tile != meld.called_tile || meld.kind == MeldKind::ClosedKan) Remove(p, tile);
            p.melds.push_back(meld);
        }
        p.drawn_tile = -1; p.draw_pending = false; s.turn = who; s.last_discard_seat = -1; return;
    }
    if (TileTag(tag)) {
        const char code = static_cast<char>(std::toupper(static_cast<unsigned char>(tag[0])));
        const auto draw = std::string("TUVW").find(code), discard = std::string("DEFG").find(code);
        const bool drawing = draw != std::string::npos;
        const int who = static_cast<int>(drawing ? draw : discard); auto& p = s.players[who];
        if (!SeatPresent(s.seat_mask,who)) Fail("absent tile actor");
        if (s.awaiting_context) {
            s.turn = who;
            if (!drawing && who != 0 && Count(p) % 3 == 1) ++p.hidden_count;
            s.awaiting_context = false;
        }
        if (s.turn != who) Fail("tile event out of turn");
        // t is an action-availability hint. No action is inferred or transmitted.
        for (const auto& a : m.attributes) if (a.first == "t") (void)Number(a.second, 0, 65535); else Fail("unsupported tile attribute");
        const int tile = tag.size() > 1 ? Number(tag.substr(1), 0, 135) : -1;
        if (drawing) {
            if (Count(p) % 3 != 1 || p.draw_pending) Fail("invalid draw phase");
            if (who == 0 || s.open_hands) {
                if (tile < 0) Fail("local draw needs a known tile");
                p.known_hand.push_back(tile); std::sort(p.known_hand.begin(), p.known_hand.end()); p.drawn_tile = tile;
            } else {
                if (tile >= 0) Fail("opponent draw must be hidden in this stream");
                ++p.hidden_count;
            }
            p.draw_pending = true; s.last_discard_seat = -1;
            s.wall.Draw();
        } else {
            if (tile < 0 || Count(p) % 3 != 2) Fail("invalid discard phase/tile");
            const bool riichi = p.reach == 1 && std::none_of(p.discards.begin(), p.discards.end(), [](const ReplayDiscard& d) { return d.riichi; });
            const bool tsumogiri = who == 0 ? p.drawn_tile == tile : std::islower(static_cast<unsigned char>(tag[0])) != 0;
            // A newly revealed hidden tile cannot duplicate any already-known entity.
            if (who != 0 && !s.open_hands) {
                for (const auto& other : s.players) {
                    if (Contains(other.known_hand, tile)) Fail("discard duplicates a known hand tile");
                    for (const auto& d : other.discards) if (d.tile == tile) Fail("discard duplicates historical tile");
                    for (const auto& meld : other.melds) if (Contains(meld.tiles, tile)) Fail("discard duplicates a meld tile");
                }
                if (Contains(s.dora_indicators, tile)) Fail("discard duplicates dora");
            }
            Remove(p, tile); p.discards.push_back({tile, false, riichi, tsumogiri});
            p.drawn_tile = -1; p.draw_pending = false; s.last_discard_seat = who; s.turn = NextSeat(s.seat_mask,who);
        }
        return;
    }
    Fail("unsupported event");
}
void OnlineGame::Validate() const {
    const auto& s = state; std::array<bool, 136> used{}, historical{};
    auto use = [&](int tile) {
        if (tile < 0 || tile > 135 || used[static_cast<std::size_t>(tile)]) Fail("duplicate/invalid known physical tile");
        used[static_cast<std::size_t>(tile)] = true;
    };
    for (int tile : s.dora_indicators) use(tile);
    for (int i = 0; i < 4; ++i) {
        const auto& p = s.players[i];
        if (Count(p) < 0 || Count(p) > 14 || p.hidden_count < 0 || p.discards.size() > 136) Fail("hand/river exceeds bounds");
        if ((i == 0 && p.hidden_count) || (i != 0 && !s.open_hands && !p.known_hand.empty()) || (s.open_hands && p.hidden_count)) Fail("invalid information visibility");
        for (int tile : p.known_hand) use(tile);
        for (const auto& d : p.discards) {
            if (historical[static_cast<std::size_t>(d.tile)]) Fail("duplicate historical discard");
            historical[static_cast<std::size_t>(d.tile)] = true;
            if (!d.claimed) use(d.tile);
        }
        for (const auto& meld : p.melds) for (int tile : meld.tiles) use(tile);
        for (int tile : p.nuki) use(tile);
    }
}
ReplayState OnlineTableState(const OnlineState& s) {
    ReplayState r; r.initialized = s.initialized; r.ended = s.ended;
    r.furiten = s.furiten; r.type = s.type;
    r.seat_mask=s.seat_mask;
    r.red_tiles=s.red_tiles;
    r.wall = s.wall;
    r.round = s.round; r.honba = s.honba; r.sticks = s.sticks; r.dealer = s.dealer;
    r.turn = s.turn; r.last_discard_seat = s.last_discard_seat; r.dora_indicators = s.dora_indicators;
    r.result = s.result; r.last_event = s.last_event; r.winners = s.winners;
    r.settlement = s.settlement;
    for (int i = 0; i < 4; ++i) {
        const auto& p = s.players[i]; auto& q = r.players[i]; q.name = p.name; q.gender = p.gender; q.connected = p.connected; q.score = p.score;
        q.reach = p.reach; q.drawn_tile = p.drawn_tile; q.hand = p.known_hand; q.deal_hand = p.deal_hand;
        q.hand.insert(q.hand.end(), static_cast<std::size_t>(p.hidden_count), -1);
        q.discards = p.discards; q.melds = p.melds; q.nuki=p.nuki;
    }
    return r;
}
std::string OnlineStateJson(const OnlineState& s) {
    std::ostringstream out;
    out << "{\"started\":" << (s.started ? "true" : "false") << ",\"complete\":" << (s.complete ? "true" : "false")
        << ",\"awaiting_context\":" << (s.awaiting_context ? "true" : "false")
        << ",\"type\":" << s.type << ",\"events\":" << s.events << ",\"final_scores\":" << NetworkJson(s.final_scores)
        << ",\"settlement\":" << SettlementJson(s.settlement)
        << ",\"table\":" << ReplayStateJson(OnlineTableState(s)) << ",\"visibility\":[";
    for (int i = 0; i < 4; ++i) {
        if (i) out << ','; const auto& p = s.players[i];
        out << "{\"known\":" << p.known_hand.size() << ",\"hidden\":" << p.hidden_count
            << ",\"draw_pending\":" << (p.draw_pending ? "true" : "false") << '}';
    }
    out << "]}"; return out.str();
}
} // namespace tenhou
