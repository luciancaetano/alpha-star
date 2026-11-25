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
    // Pixel art: whole-number scaling only (nearest filtering), so every pixel stays square.
    window: { scaling: "integer" },
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
    // Tiled map (JSON). Its tileset (tiles.tsj) and the tileset image are packed with it.
    maps: { level1: "assets/maps/level1.tmj" },
    fonts: { pixel: "assets/fonts/PressStart2P-Regular.ttf" },
    ui: { hud: "assets/ui/hud.rml", won: "assets/ui/won.rml" },
  },
});
