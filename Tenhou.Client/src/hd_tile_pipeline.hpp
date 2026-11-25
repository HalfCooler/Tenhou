#pragma once
#include "hd_tile_quads.hpp"
#include <d3d11.h>
#include <wrl/client.h>
#include <filesystem>

namespace tenhou {
    // D3D11 GPU 4-point projective homography renderer for SVG-baked HD tile faces.
    class HdTilePipeline {
    public:
        bool EnsureReady(ID3D11Device *d3d, ID3D11DeviceContext *ctx, const std::filesystem::path &resource_dir);
        void Reset();
        bool Ready() const { return ready_; }

        // Projects slice [0..43] from the 44-layer Texture2DArray (with full mip chain and
        // 16x anisotropic filtering) onto the 4-vertex perspective quad scaled by (sx, sy).
        bool DrawPerspectiveFace(ID3D11DeviceContext *ctx, ID3D11RenderTargetView *rtv,
                                 int rt_w, int rt_h, int slice, const HdTileFaceQuad &quad,
                                 float sx, float sy);

    private:
        bool ready_ = false;
        bool load_attempted_ = false;
        std::filesystem::path loaded_path_;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> atlas_array_;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> atlas_srv_;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> vs_;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> ps_;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> vb_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> ib_;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_;
        Microsoft::WRL::ComPtr<ID3D11BlendState> blend_;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_;
    };
} // namespace tenhou
