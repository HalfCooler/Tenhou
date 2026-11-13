#include "variant_state.hpp"
#include "network_protocol.hpp"
#include <algorithm>
#include <charconv>
#include <limits>
#include <stdexcept>

namespace tenhou {
    namespace {
        using Fields = std::map<std::string, std::string>;
        std::string Get(const Fields &f, const std::string &key, const std::string &fallback = "") {
            const auto it = f.find(key);
            return it == f.end() ? fallback : it->second;
        }
        int Number(const std::string &text, int low, int high) {
            int n = 0;
            const auto read = std::from_chars(text.data(), text.data() + text.size(), n);
            if (read.ec != std::errc() || read.ptr != text.data() + text.size() || n < low || n > high)
                throw std::runtime_error("Invalid variant number");
            return n;
        }
        std::vector<std::string> Split(const std::string &text, char separator) {
            std::vector<std::string> result;
            if (text.empty()) return result;
            std::size_t start = 0;
            for (;;) {
                const auto end = text.find(separator, start);
                const auto value = text.substr(start, end == std::string::npos ? end : end - start);
                if (value.empty()) throw std::runtime_error("Empty variant list item");
                result.push_back(value);
                if (end == std::string::npos) break;
                start = end + 1;
            }
            return result;
        }
        std::vector<int> Numbers(const std::string &text, int low, int high, std::size_t limit, bool unique = true) {
            std::vector<int> result;
            for (const auto &part: Split(text, ',')) {
                const int n = Number(part, low, high);
                if (result.size() >= limit || (unique && std::find(result.begin(), result.end(), n) != result.end()))
                    throw std::runtime_error("Invalid variant list");
                result.push_back(n);
            }
            return result;
        }
        std::string Escape(const std::string &s) {
            std::string result;
            for (char c: s) {
                if (c == '&') result += "&amp;";
                else if (c == '"') result += "&quot;";
                else if (c == '<') result += "&lt;";
                else result += c;
            }
            return result;
        }
    }

