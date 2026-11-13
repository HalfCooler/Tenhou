#include "replay_effects.hpp"
#include "animation_clock.hpp"
#include "wall_scene.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace tenhou {
    std::vector<PositionedTile> ReplayTransition::Frame(double elapsed) const {
        std::vector<PositionedTile> result;
        for (const auto &item: tiles) {
            if (elapsed < item.delay) continue;
            if (nuki &&std::string(item.tile.role)=="nuki")
            {
                // 0048f70...0048f795: count=trunc(1+length/40), residual
                // initialized from count-1, then one normalized step per update.
                const float dx = static_cast<float>(item.from_x - item.to_x), dy = static_cast<float>(
                    item.from_y - item.to_y);
                const float length = std::sqrt(dx * dx + dy * dy);
                const unsigned count = static_cast<unsigned>(1 + length / 40);
                const unsigned tick = std::min(count, static_cast<unsigned>(std::max(0.0, elapsed) * 60 / 1000 + 1e-6));
                const float sx = length ? -dx / length * 40 : 0, sy = length ? -dy / length * 40 : 0;
                float rx = -static_cast<float>(count - 1) * sx, ry = -static_cast<float>(count - 1) * sy;
                // 004900a0 adds float steps after each draw; recomputing a
                // multiple loses those rounding points. Count zero draws target,
                // count -1 is removed by 00485b40 on the following queue pass.
                for (unsigned step = 0; step < tick; ++step) {
                    rx += sx;
                    ry += sy;
                }
                // x87 adds the integer anchor without a float store before ftol.
                const int x = tick < count ? static_cast<int>(double(item.to_x) + rx) : item.to_x;
                const int y = tick < count ? static_cast<int>(double(item.to_y) + ry) : item.to_y;
                result.push_back({item.tile, x, y});
                continue;
            }
            const auto span = std::max(1u, duration - std::min(duration, item.delay));
            const auto tick = std::min(double(span), elapsed - item.delay);
            const auto interpolate = [&](int start, int end) {
                if (manual || (nuki && std::string(item.tile.role) == "hand")) {
                    int residual = start - end;
                    const unsigned steps = static_cast<unsigned>((tick + 1e-6) * 60 / 1000);
                    for (unsigned i = 0; i < steps && residual; ++i)
                        residual = static_cast<int>(residual * double(.3f));
                    return tick >= span ? end : end + residual;
                }
                return start + static_cast<int>((double(end - start) * tick) / span);
            };
            result.push_back({item.tile, interpolate(item.from_x, item.to_x), interpolate(item.from_y, item.to_y)});
        }
        return result;
    }

    ReplayTransition BuildReplayTransition(const ReplayState &before, const ReplayState &after,
                                           const ReplayEvent &event, unsigned duration, int canvas_width,
                                           const TileOrigin &origin, bool sort_self) {
        ReplayTransition result;
        if (!duration || !after.initialized || (event.kind != ReplayEventKind::Init &&
                                                event.kind != ReplayEventKind::Draw && event.kind !=
                                                ReplayEventKind::Discard &&
                                                event.kind != ReplayEventKind::Meld && event.tag != "SORT" && event.tag
                                                != "MANUAL" && event.tag != "VARIANT"))
            return result;
        result.duration = duration;
        if (event.kind == ReplayEventKind::Meld && DecodeReplayMeld(
                static_cast<unsigned>(std::stoul(event.attributes.at("m"))), event.who).kind == MeldKind::Nuki) {
            result.nuki = true;
            result.duration = 0;
        }
        const auto previous = event.kind == ReplayEventKind::Init
                                  ? std::vector<TableTile>{}
                                  : BuildTableScene(before, event.tag == "SORT" ? false : sort_self);
        const auto current = BuildTableScene(after, sort_self);
        std::vector<bool> used(previous.size(), false);
        const double unit = canvas_width / 1024.0;
        for (std::size_t n = 0; n < current.size(); ++n) {
            const auto &tile = current[n];
            const auto target = origin(tile);
            auto source = target;
            std::size_t found = previous.size();
            // Entity IDs match the moving hand/river/meld tile. Covered kan backs
            // have no public entity ID and retain matching slots where possible.
            for (std::size_t i = 0; i < previous.size(); ++i)
                if (!used[i] &&
                    previous[i].tile == tile.tile && previous[i].direction == tile.direction &&
                    (tile.tile >= 0 || (previous[i].family == tile.family && previous[i].slot == tile.slot))) {
                    found = i;
                    break;
                }
            // A called river tile changes direction when it joins the caller's meld.
            if (found == previous.size() && tile.tile >= 0 && std::string(tile.role) != "hand")
                for (std::size_t i = 0; i < previous.size(); ++i)
                    if (!used[i] && previous[i].tile == tile.tile) {
                        found = i;
                        break;
                    }
            unsigned delay = 0;
            if (found != previous.size()) {
                used[found] = true;
                source = origin(previous[found]);
            } else if (event.kind == ReplayEventKind::Init) {
                source = {static_cast<int>(480 * unit), static_cast<int>(350 * unit)};
                // Rebuild deal order; mjlog INIT contains hands, not original deal timing.
                delay = static_cast<unsigned>(n % 13) * duration / 26;
            } else if (event.kind == ReplayEventKind::Discard && tile.direction == event.who && std::string(tile.role)
                       == "river") {
                // A hidden opponent discard acquires an identity only when public.
                // Its source is a public hand slot, never an invented hidden entity.
                const auto hand = std::find_if(previous.rbegin(), previous.rend(), [&](const TableTile &old) {
                    return old.direction == tile.direction && std::string(old.role) == "hand";
                });
                if (hand != previous.rend()) source = origin(*hand);
            } else if (event.kind == ReplayEventKind::Draw || event.kind == ReplayEventKind::Meld) {
                source = {static_cast<int>(480 * unit), static_cast<int>(350 * unit)};
                if (event.kind == ReplayEventKind::Draw && before.wall.known && after.wall.known &&
                    tile.direction == event.who && std::string(tile.role) == "hand") {
                    // FUN_00491fe0 T/U/V/W: the public wall slot is the origin,
                    // including the reversed replacement sequence for a kan.
                    const int index = before.wall.pending_kan
                                          ? (135 - before.wall.replacements) ^ 1
                                          : before.wall.front;
                    const auto position = before.wall.Position(index);
                    TableTile wall{'y', position.second / 2 + (position.second % 2 ? 17 : 0), -1, position.first};
                    for (const auto &visible: BuildWallScene(before))
                        if (visible.direction == wall.direction && visible.slot % 17 == wall.slot % 17) {
                            wall.dx = visible.dx;
                            wall.dy = visible.dy;
                            break;
                        }
                    source = origin(wall);
                }
                if (event.kind == ReplayEventKind::Meld && std::string(tile.role) != "hand") {
                    const auto hand = std::find_if(previous.begin(), previous.end(), [&](const TableTile &old) {
                        return old.direction == tile.direction && std::string(old.role) == "hand";
                    });
                    if (hand != previous.end()) source = origin(*hand);
                }
            }
            if (result.nuki) {
                if (std::string(tile.role) == "nuki" && tile.direction == event.who) {
                    // 0049a570: a hidden entity cannot be located by identity;
                    // use the final occupied t slot, not an invented hand entity.
                    if (found == previous.size()) {
                        const auto hand = std::find_if(previous.rbegin(), previous.rend(), [&](const TableTile &old) {
                            return old.direction == tile.direction && std::string(old.role) == "hand";
                        });
                        if (hand != previous.rend()) source = origin(*hand);
                    }
                    const double length = std::hypot(double(source.first - target.first),
                                                     double(source.second - target.second));
                    result.duration = std::max(result.duration,
                                               static_cast<unsigned>(std::ceil(
                                                   (static_cast<unsigned>(1 + length / 40) + 1) * 1000.0 / 60)));
                } else if (std::string(tile.role) == "hand" && tile.direction == event.who) {
                    // N sets the actor's sort flag (0049b800). The next timer
                    // consumes it via 004894a0 -> 00488220 -> 00488650;
                    // existing 0.3f hand residuals compact concurrently.
                    int dx = source.first - target.first, dy = source.second - target.second;
                    unsigned steps = 0;
                    while (dx || dy) {
                        dx = static_cast<int>(dx * double(.3f));
                        dy = static_cast<int>(dy * double(.3f));
                        ++steps;
                    }
                    result.duration = std::max(result.duration,
                                               static_cast<unsigned>(std::ceil((steps + 1) * 1000.0 / 60)));
                } else source = target;
            }
            result.tiles.push_back({tile, source.first, source.second, target.first, target.second, delay});
        }
        return result;
    }

    void TablePresentation::Begin(const ReplayState &before, const ReplayState &after, const ReplayEvent &event,
                                  unsigned duration, int width, const TileOrigin &origin, double now, bool sort_self) {
        Cancel();
        transition_ = BuildReplayTransition(before, after, event, duration, width, origin, sort_self);
        started_ = now;
    }

    void TablePresentation::BeginManual(const ReplayState &before, const ReplayState &after, int width,
                                        const TileOrigin &origin, double now) {
        const auto visible = Active() ? Frame(now) : std::vector<PositionedTile>{};
        ReplayEvent event;
        event.tag = "MANUAL";
        Begin(before, after, event, 1, width, origin, now, false);
        transition_.manual = true;
        unsigned steps = 0;
        for (auto &tile: transition_.tiles) {
            for (const auto &old: visible)
                if (old.tile.tile == tile.tile.tile && old.tile.direction == tile.tile.direction &&
                    old.tile.family == tile.tile.family && old.tile.slot == tile.tile.slot) {
                    tile.from_x = old.x;
                    tile.from_y = old.y;
                    break;
                }
            // Entity follows the hand even when insertion changes its slot.
            for (const auto &old: visible)
                if (tile.tile.tile >= 0 && old.tile.tile == tile.tile.tile && old.tile.direction == tile.tile.
                    direction) {
                    tile.from_x = old.x;
                    tile.from_y = old.y;
                    break;
                }
            int dx = tile.from_x - tile.to_x, dy = tile.from_y - tile.to_y;
            unsigned count = 0;
            while (dx || dy) {
                dx = static_cast<int>(dx * double(.3f));
                dy = static_cast<int>(dy * double(.3f));
                ++count;
            }
            steps = std::max(steps, count);
        }
        // Nominal 60 Hz is the existing reconstruction clock, not original cadence evidence.
        transition_.duration = static_cast<unsigned>(std::ceil(steps * 1000.0 / 60));
    }

    bool TablePresentation::Tick(double now) {
        if (!Active() || now < started_ || now - started_ < transition_.duration) return false;
        Cancel();
        return true;
    }

    std::vector<PositionedTile> TablePresentation::Frame(double now) const {
        return transition_.Frame(std::min(now < started_ ? 0 : now - started_, double(transition_.duration)));
    }

    void OpeningPresentation::Begin(const ReplayState &state, bool wall, TableArtwork *artwork) {
        initial_ = state;
        progress_ = sound_ = 0;
        hands_.clear();
        sounds_.clear();
        packets_.clear();
        sort_tiles_.clear();
        sort_steps_ = 0;
        for (auto &player: initial_.players) if (!player.deal_hand.empty()) player.hand = player.deal_hand;
        wall_tiles_ = wall && state.wall.known ? 136 : 0;
        std::array < unsigned, 4 > counts{};
        for (unsigned step = 0; step < wall_tiles_ / 4; ++step) {
            hands_.push_back(counts);
            sounds_.push_back(104);
        }
        unsigned wall_index = 0;
        for (unsigned packet = 0; packet < 4; ++packet)
            for (int who = 0; who < 4; ++who) {
                const auto seat = (state.dealer + who) % 4;
                if (state.players[seat].hand.empty()) continue;
                const auto begin = counts[seat];
                counts[seat] = std::min(packet < 3 ? (packet + 1) * 4 : 13u,
                                        static_cast<unsigned>(state.players[seat].hand.size()));
                hands_.push_back(counts);
                sounds_.push_back(105);
                packets_.push_back({
                    static_cast<unsigned>(hands_.size()), static_cast<unsigned>(seat), begin, counts[seat], wall_index
                });
                wall_index += counts[seat] - begin;
                // Packet dispatch is a queue entry, followed by event 100 / nTick=6.
                for (unsigned wait = 0; wait < 6; ++wait) {
                    hands_.push_back(counts);
                    sounds_.push_back(0);
                }
            }
        // Native tail wait (9), followed by sort / movement completion.
        for (unsigned wait = 0; wait < 9; ++wait) {
            hands_.push_back(counts);
            sounds_.push_back(0);
        }
        deal_steps_ = static_cast<unsigned>(hands_.size());
        if (artwork) {
            const auto before = BuildTableScene(initial_, false, false);
            for (const auto &tile: BuildTableScene(initial_)) {
                const auto source = std::find_if(before.begin(), before.end(), [&](const TableTile &old) {
                    return old.direction == tile.direction && old.tile == tile.tile && (
                               tile.tile >= 0 || old.slot == tile.slot) && std::string(old.role) == tile.role;
                });
                const auto &to = artwork->Get(tile);
                const auto &from = artwork->Get(source == before.end() ? tile : *source);
                sort_tiles_.push_back({tile, from.x, from.y, to.x, to.y, 0});
                const double distance = std::hypot(double(to.x - from.x), double(to.y - from.y));
                sort_steps_ = std::max(sort_steps_, static_cast<unsigned>(std::ceil(distance / 25)));
            }
            for (unsigned tick = 0; tick < sort_steps_; ++tick) {
                hands_.push_back(counts);
                sounds_.push_back(0);
            }
        }
        active_ = !hands_.empty();
    }

    bool OpeningPresentation::Tick(unsigned steps) {
        if (!active_ || !steps) return false;
        const auto total = static_cast<unsigned>(hands_.size());
        sound_ = 0;
        progress_ += std::min(steps, total - progress_);
        active_ = progress_ < total;
        sound_ = sounds_[progress_ - 1];
        return true;
    }

    unsigned OpeningPresentation::dealt() const {
        unsigned count = 0;
        if (progress_) for (auto n: hands_[progress_ - 1]) count += n;
        return count;
    }

    ReplayState OpeningPresentation::Frame() const {
        auto frame = initial_;
        for (unsigned seat = 0; seat < 4; ++seat) {
            auto &p = frame.players[seat];
            p.hand.resize(progress_ ? hands_[progress_ - 1][seat] : 0);
            p.drawn_tile = -1;
        }
        if (wall_tiles_) frame.wall.front = initial_.wall.initial_removed + static_cast<int>(dealt());
        if (progress_ < wall_tiles_ / 4) frame.dora_indicators.clear();
        return frame;
    }

    std::vector<PositionedTile> OpeningPresentation::Scene(TableArtwork &artwork, bool red) const {
        std::vector<PositionedTile> scene;
        if (progress_ >= deal_steps_ && !sort_tiles_.empty()) {
            const unsigned tick = progress_ - deal_steps_;
            for (const auto &tile: sort_tiles_) {
                const double distance = std::hypot(double(tile.to_x - tile.from_x), double(tile.to_y - tile.from_y));
                const double fraction = distance > 0 ? std::min(1.0, tick * 25.0 / distance) : 1.0;
                scene.push_back({
                    tile.tile, tile.from_x + static_cast<int>((tile.to_x - tile.from_x) * fraction),
                    tile.from_y + static_cast<int>((tile.to_y - tile.from_y) * fraction)
                });
            }
            return scene;
        }
        const auto frame = Frame();
        auto wall_state = initial_;
        wall_state.wall.front = 0;
        const auto wall_scene = BuildWallScene(wall_state);
        for (const auto &tile: BuildTableScene(frame, false, false)) {
            const auto &image = artwork.Get(tile, red);
            const auto offset = artwork.Offset(tile);
            int x = image.x + offset.x, y = image.y + offset.y;
            const int slot = tile.direction && tile.tile >= 0 ? 17 - tile.slot : tile.slot;
            if (wall_tiles_ &&std::string(tile.role)=="hand") for (const auto &packet: packets_) {
                if (packet.who != static_cast<unsigned>(tile.direction) || slot < static_cast<int>(packet.begin) ||
                    slot >= static_cast<int>(packet.end) || progress_ < packet.step || progress_ - packet.step >= 12)
                    continue;
                // FUN_0048de30 packet branch -> FUN_0048f2b0 / 0048f650:
                // wall entity origin, speed distance/12, 12 position advances.
                // Keep the user-authorized clean single sprite (no mode-3 trails).
                const auto position = initial_.wall.Position(
                    initial_.wall.initial_removed + static_cast<int>(packet.wall) + slot - static_cast<int>(packet.
                        begin));
                TableTile source{'y', position.second / 2 + (position.second % 2 ? 17 : 0), -1, position.first};
                for (const auto &visible: wall_scene)
                    if (visible.direction == source.direction && visible.slot % 17 == source.slot % 17) {
                        source.dx = visible.dx;
                        source.dy = visible.dy;
                        break;
                    }
                const auto &origin = artwork.Get(source, red);
                const auto shift = artwork.Offset(source);
                const int remaining = 12 - static_cast<int>(progress_ - packet.step);
                x += static_cast<int>(float(origin.x + shift.x - x) * remaining / 12);
                y += static_cast<int>(float(origin.y + shift.y - y) * remaining / 12);
                break;
            }
            scene.push_back({tile, x, y});
        }
        return scene;
    }

    unsigned ActionPauseTicks(const ReplayEvent &event) {
        // FUN_00489c40, C93638 / C93717; own T has no artificial think wait.
        if (event.kind == ReplayEventKind::Discard) return 15;
        if (event.kind == ReplayEventKind::Draw && event.who != 0) return 41;
        return 0;
    }

    unsigned DrawEndChangedSeats(const ReplayState &state, unsigned slot) {
        if (slot >= 14) return 0;
        auto before = state, after = state;
        before.draw_end_slots = slot;
        after.draw_end_slots = slot + 1;
        const auto old_scene = BuildTableScene(before), new_scene = BuildTableScene(after);
        unsigned changed = 0;
        for (std::size_t i = 0; i < old_scene.size(); ++i) {
            const auto &a = old_scene[i];
            const auto &b = new_scene[i];
            if (std::string(a.role) == "hand" && (a.family != b.family || a.slot != b.slot || a.tile != b.tile ||
                                                  a.dx != b.dx || a.dy != b.dy || a.drawn != b.drawn))
                changed |= 1u << a.direction;
        }
        return changed;
    }

    std::vector<unsigned> ReplaySounds(const ReplayEvent &event, char gender, bool effects_only,
                                       bool individual_voices) {
        if (effects_only) gender = '?';
        const auto voice = [gender,individual_voices,&event](unsigned neutral, unsigned male, unsigned female) {
            const unsigned seat = individual_voices && event.who >= 0 && event.who < 4
                                      ? static_cast<unsigned>(event.who)
                                      : 0;
            return gender == 'M' ? male + seat : gender == 'F' ? female + seat : neutral;
        };
        switch (event.kind) {
            case ReplayEventKind::Init: {
                // START is the match opening (East 1, zero honba), not each new hand.
                std::istringstream seed(event.attributes.at("seed"));
                int round = -1, honba = -1;
                char comma = 0;
                if (seed >> round >> comma >> honba && comma == ',' && round == 0 && honba == 0) return {112};
                return {};
            }
            case ReplayEventKind::Draw: return {0x69};
            case ReplayEventKind::Discard: return {0x67};
            case ReplayEventKind::Meld: {
                const auto meld = DecodeReplayMeld(static_cast<std::uint32_t>(std::stoul(event.attributes.at("m"))),
                                                   event.who);
                if (meld.kind == MeldKind::Nuki) return {107};
                return {
                    113,
                    meld.kind == MeldKind::Chi
                        ? voice(118, 119, 123)
                        : meld.kind == MeldKind::Pon
                              ? voice(136, 137, 141)
                              : voice(127, 128, 132)
                };
            }
            case ReplayEventKind::Reach:
                if (std::stoi(event.attributes.at("step")) != 1) return {};
                // 145 is the declaration effect, also retained when a voiced call exists.
                return gender == 'M' || gender == 'F'
                           ? std::vector<unsigned>{145, voice(145, 146, 150)}
                           : std::vector<unsigned>{145};
            case ReplayEventKind::Dora: return {}; // SE_DORA (111) is the start/yakuman sound.
            case ReplayEventKind::Win: return {
                    114,
                    std::stoi(event.attributes.at("who")) == std::stoi(event.attributes.at("fromWho"))
                        ? voice(163, 164, 168)
                        : voice(154, 155, 159)
                };
            default: return {};
        }
    }

    unsigned DefaultSoundResource(unsigned id) {
        if (id == 0x70) return 0x6f;
        if (id == 0x7f) return 0x88;
        for (unsigned base: {0x77u, 0x7bu, 0x80u, 0x84u, 0x89u, 0x8du, 0x92u, 0x96u, 0x9bu, 0x9fu, 0xa4u, 0xa8u})
            if (id > base && id <= base + 3) return base;
        return id;
    }

    const std::map<unsigned, std::string> &SoundKeys() {
        static const std::map<unsigned, std::string> keys{
            {0x65, "SE_AGARI"},
            {0x66, "SE_CLOCK"},
            {0x67, "SE_DA"},
            {0x68, "SE_KACHI1"},
            {0x69, "SE_KACHI2"},
            {0x6a, "SE_NAKI"},
            {0x6b, "SE_NUKI"},
            {0x6c, "SE_TICK"},
            {0x6d, "SE_YAKU"},
            {0x6f, "SE_DORA"},
            {0x70, "SE_START"},
            {0x71, "SE_SHA"},
            {0x72, "SE_KIRA"},
            {0x73, "SE_GAMEEND"},
            {0x74, "SE_LOGIN"},
            {0x75, "SE_LOGOUT"},
            {0x76, "SE_CHII"},
            {0x77, "SE_CHII_M"},
            {0x78, "SE_CHII_M1"},
            {0x79, "SE_CHII_M2"},
            {0x7a, "SE_CHII_M3"},
            {0x7b, "SE_CHII_F"},
            {0x7c, "SE_CHII_F1"},
            {0x7d, "SE_CHII_F2"},
            {0x7e, "SE_CHII_F3"},
            {0x7f, "SE_KAN"},
            {0x80, "SE_KAN_M"},
            {0x81, "SE_KAN_M1"},
            {0x82, "SE_KAN_M2"},
            {0x83, "SE_KAN_M3"},
            {0x84, "SE_KAN_F"},
            {0x85, "SE_KAN_F1"},
            {0x86, "SE_KAN_F2"},
            {0x87, "SE_KAN_F3"},
            {0x88, "SE_PON"},
            {0x89, "SE_PON_M"},
            {0x8a, "SE_PON_M1"},
            {0x8b, "SE_PON_M2"},
            {0x8c, "SE_PON_M3"},
            {0x8d, "SE_PON_F"},
            {0x8e, "SE_PON_F1"},
            {0x8f, "SE_PON_F2"},
            {0x90, "SE_PON_F3"},
            {0x91, "SE_REACH"},
            {0x92, "SE_REACH_M"},
            {0x93, "SE_REACH_M1"},
            {0x94, "SE_REACH_M2"},
            {0x95, "SE_REACH_M3"},
            {0x96, "SE_REACH_F"},
            {0x97, "SE_REACH_F1"},
            {0x98, "SE_REACH_F2"},
            {0x99, "SE_REACH_F3"},
            {0x9a, "SE_RON"},
            {0x9b, "SE_RON_M"},
            {0x9c, "SE_RON_M1"},
            {0x9d, "SE_RON_M2"},
            {0x9e, "SE_RON_M3"},
            {0x9f, "SE_RON_F"},
            {0xa0, "SE_RON_F1"},
            {0xa1, "SE_RON_F2"},
            {0xa2, "SE_RON_F3"},
            {0xa3, "SE_TSUMO"},
            {0xa4, "SE_TSUMO_M"},
            {0xa5, "SE_TSUMO_M1"},
            {0xa6, "SE_TSUMO_M2"},
            {0xa7, "SE_TSUMO_M3"},
            {0xa8, "SE_TSUMO_F"},
            {0xa9, "SE_TSUMO_F1"},
            {0xaa, "SE_TSUMO_F2"},
            {0xab, "SE_TSUMO_F3"},
        };
        return keys;
    }
} // namespace tenhou
