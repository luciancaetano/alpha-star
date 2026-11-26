// Entry point: `alpha export` bundles this and everything it imports into dist/main.js.
import type { Note } from "alpha";
import { game } from "./alpha";
import over from "./scenes/over";
import play from "./scenes/play";
import title from "./scenes/title";
import { VIEW } from "./tuning";

const fly = (frame: [number, number]) => ({ image: "", frame, animations: { fly: { frames: [0, 1], fps: 5 } } });
const sheet = (name: string, cell: number) => ({ ...fly([cell, cell]), image: `assets/img/${name}.png` });
const hero = (name: string) => ({
  image: `assets/img/${name}.png`, frame: [32, 32] as [number, number],
  animations: { idle: { frames: [0] }, walk: { frames: [1, 2, 3, 0], fps: 9 } },
});
const strip = <N extends string>(names: N[]) => Object.fromEntries(names.map((n, i) => [n, { frames: [i] }])) as Record<N, { frames: number[] }>;

// A slow minor lullaby (A minor arpeggios), looped quietly under the night.
const ARP = [220, 262, 330, 262, 175, 220, 262, 220, 196, 247, 294, 247, 165, 208, 247, 330];
const theme: Note[] = ARP.flatMap((f) => [{ wave: "triangle", freq: f, duration: 0.42, volume: 0.22, release: 0.2 }]);

export default game({
  config: {
    title: "Candlewick",
    size: [VIEW.width, VIEW.height],
    uiFilter: "sharp",
    background: "#0b0a12",
    seed: 13,
    gravity: [0, 0],
    actions: {
      pick1: ["1"], pick2: ["2"], pick3: ["3"],
      pause: ["Escape", "P", "GamepadStart", "TouchPause"],
      restart: ["R"],
      showcase: ["V"],
    },
    axes: { moveX: ["GamepadLeftX", "A/D", "Left/Right", "TouchMoveX"], moveY: ["GamepadLeftY", "W/S", "Up/Down", "TouchMoveY"] },
    // Phones: a floating stick anywhere on the left half moves; a small button in the top-right corner pauses.
    touchSticks: { move: { zone: "left", radius: 28, scenes: ["play"] } },
    touchButtons: { pause: { pos: [0.95, 0.08], radius: 14, label: "II", scenes: ["play"] } },
    window: { scale: "integer" },
    layers: { floor: { z: -50 }, fx: { z: 50 } },
    collision: { categories: ["hero", "enemy", "shot"] },
    lighting: { ambient: "#3a3450" },
    locale: "pt",   // default language (and fallback for a missing key); the title button switches and remembers
  },
  start: "title",
  scenes: { title, play, over },
  assets: {
    sounds: {
      bolt: { wave: "square", freq: [700, 300], duration: 0.07, volume: 0.15 },
      whip: { wave: "noise", freq: 1, duration: 0.12, volume: 0.25, attack: 0.02 },
      zap: [{ wave: "saw", freq: [1400, 200], duration: 0.08, volume: 0.25 }, { wave: "noise", freq: 1, duration: 0.08, volume: 0.15 }],
      pool: { wave: "sine", freq: [180, 90], duration: 0.2, volume: 0.3 },
      hit: { wave: "square", freq: [300, 150], duration: 0.04, volume: 0.15 },
      kill: { wave: "triangle", freq: [500, 120], duration: 0.08, volume: 0.25 },
      gem: { wave: "sine", freq: [1200, 1600], duration: 0.05, volume: 0.2 },
      coin: [{ wave: "square", freq: 988, duration: 0.05, volume: 0.2 }, { wave: "square", freq: 1319, duration: 0.12, volume: 0.2 }],
      levelup: [523, 659, 784, 1047].map((f) => ({ wave: "triangle" as const, freq: f, duration: 0.08, volume: 0.35 })),
      chest: [392, 523, 659, 784, 1047, 1319].map((f) => ({ wave: "square" as const, freq: f, duration: 0.07, volume: 0.25 })),
      evolve: [262, 330, 392, 523, 659, 784, 1047, 1568].map((f) => ({ wave: "triangle" as const, freq: f, duration: 0.09, volume: 0.4 })),
      hurt: { wave: "saw", freq: [240, 70], duration: 0.18, volume: 0.4 },
      heal: [{ wave: "sine", freq: 660, duration: 0.08 }, { wave: "sine", freq: 880, duration: 0.15 }],
      bell: [{ wave: "sine", freq: 1568, duration: 0.6, volume: 0.5, release: 0.5 }],
      boss: [{ wave: "saw", freq: [110, 55], duration: 1.2, volume: 0.45, release: 0.4 }],
      select: { wave: "triangle", freq: [660, 990], duration: 0.08, volume: 0.3 },
      death: [{ wave: "triangle", freq: [440, 110], duration: 0.8, volume: 0.45 }],
      theme,
    },
    locales: { pt: "assets/locales/pt.json", en: "assets/locales/en.json" },
    images: { ground: "assets/img/ground.png", logo: "assets/ui/logo.png" },
    sheets: {
      wick: hero("wick"), taper: hero("taper"), lumen: hero("lumen"),
      ash: hero("ash"), vesper: hero("vesper"), tallow: hero("tallow"),
      moth: sheet("moth", 24), shade: sheet("shade", 24), wisp: sheet("wisp", 24), ghoul: sheet("ghoul", 32),
      gargoyle: sheet("gargoyle", 48), queen: sheet("queen", 96), snuffer: sheet("snuffer", 72),
      items: { image: "assets/img/items.png", frame: [14, 14], animations: strip(["xp1", "xp2", "xp3", "coin", "cake", "magnet", "bell", "wax"]) },
      props: { image: "assets/img/props.png", frame: [32, 32], animations: { ...strip(["chest", "open", "brazier", "grave", "reeds", "shrooms", "stones", "stump"]), brazier: { frames: [2, 3], fps: 6 }, grave: { frames: [4] }, reeds: { frames: [5] }, shrooms: { frames: [6] }, stones: { frames: [7] }, stump: { frames: [8] } } },
      fx: { image: "assets/img/fx.png", frame: [32, 32], animations: strip(["bolt", "lance", "slash", "lantern", "pool", "spark"]) },
    },
    fonts: {
      pixel: { file: "assets/fonts/PixelifySans.ttf", license: "assets/fonts/OFL.txt" },
      icons: "assets/fonts/icons.fnt",
      bigicons: "assets/fonts/bigicons.fnt",
    },
    ui: {
      title: "assets/ui/title.rml", heroes: "assets/ui/heroes.rml", shop: "assets/ui/shop.rml", hud: "assets/ui/hud.rml",
      levelup: "assets/ui/levelup.rml", chest: "assets/ui/chest.rml", paused: "assets/ui/paused.rml", over: "assets/ui/over.rml",
    },
  },
});
