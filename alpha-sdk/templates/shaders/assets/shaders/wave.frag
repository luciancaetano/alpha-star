#version 450
// Wave: rows slide sideways on a moving sine, like heat haze or water.
// Uniforms in declared order (assets.shaders.wave.uniforms): intensity 0..1, time = ctx.time.
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 o_color;
layout(set = 2, binding = 0) uniform sampler2D u_frame;
layout(set = 3, binding = 0, std140) uniform Params {
    float intensity;
    float time;
};

void main() {
    vec2 uv = v_uv;
    uv.x += sin(uv.y * 30.0 + time * 4.0) * 0.012 * intensity;
    uv.y += cos(uv.x * 20.0 + time * 3.0) * 0.006 * intensity;
    o_color = texture(u_frame, uv) * v_color;
}
