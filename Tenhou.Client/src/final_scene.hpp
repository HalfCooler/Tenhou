#pragma once
#include "text_renderer.hpp"
#include <functional>

namespace tenhou {
    struct FinalSceneStamp {
        std::string key;
        RECT bounds{}; // Destination-local physical pixels, including effects and clip.
        // Keep the identity alive until the next frame; allocator reuse cannot hide a change.
        std::shared_ptr<const TextLayoutData> layout;
    };

    // Ordered commands AFTER the native base is scaled. Coordinates and per-command
    // clips are HD design pixels. Images may be interleaved with text, preserving
    // occlusion (e.g. a plate, text, then a tile/count graphic).
    // The renderer and any image resources must outlive Paint; snapshots own no callbacks.
    class FinalScene {
    public:
        explicit FinalScene(TextRenderer &renderer, bool sd = false, unsigned dpi = 96)
            : renderer_(renderer), sd_(sd), dpi_(dpi) {}
        void Text(SceneTextRun run, std::optional<Gdiplus::RectF> clip = {});
        // key must identify ALL image/animation changes, including opacity and transform.
        // bounds conservatively encloses everything the callback paints, in HD pixels.
        void Image(std::string key, Gdiplus::RectF bounds, std::function<void(Gdiplus::Graphics &)> draw,
                   std::optional<Gdiplus::RectF> clip = {});
        std::vector<FinalSceneStamp> Snapshot(int width, int height, int dw, int dh) const;
        void Paint(HDC target, int width, int height, int dw, int dh, RECT dirty) const;
    private:
        struct Command {
            SceneTextRun text;
            std::string key;
            Gdiplus::RectF bounds;
            std::function<void(Gdiplus::Graphics &)> image;
            std::optional<Gdiplus::RectF> clip;
        };
        TextRenderContext Context(int width, int height, int dw, int dh) const;
        TextRenderContext Clipped(TextRenderContext context, const Command &command) const;
        Gdiplus::RectF Bounds(const TextRenderContext &context, const Command &command) const;
        TextRenderer &renderer_;
        bool sd_;
        unsigned dpi_;
        std::vector<Command> commands_;
    };
}
