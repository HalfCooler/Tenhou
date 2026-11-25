// Offline SVG-to-PNG baker using Windows Direct2D ID2D1DeviceContext5 + WIC.
// Zero third-party dependencies; supports inline CSS <style> class expansion.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d2d1_3.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <shlwapi.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

namespace {
    using Microsoft::WRL::ComPtr;

    void Check(HRESULT hr, const char *what) {
        if (FAILED(hr)) {
            std::ostringstream oss;
            oss << what << " failed: HRESULT=0x" << std::hex << static_cast<unsigned long>(hr);
            throw std::runtime_error(oss.str());
        }
    }

    std::string ReadText(const std::filesystem::path &path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) throw std::runtime_error("Cannot open SVG: " + path.string());
        std::ostringstream oss;
        oss << f.rdbuf();
        return oss.str();
    }

    // Inline simple CSS class rules from <style> blocks into element attributes,
    // because Direct2D ID2D1DeviceContext5 does not evaluate <style> stylesheets.
    std::string InlineCssClasses(const std::string &svg) {
        std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> rules;
        std::regex style_re(R"(<style[^>]*>([\s\S]*?)</style>)", std::regex::icase);
        std::regex rule_re(R"(\.([A-Za-z0-9_-]+)\s*\{([^}]*)\})");
        std::regex decl_re(R"(([a-zA-Z-]+)\s*:\s*([^;]+);?)");

        for (std::sregex_iterator it(svg.begin(), svg.end(), style_re), end; it != end; ++it) {
            const std::string body = (*it)[1].str();
            for (std::sregex_iterator rit(body.begin(), body.end(), rule_re); rit != end; ++rit) {
                const std::string cls = (*rit)[1].str();
                const std::string decls = (*rit)[2].str();
                auto &vec = rules[cls];
                for (std::sregex_iterator dit(decls.begin(), decls.end(), decl_re); dit != end; ++dit) {
                    std::string prop = (*dit)[1].str();
                    std::string val = (*dit)[2].str();
                    while (!val.empty() && std::isspace(static_cast<unsigned char>(val.back()))) val.pop_back();
                    vec.emplace_back(std::move(prop), std::move(val));
                }
            }
        }
        if (rules.empty()) return svg;

        // Remove <style>...</style> blocks so D2D SVG parser stays clean.
        std::string out = std::regex_replace(svg, style_re, "");

        // Replace class="cls-1 cls-2" with inline presentation attributes.
        std::regex class_re(R"re(class\s*=\s*"([^"]+)")re");
        std::string expanded;
        expanded.reserve(out.size() + 512);
        std::size_t last = 0;
        for (std::sregex_iterator it(out.begin(), out.end(), class_re), end; it != end; ++it) {
            const auto &m = *it;
            expanded.append(out, last, static_cast<std::size_t>(m.position()) - last);
            std::istringstream iss(m[1].str());
            std::string cls;
            bool any = false;
            while (iss >> cls) {
                auto found = rules.find(cls);
                if (found == rules.end()) continue;
                for (const auto &[prop, val] : found->second) {
                    if (any) expanded.push_back(' ');
                    expanded += prop + "=\"" + val + "\"";
                    any = true;
                }
            }
            last = static_cast<std::size_t>(m.position() + m.length());
        }
        expanded.append(out, last, out.size() - last);
        return expanded;
    }

    struct ViewBox {
        float x = 0, y = 0, w = 0, h = 0;
        bool valid = false;
    };

    ViewBox ParseViewBox(const std::string &svg) {
        std::regex vb_re(R"re(viewBox\s*=\s*"([^"]+)")re", std::regex::icase);
        std::smatch m;
        if (std::regex_search(svg, m, vb_re)) {
            std::string s = m[1].str();
            for (char &c : s) if (c == ',') c = ' ';
            std::istringstream iss(s);
            ViewBox vb{};
            if (iss >> vb.x >> vb.y >> vb.w >> vb.h && vb.w > 0 && vb.h > 0) {
                vb.valid = true;
                return vb;
            }
        }
        return {};
    }

    struct SvgRenderer {
        ComPtr<ID3D11Device> d3d;
        ComPtr<ID3D11DeviceContext> d3d_ctx;
        ComPtr<ID2D1Factory6> factory;
        ComPtr<ID2D1Device5> d2d_device;
        ComPtr<ID2D1DeviceContext5> dc;
        ComPtr<IWICImagingFactory> wic;

        SvgRenderer() {
            Check(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED), "CoInitializeEx");
            Check(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic)),
                  "Create WIC factory");
            UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
            D3D_FEATURE_LEVEL fl{};
            HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
                                           nullptr, 0, D3D11_SDK_VERSION, &d3d, &fl, &d3d_ctx);
            if (FAILED(hr)) {
                Check(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags,
                                        nullptr, 0, D3D11_SDK_VERSION, &d3d, &fl, &d3d_ctx),
                      "D3D11CreateDevice WARP");
            }
            ComPtr<IDXGIDevice> dxgi;
            Check(d3d.As(&dxgi), "Query IDXGIDevice");
            Check(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory.GetAddressOf()), "D2D1CreateFactory");
            Check(factory->CreateDevice(dxgi.Get(), &d2d_device), "CreateDevice5");
            Check(d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &dc), "CreateDeviceContext5");
            dc->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
            dc->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        }

        ~SvgRenderer() {
            dc.Reset();
            d2d_device.Reset();
            factory.Reset();
            d3d_ctx.Reset();
            d3d.Reset();
            wic.Reset();
            CoUninitialize();
        }

        // Renders an SVG into premultiplied BGRA pixels of size (w, h).
        std::vector<std::uint8_t> RenderSvg(const std::filesystem::path &path, int w, int h, bool preserve_aspect) {
            const std::string raw = ReadText(path);
            const std::string svg = InlineCssClasses(raw);
            const ViewBox vb = ParseViewBox(svg);

            ComPtr<IStream> stream;
            stream.Attach(SHCreateMemStream(reinterpret_cast<const BYTE *>(svg.data()),
                                            static_cast<UINT>(svg.size())));
            if (!stream) throw std::runtime_error("SHCreateMemStream failed");

            const D2D1_SIZE_F viewport = vb.valid ? D2D1::SizeF(vb.w, vb.h) : D2D1::SizeF(float(w), float(h));
            ComPtr<ID2D1SvgDocument> doc;
            Check(dc->CreateSvgDocument(stream.Get(), viewport, &doc), "CreateSvgDocument");

            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = static_cast<UINT>(w);
            desc.Height = static_cast<UINT>(h);
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

            ComPtr<ID3D11Texture2D> rt_tex;
            Check(d3d->CreateTexture2D(&desc, nullptr, &rt_tex), "Create RT texture");
            ComPtr<IDXGISurface> surface;
            Check(rt_tex.As(&surface), "Query IDXGISurface");
            ComPtr<ID2D1Bitmap1> target;
            Check(dc->CreateBitmapFromDxgiSurface(
                      surface.Get(),
                      D2D1::BitmapProperties1(
                          D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                          D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96, 96),
                      &target),
                  "CreateBitmapFromDxgiSurface");

            dc->SetTarget(target.Get());
            dc->BeginDraw();
            dc->Clear(D2D1::ColorF(0, 0, 0, 0));

            if (vb.valid) {
                float sx = float(w) / vb.w;
                float sy = float(h) / vb.h;
                float tx = -vb.x * sx;
                float ty = -vb.y * sy;
                if (preserve_aspect) {
                    const float s = std::min(sx, sy);
                    sx = sy = s;
                    tx = (float(w) - vb.w * s) * 0.5f - vb.x * s;
                    ty = (float(h) - vb.h * s) * 0.5f - vb.y * s;
                }
                dc->SetTransform(D2D1::Matrix3x2F::Scale(sx, sy) * D2D1::Matrix3x2F::Translation(tx, ty));
            } else {
                dc->SetTransform(D2D1::Matrix3x2F::Identity());
            }

            dc->DrawSvgDocument(doc.Get());
            Check(dc->EndDraw(), "EndDraw");
            dc->SetTarget(nullptr);

            desc.Usage = D3D11_USAGE_STAGING;
            desc.BindFlags = 0;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            ComPtr<ID3D11Texture2D> staging;
            Check(d3d->CreateTexture2D(&desc, nullptr, &staging), "Create staging texture");
            d3d_ctx->CopyResource(staging.Get(), rt_tex.Get());

            D3D11_MAPPED_SUBRESOURCE mapped{};
            Check(d3d_ctx->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped), "Map staging");
            std::vector<std::uint8_t> pixels(static_cast<std::size_t>(w) * h * 4);
            for (int y = 0; y < h; ++y) {
                std::memcpy(pixels.data() + static_cast<std::size_t>(y) * w * 4,
                            static_cast<const std::uint8_t *>(mapped.pData) + static_cast<std::size_t>(y) * mapped.RowPitch,
                            static_cast<std::size_t>(w) * 4);
            }
            d3d_ctx->Unmap(staging.Get(), 0);
            return pixels;
        }

        // Saves straight-alpha BGRA pixels as PNG.
        void SavePng(const std::filesystem::path &out, int w, int h, const std::vector<std::uint8_t> &premul_bgra) {
            std::filesystem::create_directories(out.parent_path());
            // Unpremultiply for standard PNG straight alpha.
            std::vector<std::uint8_t> straight(premul_bgra.size());
            for (std::size_t i = 0; i < premul_bgra.size(); i += 4) {
                const std::uint8_t a = premul_bgra[i + 3];
                if (a == 0) {
                    straight[i] = straight[i + 1] = straight[i + 2] = straight[i + 3] = 0;
                } else if (a == 255) {
                    straight[i] = premul_bgra[i];
                    straight[i + 1] = premul_bgra[i + 1];
                    straight[i + 2] = premul_bgra[i + 2];
                    straight[i + 3] = 255;
                } else {
                    for (int c = 0; c < 3; ++c) {
                        const int v = (static_cast<int>(premul_bgra[i + c]) * 255 + a / 2) / a;
                        straight[i + c] = static_cast<std::uint8_t>(std::min(255, v));
                    }
                    straight[i + 3] = a;
                }
            }

            ComPtr<IWICStream> stream;
            Check(wic->CreateStream(&stream), "WIC CreateStream");
            Check(stream->InitializeFromFilename(out.wstring().c_str(), GENERIC_WRITE), "WIC InitializeFromFilename");
            ComPtr<IWICBitmapEncoder> encoder;
            Check(wic->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder), "WIC CreateEncoder");
            Check(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache), "WIC Encoder Initialize");
            ComPtr<IWICBitmapFrameEncode> frame;
            Check(encoder->CreateNewFrame(&frame, nullptr), "WIC CreateNewFrame");
            Check(frame->Initialize(nullptr), "WIC Frame Initialize");
            Check(frame->SetSize(static_cast<UINT>(w), static_cast<UINT>(h)), "WIC SetSize");
            WICPixelFormatGUID fmt = GUID_WICPixelFormat32bppBGRA;
            Check(frame->SetPixelFormat(&fmt), "WIC SetPixelFormat");
            Check(frame->WritePixels(static_cast<UINT>(h), static_cast<UINT>(w * 4),
                                     static_cast<UINT>(straight.size()), straight.data()),
                  "WIC WritePixels");
            Check(frame->Commit(), "WIC Frame Commit");
            Check(encoder->Commit(), "WIC Encoder Commit");
        }
    };
} // namespace