    VariantState ParseVariantState(const std::map<std::string, std::string> &f) {
        VariantState s;
        s.rule = Get(f, "rule");
        const int tile_max = s.Sichuan() ? 107 : 135;
        const auto phase = Get(f, "phase");
        const std::map<std::string, VariantPhase> phases{{"exchange", VariantPhase::Exchange}, {"missing", VariantPhase::Missing},
            {"turn", VariantPhase::Turn}, {"claims", VariantPhase::Claims}, {"ended", VariantPhase::Ended}, {"complete", VariantPhase::Complete}};
        const auto stage = phases.find(phase);
        if (stage == phases.end()) throw std::runtime_error("Invalid variant phase");
        s.phase = stage->second;
        s.local = Number(Get(f, "seat"), 0, 3);
        s.turn = Number(Get(f, "turn"), 0, 3);
        s.dealer = Number(Get(f, "dealer"), 0, 3);
        s.hand_number = Number(Get(f, "hand_number"), 1, 256);
        s.total_hands = Number(Get(f, "total_hands"), s.hand_number, 256);
        s.wall_total = Number(Get(f, "wall_total", s.Sichuan() ? "108" : "144"), 108, 144);
        if (s.wall_total != 108 && s.wall_total != 136 && s.wall_total != 144) throw std::runtime_error("Invalid variant wall");
        s.wall = Number(Get(f, "wall"), 0, s.wall_total);
        s.wall_front = Number(Get(f, "wall_front", std::to_string(s.wall_total - s.wall)), 0, s.wall_total);
        s.wall_tail = Number(Get(f, "wall_tail", "0"), 0, s.wall_total);
        s.wall_offset = Number(Get(f, "wall_offset", "0"), 0, s.wall_total - 1);
        if (s.wall_front + s.wall_tail + s.wall != s.wall_total) throw std::runtime_error("Inconsistent variant wall cursors");
        const auto seq = Get(f, "seq", "0");
        const auto read = std::from_chars(seq.data(), seq.data() + seq.size(), s.seq);
        if (read.ec != std::errc() || read.ptr != seq.data() + seq.size()) throw std::runtime_error("Invalid variant sequence");
        s.last_actor = Number(Get(f, "last_actor", "-1"), -1, 3);
        s.last_tile = Number(Get(f, "last_tile", "-1"), -1, s.wall_total - 1);
        s.direction = Get(f, "exchange_direction");
        if (!s.direction.empty() && s.direction != "clockwise" && s.direction != "counterclockwise" && s.direction != "opposite" && s.direction != "none")
            throw std::runtime_error("Invalid exchange direction");
        s.result = Get(f, "result"); s.history = Get(f, "history");
        const auto event_seq = Get(f, "event_seq", "0");
        const auto event_read = std::from_chars(event_seq.data(), event_seq.data() + event_seq.size(), s.event_seq);
        if (event_read.ec != std::errc() || event_read.ptr != event_seq.data() + event_seq.size()) throw std::runtime_error("Invalid event sequence");
        for (const auto &item: Split(Get(f, "events"), ';')) {
            const auto parts = Split(item, ':');
            if (parts.size() != 5 || s.events.size() >= 32) throw std::runtime_error("Invalid variant event");
            VariantEvent event;
            const auto parsed = std::from_chars(parts[0].data(), parts[0].data() + parts[0].size(), event.seq);
            if (parsed.ec != std::errc() || parsed.ptr != parts[0].data() + parts[0].size() || !event.seq || event.seq > s.event_seq ||
                (!s.events.empty() && event.seq != s.events.back().seq + 1)) throw std::runtime_error("Invalid variant event order");
            event.kind = parts[1]; event.actor = Number(parts[2], 0, 3);
            event.tile = Number(parts[3], -1, s.wall_total - 1); event.source = Number(parts[4], -1, 3);
            if (event.kind == "draw" && event.actor != s.local && event.tile != -1) throw std::runtime_error("Private draw disclosure");
            s.events.push_back(std::move(event));
        }
        if (!s.events.empty() && s.events.back().seq != s.event_seq) throw std::runtime_error("Variant event boundary mismatch");
        const int receipts = Number(Get(f, "receipt_count", "0"), 0, 4);
        s.settlement.red_tiles.clear();
        for (int i = 0; i < receipts; ++i) {
            const auto key = "receipt" + std::to_string(i) + "_";
            SettlementReceipt r;
            r.generic = true;
            const int who = Number(Get(f, key + "who"), -1, 3), source = Number(Get(f, key + "from"), -1, 3);
            r.who = who < 0 ? -1 : s.Relative(who); r.from = source < 0 ? -1 : s.Relative(source);
            r.tag = who < 0 ? "draw" : "win";
            r.reason = Get(f, key + "reason"); r.fan = Number(Get(f, key + "fan"), 0, 64);
            r.winning_tile = Number(Get(f, key + "tile"), -1, tile_max);
            const auto base = Numbers(Get(f, key + "base"), -100000000, 100000000, 4, false);
            const auto delta = Numbers(Get(f, key + "delta"), -100000000, 100000000, 4, false);
            if (base.size() != 4 || delta.size() != 4) throw std::runtime_error("Invalid receipt payments");
            for (int seat = 0; seat < 4; ++seat) { r.base[s.Relative(seat)] = base[seat]; r.delta[s.Relative(seat)] = delta[seat]; }
            r.hand = Numbers(Get(f, key + "hand"), 0, tile_max, 14);
            r.has_hand = who >= 0; r.has_melds = true;
            if (who >= 0 && (source < 0 || std::find(r.hand.begin(), r.hand.end(), r.winning_tile) == r.hand.end())) throw std::runtime_error("Invalid winning receipt");
            for (const auto &pattern: Split(Get(f, key + "patterns"), ';')) r.entries.push_back({pattern});
            for (const auto &meld: Split(Get(f, key + "melds"), ';')) {
                const auto parts = Split(meld, ':');
                if (parts.size() != 5 || r.public_melds.size() >= 4) throw std::runtime_error("Invalid receipt meld");
                SettlementMeld m;
                m.kind = parts[0]; m.tiles = Numbers(parts[1], 0, tile_max, 4);
                const bool triple = m.kind == "chi" || m.kind == "pon";
                if ((!triple && m.kind != "closed_kan" && m.kind != "open_kan" && m.kind != "added_kan") || m.tiles.size() != (triple ? 3u : 4u)) throw std::runtime_error("Invalid receipt meld size");
                m.source = s.Relative(Number(parts[2], 0, 3)); m.called = Number(parts[3], -1, 135); m.added = Number(parts[4], -1, 135);
                if (m.kind != "closed_kan" && std::find(m.tiles.begin(), m.tiles.end(), m.called) == m.tiles.end()) throw std::runtime_error("Invalid receipt called tile");
                if (m.kind == "added_kan" && (m.added == m.called || std::find(m.tiles.begin(), m.tiles.end(), m.added) == m.tiles.end())) throw std::runtime_error("Invalid receipt added tile");
                r.public_melds.push_back(std::move(m));
            }
            s.settlement.receipts.push_back(std::move(r));
        }
        const auto scores = Numbers(Get(f, "scores"), -100000000, 100000000, 4, false);
        if (scores.size() != 4) throw std::runtime_error("Invalid variant scores");
        for (int seat = 0; seat < 4; ++seat) {
            auto &p = s.players[seat];
            const auto id = std::to_string(seat);
            p.score = scores[seat];
            p.hand = Numbers(Get(f, "hand" + id), 0, tile_max, 14);
            p.hidden = Number(Get(f, "hidden" + id), 0, 14);
            if (!p.hand.empty() && p.hand.size() != static_cast<std::size_t>(p.hidden)) throw std::runtime_error("Inconsistent variant hand");
            p.drawn = Number(Get(f, "drawn" + id, "-1"), -1, tile_max);
            if (p.drawn >= 0 && std::find(p.hand.begin(), p.hand.end(), p.drawn) == p.hand.end()) throw std::runtime_error("Unowned drawn tile");
            p.river = Numbers(Get(f, "river" + id), 0, tile_max, 108);
            p.flowers = Numbers(Get(f, "flowers" + id), 136, 143, 8);
            p.winning = Numbers(Get(f, "winning" + id), 0, tile_max, 1);
            p.missing = Number(Get(f, "missing" + id, "-1"), -1, 2);
            p.retired = Number(Get(f, "retired" + id, "0"), 0, 1) != 0;
            const auto melds = Split(Get(f, "melds" + id), ';'), info = Split(Get(f, "meld_info" + id), ';');
            if (melds.size() > 4 || (!info.empty() && info.size() != melds.size())) throw std::runtime_error("Invalid variant meld metadata");
            for (std::size_t i = 0; i < melds.size(); ++i) {
                const auto parts = Split(melds[i], ':');
                if (parts.size() != 2) throw std::runtime_error("Invalid variant meld");
                ReplayMeld m;
                const auto &kind = parts[0];
                if (kind == "pon") m.kind = MeldKind::Pon;
                else if (kind == "chi") m.kind = MeldKind::Chi;
                else if (kind == "open_kan") m.kind = MeldKind::OpenKan;
                else if (kind == "closed_kan") m.kind = MeldKind::ClosedKan;
                else if (kind == "added_kan") m.kind = MeldKind::AddedKan;
                else throw std::runtime_error("Invalid variant meld kind");
                const std::size_t count = kind == "pon" || kind == "chi" ? 3 : 4;
                if (parts[1] == "?" && m.kind == MeldKind::ClosedKan) m.tiles.assign(4, -1);
                else m.tiles = Numbers(parts[1], 0, tile_max, count);
                if (m.tiles.size() != count) throw std::runtime_error("Invalid variant meld size");
                m.from = seat;
                if (!info.empty()) {
                    const auto metadata = Numbers(info[i], -1, 135, 2, false);
                    if (metadata.size() != 2 || metadata[0] < 0 || metadata[0] > 3) throw std::runtime_error("Invalid variant meld source");
                    m.from = metadata[0]; m.called_tile = metadata[1];
                }
                if (m.kind != MeldKind::ClosedKan && std::find(m.tiles.begin(), m.tiles.end(), m.called_tile) == m.tiles.end())
                    throw std::runtime_error("Invalid called tile");
                if (m.kind == MeldKind::AddedKan) {
                    const auto added = Split(Get(f, "meld_added" + id), ',');
                    if (added.size() != melds.size()) throw std::runtime_error("Missing added kan entity");
                    m.added_tile = Number(added[i], 0, 135);
                    if (m.added_tile == m.called_tile || std::find(m.tiles.begin(), m.tiles.end(), m.added_tile) == m.tiles.end())
                        throw std::runtime_error("Invalid added kan entity");
                }
                p.melds.push_back(std::move(m));
            }
        }
        s.initialized = true;
        return s;
    }

