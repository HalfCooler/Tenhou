// Projective 4-point homography tile-face shader for HD SVG-baked atlas.
// Compiled offline with fxc.exe (/T vs_4_0, /T ps_4_0) into hd_tile_shader_bc.hpp.

Texture2DArray<float4> FaceAtlas : register(t0);
SamplerState FaceSampler : register(s0);

struct VSInput {
    float2 pos : POSITION;   // NDC coordinates [-1, 1]
    float3 uvq : TEXCOORD0;  // (u * q, v * q, q) for exact perspective-correct homography
    float slice : TEXCOORD1; // Texture2DArray slice index [0..43]
};

struct PSInput {
    float4 pos : SV_POSITION;
    float3 uvq : TEXCOORD0;
    nointerpolation float slice : TEXCOORD1;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    output.pos = float4(input.pos, 0.0f, 1.0f);
    output.uvq = input.uvq;
    output.slice = input.slice;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float2 uv = input.uvq.xy / input.uvq.z;
    // Sample premultiplied BGRA/RGBA face texture with full mip chain and anisotropic filtering.
    return FaceAtlas.Sample(FaceSampler, float3(uv, input.slice));
}
