#include "game.hpp"
#include <numeric>
#include <tuple>

namespace authority {
    using namespace std;
    namespace {
        std::set<int> kuikae(const int called, const int low) {
            std::set r{called};
            if (called == low && low % 9 < 6) r.insert(low + 3);
            if (called == low + 2 && low % 9 > 0) r.insert(low - 1);
            return r;
        }

        int priority(const Action &a) {
            return a.kind == "ron" ? 0 : a.kind == "pon" || a.kind == "open_kan" ? 1 : a.kind == "chi" ? 2 : 3;
        }

        int kan_count(const std::array<Player, 4> &players) {
            int n = 0;
            for (const auto &p: players) for (const auto &m: p.melds) if (m.kind.find("kan") != std::string::npos) ++n;
            return n;
        }
    }

    Game::Game(Config config, const std::uint64_t seed) : cfg(std::move(config)), rng(seed) {
        for (int s = 0; s < n(); ++s) scores[s] = cfg.starting;
    }

    void Game::emit(std::string tag, Attrs attrs, std::map<int, int> hints_) {
        events.push_back({std::move(tag), std::move(attrs), std::move(hints_)});
    }

    std::vector<int> Game::dora(const bool ura) const {
        std::vector<int> r;
        r.reserve(dora_count);
        for (int i = 0; i < dora_count; ++i) r.push_back(dead[cfg.replacement_limit() + 2 * i + (ura ? 1 : 0)]);
        return r;
    }

    void Game::start(const std::vector<int> &wall) {
        if (phase != "new" && phase != "ended") throw std::runtime_error("Invalid start phase");
        if (phase == "ended") {
            round = next_round;
            honba = next_honba;
        }
        std::vector<int> tiles;
        for (int t = 0; t < 136; ++t) if (n() != 3 || t / 4 < 1 || t / 4 > 7) tiles.push_back(t);
        if (!wall.empty()) {
            auto a = wall;
            std::sort(a.begin(), a.end());
            if (a != tiles) throw std::runtime_error("Invalid physical wall");
            tiles = wall;
        } else std::shuffle(tiles.begin(), tiles.end(), rng);
        dealer = round % n();
        players = {};
        Attrs attrs;
        for (int i = 0; i < 13 * n(); ++i) players[(dealer + i % n()) % n()].hand.push_back(tiles[i]);
        for (int s = 0; s < n(); ++s) {
            attrs["hai" + std::to_string(s)] = list(players[s].hand);
            std::sort(players[s].hand.begin(), players[s].hand.end());
        }
        const auto split = tiles.end() - (cfg.replacement_limit() + 10);
        live = std::deque(tiles.begin() + 13 * n(), split);
        dead.assign(split, tiles.end());
        sealed.clear();
        replacements = pending_dora = 0;
        dora_count = 1;
        interrupted = kan_discard = false;
        pending_riichi = -1;
        claims.clear();
        responses.clear();
        ++hand_number;
        std::array < int, 4 > ten{};
        for (int s = 0; s < 4; ++s) ten[s] = scores[s] / 100;
        attrs["seed"] = list({
            (round / n() * 4 + round % n()) % 16, honba, sticks, static_cast<int>(rng() % 6),
            static_cast<int>(rng() % 6), dora()[0]
        });
        attrs["ten"] = list(ten);
        attrs["oya"] = std::to_string(dealer);
        emit("INIT", std::move(attrs));
        draw(dealer);
        invariants();
    }

    void Game::draw(int s, const bool replacement, const bool nuki) {
        if (live.empty()) {
            draw_end();
            return;
        }
        int t;
        if (replacement) {
            if (replacements >= cfg.replacement_limit()) throw std::runtime_error("Replacement exhausted");
            t = dead[replacements++];
            sealed.push_back(live.back());
            live.pop_back();
        } else {
            t = live.front();
            live.pop_front();
        }
        auto &p = players[s];
        p.hand.push_back(t);
        std::sort(p.hand.begin(), p.hand.end());
        p.drawn = t;
        ++p.draws;
        p.temp_furiten = false;
        p.forbidden.clear();
        turn = s;
        phase = "turn";
        rinshan = replacement;
        kan_discard = replacement && !nuki;
        int hint = 0;
        for (const auto &[kind, tiles]: options(s)) {
            if (kind == "tsumo") hint |= 16;
            if (kind == "riichi") hint |= 32;
            if (kind == "closed_kan" || kind == "added_kan") hint |= 2;
            if (kind == "draw") hint |= 64;
            if (kind == "nuki") hint |= 128;
        }
        emit(std::string(1, "TUVW"[s]) + std::to_string(t), {}, {{s, hint}});
    }