    std::vector<OnlineAction> ParseVariantActions(const std::string &encoded, const std::string &table,
                                                 std::uint64_t window, int tile_limit) {
        std::vector<OnlineAction> result;
        const std::map<std::string, std::pair<std::string, std::size_t>> kinds{{"discard", {"打出", 1}}, {"exchange", {"换三张", 3}},
            {"dingque", {"定缺", 1}}, {"pon", {"碰", 2}}, {"chi", {"吃", 2}}, {"open_kan", {"明杠", 3}},
            {"closed_kan", {"暗杠", 4}}, {"added_kan", {"补杠", 1}}, {"ron", {"和牌", 0}}, {"tsumo", {"自摸", 0}}, {"skip", {"过", 0}}};
        for (const auto &offer: Split(encoded, ';')) {
            const auto colon = offer.find(':');
            if (colon == std::string::npos || result.size() >= 512) throw std::runtime_error("Invalid variant action");
            OnlineAction a;
            a.kind = offer.substr(0, colon);
            const auto found = kinds.find(a.kind);
            if (found == kinds.end()) throw std::runtime_error("Unknown variant action");
            const auto parameters = offer.substr(colon + 1);
            auto ids = Numbers(parameters, 0, a.kind == "dingque" ? 2 : std::min(tile_limit, 136) - 1, found->second.second);
            if (ids.size() != found->second.second) throw std::runtime_error("Invalid variant action arity");
            if (a.kind == "dingque") a.suit = ids.front(); else a.tiles = std::move(ids);
            a.label = found->second.first + " " + parameters;
            a.messages = {"<VACT table=\"" + Escape(table) + "\" window=\"" + std::to_string(window) +
                "\" kind=\"" + a.kind + "\" tiles=\"" + parameters + "\"/>"};
            result.push_back(std::move(a));
        }
        return result;
    }

