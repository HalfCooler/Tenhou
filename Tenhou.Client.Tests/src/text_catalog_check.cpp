#include "table_render_frame.hpp"
#include "text_catalog.hpp"
#include "action_captions.hpp"
#include "settlement_scene.hpp"
#include "sprite_assets.hpp"
#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

namespace {
void Check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    try {
        using namespace tenhou;
        Check(std::size(kTextDefinitions) == 296 && std::size(kTextImageDefinitions) == 7, "inventory totals");
        std::set<unsigned> ids;
        std::size_t info_count = 0;
        for (const auto &text : kTextDefinitions) {
            Check(ids.insert(text.origin_id).second, "duplicate text OriginId");
            Check(!text.label.empty() && FindText(text.key) == &text, "semantic key lookup");
            Check(FindTextByOrigin(text.origin_id) == &text, "legacy adapter lookup");
            const auto *style = FindTextStyle(text.style);
            Check(style && style->font == L"HGSeikaishotaiPRO" && style->size_pt > 0, "font/style reference");
            Check(!FindTextImageByOrigin(text.origin_id), "text in image inventory");
            if (text.category == TextCategory::Info) {
                ++info_count;
                Check(text.style == TextStyleKey::info && style->size_pt == 16 && style->bold &&
                    style->alignment == TextAlignment::right && style->decoration == TextDecoration::info_gradient,
                    "Info shared inherited style");
            }
        }
        const std::set<unsigned> expected_images{401, 402, 440, 466, 589, 591, 592};
        std::set<unsigned> image_ids;
        for (const auto &image : kTextImageDefinitions) {
            Check(image_ids.insert(image.origin_id).second && ids.insert(image.origin_id).second, "duplicate image ID");
            Check(!FindTextByOrigin(image.origin_id), "image can resolve as text");
        }
        Check(image_ids == expected_images && info_count == 25, "Image/Info classifications");
        std::set<unsigned> sprite_ids;
        for (const auto &sprite : kSpriteAssets)
            Check(sprite_ids.insert(sprite.id).second, "duplicate runtime sprite ID");
        Check(sprite_ids == image_ids, "runtime sprite inventory differs from Image definitions");
        for (unsigned id = 401; id <= 705; ++id)
            Check(ids.count(id) == (id == 434 || id == 593 ? 0u : 1u), "complete original inventory");
        Check(!FindText(TextKey::Count) && !FindTextStyle(TextStyleKey::Count), "invalid enum lookup");
        Check(FindTextByOrigin(481)->label == L"門前清自摸和" &&
            FindTextByOrigin(481)->style == TextStyleKey::yaku &&
            FindTextStyle(FindTextByOrigin(481)->style)->size_pt == 24 &&
            !FindTextStyle(FindTextByOrigin(481)->style)->bold &&
            !FindTextStyle(FindTextByOrigin(481)->style)->shadow, "481 ordinary yaku style correction");
        Check(FindTextByOrigin(529)->label == L"国士無双13面" && FindTextByOrigin(607)->label == L"竜七対" &&
            FindTextByOrigin(595)->label == L"交換" && FindTextByOrigin(594)->label == L"カンリョウ" &&
            FindTextByOrigin(705)->label == L"国標", "exact specification glyphs");
        Check(FindTextByOrigin(459)->composition == TextComposition::chi_selection &&
            kChiSelection.main == L"チー" && kChiSelection.secondary == L"（選択）" &&
            kChiSelection.secondary_size_pt == 12, "chi selection secondary text is 12 pt");
        Check(FindTextByOrigin(424)->label == FindTextByOrigin(429)->label &&
            FindTextByOrigin(424)->label_key == FindTextByOrigin(429)->label_key &&
            FindTextByOrigin(441)->label_key == FindTextByOrigin(450)->label_key &&
            FindTextByOrigin(424)->style == TextStyleKey::option_checked &&
            FindTextByOrigin(429)->style == TextStyleKey::option_unchecked, "checked versus hover states");
        Check(FindTextStyle(TextStyleKey::option_checked)->spacing_pt == -0.5 &&
            FindTextStyle(TextStyleKey::button_orange_outline)->spacing_pt == 1, "spacing in points");
        Check(FindText(TextKey::button_hover_exchange)->origin_id == 595 &&
            FindText(TextKey::button_normal_exchange)->origin_id == 599, "variant button state order");
        for (unsigned control : {5121u, 5122u, 5123u, 5124u, 5125u, 5126u, 5127u, 5128u, 5151u, 5170u, 5171u, 5172u, 5173u}) {
            for (bool hot : {false, true}) {
                const auto key = ActionButtonText(control, hot);
                const auto *definition = key ? FindText(*key) : nullptr;
                const auto *style = definition ? FindTextStyle(definition->style) : nullptr;
                Check(style && style->outline == (hot ? TextColor::orange : TextColor::black),
                    "production button state must select black normal / orange hover");
            }
        }
        Check(ActionButtonText(5173, true) == TextKey::button_hover_exchange &&
            ActionButtonText(5173, false) == TextKey::button_normal_exchange, "exchange button mapping");
        for (unsigned control = 5170; control <= 5172; ++control) {
            const auto normal = ActionButtonText(control, false), hover = ActionButtonText(control, true);
            Check(normal && hover && FindText(*normal)->origin_id == 600 + control - 5170 &&
                FindText(*hover)->origin_id == 596 + control - 5170, "missing-suit button mapping");
        }
        Check(!ActionButtonText(0, false), "unknown button control");
        Check(std::size(kInfoBodyGradient) == 3 && std::size(kInfoPenultimateGradient) == 6 &&
            std::size(kInfoLastGradient) == 3 && kInfoPenultimateGradient[2].position == 0.30 &&
            kInfoPenultimateGradient[2].gray == 239 && kInfoLastGradient[1].gray == 156 &&
            kInfoLastGradient[2].gray == 134 && kInfoWidthBaseAdvances == 6.5 && !kInfoBackgroundAlpha,
            "independent Info gradient decorations");
        std::set<unsigned> pattern_ids;
        for (const auto &pattern : kTextRulePatterns) {
            const auto *text = FindText(pattern.text);
            Check(text && text->category == TextCategory::Yaku && pattern_ids.insert(text->origin_id).second,
                "each variant pattern maps to unique yaku text");
            Check(pattern.rule == (text->origin_id <= 621 ? "sichuan" : "guobiao"), "variant pattern rule family");
            const auto display = ResolveSettlementEntry({std::string(pattern.server_name)}, pattern.rule);
            Check(display.pattern == &pattern && display.name == TextUtf8(pattern.text) && display.total_value == pattern.default_value,
                "variant name and default resolve directly from the text catalog");
        }
        Check(pattern_ids.size() == 101 && *pattern_ids.begin() == 603 && *pattern_ids.rbegin() == 703,
            "all variant pattern labels are mapped");
        for (int id = 0; id < 55; ++id)
            Check(SettlementYakuName(id) == TextUtf8(FindTextByOrigin(481 + id)->key), "all riichi fallback labels");
        Check(SettlementYakuName(8) == "断幺九" && SettlementYakuName(48) == "国士無双13面" &&
            SettlementYakuName(999) == "Unknown yaku #999", "updated glyphs and unknown yaku fallback");
        Check(SettlementEntryLabel({"龙七对×3"}, "sichuan-bloodbattle") == "竜七対×3" &&
            SettlementEntryLabel({"未知番型", 0, 2}, "guobiao") == "未知番型×2", "known/unknown names and counts");
        SettlementState state;
        SettlementReceipt receipt;
        receipt.generic = true; receipt.who = receipt.from = 0; receipt.winning_tile = 0; receipt.rule = "sichuan";
        receipt.entries = {{"龙七对"}, {"根", 0, 3}, {"未知番型", 0, 2}};
        state.receipts.push_back(receipt);
        const auto before = SettlementJson(state);
        const auto scene = BuildSettlementScene(state, 0, {});
        std::vector<std::string> labels;
        for (const auto &item : scene.text)
            if (item.role == "variant_pattern" || item.role == "variant_multiplier") labels.push_back(item.text);
        Check(labels == std::vector<std::string>{"竜七対", "根×3", "未知番型", "×2"},
            "known labels, unknown names and multipliers are preserved");
        Check(SettlementText(state, 0, {}).find("竜七対") != std::string::npos && before == SettlementJson(state),
            "text details use display names without modifying server receipts");
        ActionCaptions captions;
        captions.AcceptResource(594, 0, L"选择完毕");
        Check(captions.Frames().front().text == TextLabel(TextKey::caption_complete), "completion caption fallback");
        const auto *dragon = FindTextRulePattern("sichuan_blood", "龙七对");
        Check(dragon && FindText(dragon->text)->label == L"竜七対", "rule name is distinct from display label");
        Check(!FindTextRulePattern("sichuan", "竜七対") && !FindTextRulePattern("guobiao", "unknown"),
            "unknown names remain available for caller fallback");
        std::cout << "Text catalog: 296 Text, 7 Image, 101 rule mappings checked\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "Text catalog check: " << error.what() << '\n';
        return 1;
    }
}
