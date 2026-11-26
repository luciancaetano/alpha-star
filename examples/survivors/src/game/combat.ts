// Damage, knockback, death and drops: the one place enemies lose hit points.
import type { Vec2 } from "alpha";
import { vec } from "alpha";
import { burst, label } from "../prefabs/effects";
import { gem, gemLook, item } from "../prefabs/pickups";
import { ENEMIES, RUN } from "../tuning";
import type { Color, EnemyKind, Ent } from "../types";
import type { Play } from "./run";

export const at = (e: Ent): Vec2 => [e.pos.x, e.pos.y];

const BLOOD: Record<EnemyKind, Color> = {
  moth: "#c58bff", shade: "#7a6cff", wisp: "#5fd8ff", ghoul: "#6fdc8c", gargoyle: "#ff7a2a", queen: "#ffd36b", snuffer: "#ff3b3b",
};

export function pan(ctx: Play, x: number) {
  const [vx, , vw] = ctx.camera.view;
  return Math.max(-1, Math.min(1, ((x - vx) / vw) * 2 - 1));
}

function dropGem(ctx: Play, pos: Vec2, value: number) {
  const gems = ctx.findAll("gem");
  if (gems.length < RUN.maxGems) return void ctx.spawn(gem(pos, value));
  const g = ctx.random.pick(gems);
  g.data.v = (g.data.v as number) + value;
  g.animation = gemLook(g.data.v as number);
}

function drops(ctx: Play, kind: EnemyKind, pos: Vec2) {
  if (ENEMIES[kind].xp > 0) dropGem(ctx, pos, ENEMIES[kind].xp);
  if (kind === "gargoyle" || kind === "queen") ctx.spawn(item(vec.add(pos, [0, 6]), "chest"));
  const roll = ctx.random.range(0, 1);
  if (roll < 0.03) ctx.spawn(item(vec.add(pos, [5, 0]), "coin"));
  else if (roll < 0.036) ctx.spawn(item(vec.add(pos, [5, 0]), "cake"));
}

export function kill(ctx: Play, e: Ent) {
  const kind = e.data.kind as EnemyKind;
  const pos = at(e);
  ctx.spawn(burst(pos, BLOOD[kind], kind === "queen" ? 80 : 10));
  ctx.audio.play("kill", { pan: pan(ctx, pos[0]), volume: 0.35, pitch: ctx.random.range(0.85, 1.2) });
  drops(ctx, kind, pos);
  ctx.state.kills += 1;
  ctx.destroy(e);
}

function popNumber(ctx: Play, pos: Vec2, dmg: number) {
  if (ctx.findAll("label").length > 30) return;
  const l = ctx.spawn(label([pos[0] + ctx.random.range(-4, 4), pos[1] - 10], String(Math.round(dmg)), "#fff4d6"));
  ctx.tween(l, { to: { y: pos[1] - 24, alpha: 0 }, seconds: 0.5, ease: "quadOut", onDone: () => ctx.destroy(l) });
}

/** Hurts an enemy; pushes it away from `from`. Returns true when it died. */
export function damage(ctx: Play, e: Ent, dmg: number, from: Vec2, push = 1): boolean {
  if (!e.alive) return false;
  e.data.hp = (e.data.hp as number) - dmg;
  e.data.flash = 0.08;
  e.tint = "#ff9090";
  popNumber(ctx, at(e), dmg);
  if ((e.data.hp as number) <= 0) {
    kill(ctx, e);
    return true;
  }
  if (!e.tags.includes("boss") && push > 0) {
    e.data.kb = 0.12;
    e.body.velocity = vec.scale(vec.norm(vec.sub(at(e), from)), 110 * push);
  }
  return false;
}

/** Per-enemy cooldown per weapon, so areas that linger hit each foe a few times a second. */
export function ready(ctx: Play, e: Ent, key: string, every: number): boolean {
  const hits = e.data.hit as Record<string, number>;
  if ((hits[key] ?? -1) > ctx.state.time) return false;
  hits[key] = ctx.state.time + every;
  return true;
}