    Value Game::value(const int s, const int t, const bool tsumo, const bool chankan, const bool closed_rob) const {
        const auto &p = players[s];
        auto hand = p.hand;
        if (!tsumo) {
            const auto w = waits(hand, n());
            if (p.temp_furiten || p.riichi_furiten) return {};
            for (const auto r: p.river) if (w.count(r / 4)) return {};
            hand.push_back(t);
        }
        const bool first = p.draws == 1 && p.river.empty() && !interrupted;
        Context x;
        x.tsumo = tsumo;
        x.dealer = s == dealer;
        x.seat_wind = 27 + (s - dealer + n()) % n();
        x.round_wind = 27 + round / n() % 4;
        x.riichi = p.riichi;
        x.double_reach = p.double_reach;
        x.ippatsu = p.ippatsu;
        x.rinshan = tsumo && rinshan;
        x.chankan = chankan && !closed_rob && claim_kind != "nuki";
        x.haitei = tsumo && live.empty() && !rinshan;
        x.houtei = !tsumo && !chankan && live.empty() && !rinshan;
        x.tenhou = tsumo && first && s == dealer;
        x.chiihou = tsumo && first && s != dealer;
        x.dora = dora();
        if (p.riichi) x.ura = dora(true);
        x.nuki = p.nuki;
        x.renhou = !tsumo && s != dealer && p.draws == 0 && !interrupted;
        x.swallow = !tsumo && claim_kind == "discard" && pending_riichi == last_actor;
        x.kan_discard = !tsumo && claim_kind == "discard" && kan_discard;
        auto v = evaluate(hand, p.melds, t, cfg, x);
        if (closed_rob && !has(v.yakuman, 47) && !has(v.yakuman, 48)) return {};
        return v;
    }

    std::vector<Action> Game::options(const int s) const {
        if (s < 0 || s >= n()) return {};
        if (phase == "claims") {
            const auto i = claims.find(s);
            return i == claims.end() || responses.count(s) ? std::vector<Action>{} : i->second;
        }
        if (phase != "turn" || s != turn) return {};
        const auto &p = players[s];
        std::vector<Action> r;
        for (auto t: p.hand) if ((!p.riichi || t == p.drawn) && !p.forbidden.count(t / 4)) r.
                push_back({"discard", {t}});
        if (p.drawn < 0) return r;
        if (value(s, p.drawn, true).han) r.push_back({"tsumo"});
        std::set<int> terminals;
        for (const auto t: p.hand) if (terminal(t / 4)) terminals.insert(t / 4);
        if (p.river.empty() && !interrupted && terminals.size() >= 9) r.push_back({"draw"});
        // With no intervening call, every active seat draws before our next draw.
        if (!p.riichi && p.closed() && scores[s] >= 1000 && live.size() >= static_cast<std::size_t>(n())) {
            const auto discards = r;
            for (const auto &[kind, tiles]: discards) if (kind == "discard") {
                auto rest = p.hand;
                remove(rest, tiles[0]);
                if (!waits(rest, n()).empty()) r.push_back({"riichi", tiles});
            }
        }
        if (n() == 3 && !live.empty() && replacements < cfg.replacement_limit()) for (auto t: p.hand) if (
            t / 4 == 30 && (!p.riichi || t == p.drawn)) r.push_back({"nuki", {t}});
        if (!live.empty() && replacements < cfg.replacement_limit() && kan_count(players) < 4) {
            const auto c = counts(p.hand);
            for (int k = 0; k < 34; ++k)
                if (c[k]) {
                    std::vector<int> same;
                    for (auto t: p.hand) if (t / 4 == k) same.push_back(t);
                    if (c[k] == 4) {
                        bool allowed = !p.riichi;
                        if (p.riichi && p.drawn / 4 == k) {
                            auto before = p.hand, rest = p.hand;
                            remove(before, p.drawn);
                            for (const auto t: same) remove(rest, t);
                            auto w = waits(before, n());
                            allowed = !w.empty() && w == waits(rest, n());
                        }
                        if (allowed) r.push_back({"closed_kan", same});
                    }
                    if (!p.riichi) for (const auto &m: p.melds) if (m.kind == "pon" && m.tiles[0] / 4 == k) for (
                        auto t: same) r.push_back({"added_kan", {t}});
                }
        }
        return r;
    }

