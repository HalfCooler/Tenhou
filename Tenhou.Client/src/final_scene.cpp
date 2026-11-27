#include "final_scene.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tenhou {
    namespace {
        template<class T> void Key(std::string &key, const T &value) {
            key.append(reinterpret_cast<const char *>(&value), sizeof(value));
        }
        void Key(std::string &key, const Gdiplus::RectF &r) {
            Key(key, r.X); Key(key, r.Y); Key(key, r.Width); Key(key, r.Height);
        }
        Gdiplus::RectF Device(const TextRenderContext &c, const Gdiplus::RectF r) {
            const auto sx = c.native_layout_scale * c.viewport_scale_x;
            const auto sy = c.native_layout_scale * c.viewport_scale_y;
            return {static_cast<float>(r.X * sx), static_cast<float>(r.Y * sy),
                    static_cast<float>(r.Width * sx), static_cast<float>(r.Height * sy)};
        }
        void Validate(const Gdiplus::RectF r) {
            if (!std::isfinite(r.X) || !std::isfinite(r.Y) || !std::isfinite(r.Width) || !std::isfinite(r.Height) ||
                std::abs(r.X) > 1000000 || std::abs(r.Y) > 1000000 || r.Width < 0 || r.Height < 0 ||
                r.Width > 1000000 || r.Height > 1000000) throw std::runtime_error("Invalid final scene rectangle");
        }
    }
    void FinalScene::Text(SceneTextRun run, const std::optional<Gdiplus::RectF>& clip) {
        if (clip) Validate(*clip);
        commands_.push_back({std::move(run), {}, {}, {}, clip});
    }
    void FinalScene::Image(std::string key, const Gdiplus::RectF bounds,
                           std::function<void(Gdiplus::Graphics &)> draw, const std::optional<Gdiplus::RectF>& clip) {
        Validate(bounds);
        if (clip) Validate(*clip);
        if (key.empty() || !draw) throw std::runtime_error("Final image requires a revision key and draw callback");
        commands_.push_back({{}, std::move(key), bounds, std::move(draw), clip});
    }
    TextRenderContext FinalScene::Context(const int width, const int height, const int dw, const int dh) const {
        return TextRenderContext::Viewport(width, height,
            Gdiplus::RectF(0, 0, static_cast<float>(dw), static_cast<float>(dh)), sd_, dpi_);
    }
    TextRenderContext FinalScene::Clipped(TextRenderContext context, const Command &command)
    {
        if (command.clip) {
            auto clip = Device(context, *command.clip);
            clip.Intersect(*context.device_clip);
            context.device_clip = clip;
        }
        return context;
    }
    Gdiplus::RectF FinalScene::Bounds(const TextRenderContext &context, const Command &command)
    {
        auto bounds = command.image ? Device(context, command.bounds) : TextRenderer::Bounds(context, command.text);
        if (command.image) bounds.Inflate(2, 2); // Antialias/filter footprint.
        bounds.Intersect(*context.device_clip);
        return bounds;
    }
    std::vector<FinalSceneStamp> FinalScene::Snapshot(const int width, const int height, const int dw, const int dh) const {
        const auto viewport = Context(width, height, dw, dh);
        std::vector<FinalSceneStamp> result;
        result.reserve(commands_.size());
        for (const auto &command : commands_) {
            const auto c = Clipped(viewport, command);
            const auto bounds = Bounds(c, command);
            std::string key;
            Key(key, sd_); Key(key, dpi_); Key(key, width); Key(key, height); Key(key, dw); Key(key, dh);
            Key(key, *c.device_clip);
            Key(key, static_cast<bool>(command.image));
            if (command.image) key += command.key;
            else {
                const auto &run = command.text;
                Key(key, run.anchor.X); Key(key, run.anchor.Y); Key(key, run.alignment); Key(key, run.opacity);
                for (const auto value : run.transform) Key(key, value);
            }
            RECT rect{};
            if (!bounds.IsEmptyArea()) rect = {static_cast<LONG>(std::floor(bounds.X)), static_cast<LONG>(std::floor(bounds.Y)),
                static_cast<LONG>(std::ceil(bounds.GetRight())), static_cast<LONG>(std::ceil(bounds.GetBottom()))};
            result.push_back({std::move(key), rect, command.text.layout.data});
        }
        return result;
    }
    void FinalScene::VisitText(
        const int width, const int height, const int dw, const int dh,
        const std::function<void(TextRenderer &, const TextRenderContext &, const SceneTextRun &)> &draw) const {
        const auto viewport = Context(width, height, dw, dh);
        for (const auto &command : commands_) {
            if (command.image) throw std::logic_error("Text visitor requires a text-only scene");
            const auto context = Clipped(viewport, command);
            if (!Bounds(context, command).IsEmptyArea()) draw(renderer_, context, command.text);
        }
    }

    void FinalScene::Paint(const HDC target, const int width, const int height, const int dw, const int dh, const RECT dirty) const {
        const auto viewport = Context(width, height, dw, dh);
        for (const auto &command : commands_) {
            const auto c = Clipped(viewport, command);
            auto bounds = Bounds(c, command);
            bounds.Intersect(Gdiplus::RectF(static_cast<float>(dirty.left), static_cast<float>(dirty.top),
                static_cast<float>(dirty.right - dirty.left), static_cast<float>(dirty.bottom - dirty.top)));
            if (bounds.IsEmptyArea()) continue;
            Gdiplus::Graphics graphics(target);
            graphics.SetPageUnit(Gdiplus::UnitPixel);
            graphics.SetClip(Gdiplus::Rect(dirty.left, dirty.top, dirty.right - dirty.left, dirty.bottom - dirty.top));
            graphics.SetClip(*c.device_clip, Gdiplus::CombineModeIntersect);
            if (command.image) {
                Gdiplus::Matrix matrix(static_cast<float>(c.native_layout_scale * c.viewport_scale_x), 0, 0,
                    static_cast<float>(c.native_layout_scale * c.viewport_scale_y), 0, 0);
                graphics.SetTransform(&matrix);
                command.image(graphics);
            } else renderer_.Draw(graphics, c, command.text);
            graphics.Flush(Gdiplus::FlushIntentionSync);
        }
    }
}
