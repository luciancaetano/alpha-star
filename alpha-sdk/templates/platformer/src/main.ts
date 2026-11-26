// Entry point: `alpha export` bundles this and everything it imports into dist/main.js.
import { game } from "./alpha";
import play from "./scenes/play";
import won from "./scenes/won";
import { GRAVITY, VIEW } from "./tuning";

export default game({
  config: {
    title: "Platformer",
    size: [VIEW.width, VIEW.height],
    background: "#87ceeb",
    seed: 1,
    gravity: [0, GRAVITY],
    actions: {
      jump: ["Space", "W", "Up", "GamepadA"],
      restart: ["R", "GamepadStart"],
    },
    axes: { moveX: ["GamepadLeftX", "A/D", "Left/Right"] },
    // Far hills scroll at a fifth of the camera's speed; effects draw over the level.
    layers: { far: { z: -100, parallax: 0.2 }, fx: { z: 100 } },
    // Pixel art: whole-number scale only (nearest filtering), so every pixel stays square. A side
    // scroller keeps its height (what jumps are tuned for); wider screens show more level.
    window: { aspect: "keep_height", scale: "integer" },
  },
  start: "play",
  scenes: { play, won },
  assets: {
    sounds: {
      jump: { wave: "square", freq: [250, 500], duration: 0.1, volume: 0.2 },
      coin: [
        { wave: "triangle", freq: 988, duration: 0.05, volume: 0.35 },
        { wave: "triangle", freq: 1319, duration: 0.1, volume: 0.35 },
      ],
      land: { wave: "noise", freq: 1, duration: 0.08, volume: 0.25 },
    },
    // Generated placeholder art (tools/make-assets.mjs). `alpha export` packs both into one atlas page.
    sheets: {
      hero: {
        image: "assets/img/hero.png",
        frame: [16, 16],
        animations: {
          idle: { frames: [0] },
          run: { frames: [1, 2, 3, 4], fps: 12 },
          jump: { frames: [5] },
          fall: { frames: [6] },
        },
      },
      coin: { image: "assets/img/coin.png", frame: [8, 8], animations: { spin: { frames: [0, 1, 2, 3], fps: 8 } } },
    },
    // LDtk world (.ldtk). Its tileset image is packed with it.
    worlds: { main: "assets/levels/world.ldtk" },
    fonts: { pixel: { file: "assets/fonts/PressStart2P-Regular.ttf", license: "assets/fonts/OFL.txt" } },
    ui: { hud: "assets/ui/hud.rml", won: "assets/ui/won.rml" },
  },
});
