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
  assets: {
    // Procedural: no audio files to ship. A string here would load a WAV instead.
    sounds: {
      flap: { wave: "square", freq: [300, 600], duration: 0.08, volume: 0.25 },
      point: [
        { wave: "triangle", freq: 880, duration: 0.06, volume: 0.4 },
        { wave: "triangle", freq: 1320, duration: 0.12, volume: 0.4 },
      ],
    },
  },
});
