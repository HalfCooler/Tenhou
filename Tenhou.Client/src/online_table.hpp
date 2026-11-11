#pragma once
#include "background.hpp"
#include "online_game.hpp"
#include "table_scene.hpp"
#include "original_display.hpp"
#include "replay_effects.hpp"
#include "retained_scene.hpp"
#include "cached_hud.hpp"
#include "tile_shadow.hpp"
#include <windows.h>

namespace tenhou {
    struct TableInteraction {
        int hovered = -1;
        std::vector<int> playable;
        std::vector<int> selected;
        bool selecting = false;
        std::wstring countdown;
        std::array<bool, 4> automatic{true, false, false, false};
        int hovered_button = -1;
    };

    class OnlineTable {
    public:
        std::size_t HudCacheHits() const { return hud_cache_.hits(); }

        void SetReferenceRendering(bool enabled) {
            native_buffer_.SetReferenceRendering(enabled);
            reference_rendering_ = enabled;
        }

        POINT ActionAnchor(const OnlineState &online);

        void Accept(const OnlineState &online, const ReplayEvent &event, bool animate, double now = AnimationNow());

        void CancelAnimation() { presentation_.Cancel(); }
        bool Animating() const { return presentation_.Active(); }
        bool ManualAnimation() const { return presentation_.Manual(); }
        bool Tick() { return presentation_.Tick(AnimationNow()); }

        void SetPaused(bool paused) {
            if (paused && !paused_at_) paused_at_ = AnimationNow();
            else if (!paused && paused_at_) {
                presentation_.ShiftTime(AnimationNow() - paused_at_);
                paused_at_ = 0;
            }
        }

        void SetDrawEndSlots(unsigned slots) { draw_end_slots_ = slots; }

        void Configure(const std::filesystem::path &directory, int width);

        void SetPreferences(const ClientPreferences &preferences, const std::filesystem::path &config,
                            bool rebuild = true);

        // Move the first _obj pack read/background build out of the first visible
        // frame (called from the match-start confirmation window). Failures stay
        // lazy: Paint retries and surfaces the resource error as before.
        void Prepare();

        void BeginOpening(OpeningPresentation &opening, const OnlineState &state, bool wall) {
            Prepare();
            opening.Begin(OnlineTableState(state), wall, &artwork_);
        }

        void Paint(HDC dc, RECT bounds, const OnlineState &state, bool interactive = false,
                   const OpeningPresentation *opening = nullptr, const TableInteraction *interaction = nullptr,
                   double now = AnimationNow());

        int HitTile(RECT bounds, const OnlineState &state, int x, int y);

        void SetOriginalDisplay(bool enabled) { original_display_ = enabled; }

        void SetAutoSort(bool enabled, const OnlineState &state);

        bool MoveHandTile(int tile, int target, const OnlineState &state);

        void SetRelativeScores(bool relative) { relative_scores_ = relative; }

    private:
        ReplayState Project(const OnlineState &state);

        std::vector<int> hand_order_;
        std::uint64_t hand_events_ = 0;
        int reordered_draw_ = -1;
        std::filesystem::path directory_;
        int width_ = 1440;
        TableArtwork artwork_;
        Image background_;
        ClientPreferences preferences_;
        std::filesystem::path config_;
        RetainedScene native_buffer_;
        CachedHud hud_cache_;
        TileShadowLayer shadow_layer_;
        DisplayArtwork display_artwork_;
        bool original_display_ = true;
        bool auto_sort_ = true;
        bool relative_scores_ = false;
        bool reference_rendering_ = RetainedScene::Reference();
        TablePresentation presentation_;
        ReplayState previous_;
        unsigned draw_end_slots_ = 14;
        double paused_at_ = 0;
    };
} // namespace tenhou
