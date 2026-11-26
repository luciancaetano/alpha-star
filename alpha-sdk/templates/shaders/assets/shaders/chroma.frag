#version 450
// Chroma: red and blue split away from the center (chromatic aberration), breathing with time.
// Uniforms in declared order (assets.shaders.chroma.uniforms): intensity 0..1, time = ctx.time.
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 o_color;
layout(set = 2, binding = 0) uniform sampler2D u_frame;
layout(set = 3, binding = 0, std140) uniform Params {
    float intensity;
    float time;
};

void main() {
    vec2 off = (v_uv - 0.5) * (0.012 + 0.006 * sin(time * 2.0)) * intensity;
    vec4 c = texture(u_frame, v_uv);
    c.r = texture(u_frame, v_uv + off).r;
    c.b = texture(u_frame, v_uv - off).b;
    o_color = c * v_color;
}
