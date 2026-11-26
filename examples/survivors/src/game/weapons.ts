// Every owned weapon fires on its own cooldown; the hero only walks.
import type { Vec2 } from "alpha";
import { vec } from "alpha";
import { weaponLevel } from "../data/items";
import { bolt, lantern, pool, slash, zap } from "../prefabs/shots";
import type { Ent, WeaponId } from "../types";
import { at, damage, pan, ready } from "./combat";
import type { OwnedWeapon, Play, Stats } from "./run";

interface Fire { ctx: Play; hero: Ent; w: OwnedWeapon; st: Stats; dmg: number; count: number; area: number }

function nearest(ctx: Play, from: Vec2, n: number): Ent[] {
  const foes = ctx.queryCircle(from, 260, { tags: ["enemy"] }).filter((e) => e.alive);
  return foes.map((e) => ({ e, d: vec.dist(at(e), from) })).sort((a, b) => a.d - b.d).slice(0, n).map((x) => x.e);
}

function fireBolt(f: Fire) {
  const from = at(f.hero);
  const targets = nearest(f.ctx, from, f.count);
  const lv = weaponLevel("bolt", f.w.level, f.w.evo);
  for (let i = 0; i < f.count; i++) {
    const t = targets[i % Math.max(1, targets.length)];
    const base = t ? vec.norm(vec.sub(at(t), from)) : ([f.ctx.state.facing, 0] as Vec2);
    const dir = vec.fromAngle(vec.angle(base) + (i >= targets.length ? (i - targets.length + 1) * 12 : 0));
    f.ctx.spawn(bolt(from, dir, f.w.evo ? 300 : 230, f.w.evo, { dmg: f.dmg, pierce: lv.pierce, life: lv.life, weapon: "bolt" }));
  }
  f.ctx.audio.play("bolt", { volume: 0.25, pan: pan(f.ctx, from[0]), pitch: f.ctx.random.range(0.9, 1.1) });
}

function fireWhip(f: Fire) {
  const sides = f.w.evo ? [1, -1] : f.count >= 2 ? [f.ctx.state.facing, -f.ctx.state.facing] : [f.ctx.state.facing];
  const from = at(f.hero);
  sides.forEach((side, i) => {
    f.ctx.after(0.001 + i * 0.12, () => {
      if (!f.hero.alive) return;
      const center: Vec2 = [f.hero.pos.x + side * 30 * f.area, f.hero.pos.y - 2];
      f.ctx.spawn(slash(center, side < 0, f.area));
      for (const e of f.ctx.queryRect([center[0] - 30 * f.area, center[1] - 12 * f.area, 60 * f.area, 26 * f.area], { tags: ["enemy"] }).filter((e) => e.alive)) {
        damage(f.ctx, e, f.dmg, from, 1.4);
      }
    });
  });
  f.ctx.audio.play("whip", { volume: 0.4, pan: pan(f.ctx, from[0]) });
}

function firePool(f: Fire) {
  const foes = f.ctx.queryCircle(at(f.hero), 170, { tags: ["enemy"] }).filter((e) => e.alive);
  const lv = weaponLevel("pool", f.w.level, f.w.evo);
  for (let i = 0; i < f.count; i++) {
    const spot: Vec2 = foes.length ? at(f.ctx.random.pick(foes)) : vec.add(at(f.hero), vec.fromAngle(f.ctx.random.range(0, 360), 50));
    f.ctx.spawn(pool(spot, 16 * f.area, { dmg: f.dmg, pierce: 99, life: lv.life * f.st.duration, weapon: "pool" }));
  }
  f.ctx.audio.play("pool", { volume: 0.3 });
}

function fireSpark(f: Fire) {
  const [vx, vy, vw, vh] = f.ctx.camera.view;
  const foes = f.ctx.queryRect([vx, vy, vw, vh], { tags: ["enemy"] }).filter((e) => e.alive);
  for (let i = 0; i < f.count && foes.length; i++) {
    const t = foes.splice(f.ctx.random.int(0, foes.length - 1), 1)[0];
    if (!t.alive) continue;
    const spot = at(t);
    f.ctx.spawn(zap(spot, f.area));
    for (const e of f.ctx.queryCircle(spot, 14 * f.area, { tags: ["enemy"] }).filter((e) => e.alive)) damage(f.ctx, e, f.dmg, spot, 0.5);
  }
  f.ctx.audio.play("zap", { volume: 0.3, pitch: f.ctx.random.range(0.9, 1.2) });
}

/** Lanterns are permanent while active: spawned once per cycle, removed when it ends. */
function fireLanterns(f: Fire) {
  for (const o of f.ctx.findAll("orb")) f.ctx.destroy(o);
  const life = weaponLevel("lanterns", f.w.level, f.w.evo).life * f.st.duration;
  for (let i = 0; i < f.count; i++) f.ctx.spawn(lantern(at(f.hero), i, f.ctx.state.time + life, f.dmg));
  f.w.cd = life + 3 * f.st.cooldown;
}

const FIRE: Record<WeaponId, (f: Fire) => void> = { bolt: fireBolt, whip: fireWhip, pool: firePool, spark: fireSpark, lanterns: fireLanterns };

export function fireWeapons(ctx: Play, hero: Ent, st: Stats) {
  for (const w of ctx.state.weapons) {
    w.cd -= ctx.dt;
    if (w.cd > 0) continue;
    const lv = weaponLevel(w.id, w.level, w.evo);
    w.cd = lv.cd * st.cooldown;
    FIRE[w.id]({ ctx, hero, w, st, dmg: lv.dmg * st.might, count: lv.count + st.amount, area: lv.area * st.area });
  }
}
