#pragma once
#include "animation_clock.hpp"
#include "hand_projection.hpp"
#include "online_game.hpp"
#include "replay_effects.hpp"
#include "table_canvas_renderer.hpp"
#include <windows.h>
#include <memory>

namespace tenhou {
    class ActionCaptions;

    class OnlineTable {
    public:
        OnlineTable();
        explicit OnlineTable(std::unique_ptr<ITableCanvasRenderer> renderer);

        void SetRenderer(std::unique_ptr<ITableCanvasRenderer> renderer);
        ITableCanvasRenderer &Renderer() { return *renderer_; }
        const ITableCanvasRenderer &Renderer() const { return *renderer_; }

        std::size_t HudCacheHits() const { return renderer_->HudCacheHits(); }
        void SetReferenceRendering(bool enabled) { renderer_->SetReferenceRendering(enabled); }

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

        void SetDrawEndSlots(unsigned slots) { hand_.SetDrawEndSlots(slots); }
        void PresentSettlement() { settlement_presented_ = true; }

        void Configure(const std::filesystem::path &directory, int width);

        void SetPreferences(const ClientPreferences &preferences, const std::filesystem::path &config,
                            bool rebuild = true);

        void Prepare();

        void BeginOpening(OpeningPresentation &opening, const OnlineState &state, bool wall) {
            Prepare();
            auto &geometry = renderer_->EnsureGeometry(state.red_tiles);
            opening.Begin(OnlineTableState(state), wall, &geometry, (state.type & 2) == 0);
        }

        void Paint(HDC dc, RECT bounds, const OnlineState &state, bool interactive = false,
                   const OpeningPresentation *opening = nullptr, const TableInteraction *interaction = nullptr,
                   double now = AnimationNow(), const ActionCaptions *captions = nullptr, unsigned dpi = 0,
                   const std::vector<TableActionButton> *buttons = nullptr, HWND window = nullptr);

        int HitTile(RECT bounds, const OnlineState &state, int x, int y);

        void SetOriginalDisplay(bool enabled) { original_display_ = enabled; }

        void SetAutoSort(bool enabled, const OnlineState &state);

        bool MoveHandTile(int tile, int target, const OnlineState &state);

        void SetRelativeScores(bool relative) { relative_scores_ = relative; }

    private:
        ReplayState Project(const OnlineState &state) { return hand_.Project(state); }

        HandProjection hand_;
        std::unique_ptr<ITableCanvasRenderer> renderer_;
        int width_ = 1440;
        bool original_display_ = true;
        bool relative_scores_ = false;
        TablePresentation presentation_;
        ReplayState previous_;
        bool settlement_presented_ = false;
        double paused_at_ = 0;
    };
} // namespace tenhou
