#include "online_actions.hpp"
#include "network_protocol.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <functional>
#include <sstream>
#include <stdexcept>

namespace tenhou {
    namespace {
        [[noreturn]] void Fail(const char *text) { throw std::runtime_error(std::string("Online action: ") + text); }

        int AttributeNumber(const NetworkMessage &m, const char *key, int fallback = -1) {
            const auto it = m.attributes.find(key);
            if (it == m.attributes.end()) return fallback;
            int n = -1;
            const auto p = std::from_chars(it->second.data(), it->second.data() + it->second.size(), n);
            if (p.ec != std::errc() || p.ptr != it->second.data() + it->second.size()) Fail("invalid action attribute");
            return n;
        }

        std::string One(const char *tag, const char *attr, int value) {
            return std::string("<") + tag + " " + attr + "=\"" + std::to_string(value) + "\" />";
        }

        bool Groups(std::array<int, 34> &counts, int remaining) {
            int i = 0;
            while (i < 34 && !counts[i]) ++i;
            if (i == 34) return remaining == 0;
            if (remaining <= 0) return false;
            if (counts[i] >= 3) {
                counts[i] -= 3;
                const bool ok = Groups(counts, remaining - 1);
                counts[i] += 3;
                if (ok) return true;
            }
            if (i < 27 && i % 9 <= 6 && counts[i + 1] && counts[i + 2]) {
                --counts[i];
                --counts[i + 1];
                --counts[i + 2];
                const bool ok = Groups(counts, remaining - 1);
                ++counts[i];
                ++counts[i + 1];
                ++counts[i + 2];
                if (ok) return true;
            }
            return false;
        }

        bool Complete(std::array<int, 34> counts, int melds) {
            int total = 0, pairs = 0;
            for (int n: counts) {
                total += n;
                if (n == 2) ++pairs;
            }
            if (total != 14 - melds * 3) return false;
            if (!melds) {
                if (pairs == 7) return true;
                constexpr int terminals[]{0, 8, 9, 17, 18, 26, 27, 28, 29, 30, 31, 32, 33};
                int sum = 0;
                bool all = true;
                for (int i: terminals) {
                    sum += counts[i];
                    all = all && counts[i] > 0;
                }
                if (all && sum == 14) return true;
            }
            for (int i = 0; i < 34; ++i)
                if (counts[i] >= 2) {
                    counts[i] -= 2;
                    const bool ok = Groups(counts, 4 - melds);
                    counts[i] += 2;
                    if (ok) return true;
                }
            return false;
        }

        std::vector<int> Waits(const std::vector<int> &hand, int melds, const std::vector<int> &locked = {}) {
            std::array < int, 34 > counts{};
            for (int tile: hand) ++counts[tile / 4];
            std::array < int, 34 > unavailable{};
            for (int tile: locked) ++unavailable[tile / 4];
            std::vector<int> waits;
            // The local rule.md accepts virtual/empty waits for riichi tenpai. A
            // fifth-copy shape is permitted here; actual tile events remain 0..135.
            for (int i = 0; i < 34; ++i)
                if (counts[i] + unavailable[i] <= 4) {
                    ++counts[i];
                    if (Complete(counts, melds)) waits.push_back(i);
                    --counts[i];
                }
            return waits;
        }

        std::vector<int> Without(std::vector<int> hand, int tile) {
            const auto it = std::find(hand.begin(), hand.end(), tile);
            if (it == hand.end()) Fail("tile absent from hand");
            hand.erase(it);
            return hand;
        }

        bool Decisive(const std::string &tag) {
            if (tag == "GO" || tag == "INIT" || tag == "N" || tag == "AGARI" || tag == "RYUUKYOKU" || tag == "PROF")
                return true;
            return tag.size() > 0 && std::string("TUVWDEFGtuvwdefg").find(tag[0]) != std::string::npos &&
                   (tag.size() == 1 || (tag[1] >= '0' && tag[1] <= '9'));
        }
    }

