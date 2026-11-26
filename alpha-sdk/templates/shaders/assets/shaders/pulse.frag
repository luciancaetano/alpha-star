#version 450
// Pulse: a vignette whose dark edge throbs with time.
// Uniforms in declared order (assets.shaders.pulse.uniforms): intensity 0..1, time = ctx.time.
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 o_color;
layout(set = 2, binding = 0) uniform sampler2D u_frame;
layout(set = 3, binding = 0, std140) uniform Params {
    float intensity;
    float time;
};

void main() {
    vec4 c = texture(u_frame, v_uv) * v_color;
    float d = length(v_uv - 0.5) * 1.4;
    float radius = 0.75 - 0.15 * (0.5 + 0.5 * sin(time * 3.0));
    float dark = smoothstep(radius - 0.3, radius, d) * intensity;
    o_color = vec4(c.rgb * (1.0 - dark), c.a);
}
