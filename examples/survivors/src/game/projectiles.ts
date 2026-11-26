// What weapons left in the world: bolts age, pierce and turn back, lanterns orbit, pools and
// auras burn, flashes fade. Every hit goes through strike.ts.
import { vec } from "alpha";
import type { Ent, WeaponHit } from "../types";
import { at, ready } from "./combat";
import { type Play, type Stats, stats } from "./run";
import { strike } from "./strike";

const hitOf = ({ data }: Ent<"shot"> | Ent<"orb"> | Ent<"pool">): WeaponHit => ({ dmg: data.dmg, mod: data.mod, weapon: data.weapon });

/** A bolt touched an enemy (sensor overlap): hurt it, use one pierce. */
export function boltHit(ctx: Play, shot: Ent<"shot">, e: Ent<"enemy">) {
  if (!shot.alive || !e.alive) return;
  strike(ctx, e, hitOf(shot), at(shot), 0.7, stats(ctx.state));
  shot.data.pierce -= 1;
  if (shot.data.pierce <= 0) ctx.destroy(shot);
}

function ageShots(ctx: Play) {
  for (const s of ctx.findAll("shot")) {
    s.data.life -= ctx.dt;
    if (s.data.life <= 0) ctx.destroy(s);
  }
  for (const s of [...ctx.findAll("slash"), ...ctx.findAll("zap")]) {
    s.data.life -= ctx.dt;
    s.alpha = Math.max(0, s.data.life / 0.2);
    if (s.data.life <= 0) ctx.destroy(s);
  }
}

function orbit(ctx: Play, hero: Ent<"hero">, st: Stats) {
  for (const o of ctx.findAll("orb")) {
    if (o.data.until < ctx.state.time) {
      ctx.destroy(o);
      continue;
    }
    const spin = o.data.weapon.length * 37;   // weapons orbit out of step with each other
    const angle = ctx.state.time * 200 + spin + o.data.index * (360 / o.data.count);
    const p = vec.add(at(hero), vec.fromAngle(angle, o.data.radius));
    o.pos.x = p[0];
    o.pos.y = p[1];
    for (const e of ctx.queryCircle(p, 10, { tags: ["enemy"] })) {
      if (ready(ctx, e, `orb:${o.data.weapon}`, 0.5)) strike(ctx, e, hitOf(o), at(hero), 0.8, st);
    }
  }
}

function burn(ctx: Play, hero: Ent<"hero">, st: Stats) {
  for (const p of ctx.findAll("pool")) {
    p.data.life -= ctx.dt;
    if (!p.data.follow && p.data.life < 0.5) p.alpha = Math.max(0, p.data.life / 0.5);
    if (p.data.life <= 0) {
      ctx.destroy(p);
      continue;
    }
    if (p.data.follow) [p.pos.x, p.pos.y] = at(hero);
    for (const e of ctx.queryCircle(at(p), p.data.radius, { tags: ["enemy"] })) {
      if (ready(ctx, e, `pool:${p.data.weapon}`, 0.4)) strike(ctx, e, hitOf(p), at(p), 0, st);
    }
  }
}

/** Shots keep their speed even after a knock; boomerangs turn back toward the hero once. */
function steerShots(ctx: Play, hero: Ent<"hero">) {
  for (const s of ctx.findAll("shot")) {
    if (s.data.turn > 0 && s.data.life < s.data.turn) {
      s.data.turn = -1;
      s.data.vel = vec.scale(vec.norm(vec.sub(at(hero), at(s))), vec.len(s.data.vel));
      s.rotation += 180;
    }
    s.body.velocity = s.data.vel;
  }
}

export function updateProjectiles(ctx: Play, hero: Ent<"hero">, st: Stats) {
  ageShots(ctx);
  steerShots(ctx, hero);
  orbit(ctx, hero, st);
  burn(ctx, hero, st);
}