    void Game::submit(int s, const Action &a) {
        const auto opts = options(s);
        if (std::find(opts.begin(), opts.end(), a) == opts.end()) throw std::runtime_error("Illegal or stale action");
        if (phase == "claims") {
            responses[s] = a;
            if (std::find(opts.begin(), opts.end(), Action{"ron"}) != opts.end() && a.kind != "ron") {
                players[s].temp_furiten = true;
                players[s].riichi_furiten = players[s].riichi;
            }
            auto order = [&](const int seat, const Action &act) {
                return std::make_pair(priority(act), act.kind == "ron" ? 0 : (seat - last_actor + 4) % 4);
            };
            std::pair best{4, 4};
            for (const auto &[fst, snd]: responses) if (snd.kind != "skip") best = std::min(best, order(fst, snd));
            if (best.first < 4) for (auto i = claims.begin(); i != claims.end();) {
                bool viable = responses.count(i->first) != 0;
                for (const auto &act: i->second)
                    viable = viable || (act.kind != "skip" && order(i->first, act) <= best);
                if (!viable) i = claims.erase(i);
                else ++i;
            }
            bool answered = true;
            for (const auto &[fst, snd]: claims) answered = answered && responses.count(fst) != 0;
            if (answered) resolve_claims();
        } else if (a.kind == "discard" || a.kind == "riichi") discard(s, a.tiles[0], a.kind == "riichi");
        else if (a.kind == "tsumo") win({s}, s, true);
        else if (a.kind == "draw") draw_end("yao9");
        else if (a.kind == "nuki") {
            auto &p = players[s];
            remove(p.hand, a.tiles[0]);
            p.nuki.push_back(a.tiles[0]);
            p.drawn = -1;
            last_actor = s;
            last_tile = a.tiles[0];
            claim_kind = "nuki";
            prepare_claims();
            emit("N", {{"who", std::to_string(s)}, {"m", std::to_string((last_tile << 8) | 32)}}, hints());
            if (claims.empty()) {
                interrupt();
                draw(s, true, true);
            }
        } else kan(s, a);
        invariants();
    }

    void Game::discard(const int s, const int t, const bool reach) {
        auto &p = players[s];
        if (p.riichi) p.ippatsu = false;
        if (reach) {
            pending_riichi = s;
            p.double_reach = p.river.empty() && !interrupted;
            emit("REACH", {{"who", std::to_string(s)}, {"step", "1"}});
        }
        const bool tsumogiri = p.drawn == t;
        remove(p.hand, t);
        p.river.push_back(t);
        p.drawn = -1;
        p.forbidden.clear();
        last_actor = s;
        last_tile = t;
        claim_kind = "discard";
        prepare_claims();
        char code = "DEFG"[s];
        if (tsumogiri) code = static_cast<char>(tolower(code));
        emit(std::string(1, code) + std::to_string(t), {}, hints());
        if (claims.empty()) resolve_claims();
    }

    void Game::prepare_claims() {
        phase = "claims";
        claims.clear();
        responses.clear();
        const int k = last_tile / 4;
        for (int s = 0; s < n(); ++s)
            if (s != last_actor) {
                auto &p = players[s];
                std::vector<Action> a;
                if (value(s, last_tile, false, claim_kind != "discard", claim_kind == "closed_kan").han) a.push_back({
                    "ron"
                });
                else if (claim_kind == "discard" && waits(p.hand, n()).count(k)) p.temp_furiten = true;
                if (claim_kind == "discard" && !p.riichi && !live.empty()) {
                    std::vector<int> same;
                    for (auto t: p.hand) if (t / 4 == k) same.push_back(t);
                    for (std::size_t i = 0; i < same.size(); ++i) for (std::size_t j = i + 1; j < same.size(); ++j) if (
                        std::any_of(p.hand.begin(), p.hand.end(), [&](const int t) { return t / 4 != k; })) a.push_back({
                        "pon", {same[i], same[j]}
                    });
                    if (same.size() == 3 && replacements < cfg.replacement_limit() && kan_count(players) < 4) a.
                            push_back({"open_kan", same});
                    if (n() == 4 && s == (last_actor + 1) % n() && k < 27)
                        for (int low = std::max(k / 9 * 9, k - 2); low <= std::min(k, k / 9 * 9 + 6); ++low) {
                            std::vector<int> need;
                            for (int z = low; z < low + 3; ++z) if (z != k) need.push_back(z);
                            for (int t: p.hand) if (t / 4 == need[0]) for (int u: p.hand) if (u / 4 == need[1]) {
                                auto forbidden = kuikae(k, low);
                                if (std::any_of(p.hand.begin(), p.hand.end(), [&](const int v) {
                                    return v != t && v != u && !forbidden.count(v / 4);
                                })) a.push_back({"chi", {std::min(t, u), std::max(t, u)}});
                            }
                        }
                }
                if (!a.empty()) {
                    a.push_back({"skip"});
                    claims[s] = std::move(a);
                }
            }
    }

