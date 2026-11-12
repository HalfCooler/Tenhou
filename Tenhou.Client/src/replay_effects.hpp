#pragma once
#include "table_scene.hpp"
#include <algorithm>
#include <functional>

namespace tenhou {
    // Presentation only: the ReplaySession remains at the applied event's final state.
    struct AnimatedTile {
        TableTile tile;
        int from_x = 0, from_y = 0, to_x = 0, to_y = 0;
        unsigned delay = 0;
    };

    struct ReplayTransition {
        unsigned duration = 0;
        bool manual = false;
        bool nuki = false; // Native 40-pixel steps on the existing 60 Hz display clock.
        std::vector<AnimatedTile> tiles;

        std::vector<PositionedTile> Frame(double elapsed) const;
    };

    using TileOrigin = std::function<std::pair<int, int>(const TableTile &)>;

    ReplayTransition BuildReplayTransition(const ReplayState &before, const ReplayState &after,
                                           const ReplayEvent &event, unsigned duration, int canvas_width,
                                           const TileOrigin &origin, bool sort_self = true);

    // A single presentation slot: a new accepted event replaces the old movement.
    // Caller-owned authoritative states and clocks are never advanced by this class.
    class TablePresentation {
    public:
        void Begin(const ReplayState &before, const ReplayState &after, const ReplayEvent &event,
                   unsigned duration, int width, const TileOrigin &origin, double now, bool sort_self = true);

        void BeginManual(const ReplayState &before, const ReplayState &after, int width, const TileOrigin &origin,
                         double now);

        bool Manual() const { return Active() && transition_.manual; }
        void Cancel() { transition_ = {}; }
        bool Active() const { return transition_.duration != 0; }

        bool Tick(double now);

        void ShiftTime(double delay) { started_ += delay; }

        std::vector<PositionedTile> Frame(double now) const;

    private:
        ReplayTransition transition_;
        double started_ = 0;
    };

    // Unknown gender keeps the original neutral resource; never infer it from names.
    std::vector<unsigned> ReplaySounds(const ReplayEvent &event, char gender = '?', bool effects_only = false,
                                       bool individual_voices = false);

    // Wall preparation retains the earlier user-authorized simultaneous sides.
    // Deal packets follow FUN_00489c40: dealer first, 4 x 3 then 1, wait 6 ticks.
    class OpeningPresentation {
    public:
        void Begin(const ReplayState &state, bool wall, TableArtwork *artwork = nullptr);

        bool Tick(unsigned steps = 1);

        bool Active() const { return active_; }
        void Cancel() { active_ = false; }

        ReplayState Frame() const;

        std::vector<PositionedTile> Scene(TableArtwork &artwork, bool red = true) const;

        unsigned wall_tiles() const { return wall_tiles_; }
        unsigned placed() const { return std::min(progress_, wall_tiles_ / 4) * 4; }

        unsigned dealt() const;

        unsigned steps() const { return progress_; }
        unsigned interval() const { return progress_ < wall_tiles_ / 4 ? 25 : 15; }
        unsigned sound() const { return sound_; }

    private:
        ReplayState initial_;
        bool active_ = false;
        unsigned wall_tiles_ = 0, progress_ = 0, sound_ = 0;
        std::vector<std::array<unsigned, 4> > hands_;
        std::vector<unsigned> sounds_;

        struct DealPacket {
            unsigned step, who, begin, end, wall;
        };

        std::vector<DealPacket> packets_;
        std::vector<AnimatedTile> sort_tiles_;
        unsigned deal_steps_ = 0, sort_steps_ = 0;
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
