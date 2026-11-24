// Entry point: alpha-build bundles this and everything it imports into dist/main.js.
import { game } from "./alpha";
import over from "./scenes/over";
import play from "./scenes/play";
import title from "./scenes/title";
import { WORLD } from "./tuning";

export default game({
  config: {
    title: "Flappy",
    size: [WORLD.width, WORLD.height],
    background: "#70c5ce",
    seed: 42,
    gravity: [0, 900],
    actions: { flap: ["Space", "MouseLeft", "GamepadA"], restart: ["R"] },
  },
  start: "title",
  scenes: { title, play, over },
  assets: { sounds: { flap: "sfx/flap.wav", point: "sfx/point.wav" } },
});