    std::map<int, int> Game::hints() const {
        std::map<int, int> r;
        for (const auto &[fst, snd]: claims) for (const auto &[kind, tiles]: snd) r[fst] |=
            kind == "ron" ? 8 :
                kind == "pon" ? 1 :
                    kind == "open_kan" ? 2 :
                        kind == "chi" ? 4 : 0;
        return r;
    }

    void Game::resolve_claims() {
        std::vector<int> winners;
        for (const auto &[fst, snd]: responses) if (snd.kind == "ron") winners.push_back(fst);
        std::sort(winners.begin(), winners.end(), [&](const int a, const int b) {
            return (a - last_actor + 4) % 4 < (b - last_actor + 4) % 4;
        });
        if (winners.size() == 3) {
            draw_end("ron3");
            return;
        }
        if (!winners.empty()) {
            win(winners, last_actor, false);
            return;
        }
        if (claim_kind != "discard") {
            if (claim_kind == "nuki") {
                interrupt();
                draw(last_actor, true, true);
            } else finish_kan();
            return;
        }
        if (pending_riichi >= 0) {
            auto s = pending_riichi;
            players[s].riichi = players[s].ippatsu = true;
            scores[s] -= 1000;
            ++sticks;
            emit("REACH", {{"who", std::to_string(s)}, {"step", "2"}});
            pending_riichi = -1;
        }
        while (pending_dora) {
            flip();
            --pending_dora;
        }
        int reach = 0, maxk = 0;
        for (int s = 0; s < n(); ++s) {
            reach += players[s].riichi;
            int k = 0;
            for (const auto &m: players[s].melds) k += m.kind.find("kan") != std::string::npos;
            maxk = std::max(maxk, k);
        }
        if (reach == n()) {
            draw_end("reach4");
            return;
        }
        if (kan_count(players) == 4 && maxk != 4) {
            draw_end("kan4");
            return;
        }
        if (n() == 4 && !interrupted && std::all_of(players.begin(), players.end(),
                                                    [](const Player &p) { return p.river.size() == 1; })) {
            int k = players[0].river[0] / 4;
            if (k >= 27 && k < 31 && std::all_of(players.begin(), players.end(),
                                                 [&](const Player &p) { return p.river[0] / 4 == k; })) {
                draw_end("kaze4");
                return;
            }
        }
        int chosen = -1;
        Action action;
        std::pair<int, int> order{4, 4};
        for (const auto &e: responses) if (e.second.kind == "chi" || e.second.kind == "pon" || e.second.kind ==
                                           "open_kan") {
            auto rank = std::make_pair(priority(e.second), (e.first - last_actor + 4) % 4);
            if (rank < order) {
                chosen = e.first;
                action = e.second;
                order = rank;
            }
        }
        if (chosen >= 0) call(chosen, action);
        else draw((last_actor + 1) % n());
    }

    void Game::interrupt() {
        interrupted = true;
        for (auto &p: players) p.ippatsu = false;
    }

    void Game::call(int s, const Action &a) {
        auto &p = players[s];
        for (auto t: a.tiles) remove(p.hand, t);
        players[last_actor].called.insert(last_tile);
        auto tiles = a.tiles;
        tiles.push_back(last_tile);
        std::sort(tiles.begin(), tiles.end());
        Meld m{a.kind, tiles, s, last_actor, last_tile};
        p.melds.push_back(m);
        interrupt();
        if (m.opened() && m.kind != "chi") {
            std::set<int> kinds;
            for (const auto &z: p.melds) if (z.kind != "chi") kinds.insert(z.tiles[0] / 4);
            if (m.tiles[0] / 4 >= 31 && kinds.count(31) && kinds.count(32) && kinds.count(33)) p.pao.emplace(
                39, last_actor);
            if (m.tiles[0] / 4 >= 27 && m.tiles[0] / 4 < 31 && kinds.count(27) && kinds.count(28) && kinds.count(29) &&
                kinds.count(30)) p.pao.emplace(49, last_actor);
        }
        turn = s;
        phase = "turn";
        rinshan = false;
        p.drawn = -1;
        p.forbidden = a.kind == "pon"
                          ? std::set<int>{last_tile / 4}
                          : a.kind == "chi"
                                ? kuikae(last_tile / 4, tiles[0] / 4)
                                : std::set<int>{};
        emit("N", {{"who", std::to_string(s)}, {"m", std::to_string(m.encode())}});
        if (a.kind == "open_kan") {
            ++pending_dora;
            draw(s, true);
        }
    }