    void OnlineActions::Offer(const OnlineState &s, unsigned hint) {
        auto &choices = snapshot.choices;
        choices.clear();
        const auto add = [&](std::string kind, std::vector<int> tiles, std::vector<std::string> messages) {
            std::string label = kind;
            for (int tile: tiles) label += " " + TileLabel(tile) + " [" + std::to_string(tile) + "]";
            choices.push_back({std::move(kind), std::move(label), std::move(tiles), std::move(messages)});
        };
        if (!s.started || s.complete || s.awaiting_context) return;
        if (!s.initialized) return;
        if (s.ended) {
            add("next", {}, {"<NEXTREADY />"});
            return;
        }
        const auto &p = s.players[0];
        const auto &hand = p.known_hand;
        std::vector<int> locked;
        for (const auto &m: p.melds) locked.insert(locked.end(), m.tiles.begin(), m.tiles.end());
        const bool own = s.turn == 0 && hand.size() % 3 == 2;
        if (own) {
            std::vector<int> forbidden;
            if (!p.draw_pending && !p.melds.empty()) {
                const auto &m = p.melds.back();
                forbidden.push_back(m.called_tile / 4);
                if (m.kind == MeldKind::Chi) {
                    const int low = m.tiles.front() / 4, high = m.tiles.back() / 4, called = m.called_tile / 4;
                    if (called == low && high % 9 < 8) forbidden.push_back(high + 1);
                    if (called == high && low % 9 > 0) forbidden.push_back(low - 1);
                }
            }
            for (int tile: hand)
                if ((!p.reach || tile == p.drawn_tile) &&
                    std::find(forbidden.begin(), forbidden.end(), tile / 4) == forbidden.end())
                    add("discard", {tile}, {One("D", "p", tile)});
            // A full uninterrupted rotation must still reach this seat's next draw.
            // Restored snapshots without wall history continue to trust the server hint.
            const bool next_draw = !s.wall.known ||
                                   136 - s.wall.replacement_limit - 10 - s.wall.front - s.wall.replacements >= (
                                       (s.type & 16) ? 3 : 4);
            if ((hint & 32) && next_draw && !p.reach && p.score >= 1000 &&
                std::all_of(p.melds.begin(), p.melds.end(), [](const ReplayMeld &m) {
                    return m.kind == MeldKind::ClosedKan;
                })) {
                for (int tile: hand)
                    if (!Waits(Without(hand, tile), static_cast<int>(p.melds.size()), locked).empty())
                        add("riichi", {tile}, {One("REACH", "hai", tile), One("D", "p", tile)});
            }
            if (hint & 2) {
                for (int kind = 0; kind < 34; ++kind) {
                    std::vector<int> same;
                    for (int tile: hand) if (tile / 4 == kind) same.push_back(tile);
                    if (same.size() == 4 && p.melds.size() < 4) {
                        bool allowed = !p.reach;
                        if (p.reach && p.drawn_tile / 4 == kind) {
                            auto rest = hand;
                            for (int tile: same) rest = Without(rest, tile);
                            const auto before = Waits(Without(hand, p.drawn_tile), static_cast<int>(p.melds.size()),
                                                      locked);
                            auto after_locked = locked;
                            after_locked.insert(after_locked.end(), same.begin(), same.end());
                            allowed = !before.empty() && before == Waits(
                                          rest, static_cast<int>(p.melds.size()) + 1, after_locked);
                        }
                        if (allowed) add("closed_kan", same, {
                                             "<N type=\"4\" hai=\"" + std::to_string(same[0]) + "\" />"
                                         });
                    }
                    if (!p.reach)
                        for (const auto &meld: p.melds)
                            if (meld.kind == MeldKind::Pon && meld.tiles[0] / 4 == kind)
                                for (int tile: same) add("added_kan", {tile}, {
                                                             "<N type=\"5\" hai=\"" + std::to_string(tile) + "\" />"
                                                         });
                }
            }
            if (hint & 16) add("tsumo", {}, {"<N type=\"7\" />"});
            if (hint & 64) add("draw", {}, {"<N type=\"9\" />"});
            // Original hint 0x80 -> n%d; request type 10 with the physical entity.
            if ((hint & 128) && (s.type & 16) && p.draw_pending)
                for (int tile: hand)
                    if (tile / 4 == 30 && (!p.reach || tile == p.drawn_tile))
                        add("nuki", {tile}, {"<N type=\"10\" hai=\"" + std::to_string(tile) + "\" />"});
            return;
        }
        if (!hint) return;
        if (hint & 8) add("ron", {}, {"<N type=\"6\" />"});
        if (s.last_discard_seat > 0 && !p.reach && p.melds.size() < 4) {
            const int from = s.last_discard_seat, called = s.players[from].discards.back().tile, kind = called / 4;
            std::vector<int> same;
            for (int tile: hand) if (tile / 4 == kind) same.push_back(tile);
            if (hint & 1)
                for (std::size_t a = 0; a < same.size(); ++a)
                    for (std::size_t b = a + 1; b < same.size(); ++b)
                        add("pon", {same[a], same[b]}, {
                                "<N type=\"1\" hai0=\"" + std::to_string(same[a]) + "\" hai1=\"" + std::to_string(
                                    same[b]) + "\" />"
                            });
            if ((hint & 2) && same.size() == 3) add("open_kan", same, {"<N type=\"2\" />"});
            if ((hint & 4) && !(s.type & 16) && from == 3 && kind < 27) {
                for (int low = std::max(kind - 2, kind / 9 * 9); low <= std::min(kind, kind / 9 * 9 + 6); ++low) {
                    std::vector<int> needs;
                    for (int k = low; k < low + 3; ++k) if (k != kind) needs.push_back(k);
                    for (int a: hand)
                        if (a / 4 == needs[0])
                            for (int b: hand)
                                if (b / 4 == needs[1]) {
                                    // At least one post-call discard must survive kuikae restrictions.
                                    const auto rest = Without(Without(hand, a), b);
                                    const bool playable = std::any_of(rest.begin(), rest.end(), [&](int t) {
                                        return t / 4 != kind && !(kind == low && t / 4 == low + 3) && !(
                                                   kind == low + 2 && t / 4 == low - 1);
                                    });
                                    if (playable) add("chi", {a, b}, {
                                                          "<N type=\"3\" hai0=\"" + std::to_string(a) + "\" hai1=\"" +
                                                          std::to_string(b) + "\" />"
                                                      });
                                }
                }
            }
        }
        add("skip", {}, {"<N />"});
    }

