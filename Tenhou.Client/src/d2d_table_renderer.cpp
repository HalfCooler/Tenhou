#include "d2d_table_renderer.hpp"
#include "action_captions.hpp"
#include "background.hpp"
#include "gdi_table_renderer.hpp"
#include "hd_tile_atlas.hpp"
#include "hd_tile_pipeline.hpp"
#include "win32_utf.hpp"
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <d2d1effects.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <list>
#include <map>
#include <stdexcept>
#include <unordered_map>
#include <vector>

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

        struct TileKey {
            std::uint32_t image_key = 0;
            std::uintptr_t received = 0;
            int shadow_x = 0, shadow_y = 0;
            bool operator==(const TileKey &other) const {
                return image_key == other.image_key && received == other.received &&
                       shadow_x == other.shadow_x && shadow_y == other.shadow_y;
            }
        };

        struct TileKeyHash {
            std::size_t operator()(const TileKey &k) const noexcept {
                std::size_t h = std::hash<std::uint32_t>{}(k.image_key);
                h ^= std::hash<std::uintptr_t>{}(k.received) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
                h ^= std::hash<int>{}(k.shadow_x) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
                h ^= std::hash<int>{}(k.shadow_y) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
                return h;
            }
        };

        struct ShadowTileKey {
            char family = 0;
            int slot = 0, tile = 0, direction = 0, wall_extra = 0, x = 0, y = 0;
            const char *role = nullptr;
            bool operator==(const ShadowTileKey &o) const {
                return family == o.family && slot == o.slot && tile == o.tile &&
                       direction == o.direction && wall_extra == o.wall_extra &&
                       x == o.x && y == o.y &&
                       (role == o.role || (role && o.role && std::strcmp(role, o.role) == 0));
            }
        };

        struct BoardTileKey {
            char family = 0;
            int slot = 0, tile = 0, direction = 0, wall_extra = 0, x = 0, y = 0;
            HandColor color = HandColor::Normal;
            bool missing = false;
            bool operator==(const BoardTileKey &o) const {
                return family == o.family && slot == o.slot && tile == o.tile &&
                       direction == o.direction && wall_extra == o.wall_extra &&
                       x == o.x && y == o.y && color == o.color && missing == o.missing;
            }
        };
    }

    struct D2DTableRenderer::Impl {
        using Bitmap = ComPtr<ID2D1Bitmap1>;
        using Glyph = std::weak_ptr<const TextRasterPixels>;
        struct TileTexture { TileKey key; Bitmap bitmap; std::size_t bytes = 0, used = 0; };
        struct HudSprite { unsigned id; int x, y, w, h; std::array<float, 6> transform; };

        explicit Impl(bool warp) : warp_only(warp) {}
        ~Impl() {
            ResetDevice();
            if (surface && IsWindow(surface)) DestroyWindow(surface);
        }

        bool warp_only, reset_requested = false, reference = false, using_hardware = false;
        bool hd_active = false;
        int render_w = 1440, render_h = 1080;
        unsigned sync_interval = 1;
        BenchStats bench_stats{};
        std::filesystem::path directory, config;
        int width = 1440;
        ClientPreferences preferences;
        TableArtwork tiles;
        DisplayArtwork art;
        TileShadowLayer shadows;
        Image background;
        HdTilePipeline hd_pipeline;
        std::unique_ptr<GdiTableRenderer> fallback;
        std::size_t hud_hits = 0, presented_frames = 0, device_generation = 0;

        ComPtr<ID3D11Device> d3d;
        ComPtr<ID3D11DeviceContext> immediate;
        ComPtr<IDXGIFactory2> dxgi;
        ComPtr<ID2D1DeviceContext> dc;
        ComPtr<ID2D1SolidColorBrush> brush;
        ComPtr<ID2D1StrokeStyle> focus_stroke;
        std::array<ComPtr<ID2D1LinearGradientBrush>, 4> auto_gradients;
        int auto_gradient_w = 0, auto_gradient_h = 0;
        std::array<ComPtr<ID2D1Effect>, std::size(hand_colors)> effects;

        std::list<TileTexture> tile_lru;
        std::unordered_map<TileKey, std::list<TileTexture>::iterator, TileKeyHash> tile_index;
        std::unordered_map<std::uint32_t, Bitmap> body_textures;
        std::unordered_map<unsigned, Bitmap> sprites;
        std::unordered_map<unsigned, Bitmap> hd_sprites;
        std::unordered_map<const TextRasterPixels *, std::pair<Glyph, Bitmap>> glyphs;
        std::map<std::tuple<int, int, int, bool, bool, bool, bool, unsigned, std::string>, Bitmap> legacy_buttons;
        std::size_t tile_bytes = 0, frame_number = 0;
        std::vector<ID2D1Bitmap1 *> resolved_tiles;
        std::vector<ID2D1Bitmap1 *> resolved_dora;

        Bitmap backdrop, shadow_texture, static_board, base, hud_text;
        Bitmap hint_texture, countdown_texture;
        int hint_x = 0, hint_y = 0, countdown_x = 0, countdown_y = 0;
        std::string hint_stamp, countdown_stamp;
        std::size_t shadow_revision = 0, shadow_generation = 0;
        std::vector<HudSprite> hud_sprites;
        std::string hud_stamp;

        // Cached shadow calculation state (reused across frames without reallocations).
        bool shadow_valid = false;
        int cached_shadow_strength = 0;
        std::vector<ShadowTileKey> shadow_tile_keys, scratch_shadow_keys;
        std::vector<TileShadow> cached_shadows;
        std::vector<const TileShadow *> tile_shadow_ptrs;

        // Cached static_board layer (backdrop + shadow_texture + stationary tiles [0, split_idx)).
        bool static_board_valid = false;
        std::size_t static_board_shadow_gen = 0;
        bool static_board_red = false, static_board_online = false, static_board_has_shadow = false;
        std::vector<BoardTileKey> static_board_tiles, scratch_static_tiles;

        // Cached base idle-frame fingerprint (entire DrawNative output when no flights).
        bool base_valid = false;
        std::size_t base_shadow_gen = 0;
        bool base_red = false, base_online = false, base_has_shadow = false, base_wall_known = false;
        int base_hovered = -1;
        std::vector<int> base_dora;
        std::string base_hud_stamp, base_hint_stamp, base_countdown_stamp;
        std::vector<BoardTileKey> base_tail_tiles, scratch_tail_tiles;

        ComPtr<ID3D11Texture2D> offscreen, staging;
        Bitmap offscreen_target;
        int read_width = 0, read_height = 0;
        Bytes read_pixels;
        HWND surface = nullptr;
        ComPtr<IDXGISwapChain1> swap;
        HANDLE frame_waitable = nullptr;
        UINT swap_flags = 0;
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

        void CloseFrameWaitable() {
            if (frame_waitable) {
                CloseHandle(frame_waitable);
                frame_waitable = nullptr;
            }
        }

        void ResetTextures() {
            effects = {};
            auto_gradients = {};
            auto_gradient_w = auto_gradient_h = 0;
            tile_index.clear(); tile_lru.clear(); tile_bytes = 0;
            body_textures.clear();
            sprites.clear();
            hd_sprites.clear();
            glyphs.clear();
            legacy_buttons.clear();
            backdrop.Reset(); shadow_texture.Reset(); shadow_revision = 0;
            static_board.Reset(); base.Reset();
            hud_text.Reset(); hud_sprites.clear(); hud_stamp.clear();
            hint_texture.Reset(); hint_stamp.clear();
            countdown_texture.Reset(); countdown_stamp.clear();
            shadow_valid = false; shadow_tile_keys.clear(); cached_shadows.clear();
            static_board_valid = false; static_board_tiles.clear();
            base_valid = false; base_tail_tiles.clear();
        }

        void ResetDevice() {
            if (dc) dc->SetTarget(nullptr);
            hd_pipeline.Reset();
            hd_active = false;
            ResetTextures();
            offscreen_target.Reset(); offscreen.Reset(); staging.Reset(); read_width = read_height = 0;
            CloseFrameWaitable();
            swap_target.Reset(); swap.Reset(); swap_width = swap_height = 0; swap_flags = 0;
            focus_stroke.Reset();
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
            using_hardware = (driver == D3D_DRIVER_TYPE_HARDWARE);
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
            const float dashes[]{1.0f, 1.0f};
            Check(factory->CreateStrokeStyle(
                      D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT,
                                                  D2D1_LINE_JOIN_MITER, 10.0f, D2D1_DASH_STYLE_CUSTOM, 0.0f),
                      dashes, 2, &focus_stroke),
                  "Cannot create Direct2D focus stroke style");
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
            ++bench_stats.texture_uploads;
            bench_stats.texture_upload_bytes += static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4;
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
                CloseFrameWaitable();
                swap_target.Reset(); swap.Reset(); swap_flags = 0; surface_bounds = {};
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
                desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
                HRESULT hr = dxgi->CreateSwapChainForHwnd(d3d.Get(), surface, &desc, nullptr, nullptr, &swap);
                if (SUCCEEDED(hr)) {
                    swap_flags = desc.Flags;
                    ComPtr<IDXGISwapChain2> swap2;
                    if (SUCCEEDED(swap.As(&swap2))) {
                        swap2->SetMaximumFrameLatency(1);
                        frame_waitable = swap2->GetFrameLatencyWaitableObject();
                    }
                } else {
                    desc.Flags = 0;
                    swap_flags = 0;
                    Check(dxgi->CreateSwapChainForHwnd(d3d.Get(), surface, &desc, nullptr, nullptr, &swap),
                          "Cannot create flip-model table swap chain");
                }
            } else if (swap_width != w || swap_height != h) {
                swap_target.Reset();
                immediate->Flush();
                Check(swap->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, swap_flags),
                      "Cannot resize table swap chain");
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

        ID2D1Bitmap1 *SpriteHd(unsigned id) {
            auto &texture = hd_sprites[id];
            if (!texture) texture = Upload(art.GetHd(id));
            return texture.Get();
        }

        ID2D1Bitmap1 *TileBodyBitmap(const TableTile &tile) {
            const auto raw_key = tiles.geometry().ImageKey(tile, false);
            const unsigned variant = (raw_key >> 16) & 0xff;
            const unsigned body_variant = (variant == 37) ? 37u : 34u;
            const std::uint32_t body_key = (raw_key & 0xff00ffffu) | (body_variant << 16);
            auto &bmp = body_textures[body_key];
            if (!bmp) bmp = Upload(tiles.GetBody(tile).image);
            return bmp.Get();
        }

        ID2D1Bitmap1 *Tile(const TableTile &tile, bool red, const ShadowMask *received = nullptr,
                           int shadow_x = 0, int shadow_y = 0) {
            const TileKey key{tiles.geometry().ImageKey(tile, red),
                              reinterpret_cast<std::uintptr_t>(received), shadow_x, shadow_y};
            if (const auto it = tile_index.find(key); it != tile_index.end()) {
                it->second->used = frame_number;
                tile_lru.splice(tile_lru.begin(), tile_lru, it->second);
                return it->second->bitmap.Get();
            }
            const auto &image = tiles.Get(tile, red).image;
            Bitmap bitmap = received ? Upload(ShadeTile(image, shadow_x, shadow_y, *received, preferences.tile_shadow))
                                     : Upload(image);
            const std::size_t bytes = image.bgra.size();
            tile_lru.push_front({key, std::move(bitmap), bytes, frame_number});
            tile_index.emplace(key, tile_lru.begin());
            tile_bytes += bytes;
            return tile_lru.front().bitmap.Get();
        }

        // Composes a target-resolution HD tile texture (blank body + D3D11 4-point
        // projective homography SVG face + optional received shadow modulation).
        // Must be called OUTSIDE any active dc->BeginDraw() block.
        ID2D1Bitmap1 *TileHd(const TableTile &tile, bool red, float sx, float sy,
                             const ShadowMask *received = nullptr, int shadow_x = 0, int shadow_y = 0) {
            const std::uint32_t img_key = tiles.geometry().ImageKey(tile, red);
            const TileKey key{img_key, reinterpret_cast<std::uintptr_t>(received), shadow_x, shadow_y};
            if (const auto it = tile_index.find(key); it != tile_index.end()) {
                it->second->used = frame_number;
                tile_lru.splice(tile_lru.begin(), tile_lru, it->second);
                return it->second->bitmap.Get();
            }

            const unsigned variant = (img_key >> 16) & 0xff;
            const int slice = HdTileFaceSlice(variant);
            const HdTileFaceQuad *quad = (slice >= 0) ? FindHdTileFaceQuad(tile.family, tile.slot, tile.direction)
                                                      : nullptr;
            const auto &body_img = tiles.GetBody(tile).image;
            const int tw = std::max(1, static_cast<int>(std::ceil(body_img.width * sx)));
            const int th = std::max(1, static_cast<int>(std::ceil(body_img.height * sy)));

            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = static_cast<UINT>(tw);
            desc.Height = static_cast<UINT>(th);
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = format;
            desc.SampleDesc.Count = 1;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

            ComPtr<ID3D11Texture2D> tex;
            Check(d3d->CreateTexture2D(&desc, nullptr, &tex), "Cannot allocate HD tile texture");
            ComPtr<IDXGISurface> dxgi_surface;
            Check(tex.As(&dxgi_surface), "Cannot query HD tile DXGI surface");
            Bitmap tile_bmp;
            Check(dc->CreateBitmapFromDxgiSurface(
                      dxgi_surface.Get(), Properties(D2D1_BITMAP_OPTIONS_TARGET), &tile_bmp),
                  "Cannot bind HD tile bitmap");

            ID2D1Bitmap1 *body_bmp = TileBodyBitmap(tile);
            DrawOn(tile_bmp.Get(), [&] {
                dc->Clear(D2D1::ColorF(0, 0, 0, 0));
                dc->DrawBitmap(body_bmp, Box(0.0f, 0.0f, body_img.width * sx, body_img.height * sy),
                               1.0f, D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);
            });

            if (quad) {
                ComPtr<ID3D11RenderTargetView> rtv;
                Check(d3d->CreateRenderTargetView(tex.Get(), nullptr, &rtv), "Cannot create HD tile RTV");
                hd_pipeline.DrawPerspectiveFace(immediate.Get(), rtv.Get(), tw, th, slice, *quad, sx, sy);
            } else if (variant != 34 && variant != 37) {
                // Unexpected slot without quad entry: fall back to classic sprite upscaled.
                auto classic_bmp = Upload(tiles.Get(tile, red).image);
                DrawOn(tile_bmp.Get(), [&] {
                    dc->Clear(D2D1::ColorF(0, 0, 0, 0));
                    dc->DrawBitmap(classic_bmp.Get(), Box(0.0f, 0.0f, body_img.width * sx, body_img.height * sy),
                                   1.0f, D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);
                });
            }

            if (received && preferences.tile_shadow > 0) {
                const auto palette = OriginalShadowPalette(preferences.tile_shadow);
                Bytes shadow_bgra(static_cast<std::size_t>(body_img.width) * body_img.height * 4, 0);
                bool any_shadow = false;
                for (int row = std::max(0, received->y - shadow_y);
                     row < std::min(body_img.height, received->y + received->height - shadow_y); ++row) {
                    for (int col = std::max(0, received->x - shadow_x);
                         col < std::min(body_img.width, received->x + received->width - shadow_x); ++col) {
                        const unsigned alpha =
                            palette[received->pixels[static_cast<std::size_t>(row + shadow_y - received->y) *
                                                         received->stride +
                                                     col + shadow_x - received->x] +
                                    2] >>
                            24;
                        if (alpha) {
                            shadow_bgra[(static_cast<std::size_t>(row) * body_img.width + col) * 4 + 3] =
                                static_cast<std::uint8_t>(alpha);
                            any_shadow = true;
                        }
                    }
                }
                if (any_shadow) {
                    auto shadow_bmp = Upload(shadow_bgra.data(), body_img.width, body_img.height);
                    DrawOn(tile_bmp.Get(), [&] {
                        dc->SetTransform(D2D1::Matrix3x2F::Scale(sx, sy));
                        dc->DrawImage(static_cast<ID2D1Image *>(shadow_bmp.Get()), nullptr, nullptr,
                                      D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC,
                                      D2D1_COMPOSITE_MODE_SOURCE_ATOP);
                    });
                }
            }

            const std::size_t bytes = static_cast<std::size_t>(tw) * static_cast<std::size_t>(th) * 4;
            tile_lru.push_front({key, std::move(tile_bmp), bytes, frame_number});
            tile_index.emplace(key, tile_lru.begin());
            tile_bytes += bytes;
            return tile_lru.front().bitmap.Get();
        }

        void DrawTile(ID2D1Bitmap1 *bitmap, float x, float y, HandColor color) {
            if (color == HandColor::Normal) {
                const auto size = bitmap->GetPixelSize();
                DrawBitmap(bitmap, Box(x, y, float(size.width), float(size.height)));
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
            dc->DrawImage(effect.Get(), D2D1::Point2F(x, y), D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
        }

        void DrawText(TextRenderer &renderer, const TextRenderContext &context, const SceneTextRun &run) {
            const auto raster = renderer.Rasterize(context, run);
            if (!raster.pixels) return;
            const auto *raw = raster.pixels.get();
            auto &entry = glyphs[raw];
            if (entry.first.expired() || !entry.second) {
                entry.first = Glyph(raster.pixels);
                entry.second = Upload(raster.pixels->bgra.data(), raster.pixels->width, raster.pixels->height);
            }
            std::optional<AxisClip> clip;
            if (context.device_clip) {
                const auto &r = *context.device_clip;
                clip.emplace(*dc.Get(), Box(r.X, r.Y, r.Width, r.Height));
            }
            DrawBitmap(entry.second.Get(), raster.bounds.X, raster.bounds.Y, float(std::clamp(run.opacity, 0.0, 1.0)));
        }

        void PreResolveTiles(const TableRenderFrame &frame, std::size_t begin, std::size_t end, float sx, float sy) {
            if (resolved_tiles.size() < frame.tiles.size()) resolved_tiles.resize(frame.tiles.size(), nullptr);
            for (std::size_t i = begin; i < end; ++i) {
                const auto &placed = frame.tiles[i];
                if (placed.flight) continue;
                const bool missing = frame.online_mode && std::string_view(placed.tile.role) == "missing_suit";
                const bool red = missing || frame.red_tiles;
                const auto *shadow = i < tile_shadow_ptrs.size() ? tile_shadow_ptrs[i] : nullptr;
                const ShadowMask *received = shadow ? shadow->received : nullptr;
                const int shx = shadow ? placed.x - shadow->dx : 0;
                const int shy = shadow ? placed.y - shadow->dy : 0;
                resolved_tiles[i] = hd_active ? TileHd(placed.tile, red, sx, sy, received, shx, shy)
                                              : Tile(placed.tile, red, received, shx, shy);
            }
        }

        void PreResolveFlightsAndDora(const TableRenderFrame &frame, float sx, float sy) {
            if (resolved_tiles.size() < frame.tiles.size()) resolved_tiles.resize(frame.tiles.size(), nullptr);
            for (const auto index: TableFlightOrder(frame.tiles)) {
                const auto &placed = frame.tiles[index];
                const auto &flight = *placed.flight;
                resolved_tiles[index] = hd_active ? TileHd(flight.sprite, frame.red_tiles, sx, sy)
                                                  : Tile(flight.sprite, frame.red_tiles);
                if (!flight.cover_landing) continue;
                for (std::size_t i = index + 1; i < frame.tiles.size(); ++i) {
                    const auto &cover = frame.tiles[i];
                    if (cover.flight) continue;
                    // Ensure unshaded face texture exists in cache for landing cover pass.
                    if (hd_active) (void) TileHd(cover.tile, frame.red_tiles, sx, sy);
                    else (void) Tile(cover.tile, frame.red_tiles);
                }
            }
            resolved_dora.clear();
            if (!frame.state.wall.known) {
                resolved_dora.reserve(frame.state.dora_indicators.size());
                for (const int dora: frame.state.dora_indicators) {
                    const TableTile t{'k', 0, dora, 0};
                    resolved_dora.push_back(hd_active ? TileHd(t, frame.red_tiles, sx, sy)
                                                      : Tile(t, frame.red_tiles));
                }
            }
        }

        void DrawFlights(const TableRenderFrame &frame, float sx, float sy) {
            for (const auto index: TableFlightOrder(frame.tiles)) {
                const auto &placed = frame.tiles[index];
                const auto &flight = *placed.flight;
                auto *bitmap = resolved_tiles[index];
                const auto bsz = bitmap->GetPixelSize();
                for (unsigned i = 0; i < flight.count; ++i)
                    DrawBitmap(bitmap,
                               Box((placed.x + flight.offsets[i].x) * sx,
                                   (placed.y + flight.offsets[i].y) * sy,
                                   float(bsz.width), float(bsz.height)),
                               flight.opacity);
                if (!flight.cover_landing) continue;
                const auto &native_img = tiles.GetBody(flight.sprite).image;
                const auto bounds = TileBounds(placed, native_img.width, native_img.height);
                const auto landing = TileBounds({placed.tile, flight.landing_x, flight.landing_y, flight},
                                                native_img.width, native_img.height);
                RECT patch{};
                if (!IntersectRect(&patch, &landing, &bounds)) continue;
                AxisClip clip(*dc.Get(),
                              Box(patch.left * sx, patch.top * sy,
                                  (patch.right - patch.left) * sx, (patch.bottom - patch.top) * sy));
                for (std::size_t i = index + 1; i < frame.tiles.size(); ++i) {
                    const auto &cover = frame.tiles[i];
                    if (cover.flight) continue;
                    // Look up pre-resolved unshaded tile (guaranteed cached by PreResolveFlightsAndDora).
                    const TileKey ckey{tiles.geometry().ImageKey(cover.tile, frame.red_tiles), 0, 0, 0};
                    const auto cit = tile_index.find(ckey);
                    if (cit == tile_index.end()) continue;
                    auto *face = cit->second->bitmap.Get();
                    const auto &c_native = tiles.GetBody(cover.tile).image;
                    if (Overlaps(patch, TileBounds(cover, c_native.width, c_native.height))) {
                        const auto fsz = face->GetPixelSize();
                        DrawBitmap(face, Box(cover.x * sx, cover.y * sy, float(fsz.width), float(fsz.height)));
                    }
                }
            }
        }

        void EnsureAutoGradients(int rw, int rh, float sx, float sy) {
            if (auto_gradients[0] && auto_gradient_w == rw && auto_gradient_h == rh) return;
            const D2D1_GRADIENT_STOP stops[]{
                {0.0f, D2D1::ColorF(0u, 0.0f)},
                {1.0f, D2D1::ColorF(0u, float(0x22) / 255.0f)},
            };
            ComPtr<ID2D1GradientStopCollection> collection;
            Check(dc->CreateGradientStopCollection(stops, 2, &collection), "Cannot create auto button gradient stops");
            for (int i = 0; i < 4; ++i) {
                const RECT r = OriginalAutoButtonRect(width, i);
                auto_gradients[i].Reset();
                Check(dc->CreateLinearGradientBrush(
                          D2D1::LinearGradientBrushProperties(D2D1::Point2F(r.left * sx, r.top * sy),
                                                              D2D1::Point2F(r.left * sx, r.bottom * sy)),
                          collection.Get(), &auto_gradients[i]),
                      "Cannot create auto button gradient brush");
            }
            auto_gradient_w = rw;
            auto_gradient_h = rh;
        }

        void DrawAutoChrome(int hovered, int rw, int rh, float sx, float sy) {
            EnsureAutoGradients(rw, rh, sx, sy);
            const float stroke = hd_active ? std::max(1.0f, std::min(sx, sy)) : 1.0f;
            for (int i = 0; i < 4; ++i) {
                const RECT r = OriginalAutoButtonRect(width, i);
                const float x0 = r.left * sx, y0 = r.top * sy, x1 = r.right * sx, y1 = r.bottom * sy;
                dc->FillRectangle(D2D1::RectF(x0, y0, x1, y1), auto_gradients[i].Get());
                brush->SetColor(hovered == i ? D2D1::ColorF(0xff9933u, 1.0f)
                                             : D2D1::ColorF(0u, float(0x11) / 255.0f));
                dc->DrawRectangle(D2D1::RectF(x0 + stroke * 0.5f, y0 + stroke * 0.5f,
                                              x1 - stroke * 0.5f, y1 - stroke * 0.5f),
                                  brush.Get(), stroke);
            }
        }

        void PrepareHud(const TableRenderFrame &frame, int rw, int rh, float sx, float sy, const std::string &metadata) {
            if (hud_text && metadata == hud_stamp) { ++hud_hits; return; }
            ++bench_stats.hud_rebuilds;

            Bytes pixels(static_cast<std::size_t>(rw) * rh * 4);
            Gdiplus::Bitmap bitmap(rw, rh, rw * 4, PixelFormat32bppPARGB, pixels.data());
            Gdiplus::Graphics g(&bitmap);
            g.SetPageUnit(Gdiplus::UnitPixel);
            const float hsx = hd_active ? sx : 1.0f;
            const float hsy = hd_active ? sy : 1.0f;
            std::vector<HudSprite> commands;
            art.PaintHudImages(g, frame.state, [&](unsigned id, const Image &im, int x, int y, const auto &t) {
                commands.push_back({id, x, y, im.width, im.height, t});
            }, hsx, hsy);
            art.PaintNukiCounts(g, frame.state, tiles, hsx, hsy);
            g.Flush(Gdiplus::FlushIntentionSync);
            hud_text = Upload(pixels.data(), rw, rh);
            hud_sprites = std::move(commands);
            hud_stamp = metadata;
        }

        template<class Draw>
        Bitmap RenderOverlayBox(int box_x, int box_y, int box_w, int box_h, Draw draw) {
            if (box_w <= 0 || box_h <= 0) return {};
            Dib black(box_w, box_h), white(box_w, box_h);
            PatBlt(black.dc, 0, 0, box_w, box_h, BLACKNESS);
            PatBlt(white.dc, 0, 0, box_w, box_h, WHITENESS);
            SetViewportOrgEx(black.dc, -box_x, -box_y, nullptr);
            SetViewportOrgEx(white.dc, -box_x, -box_y, nullptr);
            draw(black.dc);
            draw(white.dc);
            GdiFlush();
            const auto bytes = static_cast<std::size_t>(box_w) * box_h * 4;
            for (std::size_t i = 0; i < bytes; i += 4)
                black.pixels[i + 3] = static_cast<std::uint8_t>(
                    255 - std::clamp(int(white.pixels[i]) - int(black.pixels[i]), 0, 255));
            return Upload(black.pixels, box_w, box_h);
        }

        void PrepareInteraction(const TableRenderFrame &frame, int h,
                                std::string &active_hint, std::string &active_countdown) {
            active_hint.clear();
            active_countdown.clear();
            if (!frame.interaction) return;
            const auto &input = *frame.interaction;
            const double unit = double(width) / 1024.0;
            if (!input.hint.empty()) {
                active_hint = input.hint;
                if (!hint_texture || hint_stamp != active_hint) {
                    ++bench_stats.interaction_rebuilds;
                    const RECT rect{width / 8, h - static_cast<int>(52 * unit),
                                    width * 7 / 8, h - static_cast<int>(28 * unit)};
                    const int bx = std::max(0, static_cast<int>(rect.left) - 2);
                    const int by = std::max(0, static_cast<int>(rect.top) - 2);
                    const int br = std::min(width, static_cast<int>(rect.right) + 2);
                    const int bb = std::min(h, static_cast<int>(rect.bottom) + 2);
                    const auto wide = Utf8ToWide(input.hint);
                    const auto font = CreateFontW(-static_cast<int>(14 * unit), 0, 0, 0, FW_NORMAL,
                                                  FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                                  CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"KaiTi");
                    hint_texture = RenderOverlayBox(bx, by, br - bx, bb - by, [&](HDC hdc) {
                        const auto old = SelectObject(hdc, font);
                        SetBkMode(hdc, TRANSPARENT);
                        SetTextColor(hdc, RGB(255, 242, 190));
                        RECT r = rect;
                        DrawTextW(hdc, wide.c_str(), -1, &r, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
                        SelectObject(hdc, old);
                    });
                    if (font) DeleteObject(font);
                    hint_x = bx; hint_y = by;
                    hint_stamp = active_hint;
                }
            }
            if (!input.countdown.empty()) {
                int right = 0, top = 0;
                for (const auto &tile: frame.tiles) {
                    if (tile.tile.direction == 0 && std::string_view(tile.tile.role) == "hand") {
                        const auto &image = tiles.Get(tile.tile, frame.red_tiles).image;
                        if (tile.x + image.width > right) {
                            right = tile.x + image.width;
                            top = tile.y;
                        }
                    }
                }
                active_countdown = input.countdown + ':' + std::to_string(right) + ',' + std::to_string(top);
                if (!countdown_texture || countdown_stamp != active_countdown) {
                    ++bench_stats.interaction_rebuilds;
                    const auto clock_font = CreateFontW(
                        -std::max(8, static_cast<int>(9 * unit)), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
                        DEFAULT_PITCH, L"Arial");
                    const auto wide = Utf8ToWide(input.countdown);
                    const int length = static_cast<int>(wide.size());
                    SIZE size{};
                    if (HDC measure = CreateCompatibleDC(nullptr)) {
                        const auto old = SelectObject(measure, clock_font ? clock_font : GetStockObject(DEFAULT_GUI_FONT));
                        GetTextExtentPoint32W(measure, wide.c_str(), length, &size);
                        SelectObject(measure, old);
                        DeleteDC(measure);
                    }
                    const int x = std::min(right + std::max(2, static_cast<int>(2 * unit)),
                                           width - static_cast<int>(size.cx) - 3);
                    const int bx = std::max(0, x - 2);
                    const int by = std::max(0, top - 2);
                    const int br = std::min(width, x + static_cast<int>(size.cx) + 3);
                    const int bb = std::min(h, top + static_cast<int>(size.cy) + 3);
                    countdown_texture = RenderOverlayBox(bx, by, br - bx, bb - by, [&](HDC hdc) {
                        const auto old = SelectObject(hdc, clock_font ? clock_font : GetStockObject(DEFAULT_GUI_FONT));
                        SetBkMode(hdc, TRANSPARENT);
                        SetTextColor(hdc, RGB(0, 0, 0));
                        TextOutW(hdc, x - 1, top, wide.c_str(), length);
                        TextOutW(hdc, x + 1, top, wide.c_str(), length);
                        TextOutW(hdc, x, top - 1, wide.c_str(), length);
                        TextOutW(hdc, x, top + 1, wide.c_str(), length);
                        SetTextColor(hdc, RGB(255, 255, 255));
                        TextOutW(hdc, x, top, wide.c_str(), length);
                        SelectObject(hdc, old);
                    });
                    if (clock_font) DeleteObject(clock_font);
                    countdown_x = bx; countdown_y = by;
                    countdown_stamp = active_countdown;
                }
            }
        }

        void UpdateShadows(const TableRenderFrame &frame) {
            if (!preferences.tile_shadow) {
                if (shadow_valid) {
                    cached_shadows.clear();
                    shadow_tile_keys.clear();
                    shadow_valid = false;
                    ++shadow_generation;
                }
                tile_shadow_ptrs.assign(frame.tiles.size(), nullptr);
                return;
            }
            scratch_shadow_keys.clear();
            scratch_shadow_keys.reserve(frame.tiles.size());
            for (const auto &placed: frame.tiles) {
                if (placed.flight) continue;
                scratch_shadow_keys.push_back({placed.tile.family, placed.tile.slot, placed.tile.tile,
                                               placed.tile.direction, placed.tile.wall_extra,
                                               placed.x, placed.y, placed.tile.role});
            }
            if (!shadow_valid || cached_shadow_strength != preferences.tile_shadow ||
                scratch_shadow_keys != shadow_tile_keys) {
                ++bench_stats.shadow_rebuilds;
                cached_shadows = BuildTileShadows(frame.tiles, tiles, ShadowPack::Embedded());
                shadow_tile_keys = scratch_shadow_keys;
                cached_shadow_strength = preferences.tile_shadow;
                shadow_valid = true;
                ++shadow_generation;
                if (const auto *image = shadows.Raster(cached_shadows, preferences.tile_shadow)) {
                    if (!shadow_texture || shadow_revision != shadows.revision()) {
                        const auto dirty = shadows.last_dirty();
                        if (shadow_texture && shadow_revision + 1 == shadows.revision() &&
                            dirty.right > dirty.left && dirty.bottom > dirty.top &&
                            dirty.left >= 0 && dirty.top >= 0 &&
                            dirty.right <= image->width && dirty.bottom <= image->height) {
                            const D2D1_RECT_U dst{
                                static_cast<UINT32>(dirty.left), static_cast<UINT32>(dirty.top),
                                static_cast<UINT32>(dirty.right), static_cast<UINT32>(dirty.bottom)};
                            const UINT32 pitch = static_cast<UINT32>(image->width * 4);
                            const auto *src = image->bgra.data() +
                                              (static_cast<std::size_t>(dirty.top) * image->width + dirty.left) * 4;
                            Check(shadow_texture->CopyFromMemory(&dst, src, pitch), "Cannot update tile shadow texture");
                            ++bench_stats.texture_uploads;
                            bench_stats.texture_upload_bytes +=
                                static_cast<std::size_t>(dirty.right - dirty.left) *
                                static_cast<std::size_t>(dirty.bottom - dirty.top) * 4;
                        } else {
                            shadow_texture = Upload(image->bgra.data(), image->width, image->height);
                        }
                        shadow_revision = shadows.revision();
                    }
                } else {
                    shadow_texture.Reset();
                }
            } else if (!shadow_texture) {
                if (const auto *image = shadows.Raster(cached_shadows, preferences.tile_shadow)) {
                    shadow_texture = Upload(image->bgra.data(), image->width, image->height);
                    shadow_revision = shadows.revision();
                }
            }
            tile_shadow_ptrs.assign(frame.tiles.size(), nullptr);
            std::size_t s = 0;
            for (std::size_t i = 0; i < frame.tiles.size() && s < cached_shadows.size(); ++i) {
                const auto &placed = frame.tiles[i];
                if (placed.flight) continue;
                const auto &sh = cached_shadows[s].placed;
                if (sh.tile.family == placed.tile.family && sh.tile.slot == placed.tile.slot &&
                    sh.tile.direction == placed.tile.direction && sh.tile.wall_extra == placed.tile.wall_extra &&
                    sh.x == placed.x && sh.y == placed.y) {
                    tile_shadow_ptrs[i] = &cached_shadows[s];
                    ++s;
                }
            }
        }

        void DrawStationaryRange(const TableRenderFrame &frame, std::size_t begin, std::size_t end,
                                 float sx, float sy) {
            for (std::size_t i = begin; i < end; ++i) {
                const auto &placed = frame.tiles[i];
                if (placed.flight) continue;
                const bool missing = frame.online_mode && std::string_view(placed.tile.role) == "missing_suit";
                const auto color = i < frame.tile_colors.size() ? frame.tile_colors[i] : HandColor::Normal;
                auto *bitmap = resolved_tiles[i];
                const float x = placed.x * sx, y = placed.y * sy;
                DrawTile(bitmap, x, y, color);
                if (missing) {
                    const auto size = bitmap->GetPixelSize();
                    Fill(Box(x, y, float(size.width), float(size.height)),
                         D2D1::ColorF(0x5a5a5au, 145.f / 255));
                }
            }
        }

        void EnsureBase(const TableRenderFrame &frame, int h) {
            if (!backdrop) backdrop = Upload(background);
            UpdateShadows(frame);

            const int rw = render_w, rh = render_h;
            const float sx = float(rw) / float(width);
            const float sy = float(rh) / float(h);

            bool has_flights = false;
            std::size_t split_idx = frame.tiles.size();
            for (std::size_t i = 0; i < frame.tiles.size(); ++i) {
                const auto &placed = frame.tiles[i];
                if (placed.flight) { has_flights = true; continue; }
                if (split_idx == frame.tiles.size() && placed.tile.direction == 0 &&
                    std::string_view(placed.tile.role) == "hand") {
                    split_idx = i;
                }
            }

            scratch_static_tiles.clear();
            scratch_static_tiles.reserve(split_idx);
            for (std::size_t i = 0; i < split_idx; ++i) {
                const auto &placed = frame.tiles[i];
                if (placed.flight) continue;
                const bool missing = frame.online_mode && std::string_view(placed.tile.role) == "missing_suit";
                const auto color = i < frame.tile_colors.size() ? frame.tile_colors[i] : HandColor::Normal;
                scratch_static_tiles.push_back({placed.tile.family, placed.tile.slot, placed.tile.tile,
                                                placed.tile.direction, placed.tile.wall_extra,
                                                placed.x, placed.y, color, missing});
            }

            scratch_tail_tiles.clear();
            scratch_tail_tiles.reserve(frame.tiles.size() - split_idx);
            for (std::size_t i = split_idx; i < frame.tiles.size(); ++i) {
                const auto &placed = frame.tiles[i];
                if (placed.flight) continue;
                const bool missing = frame.online_mode && std::string_view(placed.tile.role) == "missing_suit";
                const auto color = i < frame.tile_colors.size() ? frame.tile_colors[i] : HandColor::Normal;
                scratch_tail_tiles.push_back({placed.tile.family, placed.tile.slot, placed.tile.tile,
                                              placed.tile.direction, placed.tile.wall_extra,
                                              placed.x, placed.y, color, missing});
            }

            const bool has_shadow = preferences.tile_shadow != 0 && shadow_texture != nullptr;
            const auto metadata = TableMetadata(frame.state);
            const int hovered = frame.interaction ? frame.interaction->hovered_button : -1;
            std::string active_hint, active_countdown;
            PrepareInteraction(frame, h, active_hint, active_countdown);
            PrepareHud(frame, rw, rh, sx, sy, metadata);

            const bool static_hit = static_board && static_board_valid &&
                                    static_board_shadow_gen == shadow_generation &&
                                    static_board_red == frame.red_tiles &&
                                    static_board_online == frame.online_mode &&
                                    static_board_has_shadow == has_shadow &&
                                    static_board_tiles == scratch_static_tiles;

            const bool base_hit = !has_flights && base && base_valid && static_hit &&
                                  base_shadow_gen == shadow_generation &&
                                  base_red == frame.red_tiles &&
                                  base_online == frame.online_mode &&
                                  base_has_shadow == has_shadow &&
                                  base_wall_known == frame.state.wall.known &&
                                  (frame.state.wall.known || base_dora == frame.state.dora_indicators) &&
                                  base_hovered == hovered &&
                                  base_hud_stamp == metadata &&
                                  base_hint_stamp == active_hint &&
                                  base_countdown_stamp == active_countdown &&
                                  base_tail_tiles == scratch_tail_tiles;
            if (base_hit) {
                ++bench_stats.static_base_hits;
                return;
            }

            if (!static_hit) {
                PreResolveTiles(frame, 0, split_idx, sx, sy);
                if (!static_board)
                    Check(dc->CreateBitmap(D2D1::SizeU(rw, rh), nullptr, 0,
                                           Properties(D2D1_BITMAP_OPTIONS_TARGET), &static_board),
                          "Cannot create static board Direct2D target");
                ++bench_stats.static_base_rebuilds;
                DrawOn(static_board.Get(), [&] {
                    if (hd_active && (rw != width || rh != h)) {
                        DrawBitmap(backdrop.Get(), Box(0, 0, float(rw), float(rh)), 1.0f,
                                   D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);
                        if (has_shadow)
                            DrawBitmap(shadow_texture.Get(), Box(0, 0, float(rw), float(rh)), 1.0f,
                                       D2D1_INTERPOLATION_MODE_LINEAR);
                    } else {
                        DrawBitmap(backdrop.Get(), 0, 0);
                        if (has_shadow) DrawBitmap(shadow_texture.Get(), 0, 0);
                    }
                    DrawStationaryRange(frame, 0, split_idx, sx, sy);
                });
                static_board_shadow_gen = shadow_generation;
                static_board_red = frame.red_tiles;
                static_board_online = frame.online_mode;
                static_board_has_shadow = has_shadow;
                static_board_tiles = scratch_static_tiles;
                static_board_valid = true;
            } else {
                ++bench_stats.static_base_hits;
            }

            PreResolveTiles(frame, split_idx, frame.tiles.size(), sx, sy);
            PreResolveFlightsAndDora(frame, sx, sy);

            if (!base)
                Check(dc->CreateBitmap(D2D1::SizeU(rw, rh), nullptr, 0,
                                       Properties(D2D1_BITMAP_OPTIONS_TARGET), &base),
                      "Cannot create native Direct2D table target");
            DrawOn(base.Get(), [&] {
                DrawBitmap(static_board.Get(), 0, 0);
                DrawStationaryRange(frame, split_idx, frame.tiles.size(), sx, sy);
                if (has_flights) DrawFlights(frame, sx, sy);
                const double unit = double(width) / 1024;
                if (!frame.state.wall.known)
                    for (std::size_t i = 0; i < resolved_dora.size(); ++i) {
                        auto *bmp = resolved_dora[i];
                        const auto bsz = bmp->GetPixelSize();
                        DrawBitmap(bmp, Box(static_cast<float>((28 + i * 40) * unit) * sx,
                                            static_cast<float>(28 * unit) * sy,
                                            float(bsz.width), float(bsz.height)));
                    }
                if (!active_hint.empty() && hint_texture) {
                    if (hd_active && (rw != width || rh != h)) {
                        const auto hsz = hint_texture->GetPixelSize();
                        DrawBitmap(hint_texture.Get(),
                                   Box(hint_x * sx, hint_y * sy, hsz.width * sx, hsz.height * sy),
                                   1.0f, D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);
                    } else {
                        DrawBitmap(hint_texture.Get(), hint_x, hint_y);
                    }
                }
                if (!active_countdown.empty() && countdown_texture) {
                    if (hd_active && (rw != width || rh != h)) {
                        const auto csz = countdown_texture->GetPixelSize();
                        DrawBitmap(countdown_texture.Get(),
                                   Box(countdown_x * sx, countdown_y * sy, csz.width * sx, csz.height * sy),
                                   1.0f, D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);
                    } else {
                        DrawBitmap(countdown_texture.Get(), countdown_x, countdown_y);
                    }
                }
                for (const auto &sprite: hud_sprites) {
                    const auto &t = sprite.transform;
                    if (hd_active) {
                        dc->SetTransform(D2D1::Matrix3x2F(t[0], t[1], t[2], t[3], t[4], t[5]) *
                                         D2D1::Matrix3x2F::Scale(sx, sy));
                        DrawBitmap(SpriteHd(sprite.id),
                                   Box(float(sprite.x), float(sprite.y), float(sprite.w), float(sprite.h)),
                                   1.0f, D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);
                    } else {
                        dc->SetTransform(D2D1::Matrix3x2F(t[0], t[1], t[2], t[3], t[4], t[5]));
                        DrawBitmap(Sprite(sprite.id), sprite.x, sprite.y);
                    }
                }
                dc->SetTransform(D2D1::Matrix3x2F::Identity());
                DrawAutoChrome(hovered, rw, rh, sx, sy);
                DrawBitmap(hud_text.Get(), 0, 0);
            });

            if (!has_flights) {
                base_shadow_gen = shadow_generation;
                base_red = frame.red_tiles;
                base_online = frame.online_mode;
                base_has_shadow = has_shadow;
                base_wall_known = frame.state.wall.known;
                if (!frame.state.wall.known) base_dora = frame.state.dora_indicators;
                else base_dora.clear();
                base_hovered = hovered;
                base_hud_stamp = metadata;
                base_hint_stamp = active_hint;
                base_countdown_stamp = active_countdown;
                base_tail_tiles = scratch_tail_tiles;
                base_valid = true;
            } else {
                base_valid = false;
            }
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
            DrawBitmap(hd_active ? SpriteHd(440) : Sprite(440),
                       Box(float(r.left + offset), float(r.top + offset), float(bw), float(bh)),
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
            if (button.focused && bw > 6 && bh > 6) {
                brush->SetColor(D2D1::ColorF(0xffffffu));
                dc->DrawRectangle(
                    D2D1::RectF(float(r.left) + 3.5f, float(r.top) + 3.5f,
                                float(r.right) - 3.5f, float(r.bottom) - 3.5f),
                    brush.Get(), 1.0f, focus_stroke.Get());
            }
        }

        void DrawLetterboxBackdrop(int aw, int ah, int dl, int dt, int dw, int dh, int h) {
            if (aw <= 0 || ah <= 0) return;
            const int dr = dl + dw, db = dt + dh;
            const float sx = float(width) / float(aw);
            const float sy = float(h) / float(ah);
            auto strip = [&](int x0, int y0, int x1, int y1) {
                if (x1 <= x0 || y1 <= y0) return;
                const D2D1_RECT_F dst{float(x0), float(y0), float(x1), float(y1)};
                const D2D1_RECT_F src{float(x0) * sx, float(y0) * sy, float(x1) * sx, float(y1) * sy};
                dc->DrawBitmap(backdrop.Get(), dst, 1.0f, D2D1_INTERPOLATION_MODE_LINEAR, src);
            };
            const int cx0 = std::clamp(dl, 0, aw), cx1 = std::clamp(dr, 0, aw);
            strip(0, 0, cx0, ah);
            strip(cx1, 0, aw, ah);
            strip(cx0, 0, cx1, std::clamp(dt, 0, ah));
            strip(cx0, std::clamp(db, 0, ah), cx1, ah);
        }

        void TrimCaches() {
            if ((frame_number & 31u) == 0 || glyphs.size() > 128) {
                for (auto it = glyphs.begin(); it != glyphs.end();) {
                    if (it->second.first.expired()) it = glyphs.erase(it);
                    else ++it;
                }
            }
            if (legacy_buttons.size() > 64) legacy_buttons.clear();
            const std::size_t budget = hd_active ? (128u * 1024 * 1024) : (48u * 1024 * 1024);
            while (tile_bytes > budget && !tile_lru.empty()) {
                const auto &oldest = tile_lru.back();
                if (oldest.used == frame_number) break;
                tile_bytes -= oldest.bytes;
                tile_index.erase(oldest.key);
                tile_lru.pop_back();
            }
        }

        void Render(ID2D1Bitmap1 *target, const TableRenderFrame &frame, CanvasRect area, bool window) {
            const int h = width * 3 / 4;
            const auto &dest = frame.destination;
            const int aw = area.right - area.left, ah = area.bottom - area.top;
            const int dl = dest.left - area.left, dt = dest.top - area.top;
            const int dw = dest.right - dest.left, dh = dest.bottom - dest.top;
            ++frame_number;
            tiles.BeginFrame();

            const bool want_hd = preferences.hd_artwork && width == 1440 && dw > 0 && dh > 0 &&
                                 hd_pipeline.EnsureReady(d3d.Get(), immediate.Get(), directory);
            const int next_rw = want_hd ? std::clamp(dw, 1, 4096) : width;
            const int next_rh = want_hd ? std::clamp(dh, 1, 3072) : h;
            if (want_hd != hd_active || next_rw != render_w || next_rh != render_h) {
                hd_active = want_hd;
                render_w = next_rw;
                render_h = next_rh;
                static_board.Reset();
                base.Reset();
                hud_text.Reset();
                hud_stamp.clear();
                tile_index.clear();
                tile_lru.clear();
                tile_bytes = 0;
                auto_gradients = {};
                auto_gradient_w = auto_gradient_h = 0;
                static_board_valid = false;
                base_valid = false;
            }

            EnsureBase(frame, h);
            DrawOn(target, [&] {
                if (!frame.online_mode) {
                    dc->Clear(D2D1::ColorF(0x172d2cu));
                } else if (window && (dw != aw || dh != ah)) {
                    DrawLetterboxBackdrop(aw, ah, dl, dt, dw, dh, h);
                } else if (dw < aw || dh < ah) {
                    dc->Clear(D2D1::ColorF(0u));
                }
                dc->SetTransform(D2D1::Matrix3x2F::Translation(float(dl), float(dt)));
                if (dw > 0 && dh > 0) {
                    const auto interpolation = dw == render_w && dh == render_h ? D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR :
                        dw < render_w || dh < render_h ? D2D1_INTERPOLATION_MODE_MULTI_SAMPLE_LINEAR :
                                                        (using_hardware ? D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC
                                                                        : D2D1_INTERPOLATION_MODE_LINEAR);
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
        impl_->hd_pipeline.Reset();
        impl_->hd_active = false;
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
            if (impl_->frame_waitable) WaitForSingleObjectEx(impl_->frame_waitable, 1000, TRUE);
            impl_->Render(impl_->swap_target.Get(), frame, area, true);
            DXGI_PRESENT_PARAMETERS params{};
            Check(impl_->swap->Present1(impl_->sync_interval, 0, &params), "Cannot present table swap chain");
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
    bool D2DTableRenderer::UsingHardware() const { return impl_->using_hardware; }
    bool D2DTableRenderer::UsingHdArtwork() const { return impl_->hd_active; }
    void D2DTableRenderer::SetSyncInterval(unsigned interval) { impl_->sync_interval = interval; }
    D2DTableRenderer::BenchStats D2DTableRenderer::Stats() const { return impl_->bench_stats; }
    void D2DTableRenderer::ResetStats() { impl_->bench_stats = {}; }
    void D2DTableRenderer::SimulateDeviceLoss() { impl_->reset_requested = true; }
} // namespace tenhou
