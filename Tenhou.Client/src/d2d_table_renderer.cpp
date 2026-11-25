#include "d2d_table_renderer.hpp"
#include "action_captions.hpp"
#include "background.hpp"
#include "gdi_table_renderer.hpp"
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <d2d1effects.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <cstring>
#include <map>
#include <stdexcept>

namespace tenhou {
    namespace {
        using Microsoft::WRL::ComPtr;
        constexpr auto format = DXGI_FORMAT_B8G8R8A8_UNORM;
        constexpr wchar_t surface_class[] = L"TENHOU_DXGI_TABLE";

        struct GraphicsError : std::runtime_error {
            HRESULT status;
            GraphicsError(HRESULT hr, const char *operation) : std::runtime_error(operation), status(hr) {}
        };

        void Check(HRESULT result, const char *operation) {
            if (FAILED(result)) throw GraphicsError(result, operation);
        }

        bool DeviceLost(HRESULT hr) {
            return hr == D2DERR_RECREATE_TARGET || hr == DXGI_ERROR_DEVICE_REMOVED ||
                   hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DEVICE_HUNG;
        }

        D2D1_RECT_F Box(float x, float y, float w, float h) { return {x, y, x + w, y + h}; }
        D2D1_RECT_F Box(CanvasRect r) { return {float(r.left), float(r.top), float(r.right), float(r.bottom)}; }

        class AxisClip {
        public:
            AxisClip(ID2D1DeviceContext &dc, D2D1_RECT_F rect) : dc_(dc) {
                dc_.PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_ALIASED);
            }
            ~AxisClip() { dc_.PopAxisAlignedClip(); }
        private:
            ID2D1DeviceContext &dc_;
        };

        LRESULT CALLBACK SurfaceProc(HWND window, UINT msg, WPARAM wp, LPARAM lp) {
            if (msg == WM_NCHITTEST) return HTTRANSPARENT;
            if (msg == WM_ERASEBKGND) return 1;
            if (msg == WM_PAINT) { ValidateRect(window, nullptr); return 0; }
            return DefWindowProcW(window, msg, wp, lp);
        }

        BITMAPINFO BitmapInfo(int width, int height) {
            BITMAPINFO info{};
            info.bmiHeader = {sizeof(BITMAPINFOHEADER), width, -height, 1, 32, BI_RGB, 0, 0, 0, 0, 0};
            return info;
        }

