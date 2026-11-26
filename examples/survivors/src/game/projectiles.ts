// What weapons left in the world: bolts age and pierce, lanterns orbit, pools burn, flashes fade.
import { vec } from "alpha";
import type { Ent } from "../types";
import { at, damage, ready } from "./combat";
import type { Play, Stats } from "./run";

/** A bolt touched an enemy (sensor overlap): hurt it, use one pierce. */
export function boltHit(ctx: Play, shot: Ent, e: Ent) {
  if (!shot.alive || !e.alive) return;
  damage(ctx, e, shot.data.dmg as number, at(shot), 0.7);
  shot.data.pierce = (shot.data.pierce as number) - 1;
  if ((shot.data.pierce as number) <= 0) ctx.destroy(shot);
}

function ageShots(ctx: Play) {
  for (const s of ctx.findAll("shot")) {
    s.data.life = (s.data.life as number) - ctx.dt;
    if ((s.data.life as number) <= 0) ctx.destroy(s);
  }
  for (const s of [...ctx.findAll("slash"), ...ctx.findAll("zap")]) {
    s.data.life = (s.data.life as number) - ctx.dt;
    s.alpha = Math.max(0, (s.data.life as number) / 0.2);
    if ((s.data.life as number) <= 0) ctx.destroy(s);
  }
}

function orbit(ctx: Play, hero: Ent, st: Stats) {
  const orbs = ctx.findAll("orb");
  const radius = 36 * st.area;
  for (const o of orbs) {
    if ((o.data.until as number) < ctx.state.time) {
      ctx.destroy(o);
      continue;
    }
    const angle = ctx.state.time * 200 + (o.data.index as number) * (360 / orbs.length);
    const p = vec.add(at(hero), vec.fromAngle(angle, radius));
    o.pos.x = p[0];
    o.pos.y = p[1];
    for (const e of ctx.queryCircle(p, 10, { tags: ["enemy"] })) {
      if (ready(ctx, e, "orb", 0.5)) damage(ctx, e, o.data.dmg as number, at(hero), 0.8);
    }
  }
}

function burn(ctx: Play) {
  for (const p of ctx.findAll("pool")) {
    p.data.life = (p.data.life as number) - ctx.dt;
    if ((p.data.life as number) < 0.5) p.alpha = Math.max(0, (p.data.life as number) / 0.5);
    if ((p.data.life as number) <= 0) {
      ctx.destroy(p);
      continue;
    }
    for (const e of ctx.queryCircle(at(p), p.data.radius as number, { tags: ["enemy"] })) {
      if (ready(ctx, e, "pool", 0.4)) damage(ctx, e, p.data.dmg as number, at(p), 0);
    }
  }
}

/** Bolts keep their speed even after a knock (velocity re-applied while the run plays). */
function steerShots(ctx: Play) {
  for (const s of ctx.findAll("shot")) s.body.velocity = s.data.vel as [number, number];
}

export function updateProjectiles(ctx: Play, hero: Ent, st: Stats) {
  ageShots(ctx);
  steerShots(ctx);
  orbit(ctx, hero, st);
  burn(ctx);
}

/** While a menu is up the world holds still: bodies stop (their speeds come back next step). */
export function freeze(ctx: Play) {
  for (const e of [...ctx.findAll("enemy"), ...ctx.findAll("shot")]) e.body.velocity = [0, 0];
}