    void Game::kan(int s, const Action &a) {
        auto &p = players[s];
        Meld m;
        if (a.kind == "closed_kan") {
            m = {a.kind, a.tiles, s, s};
            for (auto t: a.tiles) remove(p.hand, t);
            p.melds.push_back(m);
            last_tile = has(a.tiles, p.drawn) ? p.drawn : a.tiles[0];
        } else {
            last_tile = a.tiles[0];
            auto old = std::find_if(p.melds.begin(), p.melds.end(), [&](const Meld &z) {
                return z.kind == "pon" && z.tiles[0] / 4 == last_tile / 4;
            });
            m = *old;
            m.kind = a.kind;
            m.tiles.push_back(last_tile);
            std::sort(m.tiles.begin(), m.tiles.end());
            m.added = last_tile;
            remove(p.hand, last_tile);
            *old = m;
        }
        p.drawn = -1;
        last_actor = s;
        claim_kind = a.kind;
        prepare_claims();
        emit("N", {{"who", std::to_string(s)}, {"m", std::to_string(m.encode())}}, hints());
        kan_flip = a.kind == "closed_kan";
        if (claims.empty()) finish_kan();
    }

    void Game::finish_kan() {
        interrupt();
        while (pending_dora) {
            flip();
            --pending_dora;
        }
        if (kan_flip) flip();
        else ++pending_dora;
        draw(last_actor, true);
    }

    void Game::flip() {
        if (dora_count >= 5) throw std::runtime_error("Too many indicators");
        ++dora_count;
        emit("DORA", {{"hai", std::to_string(dora().back())}});
    }

    std::array<int, 4> Game::transfers(int s, int source, const Value &v, bool tsumo) const {
        std::array < int, 4 > delta{};
        std::vector<std::pair<int, int> > liable;
        for (auto k: v.yakuman) {
            auto i = players[s].pao.find(k);
            if (i != players[s].pao.end()) liable.emplace_back(i->second, k == 49 ? 2 : 1);
        }
        int main = v.main, additional = v.additional;
        if (!liable.empty()) {
            int remaining = v.han / 13;
            const int unit = s == dealer ? 48000 : 32000;
            for (auto l: liable) {
                remaining -= l.second;
                if (tsumo || source == l.first) delta[l.first] -= unit * l.second;
                else {
                    delta[source] -= unit * l.second / 2;
                    delta[l.first] -= unit * l.second / 2;
                }
            }
            main = (tsumo ? 16000 : unit) * remaining;
            additional = (s == dealer ? 16000 : 8000) * remaining;
        }
        if (tsumo) {
            for (int o = 0; o < n(); ++o) if (o != s) delta[o] -= s == dealer || o == dealer ? main : additional;
            if (!liable.empty()) delta[liable[0].first] -= (n() - 1) * 100 * honba;
            else for (int o = 0; o < n(); ++o) if (o != s) delta[o] -= 100 * honba;
        } else delta[source] -= main + 300 * honba;
        delta[s] = -std::accumulate(delta.begin(), delta.end(), 0);
        return delta;
    }

