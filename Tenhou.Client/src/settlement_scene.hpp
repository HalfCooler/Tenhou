#pragma once
#include "settlement.hpp"
#include "table_scene.hpp"
#include "text_renderer.hpp"
#include <windows.h>
#include <limits>
#include <functional>

namespace tenhou {
    struct SettlementTextSpan {
        std::string text;
        TextKey key = TextKey::Count;
        TextStyleKey style = TextStyleKey::yaku;
    };

    struct SettlementTextItem {
        std::string role, text;
        RECT rect{};
        float size = 24;
        unsigned color = 0xffffffff;
        bool visible = true;
        int align = 0;
        bool original_font = false;
        std::vector<SettlementTextSpan> spans{};
        double entrance = 1;
    };

    struct SettlementTileItem {
        std::string role;
        TableTile tile;
        RECT rect{};
        bool sideways = false, native_orientation = false;
    };

    struct SettlementImageItem {
        unsigned id = 0;
        int x = 0, y = 0;
        double scale = 1;
    };

    struct SettlementLayout {
        int width = 1024, dealer = 0, honba = -1, sticks = -1;
        std::size_t pattern_page = 0;
        double yaku_entrance = 1;
        // Resource geometry is read by the view/probe, without exposing live hands.
        std::function<RECT(const TableTile &)> tile_bounds;
        std::function<float(const std::string &, float)> text_width;

        std::function<std::array<float, 2>(const std::string &, float)> text_extent;
    };

    struct SettlementScene {
        int width = 1024, height = 768;
        RECT panel{182, 64, 842, 704}, confirm{489, 654, 545, 676};
        std::vector<RECT> score_panels;
        std::vector<SettlementTextItem> text;
        std::vector<SettlementTileItem> tiles;
        std::vector<SettlementImageItem> images;
    };

    // Native 1024/1440 canvas. Text spans are laid out in HD design pixels.
    SettlementScene BuildSettlementScene(const SettlementState &state, std::size_t selected,
                                         const std::array<std::string, 4> &names,
                                         std::size_t shown_yaku = std::numeric_limits<std::size_t>::max(),
                                         std::size_t shown_tiles = std::numeric_limits<std::size_t>::max(),
                                         bool ura = true, bool totals = true,
                                         const SettlementLayout &layout = {});
    RECT SettlementConfirmRect(const SettlementState &state, std::size_t selected, int width);

    std::string SettlementSceneJson(const SettlementScene &scene);
    std::vector<SceneTextRun> LayoutSettlementText(TextRenderer &renderer, const SettlementTextItem &item, bool sd);

    // Perspective sprites overlap. Paint the outside edges before the center.
    std::vector<std::size_t> SettlementPaintOrder(const SettlementScene &scene);
} // namespace tenhou
