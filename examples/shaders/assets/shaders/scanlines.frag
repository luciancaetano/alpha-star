#version 450
// Scanlines: dark rolling lines and a slight barrel curve, a CRT screen look.
// Uniforms in declared order (assets.shaders.scanlines.uniforms): intensity 0..1, time = ctx.time.
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 o_color;
layout(set = 2, binding = 0) uniform sampler2D u_frame;
layout(set = 3, binding = 0, std140) uniform Params {
    float intensity;
    float time;
};

void main() {
    vec2 uv = v_uv - 0.5;
    uv *= 1.0 + dot(uv, uv) * 0.25 * intensity;
    uv += 0.5;
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        o_color = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    vec4 c = texture(u_frame, uv) * v_color;
    float band = 0.5 + 0.5 * sin((uv.y * 180.0 - time * 8.0) * 3.14159);
    o_color = vec4(c.rgb * (1.0 - band * 0.45 * intensity), c.a);
}
