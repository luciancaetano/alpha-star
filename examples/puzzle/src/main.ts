// Entry point: `alpha export` bundles this and everything it imports into dist/main.js.
import { game } from "./alpha";
import play from "./scenes/play";
import solved from "./scenes/solved";
import start from "./scenes/start";
import { VIEW } from "./tuning";

export default game({
  config: {
    title: "Crates",
    size: [VIEW.width, VIEW.height],
    background: "#2d1e2f",
    seed: 1,
    actions: {
      up: ["Up", "W", "GamepadUp", "GamepadLeftY-"],
      down: ["Down", "S", "GamepadDown", "GamepadLeftY+"],
      left: ["Left", "A", "GamepadLeft", "GamepadLeftX-"],
      right: ["Right", "D", "GamepadRight", "GamepadLeftX+"],
      undo: ["Z", "Backspace", "GamepadX"],
      restart: ["R", "GamepadY"],
      language: ["L", "GamepadBack"],
    },
    locale: "en",
    window: { scaling: "integer" },
  },
  start: "start",
  scenes: { start, play, solved },
  assets: {
    sounds: {
      step: { wave: "triangle", freq: 330, duration: 0.03, volume: 0.15 },
      push: { wave: "square", freq: [140, 110], duration: 0.07, volume: 0.2 },
      bump: { wave: "noise", freq: 1, duration: 0.04, volume: 0.15 },
      solved: [
        { wave: "triangle", freq: 523, duration: 0.08 },
        { wave: "triangle", freq: 659, duration: 0.08 },
        { wave: "triangle", freq: 784, duration: 0.16 },
      ],
    },
    // Generated placeholder art (tools/make-assets.mjs), CC0. Each animation is one frame.
    sheets: {
      tiles: {
        image: "assets/img/tiles.png",
        frame: [16, 16],
        animations: {
          floor: { frames: [0] }, wall: { frames: [1] }, goal: { frames: [2] },
          box: { frames: [3] }, boxOnGoal: { frames: [4] }, player: { frames: [5] },
        },
      },
    },
    locales: { en: "assets/locales/en.json", pt: "assets/locales/pt.json" },
    ui: { hud: "assets/ui/hud.rml", solved: "assets/ui/solved.rml" },
  },
});
