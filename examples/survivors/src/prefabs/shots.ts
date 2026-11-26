// Weapon entities: what each weapon puts in the world, tinted with the weapon's color.
// The hit (damage, mod, weapon id) rides in `data`.
import type { Vec2 } from "alpha";
import { vec } from "alpha";
import type { Hit } from "../game/strike";
import type { Color, Spec } from "../types";

export interface Shot extends Hit { pierce: number; life: number }

const drawnColor = (c: Color, base: Color) => (c === base ? "#ffffff" : c);

/** A flying bolt or lance: a sensor that reports overlaps with enemies (ctx.onEnter).
 *  `turn`: seconds of life left when it flies back (boomerangs). */
export function bolt(from: Vec2, dir: Vec2, speed: number, lance: boolean, hit: Shot, color: Color = "#ff9a3c", turn = -1): Spec {
  const len = lance ? 30 : 12;
  return {
    tags: ["shot"], pos: from, z: 8, layer: "fx", rotation: vec.angle(dir),
    shape: { tint: drawnColor(color, "#ff9a3c"), sheet: "fx", animation: lance ? "lance" : "bolt", size: [lance ? 36 : 16, lance ? 36 : 16] },
    light: { radius: lance ? 48 : 30, color: lance ? "#fff2b0" : color, intensity: 1.2 },
    body: {
      type: "dynamic", bullet: true, velocity: vec.scale(dir, speed), category: "shot", collidesWith: ["enemy"],
      collider: { circle: len / 4, sensor: true },
    },
    data: { ...hit, vel: vec.scale(dir, speed), turn },
  };
}

/** A whip crack or a beam: lives a moment; the weapon applies its damage with area queries. */
export function slash(at: Vec2, flip: boolean, scale: number, color: Color = "#ff7a2a"): Spec {
  return {
    tags: ["slash"], pos: at, z: 8, layer: "fx",
    shape: { tint: drawnColor(color, "#ff7a2a"), sheet: "fx", animation: "slash", size: [48 * scale, 32 * scale], flipX: flip },
    light: { radius: 50 * scale, color, intensity: 1.4 },
    data: { life: 0.18 },
  };
}

export function beam(from: Vec2, dir: Vec2, length: number, width: number, color: Color): Spec {
  return {
    tags: ["slash"], pos: vec.add(from, vec.scale(dir, length / 2)), z: 8, layer: "fx", rotation: vec.angle(dir),
    shape: { tint: color, sheet: "fx", animation: "lance", size: [length, width], blend: "add" },
    light: { radius: 40, color, intensity: 1.2 },
    data: { life: 0.2 },
  };
}

/** An orbiting lantern of weapon `weapon`: index of count, circling at `radius` until `until`. */
export interface Orbit { weapon: string; index: number; count: number; radius: number; until: number; dmg: number; mod: Hit["mod"] }
export function lantern(at: Vec2, o: Orbit, color: Color = "#ffcf6b"): Spec {
  return {
    tags: ["orb"], pos: at, z: 8, layer: "fx",
    shape: { tint: drawnColor(color, "#ffcf6b"), sheet: "fx", animation: "lantern", size: [18, 18] },
    light: { radius: 44, color, intensity: 1.3 },
    data: { ...o },
  };
}

/** A burning pool; `follow`: an aura that stays on the hero. */
export function pool(at: Vec2, radius: number, hit: Shot, color: Color = "#ff8a2a", follow = false): Spec {
  return {
    tags: ["pool"], pos: at, z: 1, layer: "floor",
    shape: { tint: drawnColor(color, "#ff8a2a"), sheet: "fx", animation: "pool", size: [radius * 2.4, radius * 2.4], alpha: follow ? 0.45 : 0.9 },
    light: { radius: radius * 2.2, color, intensity: follow ? 0.6 : 1.1 },
    emitter: { rate: 10, lifetime: [0.3, 0.7], speed: [8, 20], angle: [250, 290], size: [3, 0], color: [`${color}ff` as Color, `${color}00` as Color], blend: "add", max: 20 },
    data: { ...hit, radius, tick: 0, follow },
  };
}

/** A lightning strike drawn at the struck spot. */
export function zap(at: Vec2, scale: number, color: Color = "#c9a6ff"): Spec {
  return {
    tags: ["zap"], pos: [at[0], at[1] - 14 * scale], z: 9, layer: "fx",
    shape: { tint: drawnColor(color, "#c9a6ff"), sheet: "fx", animation: "spark", size: [28 * scale, 40 * scale], blend: "add" },
    light: { radius: 60 * scale, color, intensity: 2 },
    data: { life: 0.2 },
  };
}
