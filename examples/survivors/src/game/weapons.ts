// Every owned weapon fires on its own cooldown; the hero only walks. Shots that fly live here,
// area weapons in areas.ts; both hit through strike.ts.
import type { Vec2 } from "alpha";
import { vec } from "alpha";
import { type WeaponDef, WEAPONS, weaponLevel } from "../data/items";
import { bolt, lantern } from "../prefabs/shots";
import type { Ent, WeaponKind } from "../types";
import { firePool, fireAura, fireBeam, fireChain, fireNova, fireRain, fireSpark, fireWhip } from "./areas";
import { at, pan } from "./combat";
import type { OwnedWeapon, Play, Stats } from "./run";
import type { Hit } from "./strike";

export interface Fire { ctx: Play; hero: Ent<"hero">; w: OwnedWeapon; st: Stats; def: WeaponDef; hit: Hit; count: number; area: number; life: number; pierce: number }

export function nearest(ctx: Play, from: Vec2, n: number, reach = 260): Ent<"enemy">[] {
  const foes = ctx.queryCircle(from, reach, { tags: ["enemy"] });
  return foes.map((e) => ({ e, d: vec.dist(at(e), from) })).sort((a, b) => a.d - b.d).slice(0, n).map((x) => x.e);
}

function shoot(f: Fire, dir: Vec2, speed: number, turn = -1) {
  const lance = f.w.evo && f.def.kind === "bolt";
  f.ctx.spawn(bolt(at(f.hero), dir, speed * f.st.shotSpeed, lance, { ...f.hit, pierce: f.pierce, life: f.life }, f.def.color, turn));
}

/** Aimed at the nearest foes; extra shots fan out from the last aim. */
function fireBolt(f: Fire) {
  const from = at(f.hero);
  const targets = nearest(f.ctx, from, f.count);
  for (let i = 0; i < f.count; i++) {
    const t = targets[i % Math.max(1, targets.length)];
    const base = t ? vec.norm(vec.sub(at(t), from)) : ([f.ctx.state.facing, 0] as Vec2);
    shoot(f, vec.fromAngle(vec.angle(base) + (i >= targets.length ? (i - targets.length + 1) * 12 : 0)), f.w.evo ? 300 : 230);
  }
  f.ctx.audio.play("bolt", { volume: 0.25, pan: pan(f.ctx, from[0]), pitch: f.ctx.random.range(0.9, 1.1) });
}

/** Shots in every direction, evenly spread. */
function fireRing(f: Fire) {
  const start = f.ctx.random.range(0, 360);
  for (let i = 0; i < f.count; i++) shoot(f, vec.fromAngle(start + (i * 360) / f.count), 170);
  f.ctx.audio.play("bolt", { volume: 0.3, pitch: 0.7 });
}

/** Flies out at the nearest foes and comes back halfway through its life. */
function fireBoomerang(f: Fire) {
  const from = at(f.hero);
  const targets = nearest(f.ctx, from, f.count);
  for (let i = 0; i < f.count; i++) {
    const t = targets[i % Math.max(1, targets.length)];
    const dir = t ? vec.norm(vec.sub(at(t), from)) : vec.fromAngle(f.ctx.random.range(0, 360));
    shoot(f, vec.fromAngle(vec.angle(dir) + i * 25), 200, f.life / 2);
  }
  f.ctx.audio.play("whip", { volume: 0.3, pitch: 1.4 });
}

/** Lanterns are permanent while active: spawned once per cycle, removed when it ends. */
function fireOrbit(f: Fire) {
  for (const o of f.ctx.findAll("orb")) if (o.data.weapon === f.w.id) f.ctx.destroy(o);
  const life = f.life * f.st.duration;
  const until = f.ctx.state.time + life;
  for (let i = 0; i < f.count; i++) {
    f.ctx.spawn(lantern(at(f.hero), { weapon: f.w.id, index: i, count: f.count, radius: 36 * f.area, until, dmg: f.hit.dmg, mod: f.hit.mod }, f.def.color));
  }
  f.w.cd += life;   // dark for one cooldown between cycles
}

const FIRE: Record<WeaponKind, (f: Fire) => void> = {
  bolt: fireBolt, ring: fireRing, boomerang: fireBoomerang, orbit: fireOrbit, whip: fireWhip, beam: fireBeam,
  aura: fireAura, pool: firePool, rain: fireRain, spark: fireSpark, chain: fireChain, nova: fireNova,
};

export function fireWeapons(ctx: Play, hero: Ent<"hero">, st: Stats) {
  for (const w of ctx.state.weapons) {
    w.cd -= ctx.dt;
    if (w.cd > 0) continue;
    const lv = weaponLevel(w.id, w.level, w.evo);
    const def = WEAPONS[w.id];
    w.cd = lv.cd * st.cooldown;
    const hit = { dmg: lv.dmg * st.might, mod: def.mod, weapon: w.id };
    FIRE[def.kind]({ ctx, hero, w, st, def, hit, count: lv.count + st.amount, area: lv.area * st.area, life: lv.life, pierce: lv.pierce });
    if (def.kind !== "orbit" && def.kind !== "aura" && ctx.random.range(0, 1) < st.echo) w.cd = Math.min(w.cd, 0.15);
  }
}
