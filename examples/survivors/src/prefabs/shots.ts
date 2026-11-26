// Weapon entities: what each weapon puts in the world. Damage numbers ride in `data`.
import type { Vec2 } from "alpha";
import { vec } from "alpha";
import type { Spec } from "../types";

export interface Hit { dmg: number; pierce: number; life: number; weapon: string }

/** A flying bolt or lance: a sensor that reports overlaps with enemies (ctx.onEnter). */
export function bolt(from: Vec2, dir: Vec2, speed: number, lance: boolean, hit: Hit): Spec {
  const len = lance ? 30 : 12;
  return {
    tags: ["shot"], pos: from, z: 8, layer: "fx", rotation: vec.angle(dir),
    shape: { sheet: "fx", animation: lance ? "lance" : "bolt", size: [lance ? 36 : 16, lance ? 36 : 16] },
    light: { radius: lance ? 48 : 30, color: lance ? "#fff2b0" : "#ff9a3c", intensity: 1.2 },
    body: {
      type: "dynamic", bullet: true, velocity: vec.scale(dir, speed), category: "shot", collidesWith: ["enemy"],
      collider: { circle: len / 4, sensor: true },
    },
    data: { ...hit, vel: vec.scale(dir, speed) },
  };
}

/** A whip crack: lives a moment; the weapon applies its damage with an area query. */
export function slash(at: Vec2, flip: boolean, scale: number): Spec {
  return {
    tags: ["slash"], pos: at, z: 8, layer: "fx",
    shape: { sheet: "fx", animation: "slash", size: [48 * scale, 32 * scale], flipX: flip },
    light: { radius: 50 * scale, color: "#ff7a2a", intensity: 1.4 },
    data: { life: 0.18 },
  };
}

export function lantern(at: Vec2, index: number, until: number, dmg: number): Spec {
  return {
    tags: ["orb"], pos: at, z: 8, layer: "fx",
    shape: { sheet: "fx", animation: "lantern", size: [18, 18] },
    light: { radius: 44, color: "#ffcf6b", intensity: 1.3 },
    data: { index, until, dmg },
  };
}

export function pool(at: Vec2, radius: number, hit: Hit): Spec {
  return {
    tags: ["pool"], pos: at, z: 1, layer: "floor",
    shape: { sheet: "fx", animation: "pool", size: [radius * 2.4, radius * 2.4], alpha: 0.9 },
    light: { radius: radius * 2.2, color: "#ff8a2a", intensity: 1.1 },
    emitter: { rate: 10, lifetime: [0.3, 0.7], speed: [8, 20], angle: [250, 290], size: [3, 0], color: ["#ffd36bff", "#ff5a1a00"], blend: "add", max: 20 },
    data: { ...hit, radius, tick: 0 },
  };
}

/** A lightning strike drawn at the struck foe. */
export function zap(at: Vec2, scale: number): Spec {
  return {
    tags: ["zap"], pos: [at[0], at[1] - 14 * scale], z: 9, layer: "fx",
    shape: { sheet: "fx", animation: "spark", size: [28 * scale, 40 * scale], blend: "add" },
    light: { radius: 60 * scale, color: "#c9a6ff", intensity: 2 },
    data: { life: 0.2 },
  };
}
