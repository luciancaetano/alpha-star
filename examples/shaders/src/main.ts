// Entry point: `alpha export` bundles this and everything it imports into dist/main.js.
import { game } from "./alpha";
import lab from "./scenes/lab";
import { INTENSITY, VIEW } from "./tuning";

// Every shader declares the same uniforms (with defaults), in its uniform block order.
const shader = (name: string) => ({ file: `assets/shaders/${name}.frag`, uniforms: { intensity: INTENSITY.start, time: 0 } });

export default game({
  config: {
    title: "Shader Lab",
    size: [VIEW.width, VIEW.height],
    background: "#14162e",
    seed: 7,
    gravity: [0, 0],
    actions: {
      next: ["Right", "D", "Space", "GamepadRight", "GamepadA", "GamepadRB"],
      prev: ["Left", "A", "GamepadLeft", "GamepadLB"],
      more: ["Up", "W", "GamepadUp", "GamepadY"],
      less: ["Down", "S", "GamepadDown", "GamepadX"],
    },
    // Dim where no light reaches: the orbs carry the lights (render-only).
    lighting: { ambient: "#5a5f80" },
    window: { scaling: "integer" },
  },
  start: "lab",
  scenes: { lab },
  assets: {
    sounds: { click: { wave: "square", freq: 660, duration: 0.03, volume: 0.15 } },
    // Vulkan GLSL fragment shaders: `alpha run` compiles them with glslang (hot reload on save),
    // `alpha export` packs SPIR-V (.spv) instead. The contract is in the shaders guide.
    shaders: {
      wave: shader("wave"),
      chroma: shader("chroma"),
      sepia: shader("sepia"),
      posterize: shader("posterize"),
      scanlines: shader("scanlines"),
      pulse: shader("pulse"),
    },
    ui: { hud: "assets/ui/hud.rml", touch: "assets/ui/touch.rml" },
  },
});
