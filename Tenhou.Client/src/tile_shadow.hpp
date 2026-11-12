#pragma once
#include "table_scene.hpp"
#include "retained_scene.hpp"
#include <objidl.h>
#include <gdiplus.h>

namespace tenhou {
    struct ShadowKey {
        char family = 't';
        int slot = 0, kind = 0, direction = 0;

        bool operator==(const ShadowKey &b) const {
            return family == b.family && slot == b.slot && kind == b.kind && direction == b.direction;
        }
    };

    struct ShadowMask {
        int x = 0, y = 0, width = 0, height = 0, stride = 0;
        Bytes pixels;
    };

    class ShadowPack {
    public:
        explicit ShadowPack(Bytes raw);

        static const ShadowPack &Embedded();

        const ShadowMask *Get(ShadowKey key) const;

    private:
        Bytes raw_;
        std::map<unsigned, ShadowMask> masks_;
    };

    std::array<unsigned, 258> OriginalShadowPalette(int strength);

    // An entry represents one original placeholder, even when the face resource
    // is routed to n or the wall draws an additional lower-back sprite.
    struct TileShadow {
        PositionedTile placed;
        char family = 't';
        int slot = 0, code = 0xfe;
        int dx = 0, dy = 0;
        const ShadowMask *ground = nullptr;
        const ShadowMask *extra = nullptr;
        const ShadowMask *received = nullptr;
    };

    std::vector<TileShadow> BuildTileShadows(const std::vector<PositionedTile> &scene, TableArtwork &artwork,
                                             const ShadowPack &pack);

    Image ComposeTileShadows(const std::vector<TileShadow> &scene, RECT region, int strength);

    Image ShadeTile(const Image &image, int x, int y, const ShadowMask &mask, int strength);

    void PaintTileShadows(Gdiplus::Graphics &graphics, const std::vector<TileShadow> &scene, RECT region, int strength);

    class TileShadowLayer {
    public:
        void Paint(Gdiplus::Graphics &graphics, const std::vector<TileShadow> &scene, int strength);

    private:
        std::vector<TileShadow> previous_;
        Image pixels_;
        int strength_ = -1;
    };

    std::vector<RetainedScene::Damage> TileShadowDamage(const std::vector<PositionedTile> &scene,
                                                        const std::vector<TileShadow> &shadows, TableArtwork &artwork,
                                                        bool red);

    const TileShadow *FindTileShadow(const std::vector<TileShadow> &shadows, const PositionedTile &tile);
} // namespace tenhou
