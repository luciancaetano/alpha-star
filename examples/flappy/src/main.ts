// Entry point: alpha-build bundles this and everything it imports into dist/main.js.
import { game } from "./alpha";
import over from "./scenes/over";
import play from "./scenes/play";
import settings from "./scenes/settings";
import title from "./scenes/title";
import { WORLD } from "./tuning";

export default game({
  config: {
    title: "Flappy",
    size: [WORLD.width, WORLD.height],
    background: "#70c5ce",
    seed: 42,
    // Smooth look: antialiased shapes and linear image filtering (the pixel font opts back in).
    pixelMode: false,
    gravity: [0, 900],
    // Any face button or the right trigger flaps, on any player's gamepad.
    actions: {
      flap: ["Space", "MouseLeft", "GamepadA", "GamepadB", "GamepadX", "GamepadY", "GamepadRT"],
      restart: ["R"],
      settings: ["S", "GamepadStart", "GamepadBack"],
    },
    locale: "en",
  },
  start: "title",
  scenes: { title, play, over, settings },
  assets: {
    // Procedural: no audio files to ship. A file path (.wav/.ogg/.mp3) here would load one instead.
    sounds: {
      flap: { wave: "square", freq: [300, 600], duration: 0.08, volume: 0.25 },
      point: [
        { wave: "triangle", freq: 880, duration: 0.06, volume: 0.4 },
        { wave: "triangle", freq: 1320, duration: 0.12, volume: 0.4 },
      ],
    },
    // Streamed while it plays (never loaded whole). CC0, made by tools/make_music.py.
    music: { theme: "assets/music/theme.ogg" },
    // ctx.t(key) and data-t="key" in the .rml documents.
    locales: { en: "assets/locales/en.json", pt: "assets/locales/pt.json" },
    // Fonts are used by name in the .rcss (font-family: pixel). OFL, see assets/fonts/OFL.txt.
    fonts: { pixel: { file: "assets/fonts/PressStart2P-Regular.ttf", pixelMode: true } },
    // Each document's own <link>/src files (flappy.rcss) are packed with it.
    ui: {
      title: "assets/ui/title.rml",
      hud: "assets/ui/hud.rml",
      over: "assets/ui/over.rml",
      settings: "assets/ui/settings.rml",
    },
  },
});
