#include "tile_shadow.hpp"
#include "resource_manager.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace tenhou {
    namespace {
        unsigned U32(const Bytes &b, std::size_t n) {
            if (n + 4 > b.size()) throw std::runtime_error("Truncated shadow resource");
            unsigned v = 0;
            std::memcpy(&v, b.data() + n, 4);
            return v;
        }

        int I16(const Bytes &b, std::size_t n) { return static_cast<short>(U32(b, n) & 65535); }
        // FUN_00410920: index into the original sdw2 cache. No inferred RGB masks.
        unsigned Slot(ShadowKey k) {
            if (k.direction < 0 || k.direction > 3 || k.slot < 0) return 0;
            const unsigned s = static_cast<unsigned>(k.slot), d = static_cast<unsigned>(k.direction);
            switch (k.family) {
                case 't':
                    if (k.kind == 0 && s < 14) return 0xc + (d * 41 + s) * 4;
                    if (k.kind == 1 && s < 13) return 0x44 + (d * 41 + s) * 4;
                    if (k.kind == 2 && s < 14) return 0x78 + (d * 41 + s) * 4;
                    break;
                case 'y': {
                    constexpr unsigned counts[]{17, 17, 16, 16, 16, 17, 17, 16, 16, 17};
                    constexpr unsigned offsets[]{0x29c, 0x2e0, 0x324, 0x364, 0x3a4, 0x3e4, 0x428, 0x46c, 0x4ac, 0x4ec};
                    if (k.kind >= 0 && k.kind < 10 && s < counts[k.kind]) return offsets[k.kind] + (d * 165 + s) * 4;
                    break;
                }
                case 'k':
                    if (s < 40) {
                        if (k.kind == 0 && s < 25) return 0xcec + d * 0xd4 + s * 4;
                        if (k.kind == 1 && s < 24) return 0xd50 + d * 0xd4 + s * 4;
                        if (k.kind == 2) {
                            const int slots[]{0, 5, 6, 11};
                            for (unsigned i = 0; i < 4; ++i) if (s == static_cast<unsigned>(slots[i])) return
                                    0xdb0 + d * 0xd4 + i * 4;
                        }
                    } else if (s < 80) {
                        if (k.kind == 0 && s - 40 < 25) return 0xf9c + (s + d * 49) * 4;
                        if (k.kind == 1 && s - 40 < 24) return 0x1000 + (s + d * 49) * 4;
                    }
                    break;
                case 'n':
                    if (s < 20) {
                        if (k.kind == 0 && s < 18) return 0x134c + (d * 35 + s) * 4;
                        if (k.kind == 1 && s < 17) return 0x1394 + (d * 35 + s) * 4;
                    } else if (s < 40) {
                        if (k.kind == 0 && s - 20 < 17) return 0x152c + (s + d * 34) * 4;
                        if (k.kind == 1 && s - 20 < 17) return 0x1570 + (s + d * 34) * 4;
                    } else if (s < 60) {
                        if (s - 40 < 17) return 0x16fc + (s + d * 17) * 4;
                    } else if (s < 80) {
                        if (k.kind == 0 && s - 60 < 17) return 0x148c + (s + d * 34) * 4;
                        if (k.kind == 1 && s - 60 < 17) return 0x14d0 + (s + d * 34) * 4;
                    }
                    break;
            }
            return 0;
        }
    }

    ShadowPack::ShadowPack(Bytes raw) : raw_(std::move(raw)) {
        if (raw_.size() < 0x18ac || U32(raw_, 0) != 0x32776473 || U32(raw_, 4) != 0x18ac || U32(raw_, 8) != raw_.size()
            - 0x18ac)
            throw std::runtime_error("Invalid fixed HD shadow resource");
        for (unsigned slot = 12; slot < 0x18ac; slot += 4) {
            const auto offset = U32(raw_, slot);
            if (!offset || masks_.count(offset)) continue;
            ShadowMask mask;
            mask.x = I16(raw_, offset);
            mask.y = I16(raw_, offset + 2);
            mask.width = I16(raw_, offset + 4);
            mask.height = I16(raw_, offset + 6);
            mask.stride = (mask.width + 3) & ~3;
            if (mask.width <= 0 || mask.height <= 0 || mask.width > 1440 || mask.height > 1080) throw
                    std::runtime_error("Invalid shadow dimensions");
            const auto size = U32(raw_, offset + 8), expected = static_cast<unsigned>(mask.stride * mask.height);
            if (offset + 12 + std::uint64_t(size) > raw_.size()) throw std::runtime_error("Invalid shadow payload");
            mask.pixels = Bytes(raw_.begin() + offset + 12, raw_.begin() + offset + 12 + size);
            if (size != expected) mask.pixels = InflateZlib(mask.pixels, expected);
            if (mask.pixels.size() != expected) throw std::runtime_error("Invalid shadow stride");
            masks_.emplace(offset, std::move(mask));
        }
        raw_.resize(0x18ac);
        raw_.shrink_to_fit(); // Only the index remains; decoded masks are shared.
    }

    const ShadowPack &ShadowPack::Embedded() {
        static const ShadowPack pack([] {
            const auto view = ResourceManager::Embedded(AssetId::TileShadows);
            return InflateZlib(Bytes(view.data, view.data + view.size), 16 * 1024 * 1024);
        }());
        return pack;
    }

    const ShadowMask *ShadowPack::Get(ShadowKey key) const {
        const auto slot = Slot(key);
        if (!slot) return nullptr;
        const auto found = masks_.find(U32(raw_, slot));
        return found == masks_.end() ? nullptr : &found->second;
    }

    std::array<unsigned, 258> OriginalShadowPalette(int strength) {
        std::array < unsigned, 258 > result{1, 256};
        // ASM 004179e8..f5 stores float(strength * .1f); 0046a269
        // multiplies the 4-spaced index then truncates/saturates, not opacity/10.
        const float factor = float(std::clamp(strength, 0, 10) * double(.1f));
        for (unsigned i = 0; i < 256; ++i) result[i + 2] = static_cast<unsigned>(std::min(
                                                               255, int(i * 4 * double(factor)))) << 24;
        return result;
    }

    std::vector<TileShadow> BuildTileShadows(const std::vector<PositionedTile> &scene, TableArtwork &artwork,
                                             const ShadowPack &pack) {
        std::vector<TileShadow> result;
        if (artwork.width() != 1440) return result; // This fixed resource is HD only.
        for (const auto &placed: scene) {
            const auto &t = placed.tile;
            if (t.family == 'y' && t.slot >= 17 && std::any_of(scene.begin(), scene.end(),
                                                               [&](const PositionedTile &other) {
                                                                   return other.tile.family == 'y' && other.tile.
                                                                          direction == t.direction && other.tile.slot ==
                                                                          t.slot - 17;
                                                               }))
                continue; // Lower sprite belongs to the same top placeholder.
            const auto &image = artwork.Get(t);
            TileShadow entry;
            entry.placed = placed;
            entry.family = t.family;
            entry.slot = t.slot;
            entry.code = t.tile < 0 ? 0xfe : t.tile;
            char ground_family = t.family;
            int ground_slot = t.slot, kind = 0;
            if (t.family == 'n' && std::string(t.role) == "hand" && t.slot <= 17) {
                entry.family = 't';
                entry.slot = 17 - t.slot;
                if (t.tile < 0) entry.code = 0xfd;
            }
            if (t.family == 'y' && t.slot >= 17) {
                entry.slot = t.slot - 17;
                entry.code = 0xfd;
                ground_slot = entry.slot;
                kind = 1;
            }
            entry.dx = placed.x - image.x;
            entry.dy = placed.y - image.y;
            entry.ground = pack.Get({ground_family, ground_slot, kind, t.direction});
            result.push_back(entry);
        }
        const auto find = [&](char f, int s, int d)-> TileShadow * {
            for (auto &item: result) if (item.family == f && item.slot == s && item.placed.tile.direction == d) return &
                    item;
            return nullptr;
        };
        const auto mask = [&](const TileShadow &e, int slot, int kind) {
            return pack.Get({e.family, slot, kind, e.placed.tile.direction});
        };
        const auto aligned = [](const TileShadow &a, const TileShadow &b) { return a.dx == b.dx && a.dy == b.dy; };
        // FUN_00487350, with original placeholders projected from the visible scene.
        // Separate receiver and extra masks are required; they are not alpha on the
        // source tile. Translations are the receiver placeholder's native offsets.
        for (auto &current: result) {
            const auto d = current.placed.tile.direction;
            if (current.family == 'k') {
                const int slot = current.slot % 40;
                auto *previous = find('k', slot - 1, d);
                if (!previous) previous = find('k', slot - 1 + 40, d);
                if (previous && !(slot <= 17 && slot % 6 == 0) && aligned(current, *previous)) {
                    if (current.slot < 40) previous->ground = mask(current, previous->slot, 1);
                    else previous->extra = mask(current, slot - 1 + 40, 1);
                }
                if (current.slot < 40 && slot > 5 && slot < 18) {
                    auto *above = find('k', slot - 6, d);
                    bool edge = slot % 6 == 0;
                    if (slot % 6 == 5) {
                        edge = true;
                        for (int i = 7; i <= 11; ++i) if (find('k', slot - i + 40, d)) edge = false;
                    }
                    if (above && edge && aligned(current, *above)) above->extra = mask(current, slot - 6, 2);
                }
            } else if (current.family == 'n' && (current.slot < 40 || current.slot >= 60)) {
                const int slot = current.slot % 20;
                auto *previous = find('n', slot - 1, d);
                if (!previous) previous = find('n', slot - 1 + 20, d);
                if (!previous) previous = find('n', slot - 1 + 60, d);
                if (previous && slot > 0) {
                    if (previous->slot < 20) previous->extra = mask(current, slot - 1, 1);
                    else previous->ground = mask(current, slot - 1 + 20, 1);
                }
            } else if (current.family == 't') {
                auto *previous = find('t', current.slot - 1, d);
                if (previous && aligned(current, *previous) && (
                        d == 0 || (current.code >= 0xfe && previous->code == 0xfe)))
                    previous->ground = mask(current, previous->slot, current.code == 0xfe || d == 0 ? 1 : 0);
                if (d == 0 && current.code < 136) {
                    auto *next = find('t', current.slot + 1, d);
                    if (next && aligned(current, *next)) current.ground = mask(current, current.slot, 1);
                }
            } else if (current.family == 'y') {
                auto *previous = find('y', current.slot - 1, d);
                if (previous && previous->code != 0xfd) {
                    if (current.code == 0xfe) previous->ground = mask(current, current.slot - 1, 2);
                    if (current.code == 0xfd) {
                        previous->ground = mask(current, current.slot - 1, 0);
                        previous->extra = mask(current, current.slot - 1, 3);
                        current.received = mask(current, current.slot - 1, 4);
                    }
                }
            }
        }
        return result;
    }

    Image ComposeTileShadows(const std::vector<TileShadow> &scene, RECT r, int strength) {
        Image result;
        result.width = r.right - r.left;
        result.height = r.bottom - r.top;
        if (result.width <= 0 || result.height <= 0) return result;
        Bytes coverage(std::size_t(result.width) * result.height);
        for (const auto &item: scene) {
            if (!item.ground) continue; // Original only merges extra if base overlaps.
            for (const auto *mask: {item.ground, item.extra})
                if (mask) {
                    const int x = mask->x + item.dx, y = mask->y + item.dy;
                    for (int row = std::max(0, int(r.top) - y); row < std::min(mask->height, int(r.bottom) - y); ++
                         row) {
                        int col = std::max(0, int(r.left) - x);
                        const int end = std::min(mask->width, int(r.right) - x);
                        for (; col < end - 3; col += 4) {
                            auto *destination = coverage.data() + std::size_t(row + y - r.top) * result.width + col + x
                                                - r.left;
                            const auto *source = mask->pixels.data() + std::size_t(row) * mask->stride + col;
                            unsigned a = 0, b = 0;
                            std::memcpy(&a, destination, 4);
                            std::memcpy(&b, source, 4);
                            // FUN_004828e0 / 00482630 uses this four-byte comparison.
                            // Preserve its low-bit ties; std::max changes original pixels.
                            const auto select =
                            (0x80808080u - (
                                 ((b >> 1 & 0x7f7f7f7fu) - (a >> 1 & 0x7f7f7f7fu)) >> 7 & 0x01010101u)) ^ 0x80808080u;
                            const unsigned merged = (~select & b) | (select & a);
                            std::memcpy(destination, &merged, 4);
                        }
                        for (; col < end; ++col) {
                            auto &value = coverage[std::size_t(row + y - r.top) * result.width + col + x - r.left];
                            value = std::max(value, mask->pixels[std::size_t(row) * mask->stride + col]);
                        }
                    }
                }
        }
        const auto palette = OriginalShadowPalette(strength);
        result.bgra.resize(coverage.size() * 4);
        for (std::size_t i = 0; i < coverage.size(); ++i)
            result.bgra[i * 4 + 3] = static_cast<std::uint8_t>(palette[coverage[i] + 2] >> 24);
        return result;
    }

    Image ShadeTile(const Image &image, int x, int y, const ShadowMask &mask, int strength) {
        Image result = image;
        const auto palette = OriginalShadowPalette(strength);
        for (int row = std::max(0, mask.y - y); row < std::min(image.height, mask.y + mask.height - y); ++row)
            for (int col = std::max(0, mask.x - x); col < std::min(image.width, mask.x + mask.width - x); ++col) {
                auto *pixel = result.bgra.data() + (std::size_t(row) * image.width + col) * 4;
                if (!pixel[3]) continue;
                const unsigned alpha =
                        palette[mask.pixels[std::size_t(row + y - mask.y) * mask.stride + col + x - mask.x] + 2] >> 24;
                const double factor = 1.0 - alpha * double(1.0f / 255.0f);
                for (unsigned c = 0; c < 3; ++c) pixel[c] = static_cast<std::uint8_t>(int(pixel[c] * factor));
            }
        return result;
    }

    void PaintTileShadows(Gdiplus::Graphics &g, const std::vector<TileShadow> &scene, RECT region, int strength) {
        if (!strength) return;
        auto pixels = ComposeTileShadows(scene, region, strength);
        if (pixels.bgra.empty()) return;
        Gdiplus::Bitmap bitmap(pixels.width, pixels.height, pixels.width * 4, PixelFormat32bppARGB, pixels.bgra.data());
        if (g.DrawImage(&bitmap, region.left, region.top) != Gdiplus::Ok) throw std::runtime_error(
            "Cannot draw original tile shadows");
    }

    const TileShadow *FindTileShadow(const std::vector<TileShadow> &shadows, const PositionedTile &placed) {
        for (const auto &item: shadows)
            if (item.placed.tile.family == placed.tile.family && item.placed.tile.slot == placed.tile.slot &&
                item.placed.tile.direction == placed.tile.direction && item.placed.x == placed.x && item.placed.y ==
                placed.y)
                return &item;
        return nullptr;
    }

    void TileShadowLayer::Paint(Gdiplus::Graphics &g, const std::vector<TileShadow> &scene, int strength) {
        if (!strength || scene.empty()) return;
        bool changed = strength_ != strength || scene.size() != previous_.size();
        if (!changed)
            for (std::size_t i = 0; i < scene.size(); ++i) {
                const auto &a = scene[i];
                const auto &b = previous_[i];
                if (a.dx != b.dx || a.dy != b.dy || a.ground != b.ground || a.extra != b.extra) {
                    changed = true;
                    break;
                }
            }
        if (changed) {
            // Keep the original full-canvas four-byte raster grouping stable when
            // retained painting clips a moving tile. A crop changes scalar tails.
            auto pixels = ComposeTileShadows(scene, {0, 0, 1440, 1080}, strength);
            pixels_ = std::move(pixels);
            previous_ = scene;
            strength_ = strength;
        }
        // RGB is identically black, hence already premultiplied for every alpha.
        Gdiplus::Bitmap bitmap(pixels_.width, pixels_.height, pixels_.width * 4, PixelFormat32bppPARGB,
                               pixels_.bgra.data());
        if (g.DrawImage(&bitmap, 0, 0) != Gdiplus::Ok) throw std::runtime_error("Cannot draw retained tile shadows");
    }

    std::vector<RetainedScene::Damage> TileShadowDamage(const std::vector<PositionedTile> &scene,
                                                        const std::vector<TileShadow> &shadows, TableArtwork &artwork,
                                                        bool red) {
        std::vector<RetainedScene::Damage> result;
        for (const auto &placed: scene) {
            const auto &im = artwork.Get(placed.tile, red).image;
            RetainedScene::Damage damage{TileBounds(placed, im.width, im.height), {}};
            if (const auto *shadow = FindTileShadow(shadows, placed)) {
                damage.shadows = {shadow->ground, shadow->extra, shadow->received};
                for (const auto *mask: {shadow->ground, shadow->extra})
                    if (mask) {
                        const RECT r{
                            mask->x + shadow->dx - 2, mask->y + shadow->dy - 2, mask->x + shadow->dx + mask->width + 2,
                            mask->y + shadow->dy + mask->height + 2
                        };
                        RECT united{};
                        UnionRect(&united, &damage.bounds, &r);
                        damage.bounds = united;
                    }
            }
            result.push_back(damage);
        }
        return result;
    }
} // namespace tenhou
