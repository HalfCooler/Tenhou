#pragma once
#include "action_caption_state.hpp"
#include "original_display.hpp"
#include <objidl.h>
#include <gdiplus.h>

namespace tenhou {
    inline void AppendActionCaptions(FinalScene &scene, const ActionCaptions &captions, int width,
                                     TextRenderer &renderer) {
        // Literal original anchors, expressed once in HD design coordinates.
        constexpr int xs[2][4]{{512, 844, 512, 180}, {720, 1190, 720, 250}};
        constexpr int ys[2][4]{{590, 340, 90, 340}, {830, 480, 120, 480}};
        const bool hd_layout = width != 1024;
        const float hd = hd_layout ? 1.f : 1.4f;
        for (const auto &item: captions.Frames()) {
            if (item.alpha <= 0) continue;
            const Gdiplus::PointF center(xs[hd_layout][item.who] * hd, ys[hd_layout][item.who] * hd);
            if (item.key) scene.Text(renderer.Caption(*item.key, center, item.scale, item.alpha));
            else if (!item.text.empty()) {
                auto layout = renderer.Layout(item.text, TextStyleKey::action_caption);
                const auto &ink = layout.metrics.ink;
                SceneTextRun run;
                run.layout = layout;
                run.anchor = center;
                run.opacity = item.alpha;
                run.transform = {
                    item.scale, 0, 0, item.scale,
                    -(ink.X + ink.Width / 2) * item.scale, -(ink.Y + ink.Height / 2) * item.scale
                };
                scene.Text(run);
            }
        }
    }
} // namespace tenhou
