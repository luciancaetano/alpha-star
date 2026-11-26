// Entry point: `alpha export` bundles this and everything it imports into dist/main.js.
import { game } from "./alpha";
import over from "./scenes/over";
import play from "./scenes/play";
import { VIEW } from "./tuning";

export default game({
  config: {
    title: "Arena",
    size: [VIEW.width, VIEW.height],
    background: "#1e272e",
    seed: 7,
    gravity: [0, 0],  // seen from above: nothing falls
    actions: {
      fire: ["Space", "J", "MouseLeft", "GamepadRT", "GamepadA"],
      bomb: ["K", "MouseRight", "GamepadB"],
      restart: ["R", "GamepadStart"],
    },
    // Player 1 also has the keyboard; each gamepad is its own player.
    axes: { moveX: ["GamepadLeftX", "A/D", "Left/Right"], moveY: ["GamepadLeftY", "W/S", "Up/Down"] },
    // Pixel art: whole-number scale (square pixels); aspect "expand" (default) fills any screen with more world.
    window: { scale: "integer" },
    // Dim where no light reaches: heroes and bullets carry lights (render-only).
    lighting: { ambient: "#404858" },
  },
  start: "play",
  scenes: { play, over },
  assets: {
    sounds: {
      shot: { wave: "square", freq: [900, 500], duration: 0.05, volume: 0.2 },
      hit: { wave: "saw", freq: [220, 80], duration: 0.2, volume: 0.4 },
      boom: { wave: "noise", freq: 1, duration: 0.15, volume: 0.3 },
    },
    // Generated placeholder art (tools/make-assets.mjs), CC0.
    sheets: {
      hero: { image: "assets/img/hero.png", frame: [16, 16], animations: { idle: { frames: [0] }, walk: { frames: [1, 0, 2, 0], fps: 10 } } },
    },
    ui: { hud: "assets/ui/hud.rml", over: "assets/ui/over.rml" },
  },
});
