// Test shader: the frame inverted, mixed by `amount` (0: unchanged, 1: inverted).
cbuffer Params : register(b0, space3) {
    float amount;
};
Texture2D u_texture : register(t0, space2);
SamplerState u_sampler : register(s0, space2);

struct PSInput {
    float4 v_color : COLOR0;
    float2 v_uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target {
    float4 c = u_texture.Sample(u_sampler, input.v_uv) * input.v_color;
    return float4(lerp(c.rgb, 1.0 - c.rgb, amount), c.a);
}
