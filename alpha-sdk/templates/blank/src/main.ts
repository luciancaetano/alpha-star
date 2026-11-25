// Entry point: `alpha export` bundles this and everything it imports into dist/main.js.
import { game } from "./alpha";
import play from "./scenes/play";
import { VIEW } from "./tuning";

export default game({
  config: {
    title: "My Game",
    size: [VIEW.width, VIEW.height],
    background: "#1d2b53",
    seed: 1,
    actions: { jump: ["Space", "GamepadA"] },
    axes: { moveX: ["GamepadLeftX", "A/D", "Left/Right"], moveY: ["GamepadLeftY", "W/S", "Up/Down"] },
  },
  start: "play",
  scenes: { play },
  assets: {
    sounds: { blip: { wave: "square", freq: [440, 880], duration: 0.08, volume: 0.3 } },
    // Placeholder art made by tools/make-assets.mjs (CC0). Replace freely.
    images: { player: "assets/img/player.png" },
  },
});
