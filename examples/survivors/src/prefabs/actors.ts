// Hero and enemies: plain data built by functions.
import type { Vec2 } from "alpha";
import { ENEMIES, HERO, HEROES } from "../tuning";
import type { EnemyKind, HeroId, Spec } from "../types";

export function hero(id: HeroId, at: Vec2): Spec {
  return {
    tags: ["hero"], pos: at, z: 5,
    shape: { sheet: id, animation: "idle" },
    body: { type: "kinematic", fixedRotation: true, category: "hero", collidesWith: ["enemy"], collider: { circle: HERO.radius, sensor: true } },
    light: { radius: HERO.light, color: HEROES[id].tint, intensity: 1.6 },
  };
}

export const HP_SEGMENTS = 12;
/** Under the hero: a dark plate (index -1) and 12 red segments the HUD shows or hides. */
export function hpBar(index: number): Spec {
  return index < 0
    ? { tags: ["hpbar"], pos: [0, 13], z: 6, shape: { rect: [28, 4], color: "#1a1020" } }
    : { tags: ["hpbar"], pos: [-11 + index * 2, 13], z: 7, shape: { rect: [2, 2], color: "#ff4d5e" }, data: { seg: index } };
}

const full = (hp: number) => ({ hp, max: hp });

export function enemy(kind: EnemyKind, at: Vec2, hpMul: number): Spec {
  const d = ENEMIES[kind];
  const big = kind === "queen" || kind === "snuffer";
  const tags: Spec["tags"] = big ? ["enemy", "boss"] : kind === "gargoyle" ? ["enemy", "elite"] : ["enemy"];
  return {
    tags, pos: at, z: big ? 4 : 3,
    shape: { sheet: kind, animation: "fly" },
    body: { type: "dynamic", fixedRotation: true, category: "enemy", collidesWith: ["enemy", "shot", "hero"], collider: { circle: d.radius, density: big ? 20 : 1 } },
    ...(kind === "wisp" ? { light: { radius: 40, color: "#5fd8ff", intensity: 0.8 } } : {}),
    ...(big ? { light: { radius: 90, color: kind === "queen" ? "#c58bff" : "#ff3b3b", intensity: 1.2 } } : {}),
    data: { kind, ...full(Math.round(d.hp * (kind === "snuffer" ? 1 : hpMul))), kb: 0, flash: 0, hit: {}, burn: 0, burnDps: 0, slow: 0, frozen: 0 },
  };
}
