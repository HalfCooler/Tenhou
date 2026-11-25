#pragma once
#include "replay.hpp"
#include "resource_pack.hpp"
#include "variant_artwork.hpp"
#include "final_scene.hpp"
#include <windows.h>
#include <array>
#include <map>
#include <memory>
#include <tuple>

namespace Gdiplus {
    class Graphics;
    class GraphicsPath;
}

namespace tenhou {
    class TableArtwork;

    // Display only. No model mutation, protocol requests or hidden tile inspection.
    struct DisplaySprite {
        unsigned id = 0;
        int x = 0, y = 0, seat = -1;
        bool center_x = false, center_y = false;
    };

    struct DisplayText {
        TextKey key;
        int x = 0, y = 0, seat = -1;
        bool center_x = false, center_y = false;
    };

    struct OriginalHudLayout {
        float scale = 1;
        std::uint32_t text_color = 0xff000000, outline_color = 0x44ffffff;
        float outline_width = 2;
        std::array<float, 2> center{};
        std::array<int, 4> panel{}; // Original sprite 589: x, y, width, height.
        std::array<std::array < float, 6>
        ,
        4
        >
        seats {
        };
        std::array<std::array < float, 6>
        ,
        4
        >
        sticks {
        };
        std::vector<DisplaySprite> sprites;
        std::vector<DisplayText> text;
    };

    OriginalHudLayout BuildOriginalHud(const ReplayState &state, int width);

    RECT FitNativeCanvas(RECT bounds, int width);

    RECT OriginalAutoButtonRect(int width, int index);
    // FUN_0049c900 (0049c900..0049ca75): top-right table info rows before オーラス.
    std::vector<TextKey> OriginalInfoKeys(unsigned type, bool variant = false);

    // 004aa250: n/0/254 maps to the standing-back rectangle, then odd
    // directions swap measured text extents before the count window is placed.
    struct NukiCountLayout {
        RECT bounds{};
        POINT text_origin{};
    };

    NukiCountLayout OriginalNukiCountLayout(RECT anchor, int seat, int text_width, int text_height);

    // FUN_0047a466 / timer 0x67: enter -> relative, leave -> 750 ms grace.
    class HudInteraction {
    public:
        bool Handle(HWND window, RECT board, int width, UINT message, LPARAM point);

        bool relative() const { return relative_; }

    private:
        bool relative_ = false, inside_ = false;
    };

    class DisplayArtwork {
    public:
        void SetPanelEnabled(bool enabled) { panel_enabled_ = enabled; }

        void Configure(const std::filesystem::path &directory, int width);

        const Image &Get(unsigned id);
        // Returns the high-resolution replacement sprite (images/sprites/hd/{id}.png)
        // when present; otherwise falls back to the classic sprite Get(id).
        const Image &GetHd(unsigned id);

        static float MeasureText(const std::string &text, float em);

        static std::array<float, 2> MeasureTextExtent(const std::string &text, float em);

        using HudSprite = std::function<void(unsigned, const Image &, int, int, const std::array<float, 6> &)>;
        void PaintHudImages(Gdiplus::Graphics &graphics, const ReplayState &state, const HudSprite &draw_sprite = {},
                            float scale_x = 1.0f, float scale_y = 1.0f);

        void PaintChromeBackground(Gdiplus::Graphics &graphics, int hovered = -1);


        TextRenderer &Text();
        // Native anchors are converted once to the HD design space used by FinalScene.
        SceneTextRun TextRun(TextKey key, float x, float y, bool cx = false, bool cy = false);
        void AppendHud(FinalScene &scene, const ReplayState &state);
        void AppendChrome(FinalScene &scene, unsigned type,
                          const std::array<bool, 4> &automatic = {true, false, false, false}, bool variant = false);
        void AppendStatus(FinalScene &scene, const ReplayState &state);

        void PaintNukiCounts(Gdiplus::Graphics &graphics, const ReplayState &state, TableArtwork &tiles,
                             float scale_x = 1.0f, float scale_y = 1.0f);

        void Sprite(Gdiplus::Graphics &graphics, unsigned id, int x, int y, bool cx = false, bool cy = false);

    private:
        bool panel_enabled_ = true;

        const Image *FindSprite(unsigned id);

        void Caption(Gdiplus::Graphics &graphics, const std::wstring &text, float x, float y,
                     float em, const OriginalHudLayout &hud, float width = 0);

        using CaptionKey = std::tuple<std::wstring, float, float, float, float>;
        std::map<CaptionKey, std::shared_ptr<Gdiplus::GraphicsPath> > captions_;
        std::filesystem::path directory_;
        int width_ = 1440;
        std::shared_ptr<const ResourcePack> pack_;
        VariantArtwork variants_;
        std::map<unsigned, Image> images_;
        std::shared_ptr<const TextFontService> text_font_;
        std::unique_ptr<TextRenderer> text_renderer_;
    };
} // namespace tenhou
