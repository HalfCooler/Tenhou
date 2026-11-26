#pragma once
#include "replay.hpp"
#include "text_catalog.hpp"
#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace tenhou {
    inline std::optional<TextKey> ActionCaptionText(const ReplayEvent &event) {
        if (event.kind == ReplayEventKind::Reach)
            return event.attributes.at("step") == "1" ? std::optional(TextKey::caption_riichi) : std::nullopt;
        if (event.kind == ReplayEventKind::Win)
            return event.attributes.at("who") == event.attributes.at("fromWho")
                       ? TextKey::caption_tsumo
                       : TextKey::caption_ron;
        if (event.kind != ReplayEventKind::Meld) return {};
        if (event.tag == "VARIANT") {
            const auto &kind = event.attributes.at("kind");
            return kind == "chi" ? TextKey::caption_chi : kind == "pon" ? TextKey::caption_pon : TextKey::caption_kan;
        }
        const auto encoded = static_cast<unsigned>(std::stoul(event.attributes.at("m")));
        if (encoded & 4) return TextKey::caption_chi;
        if (encoded & 0x18) return encoded & 8 ? TextKey::caption_pon : TextKey::caption_kan;
        if (encoded & 0x20) return TextKey::caption_kita;
        return TextKey::caption_kan;
    }

    inline unsigned ActionCaptionResource(const ReplayEvent &event) {
        const auto key = ActionCaptionText(event);
        return key ? FindText(*key)->origin_id : 0;
    }

    struct ActionCaptionFrame {
        unsigned resource = 0, tick = 0;
        int who = 0;
        float scale = 1, alpha = 1;
        std::string text;
        std::optional<TextKey> key;
    };

    // Pure C++ caption animation state (zero HWND, HDC, or GDI+ dependencies).
    class ActionCaptions {
    public:
        bool AcceptText(TextKey key, const int who) {
            const auto *definition = FindText(key);
            if (!definition || definition->category != TextCategory::Caption || who < 0 || who > 3) return false;
            ActionCaptionFrame frame;
            frame.resource = definition->origin_id;
            frame.who = who;
            frame.key = key;
            frame.text = std::string(definition->label);
            items_.push_back(std::move(frame));
            return true;
        }

        bool AcceptResource(const unsigned id, const int who, const std::string &text = "") {
            if (const auto *definition = FindTextByOrigin(id)) return AcceptText(definition->key, who);
            if (id || text.empty() || who < 0 || who > 3) return false;
            ActionCaptionFrame frame;
            frame.who = who;
            frame.text = text;
            items_.push_back(std::move(frame));
            return true;
        }

        bool Accept(const ReplayEvent &event) {
            if (event.kind == ReplayEventKind::Init) Clear();
            const auto key = ActionCaptionText(event);
            const int who = event.attributes.count("who") ? std::stoi(event.attributes.at("who")) : event.who;
            return key && AcceptText(*key, who);
        }

        void Clear() { items_.clear(); }
        [[nodiscard]] bool Active() const { return !items_.empty(); }

        // LAB_00491ac0 increments on the effect update, never on a repaint.
        void Tick() {
            for (auto &item: items_) ++item.tick;
            items_.erase(std::remove_if(items_.begin(), items_.end(), [](const ActionCaptionFrame &item) {
                return item.tick > 100;
            }), items_.end());
        }

        [[nodiscard]] std::vector<ActionCaptionFrame> Frames() const {
            auto result = items_;
            for (auto &item: result) {
                item.scale = item.tick < 20 ? 2.5f - static_cast<float>(item.tick) * .075f : 1.f;
                item.alpha = item.tick < 20 ? static_cast<float>(item.tick) * .05f : item.tick >= 60 ? static_cast<float>(100 - item.tick) * .025f : 1.f;
            }
            return result;
        }

    private:
        std::vector<ActionCaptionFrame> items_;
    };
} // namespace tenhou
