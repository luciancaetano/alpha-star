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
    // Any face button or the right trigger flaps, on any player's gamepad.
    actions: {
      flap: ["Space", "MouseLeft", "GamepadA", "GamepadB", "GamepadX", "GamepadY", "GamepadRT"],
      restart: ["R"],
    },
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
    // Fonts are used by name in the .rcss (font-family: pixel). OFL, see assets/fonts/OFL.txt.
    fonts: { pixel: "assets/fonts/PressStart2P-Regular.ttf" },
    // Each document's own <link>/src files (flappy.rcss) are packed with it.
    ui: { title: "assets/ui/title.rml", hud: "assets/ui/hud.rml", over: "assets/ui/over.rml" },
  },
});
