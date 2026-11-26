// A weapon's hit: crits, executes, knockback, healing and status effects (burn, slow, freeze),
// then damage (combat.ts). Statuses tick here and set how fast a foe may move.
import type { Vec2 } from "alpha";
import { burst } from "../prefabs/effects";
import { item } from "../prefabs/pickups";
import type { Ent, Mod } from "../types";
import { at, damage, ready } from "./combat";
import type { Play, Stats } from "./run";

export interface Hit { dmg: number; mod: Mod; weapon: string }

const PUSH: Partial<Record<Mod, number>> = { knock: 2.2, pull: -1.4 };

function finalDamage(ctx: Play, e: Ent, hit: Hit, st: Stats): number {
  const crit = ctx.random.range(0, 1) < st.crit + (hit.mod === "crit" ? 0.25 : 0);
  const dmg = crit ? hit.dmg * st.critMul : hit.dmg;
  const line = st.execute + (hit.mod === "execute" ? 0.15 : 0);
  const hp = e.data.hp as number;
  const doomed = line > 0 && !e.tags.includes("boss") && hp - dmg < (e.data.max as number) * line;
  return doomed ? Math.max(dmg, hp) : dmg;
}

function afflict(ctx: Play, e: Ent, hit: Hit, st: Stats) {
  const t = ctx.state.time;
  if (hit.mod === "burn" || ctx.random.range(0, 1) < st.burn) {
    e.data.burn = 3;
    e.data.burnDps = Math.max(e.data.burnDps as number, hit.dmg * 0.4);
  }
  if (hit.mod === "slow" || ctx.random.range(0, 1) < st.slow) e.data.slow = t + 2;
  if (hit.mod === "freeze" && !e.tags.includes("boss")) e.data.frozen = t + 0.9;
}

function blast(ctx: Play, pos: Vec2, hit: Hit, st: Stats) {
  ctx.spawn(burst(pos, "#ffb347", 8, 50));
  for (const o of ctx.queryCircle(pos, 20 * st.area, { tags: ["enemy"] }).filter((x) => x.alive)) {
    damage(ctx, o, hit.dmg * 0.5, pos, 0.6);
  }
}

/** Hurts `e` with a weapon's hit, pushed away from `from`. Returns true when it died. */
export function strike(ctx: Play, e: Ent, hit: Hit, from: Vec2, push: number, st: Stats): boolean {
  if (!e.alive) return false;
  const pos = at(e);
  const died = damage(ctx, e, finalDamage(ctx, e, hit, st), from, push * st.knock * (PUSH[hit.mod] ?? 1));
  const heal = st.leech + (hit.mod === "leech" ? 0.6 : 0);
  if (heal > 0) ctx.state.hp = Math.min(st.maxHp, ctx.state.hp + heal);
  if (!died) afflict(ctx, e, hit, st);
  else if (hit.mod === "gold" && ctx.random.range(0, 1) < 0.25) ctx.spawn(item(pos, "coin"));
  if (hit.mod === "blast") blast(ctx, pos, hit, st);
  return died;
}

/** Burning foes take damage twice a second; statuses wear off. */
export function tickStatuses(ctx: Play) {
  for (const e of ctx.findAll("enemy")) {
    if (!e.alive || (e.data.burn as number) <= 0) continue;
    e.data.burn = (e.data.burn as number) - ctx.dt;
    if (ready(ctx, e, "burn", 0.5)) damage(ctx, e, (e.data.burnDps as number) * 0.5, at(e), 0);
    if ((e.data.burn as number) <= 0) e.data.burnDps = 0;
  }
}

/** Movement multiplier from statuses: frozen 0, slowed half. */
export function pace(ctx: Play, e: Ent): number {
  if ((e.data.frozen as number) > ctx.state.time) return 0;
  return (e.data.slow as number) > ctx.state.time ? 0.5 : 1;
}

/** The color a foe shows while no hit flash is on. */
export function statusTint(ctx: Play, e: Ent): `#${string}` {
  if ((e.data.frozen as number) > ctx.state.time) return "#9be7ff";
  if ((e.data.burn as number) > 0) return "#ffb070";
  return (e.data.slow as number) > ctx.state.time ? "#b0c0ff" : "#ffffff";
}
