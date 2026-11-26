#version 450
// Sepia: grayscale toned brown, mixed in by intensity, with the flicker of an old film.
// Uniforms in declared order (assets.shaders.sepia.uniforms): intensity 0..1, time = ctx.time.
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
    float g = dot(c.rgb, vec3(0.299, 0.587, 0.114));
    vec3 tone = vec3(g * 1.07, g * 0.74, g * 0.43) * (0.95 + 0.05 * sin(time * 23.0));
    o_color = vec4(mix(c.rgb, tone, intensity), c.a);
}