int wmain(int argc, wchar_t **argv) {
    try {
        std::filesystem::path svg_path, manifest_path, out_path;
        int width = 0, height = 0, cell_w = 384, cell_h = 512, cols = 8, rows = 6;
        bool stretch = false;

        for (int i = 1; i < argc; ++i) {
            const std::wstring arg = argv[i];
            auto next = [&]() -> std::wstring {
                if (i + 1 >= argc) throw std::runtime_error("Missing value for option");
                return argv[++i];
            };
            if (arg == L"--svg") svg_path = next();
            else if (arg == L"--atlas") manifest_path = next();
            else if (arg == L"--out") out_path = next();
            else if (arg == L"--width") width = std::stoi(next());
            else if (arg == L"--height") height = std::stoi(next());
            else if (arg == L"--cell-width") cell_w = std::stoi(next());
            else if (arg == L"--cell-height") cell_h = std::stoi(next());
            else if (arg == L"--cols") cols = std::stoi(next());
            else if (arg == L"--rows") rows = std::stoi(next());
            else if (arg == L"--stretch") stretch = true;
            else throw std::runtime_error("Unknown argument");
        }

        if (out_path.empty()) throw std::runtime_error("Missing --out");
        SvgRenderer renderer;

        if (!svg_path.empty()) {
            if (width <= 0 || height <= 0) throw std::runtime_error("Invalid --width / --height");
            auto pixels = renderer.RenderSvg(svg_path, width, height, !stretch);
            renderer.SavePng(out_path, width, height, pixels);
            std::cout << "Baked " << svg_path.string() << " -> " << out_path.string()
                      << " (" << width << "x" << height << ")\n";
            return 0;
        }

        if (!manifest_path.empty()) {
            std::ifstream in(manifest_path);
            if (!in) throw std::runtime_error("Cannot open atlas manifest: " + manifest_path.string());
            const auto base_dir = manifest_path.parent_path();
            std::vector<std::filesystem::path> files;
            std::string line;
            while (std::getline(in, line)) {
                while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
                    line.pop_back();
                if (line.empty() || line[0] == '#') continue;
                std::filesystem::path p(line);
                if (p.is_relative()) p = base_dir / p;
                files.push_back(p);
            }
            const int atlas_w = cell_w * cols;
            const int atlas_h = cell_h * rows;
            std::vector<std::uint8_t> atlas(static_cast<std::size_t>(atlas_w) * atlas_h * 4, 0);
            for (std::size_t idx = 0; idx < files.size(); ++idx) {
                const int col = static_cast<int>(idx % static_cast<std::size_t>(cols));
                const int row = static_cast<int>(idx / static_cast<std::size_t>(cols));
                if (row >= rows) throw std::runtime_error("Manifest exceeds atlas grid capacity");
                auto cell = renderer.RenderSvg(files[idx], cell_w, cell_h, true);
                for (int y = 0; y < cell_h; ++y) {
                    std::memcpy(
                        atlas.data() + (static_cast<std::size_t>(row * cell_h + y) * atlas_w + col * cell_w) * 4,
                        cell.data() + static_cast<std::size_t>(y) * cell_w * 4,
                        static_cast<std::size_t>(cell_w) * 4);
                }
            }
            renderer.SavePng(out_path, atlas_w, atlas_h, atlas);
            std::cout << "Baked atlas (" << files.size() << " SVGs) -> " << out_path.string()
                      << " (" << atlas_w << "x" << atlas_h << ")\n";
            return 0;
        }

        throw std::runtime_error("Specify either --svg or --atlas");
    } catch (const std::exception &e) {
        std::cerr << "svg_bake ERROR: " << e.what() << "\n";
        return 1;
    }
}
