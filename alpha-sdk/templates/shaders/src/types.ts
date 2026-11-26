// Every name the game uses. The compiler rejects anything not listed here.

/** Every custom shader takes the same two uniforms, in this uniform block order. */
type Uniforms = "intensity" | "time";

export interface ShaderLab {
  actions: "next" | "prev" | "more" | "less";
  sounds: "click";
  images: never;
  tags: { orb: {}; floor: {}; label: {} };
  /** assets.shaders: Vulkan GLSL fragment shaders (.frag) in assets/shaders/ -> their uniform names. */
  shaders: { wave: Uniforms; chroma: Uniforms; sepia: Uniforms; posterize: Uniforms; scanlines: Uniforms; pulse: Uniforms };
  /** hud: what is on; touch: on-screen buttons, shown on phones only (ctx.platform.mobile). */
  ui: { hud: "name" | "level" | "hint"; touch: "prev" | "next" | "less" | "more" };
  scenes: { lab: undefined };
}
