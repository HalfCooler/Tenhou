#include "online_table.hpp"
#include "d2d_table_renderer.hpp"
#include <stdexcept>
#include <utility>

namespace tenhou {
    OnlineTable::OnlineTable() : renderer_(std::make_unique<D2DTableRenderer>()) {}

    OnlineTable::OnlineTable(std::unique_ptr<ITableCanvasRenderer> renderer)
        : renderer_(renderer ? std::move(renderer) : std::make_unique<D2DTableRenderer>()) {}

    void OnlineTable::SetRenderer(std::unique_ptr<ITableCanvasRenderer> renderer) {
        if (renderer) renderer_ = std::move(renderer);
    }

    void OnlineTable::SetPreferences(const ClientPreferences &preferences, const std::filesystem::path &config,
                                     const bool rebuild) const
    {
        renderer_->SetPreferences(preferences, config, rebuild);
    }

    POINT OnlineTable::ActionAnchor(const OnlineState &online) {
        const auto &geometry = renderer_->EnsureGeometry(online.red_tiles);
        const auto anchor = hand_.ActionAnchor(online, geometry);
        return {static_cast<LONG>(anchor.first), static_cast<LONG>(anchor.second)};
    }

    void OnlineTable::Configure(const std::filesystem::path &directory, const int width) {
        CancelAnimation();
        width_ = width;
        hand_.ClearRaised();
        renderer_->Configure(directory, width);
    }

    void OnlineTable::Prepare() const
    {
        renderer_->Prepare();
    }

    void OnlineTable::Accept(const OnlineState &online, const ReplayEvent &event, const bool animate, const double now) {
        auto after = Project(online);
        auto before = previous_;
        if (event.kind == ReplayEventKind::Init || after.round != before.round || after.settlement.receipts.empty())
            settlement_presented_ = false;
        previous_ = after; // Retain only the public projection, even if artwork fails.
        if (event.kind == ReplayEventKind::Metadata && event.tag != "VARIANT")
            return; // Identity refresh keeps a running move.
        CancelAnimation();
        if (!animate || !before.initialized || event.kind == ReplayEventKind::Init) return;
        const auto &geometry = renderer_->EnsureGeometry(online.red_tiles);
        const bool red = (online.type & 2) == 0;
        presentation_.Begin(before, after, event, 240, width_,
                            [&](const TableTile &t) { return geometry.Origin(t, red); },
                            now, false, &geometry, red);
    }

    bool OnlineTable::MoveHandTile(const int tile, const int target, const OnlineState &online) {
        ReplayState before, after;
        if (!hand_.MoveHandTile(tile, target, online, &before, &after)) return false;
        previous_ = after;
        if (renderer_->HasLoadedAssets()) {
            const auto &geometry = renderer_->EnsureGeometry(online.red_tiles);
            const bool red = (online.type & 2) == 0;
            presentation_.BeginManual(before, previous_, width_,
                                      [&](const TableTile &t) { return geometry.Origin(t, red); },
                                      AnimationNow());
        }
        return true;
    }

    void OnlineTable::SetAutoSort(const bool enabled, const OnlineState &online) {
        ReplayState before, after;
        if (!hand_.SetAutoSort(enabled, online, &before, &after)) return;
        previous_ = after;
        CancelAnimation();
        if (!enabled || !renderer_->HasLoadedAssets()) return;
        const auto &geometry = renderer_->EnsureGeometry(online.red_tiles);
        ReplayEvent event;
        event.tag = "SORT";
        const bool red = (online.type & 2) == 0;
        presentation_.Begin(before, after, event, 240, width_,
                            [&](const TableTile &tile) { return geometry.Origin(tile, red); },
                            AnimationNow(), false);
    }

    void OnlineTable::Paint(const HDC dc, const RECT bounds, const OnlineState &online, const bool interactive,
                            const OpeningPresentation *opening, const TableInteraction *interaction,
                            double now, const ActionCaptions *captions, const unsigned dpi,
                            const std::vector<TableActionButton> *buttons, const HWND window) {
        if (paused_at_) now = paused_at_;
        const auto &geometry = renderer_->EnsureGeometry(online.red_tiles);
        auto frame = BuildOnlineTableRenderFrame(
            width_, {bounds.left, bounds.top, bounds.right, bounds.bottom}, online, hand_, geometry,
            presentation_, opening, interaction, captions, settlement_presented_, relative_scores_,
            original_display_, interactive, now, dpi);
        if (buttons) frame.buttons = *buttons;
        if (!window || !renderer_->RenderToWindow(window, frame)) renderer_->RenderToDc(dc, frame);
    }

    bool OnlineTable::PaintToWindow(const HWND window, const RECT bounds, const OnlineState &online, const bool interactive,
                                    const OpeningPresentation *opening, const TableInteraction *interaction,
                                    double now, const ActionCaptions *captions, const unsigned dpi,
                                    const std::vector<TableActionButton> *buttons) {
        if (!window) return false;
        if (paused_at_) now = paused_at_;
        const auto &geometry = renderer_->EnsureGeometry(online.red_tiles);
        auto frame = BuildOnlineTableRenderFrame(
            width_, {bounds.left, bounds.top, bounds.right, bounds.bottom}, online, hand_, geometry,
            presentation_, opening, interaction, captions, settlement_presented_, relative_scores_,
            original_display_, interactive, now, dpi);
        if (buttons) frame.buttons = *buttons;
        return renderer_->RenderToWindow(window, frame);
    }

    int OnlineTable::HitTile(const RECT bounds, const OnlineState &online, const int x, const int y) {
        if (!renderer_->HasLoadedAssets() || !online.initialized) return -1;
        auto &geometry = renderer_->EnsureGeometry(online.red_tiles);
        std::vector<PositionedTile> animated;
        const std::vector<PositionedTile> *active = nullptr;
        if (presentation_.Active()) {
            animated = presentation_.Frame(AnimationNow());
            active = &animated;
        }
        return hand_.HitTile({bounds.left, bounds.top, bounds.right, bounds.bottom}, online,
                             geometry, x, y, active);
    }
} // namespace tenhou
