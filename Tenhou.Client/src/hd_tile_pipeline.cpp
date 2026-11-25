#include "hd_tile_pipeline.hpp"
#include "hd_tile_atlas.hpp"
#include "hd_tile_shader_bc.hpp"
#include "resource_manager.hpp"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace tenhou {
    namespace {
        struct HdFaceVertex {
            float x, y;       // NDC [-1, 1]
            float uq, vq, q;  // Projective texture coords (u * q, v * q, q)
            float slice;      // Texture2DArray slice index
        };

        // Ink footprint inside each 384x512 cell of mahjong_graphic faces.png
        // (measured union of 36 standard faces is [0.086..0.914] x [0.074..0.940];
        // slightly relaxed margin keeps flower extremes unclipped while matching
        // original face fullness).
        constexpr float kUvMinU = 0.070f;
        constexpr float kUvMaxU = 0.930f;
        constexpr float kUvMinV = 0.060f;
        constexpr float kUvMaxV = 0.955f;

        // Computes exact projective homogeneous weights (q0..q3) from the intersection
        // of the quad's two diagonals P0-P2 and P1-P3 so rasterization performs a
        // mathematically exact 4-point homography without diagonal crease artifacts.
        void ComputeQuadProjectiveWeights(float x0, float y0, float x1, float y1,
                                          float x2, float y2, float x3, float y3,
                                          float (&q)[4]) {
            q[0] = q[1] = q[2] = q[3] = 1.0f;
            const double ax = double(x2) - double(x0);
            const double ay = double(y2) - double(y0);
            const double bx = double(x3) - double(x1);
            const double by = double(y3) - double(y1);
            const double cx = double(x1) - double(x0);
            const double cy = double(y1) - double(y0);
            const double denom = ax * by - ay * bx;
            if (std::abs(denom) < 1e-6) return;
            const double t = (cx * by - cy * bx) / denom;
            const double u = (cx * ay - cy * ax) / denom;
            if (!(t > 0.05 && t < 0.95 && u > 0.05 && u < 0.95)) return;
            // Along P0->P2, midpoint (0.5, 0.5) lands at parameter t => q0*(1-t) == q2*t.
            // Along P1->P3, midpoint (0.5, 0.5) lands at parameter u => q1*(1-u) == q3*u.
            q[0] = static_cast<float>(1.0 / (1.0 - t));
            q[2] = static_cast<float>(1.0 / t);
            q[1] = static_cast<float>(1.0 / (1.0 - u));
            q[3] = static_cast<float>(1.0 / u);
        }
    } // namespace

    void HdTilePipeline::Reset() {
        ready_ = false;
        load_attempted_ = false;
        loaded_path_.clear();
        rasterizer_.Reset();
        blend_.Reset();
        sampler_.Reset();
        ib_.Reset();
        vb_.Reset();
        input_layout_.Reset();
        ps_.Reset();
        vs_.Reset();
        atlas_srv_.Reset();
        atlas_array_.Reset();
    }

    bool HdTilePipeline::EnsureReady(ID3D11Device *d3d, ID3D11DeviceContext *ctx,
                                     const std::filesystem::path &resource_dir) {
        if (!d3d || !ctx) return false;
        const auto atlas_path = ResourceManager(resource_dir).Path(AssetId::HdTileFaces);
        if (ready_ && loaded_path_ == atlas_path) return true;
        if (load_attempted_ && loaded_path_ == atlas_path) return false;

        Reset();
        load_attempted_ = true;
        loaded_path_ = atlas_path;

        if (!std::filesystem::is_regular_file(atlas_path)) return false;

        Gdiplus::Bitmap bitmap(atlas_path.c_str());
        const int expected_w = kHdFaceCellWidth * kHdFaceAtlasCols;
        const int expected_h = kHdFaceCellHeight * kHdFaceAtlasRows;
        if (bitmap.GetLastStatus() != Gdiplus::Ok ||
            static_cast<int>(bitmap.GetWidth()) != expected_w ||
            static_cast<int>(bitmap.GetHeight()) != expected_h) {
            return false;
        }

        Gdiplus::Rect lock_rect(0, 0, expected_w, expected_h);
        Gdiplus::BitmapData locked{};
        if (bitmap.LockBits(&lock_rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &locked) != Gdiplus::Ok)
            return false;

        D3D11_TEXTURE2D_DESC tex_desc{};
        tex_desc.Width = static_cast<UINT>(kHdFaceCellWidth);
        tex_desc.Height = static_cast<UINT>(kHdFaceCellHeight);
        tex_desc.MipLevels = 0; // Full mip chain
        tex_desc.ArraySize = static_cast<UINT>(kHdFaceSliceCount);
        tex_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        tex_desc.SampleDesc.Count = 1;
        tex_desc.Usage = D3D11_USAGE_DEFAULT;
        tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        tex_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

        if (FAILED(d3d->CreateTexture2D(&tex_desc, nullptr, &atlas_array_))) {
            bitmap.UnlockBits(&locked);
            Reset();
            return false;
        }

        D3D11_TEXTURE2D_DESC actual_desc{};
        atlas_array_->GetDesc(&actual_desc);
        const UINT mip_levels = actual_desc.MipLevels;

        std::vector<std::uint8_t> slice_buf(
            static_cast<std::size_t>(kHdFaceCellWidth) * static_cast<std::size_t>(kHdFaceCellHeight) * 4);
        const auto *src_base = static_cast<const std::uint8_t *>(locked.Scan0);

        for (int slice = 0; slice < kHdFaceSliceCount; ++slice) {
            const int col = slice % kHdFaceAtlasCols;
            const int row = slice / kHdFaceAtlasCols;
            const int src_x = col * kHdFaceCellWidth;
            const int src_y = row * kHdFaceCellHeight;
            for (int y = 0; y < kHdFaceCellHeight; ++y) {
                const auto *src_row = src_base + static_cast<std::ptrdiff_t>(src_y + y) * locked.Stride +
                                      static_cast<std::size_t>(src_x) * 4;
                auto *dst_row = slice_buf.data() + static_cast<std::size_t>(y) * kHdFaceCellWidth * 4;
                // Convert straight alpha to premultiplied BGRA before mipmap generation to avoid dark fringes.
                for (int x = 0; x < kHdFaceCellWidth; ++x) {
                    const std::uint8_t a = src_row[x * 4 + 3];
                    dst_row[x * 4 + 0] = static_cast<std::uint8_t>((src_row[x * 4 + 0] * a + 127) / 255);
                    dst_row[x * 4 + 1] = static_cast<std::uint8_t>((src_row[x * 4 + 1] * a + 127) / 255);
                    dst_row[x * 4 + 2] = static_cast<std::uint8_t>((src_row[x * 4 + 2] * a + 127) / 255);
                    dst_row[x * 4 + 3] = a;
                }
            }
            const UINT subresource = D3D11CalcSubresource(0, static_cast<UINT>(slice), mip_levels);
            ctx->UpdateSubresource(atlas_array_.Get(), subresource, nullptr, slice_buf.data(),
                                   static_cast<UINT>(kHdFaceCellWidth * 4), 0);
        }
        bitmap.UnlockBits(&locked);

        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Format = tex_desc.Format;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        srv_desc.Texture2DArray.MostDetailedMip = 0;
        srv_desc.Texture2DArray.MipLevels = mip_levels;
        srv_desc.Texture2DArray.FirstArraySlice = 0;
        srv_desc.Texture2DArray.ArraySize = static_cast<UINT>(kHdFaceSliceCount);
        if (FAILED(d3d->CreateShaderResourceView(atlas_array_.Get(), &srv_desc, &atlas_srv_))) {
            Reset();
            return false;
        }
        ctx->GenerateMips(atlas_srv_.Get());

        if (FAILED(d3d->CreateVertexShader(kHdTileVsBytecode, sizeof(kHdTileVsBytecode), nullptr, &vs_)) ||
            FAILED(d3d->CreatePixelShader(kHdTilePsBytecode, sizeof(kHdTilePsBytecode), nullptr, &ps_))) {
            Reset();
            return false;
        }

        const D3D11_INPUT_ELEMENT_DESC layout[]{
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        if (FAILED(d3d->CreateInputLayout(layout, 3, kHdTileVsBytecode, sizeof(kHdTileVsBytecode), &input_layout_))) {
            Reset();
            return false;
        }

        D3D11_BUFFER_DESC vb_desc{};
        vb_desc.ByteWidth = sizeof(HdFaceVertex) * 4;
        vb_desc.Usage = D3D11_USAGE_DYNAMIC;
        vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(d3d->CreateBuffer(&vb_desc, nullptr, &vb_))) {
            Reset();
            return false;
        }

        const std::uint16_t indices[6]{0, 1, 2, 0, 2, 3};
        D3D11_BUFFER_DESC ib_desc{};
        ib_desc.ByteWidth = sizeof(indices);
        ib_desc.Usage = D3D11_USAGE_IMMUTABLE;
        ib_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA ib_data{indices, 0, 0};
        if (FAILED(d3d->CreateBuffer(&ib_desc, &ib_data, &ib_))) {
            Reset();
            return false;
        }

        D3D11_SAMPLER_DESC samp_desc{};
        samp_desc.Filter = D3D11_FILTER_ANISOTROPIC;
        samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samp_desc.MaxAnisotropy = 16;
        samp_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samp_desc.MinLOD = 0.0f;
        samp_desc.MaxLOD = D3D11_FLOAT32_MAX;
        if (FAILED(d3d->CreateSamplerState(&samp_desc, &sampler_))) {
            Reset();
            return false;
        }

        D3D11_BLEND_DESC blend_desc{};
        blend_desc.RenderTarget[0].BlendEnable = TRUE;
        blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(d3d->CreateBlendState(&blend_desc, &blend_))) {
            Reset();
            return false;
        }

        D3D11_RASTERIZER_DESC rast_desc{};
        rast_desc.FillMode = D3D11_FILL_SOLID;
        rast_desc.CullMode = D3D11_CULL_NONE;
        rast_desc.DepthClipEnable = TRUE;
        if (FAILED(d3d->CreateRasterizerState(&rast_desc, &rasterizer_))) {
            Reset();
            return false;
        }

        ready_ = true;
        return true;
    }

    bool HdTilePipeline::DrawPerspectiveFace(ID3D11DeviceContext *ctx, ID3D11RenderTargetView *rtv,
                                             int rt_w, int rt_h, int slice, const HdTileFaceQuad &quad,
                                             float sx, float sy) {
        if (!ready_ || !ctx || !rtv || rt_w <= 0 || rt_h <= 0 || slice < 0 || slice >= kHdFaceSliceCount)
            return false;

        const float px[4]{quad.x0 * sx, quad.x1 * sx, quad.x2 * sx, quad.x3 * sx};
        const float py[4]{quad.y0 * sy, quad.y1 * sy, quad.y2 * sy, quad.y3 * sy};

        float q[4]{};
        ComputeQuadProjectiveWeights(px[0], py[0], px[1], py[1], px[2], py[2], px[3], py[3], q);

        const float us[4]{kUvMinU, kUvMaxU, kUvMaxU, kUvMinU};
        const float vs[4]{kUvMinV, kUvMinV, kUvMaxV, kUvMaxV};
        const float inv_w = 2.0f / static_cast<float>(rt_w);
        const float inv_h = 2.0f / static_cast<float>(rt_h);
        const float fslice = static_cast<float>(slice);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(ctx->Map(vb_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return false;
        auto *verts = static_cast<HdFaceVertex *>(mapped.pData);
        for (int i = 0; i < 4; ++i) {
            verts[i] = {
                px[i] * inv_w - 1.0f,
                1.0f - py[i] * inv_h,
                us[i] * q[i],
                vs[i] * q[i],
                q[i],
                fslice
            };
        }
        ctx->Unmap(vb_.Get(), 0);

        D3D11_VIEWPORT vp{0.0f, 0.0f, static_cast<float>(rt_w), static_cast<float>(rt_h), 0.0f, 1.0f};
        ID3D11RenderTargetView *rtvs[1]{rtv};
        ctx->OMSetRenderTargets(1, rtvs, nullptr);
        ctx->RSSetViewports(1, &vp);
        ctx->RSSetState(rasterizer_.Get());

        const float blend_factor[4]{0, 0, 0, 0};
        ctx->OMSetBlendState(blend_.Get(), blend_factor, 0xffffffffu);

        const UINT stride = sizeof(HdFaceVertex);
        const UINT offset = 0;
        ID3D11Buffer *vbs[1]{vb_.Get()};
        ctx->IASetInputLayout(input_layout_.Get());
        ctx->IASetVertexBuffers(0, 1, vbs, &stride, &offset);
        ctx->IASetIndexBuffer(ib_.Get(), DXGI_FORMAT_R16_UINT, 0);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ctx->VSSetShader(vs_.Get(), nullptr, 0);
        ctx->PSSetShader(ps_.Get(), nullptr, 0);
        ID3D11ShaderResourceView *srvs[1]{atlas_srv_.Get()};
        ID3D11SamplerState *samps[1]{sampler_.Get()};
        ctx->PSSetShaderResources(0, 1, srvs);
        ctx->PSSetSamplers(0, 1, samps);

        ctx->DrawIndexed(6, 0, 0);

        // Unbind RTV and SRV cleanly before handing the device back to Direct2D.
        ID3D11RenderTargetView *null_rtv[1]{nullptr};
        ID3D11ShaderResourceView *null_srv[1]{nullptr};
        ctx->OMSetRenderTargets(1, null_rtv, nullptr);
        ctx->PSSetShaderResources(0, 1, null_srv);
        return true;
    }
} // namespace tenhou
