// Showcase run (V on the title): a late-night build with every weapon kind evolved, short
// cooldowns and a big horde, walked by an autopilot. Used for the engine's demo video.
import type { Vec2 } from "alpha";
import { EVENTS } from "../data/waves";
import type { HeroId, WeaponId } from "../types";
import { newRun, type RunState } from "./run";

/** Starts just before the Moth Queen arrives. */
const START = 292;

const ARSENAL: WeaponId[] = ["flare", "ring", "arc", "hail", "wreath", "sunray", "chime", "burst", "spark", "lanterns"];

export function demoRun(hero: HeroId): RunState {
  const s = newRun(hero, {});
  s.demo = true;
  s.time = START;
  s.level = 40;
  s.events = EVENTS.filter((e) => e.at < START).map((e) => `${e.at}`);
  s.weapons = ARSENAL.map((id) => ({ id, level: 6, evo: true, cd: 0.2 }));
  // Short cooldowns, many projectiles, a wide reach and a horde sixteen times as thick (and tougher: enemies.ts).
  s.passives = { tome: 8, prism: 3, glass: 5, heart: 5, echo: 4, fang: 3, eye: 3, quill: 3, hourglass: 3, omen: 150, lode: 5 };
  return s;
}

/** A slow figure eight around the start, so the horde keeps coming from every side. */
export function autopilot(time: number): Vec2 {
  const t = (time - START) * 0.5;
  return [Math.cos(t), Math.sin(2 * t) * 0.8];
}
