// What weapons left in the world: bolts age, pierce and turn back, lanterns orbit, pools and
// auras burn, flashes fade. Every hit goes through strike.ts.
import { vec } from "alpha";
import type { Ent } from "../types";
import { at, ready } from "./combat";
import { type Play, type Stats, stats } from "./run";
import { type Hit, strike } from "./strike";

const hitOf = (e: Ent): Hit => ({ dmg: e.data.dmg as number, mod: e.data.mod as Hit["mod"], weapon: e.data.weapon as string });

/** A bolt touched an enemy (sensor overlap): hurt it, use one pierce. */
export function boltHit(ctx: Play, shot: Ent, e: Ent) {
  if (!shot.alive || !e.alive) return;
  strike(ctx, e, hitOf(shot), at(shot), 0.7, stats(ctx.state));
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
  for (const o of ctx.findAll("orb")) {
    if ((o.data.until as number) < ctx.state.time) {
      ctx.destroy(o);
      continue;
    }
    const spin = (o.data.weapon as string).length * 37;   // weapons orbit out of step with each other
    const angle = ctx.state.time * 200 + spin + (o.data.index as number) * (360 / (o.data.count as number));
    const p = vec.add(at(hero), vec.fromAngle(angle, o.data.radius as number));
    o.pos.x = p[0];
    o.pos.y = p[1];
    for (const e of ctx.queryCircle(p, 10, { tags: ["enemy"] })) {
      if (ready(ctx, e, `orb:${o.data.weapon}`, 0.5)) strike(ctx, e, hitOf(o), at(hero), 0.8, st);
    }
  }
}

function burn(ctx: Play, hero: Ent, st: Stats) {
  for (const p of ctx.findAll("pool")) {
    p.data.life = (p.data.life as number) - ctx.dt;
    if (!p.data.follow && (p.data.life as number) < 0.5) p.alpha = Math.max(0, (p.data.life as number) / 0.5);
    if ((p.data.life as number) <= 0) {
      ctx.destroy(p);
      continue;
    }
    if (p.data.follow) [p.pos.x, p.pos.y] = at(hero);
    for (const e of ctx.queryCircle(at(p), p.data.radius as number, { tags: ["enemy"] })) {
      if (ready(ctx, e, `pool:${p.data.weapon}`, 0.4)) strike(ctx, e, hitOf(p), at(p), 0, st);
    }
  }
}

/** Shots keep their speed even after a knock; boomerangs turn back toward the hero once. */
function steerShots(ctx: Play, hero: Ent) {
  for (const s of ctx.findAll("shot")) {
    if ((s.data.turn as number) > 0 && (s.data.life as number) < (s.data.turn as number)) {
      s.data.turn = -1;
      s.data.vel = vec.scale(vec.norm(vec.sub(at(hero), at(s))), vec.len(s.data.vel as [number, number]));
      s.rotation += 180;
    }
    s.body.velocity = s.data.vel as [number, number];
  }
}

export function updateProjectiles(ctx: Play, hero: Ent, st: Stats) {
  ageShots(ctx);
  steerShots(ctx, hero);
  orbit(ctx, hero, st);
  burn(ctx, hero, st);
}

/** While a menu is up the world holds still: bodies stop (their speeds come back next step). */
export function freeze(ctx: Play) {
  for (const e of [...ctx.findAll("enemy"), ...ctx.findAll("shot")]) e.body.velocity = [0, 0];
}
