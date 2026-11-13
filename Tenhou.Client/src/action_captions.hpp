#pragma once
#include "original_display.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>

namespace tenhou {
    inline unsigned ActionCaptionResource(const ReplayEvent &event) {
        if (event.kind == ReplayEventKind::Reach)
            return event.attributes.at("step") == "1" ? 478 : 0;
        if (event.kind == ReplayEventKind::Win)
            return event.attributes.at("who") == event.attributes.at("fromWho") ? 477 : 476;
        if (event.kind != ReplayEventKind::Meld) return 0;
        const auto encoded = static_cast<unsigned>(std::stoul(event.attributes.at("m")));
        if (encoded & 4) return 475;
        if (encoded & 0x18) return encoded & 8 ? 473 : 474;
        if (encoded & 0x20) return 480;
        return 474;
    }

    struct ActionCaptionFrame {
        unsigned resource = 0, tick = 0;
        int who = 0;
        float scale = 1, alpha = 1;
        std::wstring text;
    };

    class ActionCaptions {
    public:
        bool AcceptResource(unsigned id, int who, const std::wstring &text = L"") {
            if ((!id && text.empty()) || who < 0 || who > 3) return false;
            ActionCaptionFrame frame; frame.resource = id; frame.who = who; frame.text = text;
            items_.push_back(std::move(frame)); return true;
        }
        bool Accept(const ReplayEvent &event) {
            if (event.kind == ReplayEventKind::Init) Clear();
            const auto id = ActionCaptionResource(event);
            const int who = event.attributes.count("who") ? std::stoi(event.attributes.at("who")) : event.who;
            if (!id || who < 0 || who > 3) return false;
            return AcceptResource(id, who);
        }

        void Clear() { items_.clear(); }
        bool Active() const { return !items_.empty(); }
        // LAB_00491ac0 increments on the effect update, never on a repaint.
        void Tick() {
            for (auto &item: items_) ++item.tick;
            items_.erase(std::remove_if(items_.begin(), items_.end(), [](const ActionCaptionFrame &item) {
                return item.tick > 100;
            }), items_.end());
        }

        std::vector<ActionCaptionFrame> Frames() const {
            auto result = items_;
            for (auto &item: result) {
                item.scale = item.tick < 20 ? 2.5f - item.tick * .075f : 1.f;
                item.alpha = item.tick < 20 ? item.tick * .05f : item.tick >= 60 ? (100 - item.tick) * .025f : 1.f;
            }
            return result;
        }

        void Paint(HDC dc, RECT target, int width, DisplayArtwork &artwork) const {
            if (!Active()) return;
            // FUN_00467b20 / FUN_00467b50: literal PE anchors for each pack.
            constexpr int xs[2][4]{{512, 844, 512, 180}, {720, 1190, 720, 250}};
            constexpr int ys[2][4]{{590, 340, 90, 340}, {830, 480, 120, 480}};
            Gdiplus::Graphics graphics(dc);
            graphics.SetClip(Gdiplus::Rect(target.left, target.top, target.right - target.left,
                                           target.bottom - target.top));
            graphics.TranslateTransform(static_cast<float>(target.left), static_cast<float>(target.top));
            graphics.ScaleTransform(float(target.right - target.left) / width,
                                    float(target.bottom - target.top) / (width * 3 / 4));
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor); // native mode 5
            for (const auto &item: Frames()) {
                if (item.tick == 100) continue;
                if (!item.text.empty()) {
                    Gdiplus::FontFamily family(L"KaiTi");
                    Gdiplus::GraphicsPath path;
                    Gdiplus::StringFormat format; format.SetAlignment(Gdiplus::StringAlignmentCenter);
                    const float size = (width == 1440 ? 42.f : 30.f) * item.scale;
                    path.AddString(item.text.c_str(), -1, family.IsAvailable() ? &family : Gdiplus::FontFamily::GenericSansSerif(),
                        Gdiplus::FontStyleRegular, size, Gdiplus::PointF(static_cast<float>(xs[width == 1440][item.who]), ys[width == 1440][item.who] - size / 2), &format);
                    const auto alpha = static_cast<BYTE>(255 * item.alpha);
                    Gdiplus::Pen outline(Gdiplus::Color(alpha, 0, 0, 0), 3);
                    Gdiplus::SolidBrush ink(Gdiplus::Color(alpha, 255, 248, 210));
                    graphics.DrawPath(&outline, &path); graphics.FillPath(&ink, &path);
                    continue;
                }
                const auto &image = artwork.Get(item.resource);
                Gdiplus::Bitmap bitmap(image.width, image.height, image.width * 4, PixelFormat32bppARGB,
                                       const_cast<BYTE *>(image.bgra.data()));
                const int w = static_cast<int>(image.width * item.scale), h = static_cast<int>(
                    image.height * item.scale);
                const int x = static_cast<int>(xs[width == 1440][item.who] - image.width * item.scale * .5f);
                const int y = static_cast<int>(ys[width == 1440][item.who] - image.height * item.scale * .5f);
                Gdiplus::ColorMatrix matrix{
                    {{1, 0, 0, 0, 0}, {0, 1, 0, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 0, item.alpha, 0}, {0, 0, 0, 0, 1}}
                };
                Gdiplus::ImageAttributes attributes;
                attributes.SetColorMatrix(&matrix);
                if (graphics.DrawImage(&bitmap, Gdiplus::Rect(x, y, w, h), 0, 0, image.width, image.height,
                                       Gdiplus::UnitPixel, &attributes) != Gdiplus::Ok)
                    throw std::runtime_error("Cannot draw original action caption");
            }
        }

    private:
        std::vector<ActionCaptionFrame> items_;
    };
} // namespace tenhou
