// Weapons that hit an area at once: whips, beams, pools and auras, rains, sparks, chains, novas.
import type { Vec2 } from "alpha";
import { vec } from "alpha";
import { shock } from "../prefabs/effects";
import { beam, pool, slash, zap } from "../prefabs/shots";
import type { Ent } from "../types";
import { at, pan } from "./combat";
import { strike } from "./strike";
import { type Fire, nearest } from "./weapons";

const foesIn = (f: Fire, center: Vec2, radius: number) =>
  f.ctx.queryCircle(center, radius, { tags: ["enemy"] });

export function fireWhip(f: Fire) {
  const sides = f.w.evo ? [1, -1] : f.count >= 2 ? [f.ctx.state.facing, -f.ctx.state.facing] : [f.ctx.state.facing];
  const from = at(f.hero);
  sides.forEach((side, i) => {
    f.ctx.after(0.001 + i * 0.12, () => {
      if (!f.hero.alive) return;
      const center: Vec2 = [f.hero.pos.x + side * 30 * f.area, f.hero.pos.y - 2];
      f.ctx.spawn(slash(center, side < 0, f.area, f.def.color));
      const box: [number, number, number, number] = [center[0] - 30 * f.area, center[1] - 12 * f.area, 60 * f.area, 26 * f.area];
      for (const e of f.ctx.queryRect(box, { tags: ["enemy"] })) strike(f.ctx, e, f.hit, from, 1.4, f.st);
    });
  });
  f.ctx.audio.play("whip", { volume: 0.4, pan: pan(f.ctx, from[0]) });
}

/** A straight ray at each of the nearest foes (facing when none), hitting all along it. */
export function fireBeam(f: Fire) {
  const from = at(f.hero);
  const targets = nearest(f.ctx, from, f.count, 200);
  const length = 150 * f.area;
  const dirs = targets.length ? targets.map((t) => vec.norm(vec.sub(at(t), from))) : [[f.ctx.state.facing, 0] as Vec2];
  for (const dir of dirs) {
    f.ctx.spawn(beam(from, dir, length, 12 * f.area, f.def.color));
    const struck = new Set<Ent<"enemy">>();
    for (let d = 8; d < length; d += 10) {
      for (const e of foesIn(f, vec.add(from, vec.scale(dir, d)), 8 * f.area)) {
        if (struck.has(e)) continue;
        struck.add(e);
        strike(f.ctx, e, f.hit, from, 0.8, f.st);
      }
    }
  }
  f.ctx.audio.play("zap", { volume: 0.25, pitch: 0.6 });
}

export function firePool(f: Fire) {
  const foes = foesIn(f, at(f.hero), 170);
  for (let i = 0; i < f.count; i++) {
    const spot: Vec2 = foes.length ? at(f.ctx.random.pick(foes)) : vec.add(at(f.hero), vec.fromAngle(f.ctx.random.range(0, 360), 50));
    f.ctx.spawn(pool(spot, 16 * f.area, { ...f.hit, pierce: 99, life: f.life * f.st.duration }, f.def.color));
  }
  f.ctx.audio.play("pool", { volume: 0.3 });
}

/** A ring that stays on the hero; renewed every cooldown, bigger with count. */
export function fireAura(f: Fire) {
  for (const p of f.ctx.findAll("pool")) if (p.data.weapon === f.w.id) f.ctx.destroy(p);
  const radius = (24 + 4 * (f.count - 1)) * f.area;
  f.ctx.spawn(pool(at(f.hero), radius, { ...f.hit, pierce: 99, life: f.w.cd + 0.1 }, f.def.color, true));
}

function blastAt(f: Fire, spot: Vec2, radius: number) {
  f.ctx.spawn(zap(spot, f.area, f.def.color));
  for (const e of foesIn(f, spot, radius)) strike(f.ctx, e, f.hit, spot, 0.5, f.st);
}

/** Strikes rain on random spots around the hero, one after another. */
export function fireRain(f: Fire) {
  for (let i = 0; i < f.count; i++) {
    f.ctx.after(0.001 + i * 0.08, () => {
      if (!f.hero.alive) return;
      blastAt(f, vec.add(at(f.hero), vec.fromAngle(f.ctx.random.range(0, 360), f.ctx.random.range(20, 110))), 18 * f.area);
    });
  }
  f.ctx.audio.play("zap", { volume: 0.25, pitch: 0.8 });
}

/** Lightning on random foes in view. */
export function fireSpark(f: Fire) {
  const [vx, vy, vw, vh] = f.ctx.camera.view;
  const foes = f.ctx.queryRect([vx, vy, vw, vh], { tags: ["enemy"] });
  for (let i = 0; i < f.count && foes.length; i++) {
    const t = foes.splice(f.ctx.random.int(0, foes.length - 1), 1)[0];
    if (t.alive) blastAt(f, at(t), 14 * f.area);
  }
  f.ctx.audio.play("zap", { volume: 0.3, pitch: f.ctx.random.range(0.9, 1.2) });
}

/** Strikes the nearest foe, then jumps to the next nearest one `count` times. */
export function fireChain(f: Fire) {
  let from = at(f.hero);
  const struck = new Set<Ent<"enemy">>();
  for (let i = 0; i < f.count + 1; i++) {
    const next = nearest(f.ctx, from, 8, i === 0 ? 160 : 70 * f.area).find((e) => !struck.has(e));
    if (!next) break;
    struck.add(next);
    const spot = at(next);
    f.ctx.spawn(zap(spot, 0.8, f.def.color));
    strike(f.ctx, next, f.hit, from, 0.5, f.st);
    from = spot;
  }
  if (struck.size) f.ctx.audio.play("zap", { volume: 0.25, pitch: 1.3 });
}

/** A shockwave around the hero. */
export function fireNova(f: Fire) {
  const from = at(f.hero);
  const radius = (44 + 6 * (f.count - 1)) * f.area;
  f.ctx.spawn(shock(from, f.def.color, radius));
  for (const e of foesIn(f, from, radius)) strike(f.ctx, e, f.hit, from, 1.2, f.st);
  f.ctx.audio.play("pool", { volume: 0.35, pitch: 1.6 });
}
