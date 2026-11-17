#include "retained_scene.hpp"
#include "cached_hud.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace tenhou;
using namespace Gdiplus;
namespace {
    void Require(bool ok, const std::string &message) { if (!ok) throw std::runtime_error(message); }
    struct Session {
        ULONG_PTR token = 0;
        Session() { GdiplusStartupInput input; Require(GdiplusStartup(&token, &input, nullptr) == Ok, "GDI+ startup"); }
        ~Session() { GdiplusShutdown(token); }
    };
    struct Surface {
        HDC dc = CreateCompatibleDC(nullptr);
        HBITMAP bitmap = nullptr;
        HGDIOBJ previous = nullptr;
        std::uint32_t *pixels = nullptr;
        int width, height;
        Surface(int w, int h) : width(w), height(h) {
            BITMAPINFO info{}; info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = w; info.bmiHeader.biHeight = -h;
            info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
            void *data = nullptr;
            bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &data, nullptr, 0);
            Require(dc && bitmap, "Cannot allocate test surface");
            previous = SelectObject(dc, bitmap); pixels = static_cast<std::uint32_t *>(data);
            Clear();
        }
        ~Surface() { SelectObject(dc, previous); DeleteObject(bitmap); DeleteDC(dc); }
        void Clear() { GdiFlush(); std::fill_n(pixels, static_cast<std::size_t>(width) * height, 0x293544); }
        std::vector<std::uint32_t> Pixels() const {
            GdiFlush();
            std::vector<std::uint32_t> result(pixels, pixels + static_cast<std::size_t>(width) * height);
            for (auto &p : result) p &= 0xffffff;
            return result;
        }
        void Save(const std::filesystem::path &path) {
            GdiFlush(); UINT count = 0, size = 0;
            Require(GetImageEncodersSize(&count, &size) == Ok, "PNG encoders");
            std::vector<BYTE> buffer(size); auto *encoders = reinterpret_cast<ImageCodecInfo *>(buffer.data());
            Require(GetImageEncoders(count, size, encoders) == Ok, "PNG encoder list");
            for (UINT i = 0; i < count; ++i) if (wcscmp(encoders[i].MimeType, L"image/png") == 0) {
                Bitmap image(width, height, width * 4, PixelFormat32bppRGB, reinterpret_cast<BYTE *>(pixels));
                Require(image.Save(path.c_str(), &encoders[i].Clsid) == Ok, "Save composition screenshot"); return;
            }
            throw std::runtime_error("PNG encoder unavailable");
        }
    };
    TextKey TextId(int id) { const auto *d = FindTextByOrigin(id); Require(d != nullptr, "Missing test text"); return d->key; }
    void Equal(const Surface &a, const Surface &b, const std::string &label) {
        auto x = a.Pixels(), y = b.Pixels();
        const auto mismatch = std::mismatch(x.begin(), x.end(), y.begin());
        if (mismatch.first != x.end()) {
            const auto index = mismatch.first - x.begin();
            throw std::runtime_error(label + " differs at " + std::to_string(index % a.width) + "," + std::to_string(index / a.width));
        }
    }
    void Base(HDC dc, int width, int version) {
        Graphics g(dc);
        SolidBrush dark(Color(255, 25, 64, static_cast<BYTE>(70 + version * 30)));
        g.FillRectangle(&dark, 0, 0, width, width * 3 / 4);
        SolidBrush light(Color(255, 45, 86, 88));
        const int step = width / 16;
        for (int y = 0; y < width * 3 / 4; y += step)
            for (int x = 0; x < width; x += step)
                if ((x / step + y / step) % 2) g.FillRectangle(&light, x, y, step, step);
    }
    void Foreground(HDC dc, int width, int value) {
        Graphics g(dc); const float s = width == 1440 ? 1.0f : 1.0f / 1.4f;
        g.ScaleTransform(s, s);
        SolidBrush plate(Color(255, static_cast<BYTE>(70 + value * 45), 40, 25));
        g.FillRectangle(&plate, 480, 440, 440, 230);
    }
    FinalScene Scene(TextRenderer &renderer, CachedHud &hud, int native, int stage, unsigned dpi, int &image_draws) {
        FinalScene scene(renderer, native == 1024, dpi);
        if (stage >= 14) return scene;
        hud.AppendText(scene, "fixed", [&] {
            std::vector<SceneTextRun> runs;
            runs.push_back({renderer.Layout(TextId(481)), {90, 80}});
            runs.push_back({renderer.Layout(TextId(566)), {1040, 70}});
            runs.push_back({renderer.Layout(TextId(459)), {90, 190}});
            for (int seat = 0; seat < 4; ++seat) {
                const double angle = seat * 3.141592653589793 / 2;
                runs.push_back({renderer.Layout(TextId(416 + seat)), {static_cast<float>(300 + seat * 240), 365},
                    TextAlignment::center, {std::cos(angle), std::sin(angle), -std::sin(angle), std::cos(angle), 0, 0}});
            }
            return runs;
        });
        auto button = SceneTextRun{renderer.Layout(TextId(stage >= 2 ? 441 : 450)), {90, 270}};
        if (stage >= 8) button.transform = {1, .08, .12, 1, .25, .5};
        scene.Text(button, stage >= 7 ? std::optional<RectF>{{100, 260, 110, 55}} : std::nullopt);
        scene.Text({renderer.Layout(std::to_wstring(stage >= 2 ? 99800 : 25000) + L" 点", FindTextByOrigin(481)->style), {90, 880}});
        const auto caption = [&] {
            if (stage != 6) scene.Text(renderer.Caption(TextId(594), {720, 550}, stage >= 8 ? .83 : 1, stage >= 5 ? .35 : 1));
        };
        const auto image = [&] {
            const float x = stage >= 3 ? 690.0f : 630.0f;
            scene.Image("foreground-" + std::to_string(stage >= 3), {x, 530, 220, 90}, [x, &image_draws](Graphics &g) {
                ++image_draws; SolidBrush brush(Color(225, 40, 100, 200)); g.FillRectangle(&brush, x, 530.0f, 220.0f, 90.0f);
            });
        };
        if (stage == 13) { image(); caption(); } else { caption(); image(); }
        scene.Text({renderer.Layout(TextId(481)), {90, 740}}, RectF(110, 730, 180, 55));
        return scene;
    }
    void Composition(TextRenderer &renderer, int native, int device, const std::filesystem::path &output) {
        Surface cached(device + 34, device * 3 / 4 + 26), full(cached.width, cached.height);
        RetainedScene retained, reference; reference.SetReferenceRendering(true);
        TableArtwork artwork; CachedHud hud; int image_draws = 0;
        std::vector<PositionedTile> tiles;
        const auto render = [&](RetainedScene &buffer, Surface &target, const FinalScene *scene, int stage, int dest_width) {
            const int base = stage >= 4 ? 1 : 0;
            const double s = native == 1440 ? 1 : 1 / 1.4;
            const RECT plate{static_cast<LONG>(478 * s), static_cast<LONG>(438 * s),
                             static_cast<LONG>(922 * s), static_cast<LONG>(672 * s)};
            buffer.Render(target.dc, native, {17, 13, 17 + dest_width, 13 + dest_width * 3 / 4}, tiles, artwork, false,
                "base", std::to_string(base), [&](HDC dc) { Base(dc, native, 0); },
                [&](HDC dc, RECT) { Foreground(dc, native, base); }, {}, {},
                {plate}, {}, {}, scene);
        };
        for (int stage = 0; stage <= 15; ++stage) {
            const unsigned dpi = stage >= 11 ? 192 : stage >= 10 ? 144 : stage >= 9 ? 120 : 96;
            auto scene = Scene(renderer, hud, native, stage, dpi, image_draws);
            const auto before = renderer.CacheStats(); const auto draws = image_draws;
            render(retained, cached, &scene, stage, device);
            if (stage == 1 || stage == 15) {
                const auto after = renderer.CacheStats();
                Require(after.raster_builds == before.raster_builds && image_draws == draws, "Idle final scene rebuilt");
            }
            render(reference, full, &scene, stage, device);
            Equal(cached, full, "retained " + std::to_string(native) + "/" + std::to_string(device) + " stage " + std::to_string(stage));
            if (!output.empty() && (stage == 0 || stage == 5 || stage == 14)) cached.Save(output /
                ("composition-" + std::to_string(native) + "-" + std::to_string(device) + "-" + std::to_string(stage) + ".png"));
            if (stage == 9) Require(renderer.CacheStats().raster_builds > before.raster_builds, "DPI did not invalidate raster cache");
        }
        // Restore after exposure, including a target clip: cached pixels must be copied back.
        auto scene = Scene(renderer, hud, native, 0, 96, image_draws);
        render(retained, cached, &scene, 0, device); render(reference, full, &scene, 0, device);
        cached.Clear();
        const int saved = SaveDC(cached.dc); IntersectClipRect(cached.dc, 100, 100, 400, 300);
        render(retained, cached, &scene, 0, device); RestoreDC(cached.dc, saved);
        auto a = cached.Pixels(), b = full.Pixels();
        for (int y = 100; y < 300; ++y) for (int x = 100; x < 400; ++x)
            Require(a[static_cast<std::size_t>(y) * cached.width + x] == b[static_cast<std::size_t>(y) * cached.width + x], "Exposure restore");
        cached.Clear(); render(retained, cached, &scene, 0, device); Equal(cached, full, "Complete exposure restore");
        // Same buffer, changing viewport then restoring it, and switching final layer off.
        for (int dw : {device - 73, device}) {
            cached.Clear(); full.Clear(); render(retained, cached, &scene, 0, dw); render(reference, full, &scene, 0, dw);
            Equal(cached, full, "Viewport resize");
        }
        render(retained, cached, nullptr, 0, device); render(reference, full, nullptr, 0, device);
        Equal(cached, full, "Removing final layer");
        Require(hud.hits() > 0, "HUD layouts did not reuse cache");
    }
    void ScaledAndOrder(TextRenderer &renderer) {
        Surface a(900, 700), b(900, 700); PaintBuffer cached;
        for (int i = 0; i < 4; ++i) {
            FinalScene scene(renderer);
            if (i != 3) scene.Text(renderer.Caption(TextId(594), {720, 550}, 1, i == 2 ? .2 : 1));
            const RECT dest{11, 7, 888, 665};
            cached.RenderScaled(a.dc, 1440, 1080, dest, [&](HDC dc) { Base(dc, 1440, i == 1 ? 1 : 0); }, &scene);
            PaintBuffer fresh;
            fresh.RenderScaled(b.dc, 1440, 1080, dest, [&](HDC dc) { Base(dc, 1440, i == 1 ? 1 : 0); }, &scene);
            Equal(a, b, "RenderScaled final layer");
        }
        // Deliberately occlude the same text with an opaque image, then reverse order.
        const auto layout = renderer.Layout(TextId(481));
        const auto draw = [&](Surface &surface, bool image_last) {
            FinalScene scene(renderer);
            const auto text = [&] { scene.Text({layout, {100, 100}}); };
            const auto image = [&] { scene.Image("opaque", {0, 0, 1440, 1080}, [](Graphics &g) {
                SolidBrush brush(Color(255, 10, 20, 30)); g.FillRectangle(&brush, 0, 0, 1440, 1080);
            }); };
            if (image_last) { text(); image(); } else { image(); text(); }
            PaintBuffer buffer;
            buffer.RenderScaled(surface.dc, 1440, 1080, {0, 0, 900, 675}, [&](HDC dc) { Base(dc, 1440, 0); }, &scene);
        };
        draw(a, true); draw(b, false);
        Require(a.Pixels() != b.Pixels(), "Image/text ordering was flattened");
        auto pixels = a.Pixels();
        Require(pixels[100 * a.width + 100] == 0x0a141e, "Image did not occlude earlier text");
    }
    void CacheLimits(TextRenderer &renderer) {
        Surface surface(600, 100);
        for (int i = 0; i < 300; ++i) {
            Graphics g(surface.dc);
            renderer.Draw(g, {}, {renderer.Layout(std::to_wstring(i) + L" 点", FindTextByOrigin(481)->style), {0, 0}});
        }
        const auto stats = renderer.CacheStats();
        Require(stats.layouts <= 256 && stats.rasters <= 64 && stats.raster_bytes <= 32 * 1024 * 1024, "Unbounded text cache");
    }
    void DeviceOracleAndRecovery(TextRenderer &renderer) {
        Surface final(900, 700), direct(900, 700), legacy(900, 700); PaintBuffer buffer, base, native;
        const RECT dest{11, 7, 888, 665};
        const auto run = renderer.Caption(TextId(594), {720, 550}, .93, .65);
        FinalScene scene(renderer); scene.Text(run);
        const auto background = [&](HDC dc) { Base(dc, 1440, 0); };
        buffer.RenderScaled(final.dc, 1440, 1080, dest, background, &scene);
        base.RenderScaled(direct.dc, 1440, 1080, dest, background);
        const auto context = TextRenderContext::Viewport(1440, 1080, RectF(11, 7, 877, 658), false);
        {
            Graphics g(direct.dc); renderer.Draw(g, context, run);
        }
        Equal(final, direct, "Independent final-device raster oracle");
        native.RenderScaled(legacy.dc, 1440, 1080, dest, [&](HDC dc) {
            background(dc); Graphics g(dc); renderer.Draw(g, {}, run);
        });
        Require(final.Pixels() != legacy.Pixels(), "Text was rasterized before scaling");
        auto dpi = context; dpi.dpi = 192;
        Require(TextRenderer::Bounds(context, run).Equals(TextRenderer::Bounds(dpi, run)), "DPI multiplied physical size twice");
        // A failed snapshot after updating the native base must not keep a stale
        // final frame when the next successful scene has the old text identity.
        auto invalid = run; invalid.transform[0] = std::numeric_limits<double>::quiet_NaN();
        FinalScene bad(renderer); bad.Text(invalid);
        bool failed = false;
        try { buffer.RenderScaled(final.dc, 1440, 1080, dest, [&](HDC dc) { Base(dc, 1440, 1); }, &bad); }
        catch (const std::runtime_error &) { failed = true; }
        Require(failed, "Invalid transform did not fail explicitly");
        buffer.RenderScaled(final.dc, 1440, 1080, dest, [&](HDC dc) { Base(dc, 1440, 1); }, &scene);
        PaintBuffer fresh;
        fresh.RenderScaled(direct.dc, 1440, 1080, dest, [&](HDC dc) { Base(dc, 1440, 1); }, &scene);
        Equal(final, direct, "Failed final frame recovery");
    }
}
int main(int argc, char **argv) {
    try {
        Require(argc == 2 || argc == 3, "Usage: tenhou_text_composition_check RESOURCES [OUTPUT]");
        Session session; ResourceManager resources(argv[1]); TextFontService font(resources); TextRenderer renderer(font);
        const std::filesystem::path output = argc == 3 ? argv[2] : "";
        if (!output.empty()) std::filesystem::create_directories(output);
        for (int native : {1440, 1024}) for (int device : {1440, 1024, 1279}) Composition(renderer, native, device, output);
        ScaledAndOrder(renderer); DeviceOracleAndRecovery(renderer); CacheLimits(renderer);
        std::cout << "PASS final text composition: 6 native/physical size pairs; full/dirty equality, hover/numbers, "
                     "rotation/shear, clip, image order, fade/removal, exposure, resize, 96/120/144/192 DPI, bounded/idle caches\n";
        return 0;
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