    void OnlineActions::Receive(const NetworkMessage &m, const OnlineState &before, const OnlineState &after) {
        if (!snapshot.enabled) return;
        const auto &tag = m.tag;
        if (tag == "REINIT") {
            ++snapshot.revision;
            snapshot.pending.clear();
            snapshot.choices.clear();
            pending_ = {};
            return;
        }
        if (!snapshot.pending.empty() && Decisive(tag)) {
            const auto &kind = pending_.kind;
            // Additional winners keep Next locked; INIT confirms it and final results cancel it.
            if (kind == "next" && tag != "INIT" && tag != "GO" && !after.complete) {
                snapshot.choices.clear();
                return;
            }
            if ((kind == "discard" || kind == "riichi") && !after.ended) {
                if (tag != "D" + std::to_string(pending_.tiles[0]) && tag != "d" + std::to_string(pending_.tiles[0]))
                    Fail("server did not confirm requested discard");
            }
            if ((kind == "chi" || kind == "pon" || kind == "open_kan" || kind == "closed_kan" || kind == "added_kan" ||
                 kind == "nuki") && tag == "N" && AttributeNumber(m, "who") == 0) {
                const auto meld = DecodeReplayMeld(static_cast<unsigned>(AttributeNumber(m, "m")), 0);
                if (kind != MeldKindName(meld.kind)) Fail("server confirmed another meld kind");
                for (int t: pending_.tiles) if (std::find(meld.tiles.begin(), meld.tiles.end(), t) == meld.tiles.end())
                    Fail("server confirmed another meld selection");
            }
            if (kind == "gok" && tag != "INIT") Fail("expected INIT after GOK");
            if ((kind == "closed_kan" || kind == "added_kan" || kind == "nuki") && !after.ended &&
                (tag != "N" || AttributeNumber(m, "who") != 0))
                Fail("expected own kan confirmation");
            if (kind == "tsumo" && !after.ended) Fail("expected result after tsumo");
            snapshot.pending.clear();
            pending_ = {};
        }
        if (!Decisive(tag)) return;
        ++snapshot.revision;
        snapshot.choices.clear();
        if (tag == "GO") {
            snapshot.pending = "gok";
            pending_ = {"gok", "gok", {}, {}};
            return;
        }
        unsigned hint = static_cast<unsigned>(AttributeNumber(m, "t", 0));
        if (hint & ~((after.type & 16) ? 255u : 127u)) Fail("unsupported action hint bits");
        // Hints on opponent N support ron after an added kan. Others are not local draws.
        if (tag == "N" && AttributeNumber(m, "who") != 0) {
            if (hint & 8) snapshot.choices.push_back({"ron", "ron", {}, {"<N type=\"6\" />"}});
            if (hint) snapshot.choices.push_back({"skip", "skip", {}, {"<N />"}});
            return;
        }
        (void) before;
        Offer(after, hint);
    }

    std::vector<std::string> OnlineActions::Submit(std::uint64_t revision, std::size_t index) {
        if (!snapshot.enabled) Fail("interactive mode is disabled");
        if (!snapshot.pending.empty()) Fail("waiting for server confirmation");
        if (revision != snapshot.revision) Fail("stale action revision");
        if (index >= snapshot.choices.size()) Fail("unavailable action");
        pending_ = snapshot.choices[index];
        snapshot.pending = pending_.kind;
        snapshot.choices.clear();
        return pending_.messages;
    }

    std::string ActionStateJson(const ActionSnapshot &s) {
        std::ostringstream out;
        out << "{\"enabled\":" << (s.enabled ? "true" : "false") << ",\"revision\":" << s.revision
                << ",\"pending\":" << NetworkJson(s.pending) << ",\"choices\":[";
        bool first = true;
        for (const auto &a: s.choices) {
            if (!first) out << ',';
            first = false;
            out << "{\"kind\":" << NetworkJson(a.kind) << ",\"label\":" << NetworkJson(a.label) << ",\"tiles\":[";
            for (std::size_t i = 0; i < a.tiles.size(); ++i) {
                if (i) out << ',';
                out << a.tiles[i];
            }
            out << "],\"messages\":[";
            for (std::size_t i = 0; i < a.messages.size(); ++i) {
                if (i) out << ',';
                out << NetworkJson(a.messages[i]);
            }
            out << "]}";
        }
        out << "]}";
        return out.str();
    }
} // namespace tenhou