        class Dib {
        public:
            Dib(int width, int height) {
                const auto info = BitmapInfo(width, height);
                dc = CreateCompatibleDC(nullptr);
                void *data = nullptr;
                bitmap_ = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &data, nullptr, 0);
                if (dc && bitmap_) old_ = SelectObject(dc, bitmap_);
                if (!old_ || old_ == HGDI_ERROR) {
                    Release();
                    throw std::runtime_error("Cannot create table overlay DIB");
                }
                pixels = static_cast<std::uint8_t *>(data);
            }
            ~Dib() { Release(); }
            Dib(const Dib &) = delete;
            Dib &operator=(const Dib &) = delete;
            HDC dc = nullptr;
            std::uint8_t *pixels = nullptr;
        private:
            void Release() {
                if (old_ && old_ != HGDI_ERROR) SelectObject(dc, old_);
                if (bitmap_) DeleteObject(bitmap_);
                if (dc) DeleteDC(dc);
            }
            HBITMAP bitmap_ = nullptr;
            HGDIOBJ old_ = nullptr;
        };

        constexpr D2D1_MATRIX_5X4_F Matrix(float r, float g, float b, float dr, float dg, float db) {
            return {r, 0, 0, 0, 0, g, 0, 0, 0, 0, b, 0, 0, 0, 0, 1, dr, dg, db, 0};
        }
        constexpr D2D1_MATRIX_5X4_F hand_colors[]{
            Matrix(1, 1, 1, 0, 0, 0), Matrix(.95f, .5f, .75f, 0, .5f, .25f),
            Matrix(.5f, 1, 1, .5f, 0, 0), Matrix(1, 1, 1, 0, 0, 1),
            Matrix(.8f, .8f, .8f, 0, 0, 0), Matrix(1, .5f, 0, 0, .5f, 1),
            Matrix(1, .8f, .35f, 0, .2f, 0)
        };
    }

    struct D2DTableRenderer::Impl {
        using Bitmap = ComPtr<ID2D1Bitmap1>;
        using Glyph = std::weak_ptr<const TextRasterPixels>;
        struct TileTexture { Bitmap bitmap; std::size_t bytes = 0, used = 0; };
        struct HudSprite { unsigned id; int x, y; std::array<float, 6> transform; };

        explicit Impl(bool warp) : warp_only(warp) {}
        ~Impl() {
            ResetDevice();
            if (surface && IsWindow(surface)) DestroyWindow(surface);
        }

        bool warp_only, reset_requested = false, reference = false;
        std::filesystem::path directory, config;
        int width = 1440;
        ClientPreferences preferences;
        TableArtwork tiles;
        DisplayArtwork art;
        TileShadowLayer shadows;
        Image background;
        std::unique_ptr<GdiTableRenderer> fallback;
        std::size_t hud_hits = 0, presented_frames = 0, device_generation = 0;

        ComPtr<ID3D11Device> d3d;
        ComPtr<ID3D11DeviceContext> immediate;
        ComPtr<IDXGIFactory2> dxgi;
        ComPtr<ID2D1DeviceContext> dc;
        ComPtr<ID2D1SolidColorBrush> brush;
        std::array<ComPtr<ID2D1Effect>, std::size(hand_colors)> effects;
        using TileKey = std::tuple<std::uint32_t, std::uintptr_t, int, int>;
        std::map<TileKey, TileTexture> tile_textures;
        std::map<unsigned, Bitmap> sprites;
        std::map<Glyph, Bitmap, std::owner_less<Glyph>> glyphs;
        std::map<std::tuple<int, int, int, bool, bool, bool, bool, unsigned, std::wstring>, Bitmap> legacy_buttons;
        std::size_t tile_bytes = 0, frame_number = 0;
        Bitmap backdrop, shadow_texture, base, hud_text, controls_text;
        std::size_t shadow_revision = 0;
        std::vector<HudSprite> hud_sprites;
        std::string hud_stamp;
        std::wstring controls_stamp;
        ComPtr<ID3D11Texture2D> offscreen, staging;
        Bitmap offscreen_target;
        int read_width = 0, read_height = 0;
        Bytes read_pixels;
        HWND surface = nullptr;
        ComPtr<IDXGISwapChain1> swap;
        Bitmap swap_target;
        int swap_width = 0, swap_height = 0;
        CanvasRect surface_bounds{};

        GdiTableRenderer &Fallback() {
            if (!fallback) {
                auto renderer = std::make_unique<GdiTableRenderer>();
                renderer->Configure(directory, width);
                renderer->SetPreferences(preferences, config, false);
                renderer->SetReferenceRendering(reference);
                fallback = std::move(renderer);
            }
            return *fallback;
        }

        void ResetTextures() {
            effects = {};
            tile_textures.clear(); tile_bytes = 0;
            sprites.clear();
            glyphs.clear();
            legacy_buttons.clear();
            backdrop.Reset(); shadow_texture.Reset(); shadow_revision = 0; base.Reset();
            hud_text.Reset(); hud_sprites.clear(); hud_stamp.clear();
            controls_text.Reset(); controls_stamp.clear();
        }

        void ResetDevice() {
            if (dc) dc->SetTarget(nullptr);
            ResetTextures();
            offscreen_target.Reset(); offscreen.Reset(); staging.Reset(); read_width = read_height = 0;
            swap_target.Reset(); swap.Reset(); swap_width = swap_height = 0;
            brush.Reset();
            dc.Reset();
            dxgi.Reset();
            if (immediate) { immediate->ClearState(); immediate->Flush(); }
            immediate.Reset();
            d3d.Reset();
        }

        void CreateDevice(D3D_DRIVER_TYPE driver) {
            Check(D3D11CreateDevice(nullptr, driver, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                    nullptr, 0, D3D11_SDK_VERSION, &d3d, nullptr, &immediate),
                  "Cannot create D3D11 device");
            ComPtr<IDXGIDevice> dxgi_device;
            Check(d3d.As(&dxgi_device), "Cannot query DXGI device");
            ComPtr<IDXGIAdapter> adapter;
            Check(dxgi_device->GetAdapter(&adapter), "Cannot query DXGI adapter");
            Check(adapter->GetParent(IID_PPV_ARGS(&dxgi)), "Cannot query DXGI factory");
            ComPtr<ID2D1Factory1> factory;
            Check(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory.GetAddressOf()), "Cannot create Direct2D factory");
            ComPtr<ID2D1Device> device;
            Check(factory->CreateDevice(dxgi_device.Get(), &device), "Cannot create Direct2D device");
            Check(device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &dc), "Cannot create Direct2D context");
            dc->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
            dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
            Check(dc->CreateSolidColorBrush(D2D1::ColorF(0u), &brush), "Cannot create Direct2D solid brush");
            ++device_generation;
        }

        void EnsureDevice() {
            if (reset_requested) {
                reset_requested = false;
                throw GraphicsError(DXGI_ERROR_DEVICE_RESET, "Simulated D3D11 device reset");
            }
            if (dc) return;
            if (!warp_only) {
                try {
                    CreateDevice(D3D_DRIVER_TYPE_HARDWARE);
                    return;
                } catch (const GraphicsError &) {
                    ResetDevice();
                }
            }
            CreateDevice(D3D_DRIVER_TYPE_WARP);
        }

        template<class Draw>
        bool TryGraphics(Draw draw) {
            for (int attempt = 0; attempt < 2; ++attempt) {
                try {
                    EnsureDevice();
                    draw();
                    return true;
                } catch (const GraphicsError &error) {
                    const bool lost = DeviceLost(error.status) || (d3d && FAILED(d3d->GetDeviceRemovedReason()));
                    ResetDevice();
                    if (!lost) return false;
                }
            }
            return false;
        }

        TileGeometryCatalog &Geometry(const std::vector<int> &red) {
            if (!tiles.width()) {
                tiles.Load(directory, width);
                tiles.SetColor(preferences.TileColor());
            }
            tiles.SetRedTiles(red);
            return tiles.geometry();
        }

        void EnsureArtwork(const TableRenderFrame &frame) {
            if (frame.canvas_width != width) {
                width = frame.canvas_width;
                tiles = {}; art.Configure(directory, width); background = {}; fallback.reset();
                ResetTextures();
            }
            Geometry(frame.state.red_tiles);
            if (background.bgra.empty())
                background = BuildPreferredBackground(directory, config, width, width * 3 / 4, preferences);
        }

        static D2D1_BITMAP_PROPERTIES1 Properties(D2D1_BITMAP_OPTIONS options) {
            return D2D1::BitmapProperties1(options, D2D1::PixelFormat(format, D2D1_ALPHA_MODE_PREMULTIPLIED), 96, 96);
        }

        Bitmap Upload(const std::uint8_t *pixels, int w, int h) {
            Bitmap bitmap;
            Check(dc->CreateBitmap(D2D1::SizeU(w, h), pixels, static_cast<UINT32>(w * 4),
                                    Properties(D2D1_BITMAP_OPTIONS_NONE), &bitmap), "Cannot upload table texture");
            return bitmap;
        }

        Bitmap Upload(const Image &image) {
            auto pixels = image.bgra;
            for (std::size_t i = 0; i < pixels.size(); i += 4)
                for (int c = 0; c < 3; ++c)
                    pixels[i + c] = static_cast<std::uint8_t>((pixels[i + c] * pixels[i + 3] + 127) / 255);
            return Upload(pixels.data(), image.width, image.height);
        }

        Bitmap SurfaceTarget(IDXGISurface *buffer) {
            Bitmap target;
            Check(dc->CreateBitmapFromDxgiSurface(buffer,
                    Properties(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW), &target),
                  "Cannot bind DXGI surface to Direct2D");
            return target;
        }

        void EnsureReadback(int w, int h) {
            if (read_width == w && read_height == h) return;
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = w; desc.Height = h;
            desc.MipLevels = desc.ArraySize = 1;
            desc.Format = format;
            desc.SampleDesc.Count = 1;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            Check(d3d->CreateTexture2D(&desc, nullptr, &offscreen), "Cannot allocate offscreen table");
            ComPtr<IDXGISurface> view;
            Check(offscreen.As(&view), "Cannot query offscreen table DXGI surface");
            offscreen_target = SurfaceTarget(view.Get());
            desc.Usage = D3D11_USAGE_STAGING;
            desc.BindFlags = 0;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            Check(d3d->CreateTexture2D(&desc, nullptr, &staging), "Cannot allocate table staging texture");
            read_pixels.resize(static_cast<std::size_t>(w) * h * 4);
            read_width = w; read_height = h;
        }

        void EnsureSwap(HWND parent, CanvasRect bounds) {
            const int w = bounds.right - bounds.left, h = bounds.bottom - bounds.top;
            if (!surface || !IsWindow(surface) || GetParent(surface) != parent) {
                if (surface && IsWindow(surface)) DestroyWindow(surface);
                swap_target.Reset(); swap.Reset(); surface_bounds = {};
                const auto instance = GetModuleHandleW(nullptr);
                WNDCLASSW wc{};
                wc.hInstance = instance; wc.lpfnWndProc = SurfaceProc; wc.lpszClassName = surface_class;
                if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
                    throw GraphicsError(E_FAIL, "Cannot register table presentation window");
                surface = CreateWindowExW(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE, surface_class, L"",
                                          WS_CHILD | WS_CLIPSIBLINGS, bounds.left, bounds.top, w, h,
                                          parent, nullptr, instance, nullptr);
                if (!surface) throw GraphicsError(E_FAIL, "Cannot create table presentation window");
            }
            if (!swap) {
                DXGI_SWAP_CHAIN_DESC1 desc{};
                desc.Width = w; desc.Height = h; desc.Format = format; desc.SampleDesc.Count = 1;
                desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                desc.BufferCount = 2; desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
                desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
                Check(dxgi->CreateSwapChainForHwnd(d3d.Get(), surface, &desc, nullptr, nullptr, &swap),
                      "Cannot create flip-model table swap chain");
            } else if (swap_width != w || swap_height != h) {
                swap_target.Reset();
                immediate->Flush();
                Check(swap->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0), "Cannot resize table swap chain");
            }
            if (!swap_target) {
                ComPtr<IDXGISurface> buffer;
                Check(swap->GetBuffer(0, IID_PPV_ARGS(&buffer)), "Cannot get table swap chain buffer");
                swap_target = SurfaceTarget(buffer.Get());
                swap_width = w; swap_height = h;
            }
            if (!(GetWindowLongPtrW(surface, GWL_STYLE) & WS_VISIBLE) || bounds.left != surface_bounds.left ||
                bounds.top != surface_bounds.top || bounds.right != surface_bounds.right || bounds.bottom != surface_bounds.bottom) {
                if (!SetWindowPos(surface, HWND_BOTTOM, bounds.left, bounds.top, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW))
                    throw GraphicsError(E_FAIL, "Cannot position table presentation window");
                surface_bounds = bounds;
            }
        }

        template<class Draw>
        void DrawOn(ID2D1Bitmap1 *target, Draw draw) {
            dc->SetTarget(target);
            dc->SetTransform(D2D1::Matrix3x2F::Identity());
            dc->BeginDraw();
            try { draw(); } catch (...) {
                dc->EndDraw();
                dc->SetTarget(nullptr);
                throw;
            }
            const auto status = dc->EndDraw();
            dc->SetTarget(nullptr);
            Check(status, "Cannot finish Direct2D table rendering");
        }

        void Blit(HDC target, CanvasRect area) {
            immediate->CopyResource(staging.Get(), offscreen.Get());
            D3D11_MAPPED_SUBRESOURCE mapped{};
            Check(immediate->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped), "Cannot map table readback");
            const auto *source = static_cast<const std::uint8_t *>(mapped.pData);
            const auto row = static_cast<std::size_t>(read_width) * 4;
            for (int y = 0; y < read_height; ++y)
                std::memcpy(read_pixels.data() + y * row, source + y * mapped.RowPitch, row);
            immediate->Unmap(staging.Get(), 0);
            const auto info = BitmapInfo(read_width, read_height);
            // A fully clipped HDC can report zero scanlines.
            SetDIBitsToDevice(target, area.left, area.top, read_width, read_height, 0, 0, 0,
                              static_cast<UINT>(read_height), read_pixels.data(), &info, DIB_RGB_COLORS);
        }

        void Fill(D2D1_RECT_F r, D2D1_COLOR_F color) {
            brush->SetColor(color);
            dc->FillRectangle(r, brush.Get());
        }

        void DrawBitmap(ID2D1Bitmap *bitmap, D2D1_RECT_F dest, float opacity = 1,
                        D2D1_INTERPOLATION_MODE sampling = D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR) {
            dc->DrawBitmap(bitmap, dest, opacity, sampling);
        }

        void DrawBitmap(ID2D1Bitmap *bitmap, int x, int y, float opacity = 1) {
            const auto size = bitmap->GetPixelSize();
            DrawBitmap(bitmap, Box(float(x), float(y), float(size.width), float(size.height)), opacity);
        }

        ID2D1Bitmap1 *Sprite(unsigned id) {
            auto &texture = sprites[id];
            if (!texture) texture = Upload(art.Get(id));
            return texture.Get();
        }

        ID2D1Bitmap1 *Tile(const TableTile &tile, bool red, const ShadowMask *received = nullptr,
                           int shadow_x = 0, int shadow_y = 0) {
            const auto key = TileKey{tiles.geometry().ImageKey(tile, red),
                                     reinterpret_cast<std::uintptr_t>(received), shadow_x, shadow_y};
            auto &texture = tile_textures[key];
            if (!texture.bitmap) {
                const auto &image = tiles.Get(tile, red).image;
                texture.bitmap = received ? Upload(ShadeTile(image, shadow_x, shadow_y, *received, preferences.tile_shadow))
                                          : Upload(image);
                texture.bytes = image.bgra.size();
                tile_bytes += texture.bytes;
            }
            texture.used = frame_number;
            return texture.bitmap.Get();
        }

        void DrawTile(ID2D1Bitmap1 *bitmap, int x, int y, HandColor color) {
            if (color == HandColor::Normal) {
                DrawBitmap(bitmap, x, y);
                return;
            }
            const auto index = static_cast<unsigned>(color);
            auto &effect = effects[index];
            if (!effect) {
                Check(dc->CreateEffect(CLSID_D2D1ColorMatrix, &effect), "Cannot create table color matrix");
                Check(effect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, hand_colors[index]), "Cannot set hand color");
                Check(effect->SetValue(D2D1_COLORMATRIX_PROP_CLAMP_OUTPUT, TRUE), "Cannot clamp hand color");
            }
            effect->SetInput(0, bitmap);
            dc->DrawImage(effect.Get(), D2D1::Point2F(float(x), float(y)), D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
        }

        void DrawText(TextRenderer &renderer, const TextRenderContext &context, const SceneTextRun &run) {
            const auto raster = renderer.Rasterize(context, run);
            if (!raster.pixels) return;
            auto &texture = glyphs[Glyph(raster.pixels)];
            if (!texture) texture = Upload(raster.pixels->bgra.data(), raster.pixels->width, raster.pixels->height);
            std::optional<AxisClip> clip;
            if (context.device_clip) {
                const auto &r = *context.device_clip;
                clip.emplace(*dc.Get(), Box(r.X, r.Y, r.Width, r.Height));
            }
            DrawBitmap(texture.Get(), raster.bounds.X, raster.bounds.Y, float(std::clamp(run.opacity, 0.0, 1.0)));
        }

        void DrawFlights(const TableRenderFrame &frame) {
            for (const auto index: TableFlightOrder(frame.tiles)) {
                const auto &placed = frame.tiles[index];
                const auto &flight = *placed.flight;
                auto *bitmap = Tile(flight.sprite, frame.red_tiles);
                for (unsigned i = 0; i < flight.count; ++i)
                    DrawBitmap(bitmap, placed.x + flight.offsets[i].x, placed.y + flight.offsets[i].y, flight.opacity);
                if (!flight.cover_landing) continue;
                const auto size = bitmap->GetPixelSize();
                const auto bounds = TileBounds(placed, size.width, size.height);
                const auto landing = TileBounds({placed.tile, flight.landing_x, flight.landing_y, flight},
                                                size.width, size.height);
                RECT patch{};
                if (!IntersectRect(&patch, &landing, &bounds)) continue;
                AxisClip clip(*dc.Get(), Box(CanvasRect{patch.left, patch.top, patch.right, patch.bottom}));
                for (std::size_t i = index + 1; i < frame.tiles.size(); ++i) {
                    const auto &cover = frame.tiles[i];
                    if (cover.flight) continue;
                    auto *face = Tile(cover.tile, frame.red_tiles);
                    const auto sz = face->GetPixelSize();
                    if (Overlaps(patch, TileBounds(cover, sz.width, sz.height)))
                        DrawBitmap(face, cover.x, cover.y);
                }
            }
        }

        void PrepareHud(const TableRenderFrame &frame, int h) {
            const int hover = frame.interaction ? frame.interaction->hovered_button : -1;
            auto stamp = TableMetadata(frame.state) + ':' + std::to_string(hover);
            if (hud_text && stamp == hud_stamp) { ++hud_hits; return; }

            Bytes pixels(static_cast<std::size_t>(width) * h * 4);
            Gdiplus::Bitmap bitmap(width, h, width * 4, PixelFormat32bppPARGB, pixels.data());
            Gdiplus::Graphics g(&bitmap);
            g.SetPageUnit(Gdiplus::UnitPixel);
            std::vector<HudSprite> commands;
            art.PaintHudImages(g, frame.state, [&](unsigned id, const Image &, int x, int y, const auto &t) {
                commands.push_back({id, x, y, t});
            });
            art.PaintChromeBackground(g, hover);
            art.PaintNukiCounts(g, frame.state, tiles);
            g.Flush(Gdiplus::FlushIntentionSync);
            hud_text = Upload(pixels.data(), width, h);
            hud_sprites = std::move(commands);
            hud_stamp = std::move(stamp);
        }

        void DrawInteraction(const TableRenderFrame &frame, int h) {
            if (!frame.interaction || (frame.interaction->hint.empty() && frame.interaction->countdown.empty())) return;
            const auto &input = *frame.interaction;
            auto stamp = input.hint + L'\0' + input.countdown;
            if (!input.countdown.empty())
                for (const auto &tile: frame.tiles)
                    if (tile.tile.direction == 0 && std::string_view(tile.tile.role) == "hand")
                        stamp += L':' + std::to_wstring(tile.x) + L',' + std::to_wstring(tile.y) +
                                 L',' + std::to_wstring(tile.tile.tile);
            if (!controls_text || controls_stamp != stamp) {
                Dib black(width, h), white(width, h);
                PatBlt(black.dc, 0, 0, width, h, BLACKNESS);
                PatBlt(white.dc, 0, 0, width, h, WHITENESS);
                PaintTableInteraction(black.dc, frame, tiles, true);
                PaintTableInteraction(white.dc, frame, tiles, true);
                GdiFlush();
                // Recover coverage from the same grayscale glyphs over two backgrounds.
                const auto bytes = static_cast<std::size_t>(width) * h * 4;
                for (std::size_t i = 0; i < bytes; i += 4)
                    black.pixels[i + 3] = static_cast<std::uint8_t>(
                        255 - std::clamp(int(white.pixels[i]) - int(black.pixels[i]), 0, 255));
                controls_text = Upload(black.pixels, width, h);
                controls_stamp = std::move(stamp);
            }
            DrawBitmap(controls_text.Get(), 0, 0);
        }

        void DrawNative(const TableRenderFrame &frame, int h) {
            if (!backdrop) backdrop = Upload(background);
            DrawBitmap(backdrop.Get(), 0, 0);
            const auto scene = preferences.tile_shadow ? BuildTileShadows(frame.tiles, tiles, ShadowPack::Embedded())
                                                       : std::vector<TileShadow>{};
            if (const auto *image = shadows.Raster(scene, preferences.tile_shadow)) {
                if (!shadow_texture || shadow_revision != shadows.revision()) {
                    shadow_texture = Upload(image->bgra.data(), image->width, image->height);
                    shadow_revision = shadows.revision();
                }
                DrawBitmap(shadow_texture.Get(), 0, 0);
            }
            for (std::size_t i = 0; i < frame.tiles.size(); ++i) {
                const auto &placed = frame.tiles[i];
                if (placed.flight) continue;
                const bool missing = frame.online_mode && std::string_view(placed.tile.role) == "missing_suit";
                const bool red = missing || frame.red_tiles;
                const auto color = i < frame.tile_colors.size() ? frame.tile_colors[i] : HandColor::Normal;
                const auto *shadow = FindTileShadow(scene, placed);
                auto *bitmap = shadow && shadow->received ?
                    Tile(placed.tile, red, shadow->received, placed.x - shadow->dx, placed.y - shadow->dy) :
                    Tile(placed.tile, red);
                DrawTile(bitmap, placed.x, placed.y, color);
                if (missing) {
                    const auto size = bitmap->GetPixelSize();
                    Fill(Box(float(placed.x), float(placed.y), float(size.width), float(size.height)),
                         D2D1::ColorF(0x5a5a5au, 145.f / 255));
                }
            }
            DrawFlights(frame);
            const double unit = double(width) / 1024;
            if (!frame.state.wall.known)
                for (std::size_t i = 0; i < frame.state.dora_indicators.size(); ++i)
                    DrawBitmap(Tile({'k', 0, frame.state.dora_indicators[i], 0}, frame.red_tiles),
                               static_cast<int>((28 + i * 40) * unit), static_cast<int>(28 * unit));

            DrawInteraction(frame, h);
            PrepareHud(frame, h);
            for (const auto &sprite: hud_sprites) {
                const auto &t = sprite.transform;
                dc->SetTransform(D2D1::Matrix3x2F(t[0], t[1], t[2], t[3], t[4], t[5]));
                DrawBitmap(Sprite(sprite.id), sprite.x, sprite.y);
            }
            dc->SetTransform(D2D1::Matrix3x2F::Identity());
            DrawBitmap(hud_text.Get(), 0, 0);
        }

        void DrawButton(TableActionButton button, unsigned dpi) {
            const auto r = button.viewport_rect;
            const int bw = r.right - r.left, bh = r.bottom - r.top;
            if (bw <= 0 || bh <= 0) return;
            const auto key = ActionButtonText(button.id, button.enabled && button.hot);
            if (button.style != TableActionButtonStyle::Sprite440 || !key) {
                const auto legacy_key = std::make_tuple(button.id, bw, bh, button.enabled, button.hot,
                                                        button.pressed, button.focused, dpi, button.label);
                auto &texture = legacy_buttons[legacy_key];
                if (!texture) {
                    Dib dib(bw, bh);
                    button.viewport_rect = {0, 0, bw, bh};
                    Fallback().RenderButtonToDc(dib.dc, button, dpi);
                    GdiFlush();
                    const auto bytes = static_cast<std::size_t>(bw) * bh * 4;
                    for (std::size_t i = 3; i < bytes; i += 4) dib.pixels[i] = 255;
                    texture = Upload(dib.pixels, bw, bh);
                }
                DrawBitmap(texture.Get(), r.left, r.top);
                return;
            }
            AxisClip clip(*dc.Get(), Box(r));
            Fill(Box(r), D2D1::ColorF(0u));
            const int offset = button.pressed ? 1 : 0;
            DrawBitmap(Sprite(440), Box(float(r.left + offset), float(r.top + offset), float(bw), float(bh)),
                       1, D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);
            auto &renderer = art.Text();
            SceneTextRun run;
            run.layout = renderer.Layout(*key);
            const auto &ink = run.layout.metrics.ink;
            run.transform[4] = -ink.X - ink.Width / 2;
            run.transform[5] = -ink.Y - ink.Height / 2;
            TextRenderContext context;
            context.native_layout_scale = width == 1024 ? 1.0 / 1.4 : 1;
            context.viewport_scale_x = context.viewport_scale_y = double(bw) / (width == 1440 ? 140 : 100);
            context.origin_x = (r.left + r.right) / 2.0 + offset;
            context.origin_y = (r.top + r.bottom) / 2.0 + offset;
            context.dpi = dpi ? dpi : 96;
            DrawText(renderer, context, run);
            if (!button.enabled) Fill(Box(r), D2D1::ColorF(0u, 150.f / 255));
            if (button.focused) {
                const D2D1::ColorF focus(0xffffffu);
                for (int x = r.left + 3; x < r.right - 3; x += 2) {
                    Fill(Box(float(x), float(r.top + 3), 1, 1), focus);
                    Fill(Box(float(x), float(r.bottom - 4), 1, 1), focus);
                }
                for (int y = r.top + 4; y < r.bottom - 4; y += 2) {
                    Fill(Box(float(r.left + 3), float(y), 1, 1), focus);
                    Fill(Box(float(r.right - 4), float(y), 1, 1), focus);
                }
            }
        }

        void TrimCaches() {
            for (auto it = glyphs.begin(); it != glyphs.end();) {
                if (it->first.expired()) it = glyphs.erase(it);
                else ++it;
            }
            if (legacy_buttons.size() > 64) legacy_buttons.clear();
            constexpr std::size_t budget = 48u * 1024 * 1024;
            if (tile_bytes > budget)
                for (auto it = tile_textures.begin(); it != tile_textures.end() && tile_bytes > budget;) {
                    if (it->second.used == frame_number) { ++it; continue; }
                    tile_bytes -= it->second.bytes;
                    it = tile_textures.erase(it);
                }
        }

        void Render(ID2D1Bitmap1 *target, const TableRenderFrame &frame, CanvasRect area, bool window) {
            const int h = width * 3 / 4;
            const auto &dest = frame.destination;
            const int dw = dest.right - dest.left, dh = dest.bottom - dest.top;
            ++frame_number;
            tiles.BeginFrame();
            if (!base)
                Check(dc->CreateBitmap(D2D1::SizeU(width, h), nullptr, 0,
                                       Properties(D2D1_BITMAP_OPTIONS_TARGET), &base),
                      "Cannot create native Direct2D table target");
            DrawOn(base.Get(), [&] {
                dc->Clear(D2D1::ColorF(0u));
                DrawNative(frame, h);
            });
            DrawOn(target, [&] {
                dc->Clear(D2D1::ColorF(frame.online_mode ? 0u : 0x172d2cu));
                if (window && frame.online_mode && (dw != area.right - area.left || dh != area.bottom - area.top))
                    DrawBitmap(backdrop.Get(), Box(0, 0, float(area.right - area.left), float(area.bottom - area.top)),
                               1, D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);
                dc->SetTransform(D2D1::Matrix3x2F::Translation(float(dest.left - area.left), float(dest.top - area.top)));
                if (dw > 0 && dh > 0) {
                    const auto interpolation = dw == width && dh == h ? D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR :
                        dw < width || dh < h ? D2D1_INTERPOLATION_MODE_MULTI_SAMPLE_LINEAR :
                                              D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC;
                    DrawBitmap(base.Get(), Box(0, 0, float(dw), float(dh)), 1, interpolation);
                    FinalScene text(art.Text(), width == 1024, frame.dpi ? frame.dpi : 96);
                    art.AppendHud(text, frame.state);
                    art.AppendChrome(text, frame.game_type, frame.interaction ? frame.interaction->automatic :
                                     std::array<bool, 4>{true, false, false, false}, frame.state.variant);
                    art.AppendStatus(text, frame.state);
                    if (frame.captions) AppendActionCaptions(text, *frame.captions, width, art.Text());
                    text.VisitText(width, h, dw, dh, [&](auto &renderer, const auto &context, const auto &run) {
                        DrawText(renderer, context, run);
                    });
                }
                dc->SetTransform(D2D1::Matrix3x2F::Translation(float(-area.left), float(-area.top)));
                for (const auto &button: frame.buttons) DrawButton(button, frame.dpi);
            });
            TrimCaches();
        }
    };

    D2DTableRenderer::D2DTableRenderer(bool warp) : impl_(std::make_unique<Impl>(warp)) {}
    D2DTableRenderer::~D2DTableRenderer() = default;

    void D2DTableRenderer::Configure(const std::filesystem::path &directory, int canvas_width) {
        impl_->directory = directory;
        impl_->width = canvas_width;
        impl_->tiles = {};
        impl_->art.Configure(directory, canvas_width);
        impl_->shadows = {};
        impl_->background = {};
        impl_->fallback.reset();
        impl_->ResetTextures();
    }

    void D2DTableRenderer::SetPreferences(const ClientPreferences &preferences, const std::filesystem::path &config,
                                          bool rebuild_background) {
        impl_->preferences = preferences;
        impl_->config = config;
        impl_->tiles.SetColor(preferences.TileColor());
        impl_->art.SetPanelEnabled((preferences.flags & 0x2000) == 0);
        if (rebuild_background) impl_->background = {};
        impl_->fallback.reset();
        impl_->ResetTextures();
    }

    void D2DTableRenderer::Prepare() {
        try {
            if (!impl_->tiles.width()) impl_->Geometry({});
            if (impl_->background.bgra.empty())
                impl_->background = BuildPreferredBackground(impl_->directory, impl_->config, impl_->width,
                                                             impl_->width * 3 / 4, impl_->preferences);
        } catch (const std::exception &) {
            // Report asset diagnostics when the frame is rendered.
        }
    }

    void D2DTableRenderer::Invalidate() {
        impl_->ResetTextures();
        if (impl_->fallback) impl_->fallback->Invalidate();
    }

    void D2DTableRenderer::SetReferenceRendering(bool enabled) {
        impl_->reference = enabled;
        if (impl_->fallback) impl_->fallback->SetReferenceRendering(enabled);
    }

    std::size_t D2DTableRenderer::HudCacheHits() const { return impl_->hud_hits; }

    TileGeometryCatalog &D2DTableRenderer::EnsureGeometry(const std::vector<int> &red_tiles) {
        return impl_->Geometry(red_tiles);
    }

    bool D2DTableRenderer::HasLoadedAssets() const {
        return impl_->tiles.width() != 0 && !impl_->background.bgra.empty();
    }

    bool D2DTableRenderer::RenderToWindow(HWND parent, const TableRenderFrame &frame) {
        const auto area = frame.bounds;
        if (impl_->reference || !frame.original_display || area.right <= area.left || area.bottom <= area.top) {
            HideWindowSurface();
            return false;
        }
        impl_->EnsureArtwork(frame);
        const bool presented = impl_->TryGraphics([&] {
            impl_->EnsureSwap(parent, area);
            impl_->Render(impl_->swap_target.Get(), frame, area, true);
            Check(impl_->swap->Present(1, 0), "Cannot present table swap chain");
            ++impl_->presented_frames;
        });
        if (!presented) HideWindowSurface();
        return presented;
    }

    void D2DTableRenderer::HideWindowSurface() {
        if (impl_->surface && IsWindow(impl_->surface)) ShowWindow(impl_->surface, SW_HIDE);
    }

    void D2DTableRenderer::RenderToDc(HDC dc, const TableRenderFrame &frame) {
        if (impl_->reference || !frame.original_display) {
            impl_->Fallback().RenderToDc(dc, frame);
            return;
        }
        impl_->EnsureArtwork(frame);
        const auto area = frame.bounds;
        if (area.right <= area.left || area.bottom <= area.top) return;
        if (!impl_->TryGraphics([&] {
            impl_->EnsureReadback(area.right - area.left, area.bottom - area.top);
            impl_->Render(impl_->offscreen_target.Get(), frame, area, false);
            const auto &r = frame.destination;
            const int saved = SaveDC(dc);
            if (!saved) throw std::runtime_error("Cannot save table capture DC");
            POINT origin{};
            GetViewportOrgEx(dc, &origin);
            const auto region = CreateRectRgn(r.left + origin.x, r.top + origin.y, r.right + origin.x, r.bottom + origin.y);
            if (!region) { RestoreDC(dc, saved); throw std::runtime_error("Cannot create table capture region"); }
            for (const auto &button: frame.buttons) {
                const auto &b = button.viewport_rect;
                if (const auto rect = CreateRectRgn(b.left + origin.x, b.top + origin.y, b.right + origin.x, b.bottom + origin.y)) {
                    CombineRgn(region, region, rect, RGN_OR);
                    DeleteObject(rect);
                }
            }
            ExtSelectClipRgn(dc, region, RGN_AND);
            DeleteObject(region);
            try { impl_->Blit(dc, area); } catch (...) { RestoreDC(dc, saved); throw; }
            RestoreDC(dc, saved);
            ++impl_->presented_frames;
        })) impl_->Fallback().RenderToDc(dc, frame);
    }

    void D2DTableRenderer::RenderButtonToDc(HDC dc, const TableActionButton &button, unsigned dpi) {
        const auto r = button.viewport_rect;
        if (r.right <= r.left || r.bottom <= r.top) return;
        if (!impl_->TryGraphics([&] {
            impl_->EnsureReadback(r.right - r.left, r.bottom - r.top);
            impl_->DrawOn(impl_->offscreen_target.Get(), [&] {
                impl_->dc->Clear(D2D1::ColorF(0u));
                impl_->dc->SetTransform(D2D1::Matrix3x2F::Translation(float(-r.left), float(-r.top)));
                impl_->DrawButton(button, dpi);
            });
            impl_->Blit(dc, r);
            impl_->TrimCaches();
        })) impl_->Fallback().RenderButtonToDc(dc, button, dpi);
    }

    std::size_t D2DTableRenderer::PresentedFrames() const { return impl_->presented_frames; }
    std::size_t D2DTableRenderer::DeviceGeneration() const { return impl_->device_generation; }
    void D2DTableRenderer::SimulateDeviceLoss() { impl_->reset_requested = true; }
} // namespace tenhou
