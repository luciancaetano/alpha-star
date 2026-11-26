#version 450
// Posterize: colors snap to a few levels (fewer at higher intensity), a retro palette look.
// Uniforms in declared order (assets.shaders.posterize.uniforms): intensity 0..1, time = ctx.time.
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
    float levels = mix(16.0, 3.0, intensity);
    o_color = vec4(floor(c.rgb * levels + 0.5) / levels, c.a);
}