    OnlineState VariantTableState(const VariantState &s, const std::map<std::string, std::string> &identity) {
        OnlineState out;
        out.started = out.initialized = s.initialized;
        out.variant = true; out.type = 2; out.red_tiles.clear();
        out.round = s.hand_number - 1;
        out.dealer = s.Relative(s.dealer);
        out.turn = s.Preparing() || s.players[s.turn].retired ? -1 : s.Relative(s.turn);
        out.ended = s.phase == VariantPhase::Ended || s.phase == VariantPhase::Complete;
        out.complete = s.phase == VariantPhase::Complete;
        if (s.phase == VariantPhase::Claims && s.last_actor >= 0) out.last_discard_seat = s.Relative(s.last_actor);
        out.result = s.result; out.events = s.seq;
        out.settlement = s.settlement;
        out.wall.InitVariant(s.wall_total, s.wall_front, s.wall_tail, s.wall_offset);
        for (int seat = 0; seat < 4; ++seat) {
            const auto &p = s.players[seat];
            auto &q = out.players[s.Relative(seat)];
            q.name = Get(identity, "n" + std::to_string(seat));
            q.score = p.score; q.known_hand = p.hand;
            q.hidden_count = p.hand.empty() ? p.hidden : 0;
            q.drawn_tile = p.drawn; q.retired = p.retired; q.missing_suit = s.Preparing() ? -1 : p.missing;
            q.winning_tile = p.winning.empty() ? -1 : p.winning.front();
            for (int tile: p.river) q.discards.push_back({tile});
            q.melds = p.melds;
            for (auto &meld: q.melds) meld.from = s.Relative(meld.from);
        }
        return out;
    }

    std::vector<VariantEvent> VariantEventCursor::Consume(const VariantState &state, bool recovering) {
        std::vector<VariantEvent> result;
        resynchronized = !aligned || recovering || state.event_seq < applied ||
            (state.event_seq > applied && (state.events.empty() || state.events.front().seq > applied + 1));
        if (!resynchronized) for (const auto &event: state.events) if (event.seq > applied) result.push_back(event);
        applied = state.event_seq; aligned = true;
        return result;
    }
}
