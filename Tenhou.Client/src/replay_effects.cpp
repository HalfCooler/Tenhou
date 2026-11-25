#include "replay_effects.hpp"
#include "wall_geometry.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace tenhou {
    namespace {
        constexpr unsigned kDealPacketTicks = 1 + 6; // Dispatch, then event 100.
        constexpr unsigned kDealTailTicks = 9;

        float SpeedForSteps(int dx, int dy, unsigned steps) {
            const double factor = steps == 12 ? double(0.083333336f) : 1.0 / steps;
            return static_cast<float>(std::hypot(dx, dy) * factor);
        }

        TableTile WallOrigin(const WallState &wall, int index, const std::vector<TableTile> &visible_wall) {
            const auto position = wall.Position(index);
            auto source = WallTile(wall, position.first, position.second / 2, position.second % 2 == 0);
            const auto stack = std::find_if(visible_wall.begin(), visible_wall.end(), [&](const TableTile &visible) {
                return visible.direction == source.direction && visible.wall_stack == source.wall_stack;
            });
            if (stack != visible_wall.end()) {
                source.dx = stack->dx;
                source.dy = stack->dy;
            }
            return source;
        }

        int HandSlot(const TableTile &tile) {
            return tile.family == 'n' ? 17 - tile.slot : tile.slot;
        }

        TableTile HandBack(TableTile tile) {
            tile.slot = 17 - HandSlot(tile);
            tile.family = 'n';
            tile.tile = -1;
            return tile;
        }

        std::pair<int, int> HandFlightLanding(const TableTile &tile, const TableTile &back,
                                              std::pair<int, int> target, const TileGeometryCatalog &geometry,
                                              bool red) {
            const auto back_image = geometry.Bounds(back, red);
            const auto face_image = geometry.Bounds(tile, red);
            if (tile.direction == 0 && tile.tile >= 0) {
                return {target.first + (face_image.width - back_image.width) / 2,
                        target.second + (face_image.height - back_image.height) / 2};
            }
            return {target.first + back_image.x - face_image.x, target.second + back_image.y - face_image.y};
        }

        unsigned CompactSteps(int dx, int dy) {
            unsigned steps = 0;
            while (dx || dy) {
                dx = static_cast<int>(dx * double(.3f));
                dy = static_cast<int>(dy * double(.3f));
                ++steps;
            }
            return steps;
        }

        bool IsHand(const TableTile &tile) { return std::string_view(tile.role) == "hand"; }
    }

    TileFlight::TileFlight(const TableTile &sprite, int x, int y, int from_x, int from_y, float speed,
                           unsigned copies, float opacity, bool cover_landing) {
        const int dx = from_x - x, dy = from_y - y;
        if ((!dx && !dy) || speed <= 0) return;
        const double length = std::hypot(dx, dy);
        const double step_y = -dy / length * speed;
        step_x_ = static_cast<float>(-dx / length * speed);
        step_y_ = static_cast<float>(step_y);
        const auto advances = static_cast<unsigned>(1 - (dx ? dx / double(step_x_) : dy / step_y)) - 1;
        ticks = advances + 2; // The residual reaches zero before the count-zero frame.
        residual_x_ = static_cast<float>(-double(advances) * step_x_);
        residual_y_ = static_cast<float>(-double(advances) * step_y);
        image_.sprite = sprite;
        image_.landing_x = x;
        image_.landing_y = y;
        image_.cover_landing = cover_landing;
        image_.count = copies;
        image_.opacity = copies > 2 ? opacity * .5f : opacity;
        // The original places the copies in positive coordinates within a single
        // bitmap. Truncate there before translating the bitmap back to the anchor.
        const double reach = (copies - 1) * .5;
        const auto offset = [reach](float step, double distance) {
            const int inset = static_cast<int>(std::max(0.f, step) * reach);
            return static_cast<int>(inset - step * distance) - inset;
        };
        for (unsigned i = 0; i < copies; ++i) {
            const double distance = (copies - 1 - i) * .5;
            image_.offsets[i] = {offset(step_x_, distance), offset(step_y_, distance)};
        }
    }

    void TileFlight::Place(PositionedTile &tile, unsigned tick) const {
        if (tick >= ticks) return;
        tile.flight = image_;
        tile.x = image_.landing_x;
        tile.y = image_.landing_y;
        if (tick + 1 == ticks) return;
        float rx = residual_x_, ry = residual_y_;
        for (unsigned i = 0; i < tick; ++i) {
            rx += step_x_;
            ry += step_y_;
        }
        tile.x = static_cast<int>(tile.x + double(rx));
        tile.y = static_cast<int>(tile.y + double(ry));
    }

    std::vector<PositionedTile> ReplayTransition::Frame(double elapsed) const {
        std::vector<PositionedTile> result;
        result.reserve(tiles.size());
        for (const auto &item: tiles) {
            if (elapsed < item.delay) {
                if (item.before_flip) result.push_back({*item.before_flip, item.from_x, item.from_y});
                continue;
            }
            PositionedTile placed{item.tile, item.to_x, item.to_y};
            const double age = elapsed - item.delay;
            if (item.flight.ticks) {
                item.flight.Place(placed, static_cast<unsigned>(age / kTableTickMs));
            } else if (manual || item.compact_hand) {
                const unsigned steps = static_cast<unsigned>(manual ? age * 60 / 1000 : age / kTableTickMs);
                auto compact = [steps](int residual) {
                    for (unsigned i = 0; i < steps && residual; ++i)
                        residual = static_cast<int>(residual * double(.3f));
                    return residual;
                };
                placed.x += compact(item.from_x - item.to_x);
                placed.y += compact(item.from_y - item.to_y);
            } else if (elapsed < duration) {
                const auto span = std::max(1u, duration - std::min(duration, item.delay));
                placed.x = item.from_x + static_cast<int>((item.to_x - item.from_x) * age / span);
                placed.y = item.from_y + static_cast<int>((item.to_y - item.from_y) * age / span);
            }
            result.push_back(placed);
        }
        return result;
    }

    ReplayTransition BuildReplayTransition(const ReplayState &before, const ReplayState &after,
                                           const ReplayEvent &event, unsigned duration, int canvas_width,
                                           const TileOrigin &origin, bool sort_self,
                                           const TileGeometryCatalog *geometry, bool red) {
        ReplayTransition result;
        const bool draw = event.kind == ReplayEventKind::Draw;
        const bool discard = event.kind == ReplayEventKind::Discard;
        const bool meld = event.kind == ReplayEventKind::Meld;
        const bool sort = event.tag == "SORT";
        const bool native_motion = draw || discard || meld || sort;
        if (!after.initialized || (!native_motion && event.kind != ReplayEventKind::Init &&
                                  event.tag != "MANUAL" && event.tag != "VARIANT"))
            return result;
        result.duration = native_motion ? 0 : duration;
        const auto previous = event.kind == ReplayEventKind::Init
                                  ? std::vector<TableTile>{}
                                  : BuildTableScene(before, sort ? false : sort_self);
        const auto current = BuildTableScene(after, sort_self);
        std::vector<bool> used(previous.size(), false);
        const double unit = canvas_width / 1024.0;

        const bool actor = SeatPresent(after.seat_mask, event.who);
        std::vector<std::size_t> hidden, removed, covered;
        std::size_t next_hidden = 0, next_removed = 0, next_covered = 0;
        int discarded_tile = -1;
        bool tsumogiri = false;
        if (actor && (discard || meld)) {
            const auto &old = before.players[event.who];
            const auto &player = after.players[event.who];
            const auto count = std::count(player.hand.begin(), player.hand.end(), -1);
            for (std::size_t i = 0; i < previous.size(); ++i)
                if (previous[i].direction == event.who && IsHand(previous[i]) && previous[i].tile < 0) hidden.push_back(i);
            if (discard && !player.discards.empty()) {
                discarded_tile = player.discards.back().tile;
                tsumogiri = player.discards.back().tsumogiri;
            }
            if (meld && player.melds.size() > old.melds.size() && player.melds.back().kind == MeldKind::ClosedKan) {
                for (std::size_t i = 0; i < previous.size(); ++i) {
                    const auto &tile = previous[i];
                    const auto &kan = player.melds.back().tiles;
                    if (tile.direction != event.who || !IsHand(tile) || tile.tile < 0 ||
                        std::find(kan.begin(), kan.end(), tile.tile) == kan.end()) continue;
                    if (std::none_of(current.begin(), current.end(), [&](const TableTile &next) {
                            return next.direction == event.who && next.tile == tile.tile &&
                                   std::string_view(next.role) == "meld";
                        })) covered.push_back(i);
                }
            }
            // The original uses a standing slot for a hidden tedashi and the draw
            // slot for tsumogiri. Use the public tile ID to reproduce a stable choice.
            while (hidden.size() > static_cast<std::size_t>(count)) {
                const std::size_t last = hidden.size() - 1;
                const bool extraction = player.nuki.size() > old.nuki.size() ||
                    (meld && player.melds.size() == old.melds.size());
                const auto index = discard && !tsumogiri && last
                    ? static_cast<std::size_t>(std::max(discarded_tile, 0)) % last
                    : extraction && !old.hidden_draw ? 0 : last;
                removed.push_back(hidden[index]);
                used[hidden[index]] = true;
                hidden.erase(hidden.begin() + index);
            }
        }

        const auto wall = draw && before.wall.known
            ? BuildWallScene(before, static_cast<unsigned>(before.wall.total)) : std::vector<TableTile>{};
        for (std::size_t n = 0; n < current.size(); ++n) {
            const auto &tile = current[n];
            const auto role = std::string_view(tile.role);
            const auto target = origin(tile);
            auto source = target;
            const bool acting_tile = actor && tile.direction == event.who;
            const bool hand = role == "hand";
            std::size_t found = previous.size();
            if (acting_tile && hand && tile.tile < 0 && (discard || meld) && next_hidden < hidden.size()) {
                found = hidden[next_hidden++];
            } else {
                for (std::size_t i = 0; i < previous.size(); ++i)
                    if (!used[i] && previous[i].tile == tile.tile && previous[i].direction == tile.direction &&
                        (tile.tile >= 0 || (previous[i].family == tile.family && previous[i].slot == tile.slot))) {
                        found = i;
                        break;
                    }
            }
            // A called tile joins the meld from the source seat's river.
            if (found == previous.size() && tile.tile >= 0 && !hand)
                for (std::size_t i = 0; i < previous.size(); ++i)
                    if (!used[i] && previous[i].tile == tile.tile && std::string_view(previous[i].role) == "river") {
                        found = i;
                        break;
                    }

            unsigned delay = 0;
            std::optional<TableTile> before_flip;
            if (event.kind == ReplayEventKind::Win && after.variant && role == "retired") {
                const int index = 17 - tile.slot;
                for (std::size_t i = 0; i < previous.size(); ++i) {
                    const auto &old = previous[i];
                    if (old.direction == tile.direction && IsHand(old) && HandSlot(old) == index) {
                        found = i;
                        before_flip = old;
                        delay = 60u * (index + 1);
                        break;
                    }
                }
                result.duration = std::max(result.duration, 900u);
            }

            const bool drawn = draw && acting_tile && hand && found == previous.size();
            const bool discarded = discard && acting_tile && role == "river" && tile.tile == discarded_tile;
            const bool melded = meld && acting_tile && (role == "meld" || role == "added_kan" || role == "nuki" || role == "flower") &&
                (found == previous.size() || std::string_view(previous[found].role) != role);
            if (found == previous.size()) {
                if ((discarded || melded) && next_removed < removed.size()) found = removed[next_removed++];
                else if (melded && tile.tile < 0 && next_covered < covered.size()) found = covered[next_covered++];
            }
            if (found != previous.size()) {
                used[found] = true;
                source = origin(previous[found]);
            } else if (event.kind == ReplayEventKind::Init) {
                source = {static_cast<int>(480 * unit), static_cast<int>(350 * unit)};
                delay = static_cast<unsigned>(n % 13) * duration / 26;
            } else if (drawn) {
                if (before.wall.known && after.wall.known) {
                    const int index = !before.wall.dead_wall && after.wall.tail > before.wall.tail
                                          ? before.wall.total - after.wall.tail
                                          : before.wall.pending_kan
                                          ? (135 - before.wall.replacements) ^ 1
                                          : before.wall.front;
                    source = origin(WallOrigin(before.wall, index, wall));
                } else {
                    source = {static_cast<int>(480 * unit), static_cast<int>(350 * unit)};
                }
            }

            AnimatedTile animation{tile, source.first, source.second, target.first, target.second, delay, before_flip};
            if (drawn || discarded || melded) {
                auto sprite = tile;
                auto landing = target;
                if (drawn && geometry) {
                    sprite = HandBack(tile);
                    landing = HandFlightLanding(tile, sprite, target, *geometry, red);
                }
                const auto speed = melded ? 40.f : SpeedForSteps(source.first - landing.first,
                                                               source.second - landing.second, drawn ? 12u : 8u);
                animation.flight = TileFlight(sprite, landing.first, landing.second, source.first, source.second,
                                              speed, 4, melded ? .32f : .16f,
                                              discarded || (drawn && tile.direction == 1));
                result.duration = std::max(result.duration, animation.flight.ticks * kTableTickMs);
            } else if (native_motion) {
                animation.compact_hand = hand && (sort || (acting_tile && (discard || meld)));
                if (animation.compact_hand) {
                    const auto steps = CompactSteps(source.first - target.first, source.second - target.second);
                    if (steps) result.duration = std::max(result.duration, (steps + 1) * kTableTickMs);
                } else {
                    animation.from_x = animation.to_x;
                    animation.from_y = animation.to_y;
                }
            }
            result.tiles.push_back(std::move(animation));
        }
        return result;
    }

    void TablePresentation::Begin(const ReplayState &before, const ReplayState &after, const ReplayEvent &event,
                                  unsigned duration, int width, const TileOrigin &origin, double now, bool sort_self,
                                  const TileGeometryCatalog *geometry, bool red, unsigned playback_limit) {
        Cancel();
        transition_ = BuildReplayTransition(before, after, event, duration, width, origin, sort_self, geometry, red);
        started_ = now;
        playback_scale_ = playback_limit && transition_.duration > playback_limit
            ? double(transition_.duration) / playback_limit : 1;
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
        if (!Active() || now < started_ || (now - started_) * playback_scale_ < transition_.duration) return false;
        Cancel();
        return true;
    }

    std::vector<PositionedTile> TablePresentation::Frame(double now) const {
        return transition_.Frame(std::min(now < started_ ? 0 : (now - started_) * playback_scale_, double(transition_.duration)));
    }

    void OpeningPresentation::Begin(const ReplayState &state, bool wall, const TileGeometryCatalog *geometry, bool red) {
        initial_ = state;
        progress_ = sound_ = 0;
        next_step_ = 0;
        packets_.clear();
        deal_flights_ = {};
        deal_starts_ = {};
        sort_tiles_.clear();
        for (auto &player: initial_.players) {
            if (!player.deal_hand.empty()) player.hand = player.deal_hand;
            player.drawn_tile = -1;
            player.hidden_draw = false;
        }
        wall_tiles_ = wall && state.wall.known ? static_cast<unsigned>(state.wall.total) : 0;
        const unsigned stacks = wall_tiles_ / 2;
        // 0048de30: 0x6e runs through the six tail ticks, then 0x6f
        // prepares the break and 0x70 reveals the first indicator.
        wall_setup_ticks_ = stacks ? stacks + 6 + (state.wall.dead_wall ? 2 : 0) : 0;
        std::array<unsigned, 4> counts{};
        hands_.assign(wall_setup_ticks_, counts);
        sounds_.assign(wall_setup_ticks_, 0);
        for (unsigned step = 0; step < stacks; ++step) sounds_[step + 6] = 104;

        unsigned wall_index = 0;
        for (unsigned packet = 0; packet < 4; ++packet)
            for (int who = 0; who < 4; ++who) {
                const auto seat = (state.dealer + who) % 4;
                if (!SeatPresent(state.seat_mask, seat) || initial_.players[seat].hand.empty()) continue;
                const auto begin = counts[seat];
                counts[seat] = std::min(packet < 3 ? (packet + 1) * 4 : 13u,
                                        static_cast<unsigned>(initial_.players[seat].hand.size()));
                if (begin == counts[seat]) continue;
                packets_.push_back({static_cast<unsigned>(hands_.size()) + 1, static_cast<unsigned>(seat),
                                    begin, counts[seat], wall_index});
                wall_index += counts[seat] - begin;
                hands_.insert(hands_.end(), kDealPacketTicks, counts);
            }
        hands_.insert(hands_.end(), kDealTailTicks, counts);
        sounds_.resize(hands_.size());
        sort_start_ = static_cast<unsigned>(hands_.size()) + 1;
        unsigned sort_ticks = 0;
        if (geometry && geometry->loaded()) {
            const auto position = [&](const TableTile &tile) { return geometry->Origin(tile, red); };
            const auto unsorted = BuildTableScene(initial_, false, false);
            if (wall_tiles_) {
                auto wall_state = initial_;
                wall_state.wall.front = wall_state.wall.initial_removed;
                const auto visible = BuildWallScene(wall_state, wall_tiles_);
                for (const auto &packet: packets_) {
                    unsigned landing_tick = 0;
                    for (const auto &tile: unsorted) {
                        if (!IsHand(tile) || tile.direction != static_cast<int>(packet.who)) continue;
                        const auto slot = static_cast<unsigned>(HandSlot(tile));
                        if (slot < packet.begin || slot >= packet.end) continue;
                        const int index = initial_.wall.initial_removed + static_cast<int>(packet.wall + slot - packet.begin);
                        const auto source = position(WallOrigin(initial_.wall, index, visible));
                        const auto sprite = HandBack(tile);
                        const auto target = HandFlightLanding(tile, sprite, position(tile), *geometry, red);
                        const auto speed = SpeedForSteps(source.first - target.first, source.second - target.second, 12);
                        auto &flight = deal_flights_[packet.who][slot];
                        flight = TileFlight(sprite, target.first, target.second, source.first, source.second, speed,
                                            4, .16f, packet.who == 1);
                        deal_starts_[packet.who][slot] = packet.step;
                        landing_tick = std::max(landing_tick, flight.ticks);
                    }
                    sounds_[packet.step - 1 + landing_tick] = 105;
                }
            }
            for (const auto &tile: BuildTableScene(initial_)) {
                const auto source = std::find_if(unsorted.begin(), unsorted.end(), [&](const TableTile &old) {
                    return old.direction == tile.direction && old.tile == tile.tile &&
                           (tile.tile >= 0 || old.slot == tile.slot) && std::string_view(old.role) == tile.role;
                });
                const auto to = position(tile);
                const auto from = position(source == unsorted.end() ? tile : *source);
                AnimatedTile animation{tile, from.first, from.second, to.first, to.second};
                animation.flight = TileFlight(tile, to.first, to.second, from.first, from.second, 25.f, 2, .6f, false);
                sort_ticks = std::max(sort_ticks, animation.flight.ticks);
                sort_tiles_.push_back(std::move(animation));
            }
        }
        if (!wall_tiles_ || !geometry || !geometry->loaded())
            for (const auto &packet: packets_) sounds_[packet.step - 1] = 105;
        // The 0x65 gate follows the sort dispatch. Publish the settled hand
        // before 0x82 releases the first draw.
        hands_.insert(hands_.end(), std::max(1u, sort_ticks) + 2, counts);
        sounds_.resize(hands_.size());
        active_ = wall_tiles_ || !packets_.empty();
    }

    bool OpeningPresentation::Tick(unsigned steps) {
        if (!active_ || !steps) return false;
        const auto total = static_cast<unsigned>(hands_.size());
        const unsigned finish = progress_ + std::min(steps, total - progress_);
        sound_ = 0;
        while (progress_ < finish) {
            if (sounds_[progress_]) sound_ = sounds_[progress_];
            ++progress_;
        }
        active_ = progress_ < total;
        return true;
    }

    bool OpeningPresentation::Advance(double now) {
        if (!active_) return false;
        if (!next_step_) next_step_ = now;
        if (now < next_step_) return false;
        const unsigned steps = static_cast<unsigned>(std::min(1 + std::floor((now - next_step_) / kTableTickMs),
                                                               double(hands_.size() - progress_)));
        next_step_ += steps * kTableTickMs;
        return Tick(steps);
    }

    unsigned OpeningPresentation::dealt() const {
        unsigned count = 0;
        if (progress_) for (auto n: hands_[progress_ - 1]) count += n;
        return count;
    }

    ReplayState OpeningPresentation::Frame() const {
        auto frame = initial_;
        frame.turn = -1;
        for (const auto &packet: packets_)
            if (progress_ >= packet.step && progress_ - packet.step < kDealPacketTicks) {
                frame.turn = static_cast<int>(packet.who);
                break;
            }
        for (unsigned seat = 0; seat < 4; ++seat) {
            auto &p = frame.players[seat];
            p.hand.resize(progress_ ? hands_[progress_ - 1][seat] : 0);
            if (progress_ < sort_start_) p.flowers.clear();
        }
        if (wall_tiles_) {
            if (progress_ <= wall_tiles_ / 2 + 6) {
                frame.wall.dead_wall = false;
                frame.wall.front = frame.wall.initial_removed = frame.wall.tail = frame.wall.replacements = 0;
            } else {
                frame.wall.front = initial_.wall.initial_removed + static_cast<int>(dealt());
            }
            if (progress_ < wall_setup_ticks_) frame.dora_indicators.clear();
        }
        return frame;
    }

    void OpeningPresentation::SlideWall(std::vector<PositionedTile> &scene) const {
        if (!wall_tiles_ || progress_ > wall_tiles_ / 2 + 5) return;
        constexpr int dx[]{0, 160, 0, -160}, dy[]{140, 0, -140, 0};
        for (auto &item: scene) {
            if (item.tile.wall_stack < 0) continue;
            const auto seat = item.tile.direction;
            const auto start = WallStackOrder(initial_.wall, seat, item.tile.wall_stack) + 1;
            if (progress_ < start || progress_ - start > 5) continue;
            const float speed = SpeedForSteps(dx[seat], dy[seat], 4);
            const TileFlight flight(item.tile, item.x, item.y, item.x + dx[seat], item.y + dy[seat], speed,
                                    2, .08f, seat == 1);
            flight.Place(item, progress_ - start);
        }
    }

    std::vector<PositionedTile> OpeningPresentation::Scene(const TileGeometryCatalog &geometry, bool red) const {
        std::vector<PositionedTile> scene;
        if (progress_ >= sort_start_ && !sort_tiles_.empty()) {
            for (const auto &tile: sort_tiles_) {
                PositionedTile placed{tile.tile, tile.to_x, tile.to_y};
                tile.flight.Place(placed, progress_ - sort_start_);
                scene.push_back(placed);
            }
            return scene;
        }
        const auto frame = Frame();
        for (const auto &tile: BuildTableScene(frame, progress_ >= sort_start_, progress_ >= sort_start_)) {
            const auto origin = geometry.Origin(tile, red);
            PositionedTile placed{tile, origin.first, origin.second};
            if (IsHand(tile)) {
                const auto slot = static_cast<unsigned>(HandSlot(tile));
                const auto start = deal_starts_[tile.direction][slot];
                if (start && progress_ >= start)
                    deal_flights_[tile.direction][slot].Place(placed, progress_ - start);
            }
            scene.push_back(placed);
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
                if (event.tag == "VARIANT") {
                    const auto &kind = event.attributes.at("kind");
                    return {113, kind == "chi" ? voice(118, 119, 123)
                        : kind == "pon" ? voice(136, 137, 141) : voice(127, 128, 132)};
                }
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
