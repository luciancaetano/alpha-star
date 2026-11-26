#version 450
// Test shader: the frame inverted, mixed by `amount` (0: unchanged, 1: inverted).
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 o_color;
layout(set = 2, binding = 0) uniform sampler2D u_texture;
layout(set = 3, binding = 0, std140) uniform Params {
    float amount;
};

void main() {
    vec4 c = texture(u_texture, v_uv) * v_color;
    o_color = vec4(mix(c.rgb, 1.0 - c.rgb, amount), c.a);
}
