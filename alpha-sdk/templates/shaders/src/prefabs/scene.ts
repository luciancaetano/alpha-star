// Entity specs: plain data built by functions.
import type { Color, EntitySpec, Vec2 } from "alpha";
import { ORBS, VIEW } from "../tuning";
import type { ShaderLab } from "../types";

type Spec = EntitySpec<ShaderLab, "orb" | "floor" | "label">;

const COLORS: Color[] = ["#ff5d73", "#ffd166", "#06d6a0", "#4cc9f0", "#b388ff"];
const TILE = 20;

/** Where orb `i` is at `time`: evenly spaced on an ellipse that turns. */
export function orbitAt(i: number, time: number): Vec2 {
  const a = time * ORBS.speed + (i / ORBS.count) * Math.PI * 2;
  return [VIEW.width / 2 + Math.cos(a) * ORBS.orbitX, VIEW.height / 2 + Math.sin(a) * ORBS.orbitY];
}

// A glowing orb: a light (config.lighting) and a trail of sparks (emitter).
export function orb(i: number): Spec {
  const color = COLORS[i % COLORS.length];
  return {
    tags: ["orb"],
    pos: orbitAt(i, 0),
    z: 2,
    shape: { circle: ORBS.radius, color, outline: "#ffffff" },
    light: { radius: ORBS.light, color, intensity: 1.6 },
    emitter: { rate: 30, lifetime: [0.3, 0.7], speed: [10, 30], size: [3, 1], color: [color, "#ffffff00"], blend: "add", smooth: true },
  };
}

// A checkerboard floor: sharp edges make every distortion easy to see. It covers the visible
// world `[x, y, w, h]` (ctx.camera.view): aspect "expand" shows more on wide (or tall) screens.
export function floor([vx, vy, vw, vh]: readonly number[]): Spec[] {
  const tiles: Spec[] = [];
  for (let y = Math.floor(vy / TILE); y < Math.ceil((vy + vh) / TILE); y++) {
    for (let x = Math.floor(vx / TILE); x < Math.ceil((vx + vw) / TILE); x++) {
      const color = Math.abs(x + y) % 2 ? "#3a3f73" : "#262a52";
      tiles.push({ tags: ["floor"], pos: [x * TILE + TILE / 2, y * TILE + TILE / 2], shape: { rect: [TILE, TILE], color } });
    }
  }
  return tiles;
}

export function title(): Spec {
  return { tags: ["label"], pos: [VIEW.width / 2, VIEW.height / 2], z: 3, shape: { text: "ALPHA", size: 24, color: "#ffffff", align: "center" } };
}