    void Game::win(const std::vector<int> &winners, int source, bool tsumo) {
        const int tile = tsumo ? players[source].drawn : last_tile;
        std::vector<Value> values;
        for (auto s: winners) {
            auto v = value(s, tile, tsumo, !tsumo && claim_kind != "discard", !tsumo && claim_kind == "closed_kan");
            if (!v.han) throw std::runtime_error("Unvalidated win");
            values.push_back(v);
        }
        auto first = events.size();
        for (std::size_t i = 0; i < winners.size(); ++i) {
            const int s = winners[i];
            const auto &p = players[s];
            const auto &v = values[i];
            auto base = scores, delta = transfers(s, source, v, tsumo);
            int award = i == 0 ? sticks : 0;
            delta[s] += award * 1000;
            sticks -= award;
            std::vector<int> sc;
            for (int j = 0; j < 4; ++j) {
                scores[j] += delta[j];
                sc.push_back(base[j] / 100);
                sc.push_back(delta[j] / 100);
            }
            auto hand = p.hand;
            if (!tsumo) hand.push_back(tile);
            std::sort(hand.begin(), hand.end());
            Attrs a{
                {"who", std::to_string(s)}, {"fromWho", std::to_string(source)}, {"machi", std::to_string(tile)},
                {"hai", list(hand)},
                {"ten", list({v.fu, delta[s] - award * 1000 - honba * (tsumo ? (n() - 1) * 100 : 300), v.level})},
                {"doraHai", list(dora())}, {"ba", list({honba, award})}, {"sc", list(sc)}
            };
            a[v.yakuman.empty() ? "yaku" : "yakuman"] = list(v.yakuman.empty() ? v.yaku : v.yakuman);
            std::vector<int> ms;
            for (const auto &m: p.melds) ms.push_back(m.encode());
            for (auto t: p.nuki) ms.push_back((t << 8) | 32);
            if (!ms.empty()) a["m"] = list(ms);
            if (p.riichi) a["doraHaiUra"] = list(dora(true));
            emit("AGARI", std::move(a));
        }
        end(has(winners, dealer), false, first);
    }

    void Game::draw_end(const std::string &reason) {
        auto base = scores;
        std::array < int, 4 > delta{};
        Attrs a;
        if (!reason.empty()) a["type"] = reason;
        std::vector<int> ready, nagashi;
        for (int s = 0; s < n(); ++s) {
            if (!waits(players[s].hand, n()).empty()) ready.push_back(s);
            const auto &p = players[s];
            if (!p.river.empty() && p.called.empty() && std::all_of(p.river.begin(), p.river.end(),
                                                                    [](int t) {
                                                                        return terminal(t / 4);
                                                                    })) nagashi.push_back(s);
        }
        if (reason.empty()) {
            if (cfg.minimum == "haneman" || cfg.minimum == "baiman" || cfg.minimum == "sanbaiman" || cfg.minimum ==
                "yakuman") nagashi.clear();
            if (!nagashi.empty()) {
                a["type"] = "nm";
                for (auto s: nagashi) {
                    Value v;
                    v.han = 5;
                    v.fu = 30;
                    v.main = 4000;
                    v.additional = s == dealer ? 4000 : 2000;
                    auto d = transfers(s, s, v, true);
                    for (int j = 0; j < 4; ++j) delta[j] += d[j];
                }
                int recipient = *std::min_element(nagashi.begin(), nagashi.end(), [&](int s, int t) {
                    return (s - dealer + 4) % 4 < (t - dealer + 4) % 4;
                });
                delta[recipient] += sticks * 1000;
                sticks = 0;
            } else if (!ready.empty() && ready.size() < static_cast<std::size_t>(n())) {
                int pool = n() == 3 ? 2000 : 3000;
                for (int s = 0; s < n(); ++s) delta[s] = has(ready, s)
                                                             ? pool / static_cast<int>(ready.size())
                                                             : -pool / (n() - static_cast<int>(ready.size()));
            }
            for (auto s: ready) a["hai" + std::to_string(s)] = list(players[s].hand);
        }
        std::vector<int> sc;
        for (int s = 0; s < 4; ++s) {
            scores[s] += delta[s];
            sc.push_back(base[s] / 100);
            sc.push_back(delta[s] / 100);
        }
        a["sc"] = list(sc);
        auto first = events.size();
        emit("RYUUKYOKU", a);
        end(!reason.empty() || has(nagashi.empty() ? ready : nagashi, dealer), true, first);
    }

