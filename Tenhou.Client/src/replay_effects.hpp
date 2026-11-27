#pragma once
#include "table_scene.hpp"
#include <algorithm>
#include <functional>
#include <optional>

namespace tenhou {
    constexpr unsigned kTableTickMs = 15;

    // 0048f650 and 004900a0, including the separate frame at the landing point.
    struct TileFlight {
        TileFlight() = default;
        TileFlight(const TableTile &sprite, int x, int y, int from_x, int from_y, float speed,
                   unsigned copies, float opacity, bool cover_landing);

        void Place(PositionedTile &tile, unsigned tick) const;
        unsigned ticks = 0;

    private:
        TileFlightImage image_;
        float step_x_ = 0, step_y_ = 0, residual_x_ = 0, residual_y_ = 0;
    };

    // Presentation only: the ReplaySession remains at the applied event's final state.
    struct AnimatedTile {
        TableTile tile;
        int from_x = 0, from_y = 0, to_x = 0, to_y = 0;
        unsigned delay = 0;
        std::optional<TableTile> before_flip{};
        TileFlight flight{};
        bool compact_hand = false;
    };

    struct ReplayTransition {
        unsigned duration = 0;
        bool manual = false;
        std::vector<AnimatedTile> tiles;

        std::vector<PositionedTile> Frame(double elapsed) const;
    };

    using TileOrigin = std::function<std::pair<int, int>(const TableTile &)>;

    ReplayTransition BuildReplayTransition(const ReplayState &before, const ReplayState &after,
                                           const ReplayEvent &event, unsigned duration, int canvas_width,
                                           const TileOrigin &origin, bool sort_self = true,
                                           const TileGeometryCatalog *geometry = nullptr, bool red = true);

    inline ReplayTransition BuildReplayTransition(const ReplayState &before, const ReplayState &after,
                                                  const ReplayEvent &event, const unsigned duration, const int canvas_width,
                                                  const TileOrigin &origin, const bool sort_self,
                                                  TableArtwork *artwork, const bool red = true) {
        return BuildReplayTransition(before, after, event, duration, canvas_width, origin, sort_self,
                                     artwork ? &artwork->geometry() : nullptr, red);
    }

    // A single presentation slot: a new accepted event replaces the old movement.
    // Caller-owned authoritative states and clocks are never advanced by this class.
    class TablePresentation {
    public:
        void Begin(const ReplayState &before, const ReplayState &after, const ReplayEvent &event,
                   unsigned duration, int width, const TileOrigin &origin, double now, bool sort_self = true,
                   const TileGeometryCatalog *geometry = nullptr, bool red = true, unsigned playback_limit = 0);

        void Begin(const ReplayState &before, const ReplayState &after, const ReplayEvent &event,
                   const unsigned duration, const int width, const TileOrigin &origin, const double now, const bool sort_self,
                   TableArtwork *artwork, const bool red = true, const unsigned playback_limit = 0) {
            Begin(before, after, event, duration, width, origin, now, sort_self,
                  artwork ? &artwork->geometry() : nullptr, red, playback_limit);
        }

        void BeginManual(const ReplayState &before, const ReplayState &after, int width, const TileOrigin &origin,
                         double now);

        bool Manual() const { return Active() && transition_.manual; }
        void Cancel() { transition_ = {}; }
        bool Active() const { return transition_.duration != 0; }

        bool Tick(double now);

        void ShiftTime(const double delay) { started_ += delay; }

        std::vector<PositionedTile> Frame(double now) const;

    private:
        ReplayTransition transition_;
        double started_ = 0;
        double playback_scale_ = 1;
    };

    // Unknown gender keeps the original neutral resource; never infer it from names.
    std::vector<unsigned> ReplaySounds(const ReplayEvent &event, char gender = '?', bool effects_only = false,
                                       bool individual_voices = false);

    // Prepare the wall in display order, then deal starting at the dealer.
    class OpeningPresentation {
    public:
        void Begin(const ReplayState &state, bool wall, const TileGeometryCatalog *geometry = nullptr, bool red = true);

        void Begin(const ReplayState &state, const bool wall, TableArtwork *artwork, const bool red = true) {
            Begin(state, wall, artwork ? &artwork->geometry() : nullptr, red);
        }

        bool Tick(unsigned steps = 1);
        bool Advance(double now);
        void ResumeClock(const double now) { next_step_ = now + kTableTickMs; }

        bool Active() const { return active_; }
        void Cancel() { active_ = false; }

        ReplayState Frame() const;

        std::vector<PositionedTile> Scene(const TileGeometryCatalog &geometry, bool red = true) const;

        std::vector<PositionedTile> Scene(TableArtwork &artwork, const bool red = true) const {
            return Scene(artwork.geometry(), red);
        }

        void SlideWall(std::vector<PositionedTile> &scene) const;

        unsigned wall_tiles() const { return wall_tiles_; }
        unsigned placed() const { return std::min(progress_ * 2, wall_tiles_); }

        unsigned dealt() const;

        unsigned steps() const { return progress_; }
        unsigned sound() const { return sound_; }

    private:
        ReplayState initial_;
        bool active_ = false;
        unsigned wall_tiles_ = 0, progress_ = 0, sound_ = 0;
        double next_step_ = 0;
        unsigned wall_setup_ticks_ = 0;
        std::vector<std::array<unsigned, 4> > hands_;
        std::vector<unsigned> sounds_;

        struct DealPacket {
            unsigned step, who, begin, end, wall;
        };

        std::vector<DealPacket> packets_;
        std::array<std::array<TileFlight, 14>, 4> deal_flights_{};
        std::array<std::array<unsigned, 14>, 4> deal_starts_{};
        std::vector<AnimatedTile> sort_tiles_;
        unsigned sort_start_ = 0;
    };

    // Original push-node discard wait and opponent draw wait, on the 15ms clock.
    unsigned ActionPauseTicks(const ReplayEvent &event);

    // WC_ITEM_COUNT automatic flag submits on its first 1000ms WM_TIMER.
    constexpr unsigned kAutomaticDiscardWaitMs = 1000;
    // Event 132 changes the same slot at all seats, with one SE_KACHI1 (104)
    // when any tile changes. Seat 0 retains the explicitly requested adaptation.
    unsigned DrawEndChangedSeats(const ReplayState &state, unsigned slot);

    unsigned DefaultSoundResource(unsigned id);

    const std::map<unsigned, std::string> &SoundKeys();
} // namespace tenhou