    void Game::end(bool stays, bool drawn, std::size_t first) {
        next_round = stays ? round : round + 1;
        next_honba = drawn || stays ? honba + 1 : 0;
        int top = 0;
        bool bankrupt = false;
        for (int s = 0; s < n(); ++s) {
            if (scores[s] > scores[top]) top = s;
            bankrupt = bankrupt || (cfg.bankruptcy && scores[s] < 0);
        }
        bool finish = bankrupt || cfg.length == "single";
        if (round >= (cfg.length == "south" ? 2 : 1) * n() - 1 && scores[top] >= cfg.target)
            finish = finish || !stays || top == dealer;
        if (drawn && events[first].attrs.count("type") && events[first].attrs.at("type") != "nm")
            finish = bankrupt || cfg.length == "single";
        phase = finish ? "complete" : "ended";
        if (finish) {
            auto &last = events.back();
            if (sticks) {
                scores[top] += sticks * 1000;
                auto sc = integers(last.attrs.at("sc"));
                sc[top * 2 + 1] += sticks * 10;
                last.attrs["sc"] = list(sc);
                sticks = 0;
            }
            std::vector<int> order;
            for (int s = 0; s < n(); ++s) order.push_back(s);
            std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return scores[a] > scores[b]; });
            std::array < double, 4 > points{};
            const std::vector<int> uma = n() == 3 ? std::vector<int>{20, 0, -20} : std::vector<int>{20, 10, -10, -20};
            for (int rank = 0; rank < n(); ++rank) {
                int s = order[rank];
                points[s] = (scores[s] - cfg.target) / 1000.0 + uma[rank] + (rank == 0
                                                                                 ? n() * (cfg.target - cfg.starting) /
                                                                                     1000.0
                                                                                 : 0);
            }
            std::string owari;
            for (int s = 0; s < 4; ++s) {
                if (s) owari += ',';
                owari += std::to_string(scores[s] / 100) + "," + std::to_string(points[s]);
            }
            for (auto i = first; i < events.size(); ++i) events[i].attrs["owari"] = owari;
            emit("PROF");
        }
        claims.clear();
        responses.clear();
    }

    void Game::invariants() const {
        if (dead.empty()) return;
        std::vector<int> used(live.begin(), live.end());
        used.insert(used.end(), dead.begin() + replacements, dead.end());
        used.insert(used.end(), sealed.begin(), sealed.end());
        for (const auto &p: players) {
            used.insert(used.end(), p.hand.begin(), p.hand.end());
            for (const auto &m: p.melds) used.insert(used.end(), m.tiles.begin(), m.tiles.end());
            for (auto t: p.river) if (!p.called.count(t)) used.push_back(t);
            used.insert(used.end(), p.nuki.begin(), p.nuki.end());
        }
        std::sort(used.begin(), used.end());
        std::vector<int> expected;
        for (int t = 0; t < 136; ++t) if (n() != 3 || t / 4 < 1 || t / 4 > 7) expected.push_back(t);
        if (used != expected) throw std::runtime_error("Physical tile conservation");
        if (std::accumulate(scores.begin(), scores.end(), sticks * 1000) != cfg.starting * n()) throw
                std::runtime_error("Point conservation");
    }

    Action Game::bot(int s) const {
        auto opts = options(s);
        for (const char *k: {"ron", "tsumo", "nuki"}) for (const auto &a: opts) if (a.kind == k) return a;
        if (phase == "claims") return {"skip"};
        const auto &p = players[s];
        auto visible = counts(p.hand);
        for (const auto &q: players) {
            for (auto t: q.nuki) ++visible[t / 4];
            for (auto t: q.river) if (!q.called.count(t)) ++visible[t / 4];
            for (const auto &m: q.melds) for (auto t: m.tiles) ++visible[t / 4];
        }
        std::map<int, std::pair<int, int> > evaluated;
        std::tuple<int, int, bool, bool, int> best{99, 0, true, true, 999};
        Action chosen;
        for (const auto &a: opts)
            if (a.kind == "discard" || a.kind == "riichi") {
                int t = a.tiles[0], k = t / 4;
                auto i = evaluated.find(k);
                if (i == evaluated.end()) {
                    auto rest = p.hand;
                    remove(rest, t);
                    auto c = counts(rest);
                    int base = shanten(c), improving = 0;
                    for (int z = 0; z < 34; ++z) {
                        if ((n() == 3 && z >= 1 && z <= 7) || visible[z] >= 4) continue;
                        ++c[z];
                        if (shanten(c) < base) improving += 4 - visible[z];
                        --c[z];
                    }
                    i = evaluated.emplace(k, std::make_pair(base, -improving)).first;
                }
                auto key = std::make_tuple(i->second.first, i->second.second, a.kind != "riichi",
                                           has(cfg.red_tiles(), t), t);
                if (key < best) {
                    best = key;
                    chosen = a;
                }
            }
        if (chosen.kind.empty()) throw std::runtime_error("No bot action");
        return chosen;
    }

    std::string project(const Event &e, int viewer, bool open) {
        auto a = e.attrs;
        auto tag = e.tag;
        auto rotate = [&](int s) { return (s - viewer + 4) % 4; };
        if (tag == "INIT") {
            a["hai"] = a.at("hai" + std::to_string(viewer));
            for (int s = 0; s < 4; ++s) {
                auto k = "hai" + std::to_string(s);
                if (open && a.count(k)) a["open" + std::to_string(rotate(s))] = a.at(k);
                a.erase(k);
            }
            a["oya"] = std::to_string(rotate(number(a.at("oya"))));
            auto ten = integers(a.at("ten"));
            std::vector<int> r;
            for (int s = 0; s < 4; ++s) r.push_back(ten[(s + viewer) % 4]);
            a["ten"] = list(r);
        } else if (tag.size() > 1 && std::string("TUVWDEFGtuvwdefg").find(tag[0]) != std::string::npos) {
            char upper = static_cast<char>(toupper(tag[0]));
            bool draw = std::string("TUVW").find(upper) != std::string::npos;
            std::string letters = draw ? "TUVW" : "DEFG";
            int s = static_cast<int>(letters.find(upper));
            char code = letters[rotate(s)];
            if (islower(tag[0])) code = static_cast<char>(tolower(code));
            tag = std::string(1, code) + (!draw || s == viewer || open ? tag.substr(1) : "");
        }
        for (const char *k: {"who", "fromWho"}) if (a.count(k)) a[k] = std::to_string(rotate(number(a.at(k))));
        for (const char *k: {"sc", "owari"}) if (a.count(k)) {
            std::vector<std::string> fields;
            std::string raw = a[k];
            std::size_t p = 0;
            for (;;) {
                auto end = raw.find(',', p);
                fields.push_back(raw.substr(p, end == raw.npos ? end : end - p));
                if (end == raw.npos) break;
                p = end + 1;
            }
            std::string r;
            for (int s = 0; s < 4; ++s) for (int j = 0; j < 2; ++j) {
                if (!r.empty()) r += ',';
                r += fields[((s + viewer) % 4) * 2 + j];
            }
            a[k] = r;
        }
        if (e.tag == "RYUUKYOKU") {
            auto before = a;
            for (int s = 0; s < 4; ++s) a.erase("hai" + std::to_string(s));
            for (int s = 0; s < 4; ++s) {
                auto k = "hai" + std::to_string(s);
                if (before.count(k)) a["hai" + std::to_string(rotate(s))] = before.at(k);
            }
        }
        auto h = e.hints.find(viewer);
        if (h != e.hints.end() && h->second) a["t"] = std::to_string(h->second);
        return xml(tag, a);
    }

    Action decode(const Message &m, const std::vector<Action> &opts, int declared) {
        const auto &a = m.attrs;
        auto tile = [&](const char *k) { return number(a.at(k), 0, 135); };
        if (m.tag == "D" && a.size() == 1 && a.count("p")) {
            int t = tile("p");
            return {declared == t ? "riichi" : "discard", {t}};
        }
        if (m.tag != "N") throw std::runtime_error("Expected D or N");
        if (a.empty()) return {"skip"};
        static const std::map<int, std::string> kinds{
            {1, "pon"}, {2, "open_kan"}, {3, "chi"}, {4, "closed_kan"}, {5, "added_kan"}, {6, "ron"}, {7, "tsumo"},
            {9, "draw"}, {10, "nuki"}
        };
        auto i = kinds.find(number(a.at("type"), 1, 10));
        if (i == kinds.end()) throw std::runtime_error("Unsupported action");
        const auto &k = i->second;
        if (k == "pon" || k == "chi") {
            if (a.size() != 3) throw std::runtime_error("Unexpected action fields");
            std::vector<int> ts{tile("hai0"), tile("hai1")};
            std::sort(ts.begin(), ts.end());
            return {k, ts};
        }
        if (k == "closed_kan" || k == "added_kan" || k == "nuki") {
            if (a.size() != 2) throw std::runtime_error("Unexpected action fields");
            int t = tile("hai");
            for (const auto &o: opts) if (o.kind == k && has(o.tiles, t)) return o;
            throw std::runtime_error("Unavailable kan/nuki");
        }
        if (a.size() != 1) throw std::runtime_error("Unexpected action fields");
        if (k == "open_kan") {
            for (const auto &o: opts) if (o.kind == k) return o;
            throw std::runtime_error("Unavailable kan");
        }
        return {k};
    }
}
