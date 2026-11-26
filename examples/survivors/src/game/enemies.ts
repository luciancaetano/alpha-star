// The horde: wave spawns at the edge of the view, timed events, chasing, contact damage.
import type { Vec2 } from "alpha";
import { vec } from "alpha";
import { dmgScale, EVENTS, hpScale, waveAt } from "../data/waves";
import { enemy } from "../prefabs/actors";
import { ENEMIES, HERO, RUN } from "../tuning";
import type { EnemyKind, Ent } from "../types";
import { at, pan } from "./combat";
import type { Play, Stats } from "./run";

/** A point just outside the visible world, `extra` px beyond its edge, at `angle` degrees from the hero. */
function offscreen(ctx: Play, hero: Ent, angle: number, extra = 24): Vec2 {
  const [, , vw, vh] = ctx.camera.view;
  const dir = vec.fromAngle(angle);
  const reach = Math.min(Math.abs(vw / 2 / (dir[0] || 1e-6)), Math.abs(vh / 2 / (dir[1] || 1e-6)));
  return vec.add(at(hero), vec.scale(dir, reach + extra));
}

function spawnKind(ctx: Play, kind: EnemyKind, pos: Vec2) {
  ctx.spawn(enemy(kind, pos, hpScale(ctx.state.time)));
}

/** One foe at a time: `spawnCd` accumulates the wave's rate (foes per second) until one is due. */
function waveSpawns(ctx: Play, hero: Ent) {
  const wave = waveAt(ctx.state.time);
  ctx.state.spawnCd += wave.rate * ctx.dt;
  const alive = ctx.findAll("enemy").length;
  while (ctx.state.spawnCd >= 1) {
    ctx.state.spawnCd -= 1;
    if (alive >= Math.min(wave.cap, RUN.maxEnemies)) continue;
    spawnKind(ctx, ctx.random.pick(wave.kinds), offscreen(ctx, hero, ctx.random.range(0, 360)));
  }
}

function runEvent(ctx: Play, hero: Ent, ev: (typeof EVENTS)[number]) {
  ctx.state.events.push(`${ev.at}`);
  ctx.ui.set("hud", "banner", ctx.t(`event.${ev.at}`));
  ctx.after(3.5, () => ctx.ui.set("hud", "banner", ""));
  const side = ctx.random.range(0, 360);
  for (let i = 0; i < ev.count; i++) {
    const angle = ev.kind === "ring" ? (i * 360) / ev.count : ev.kind === "swarm" ? side + ctx.random.range(-20, 20) : side + i * 90;
    spawnKind(ctx, ev.enemy, offscreen(ctx, hero, angle, ev.kind === "swarm" ? ctx.random.range(10, 60) : 30));
  }
  if (ev.kind === "boss" || ev.kind === "snuffer") {
    ctx.audio.play("boss");
    ctx.camera.shake(3, 0.6);
  }
}

function events(ctx: Play, hero: Ent) {
  for (const ev of EVENTS) {
    if (ctx.state.time >= ev.at && !ctx.state.events.includes(`${ev.at}`)) runEvent(ctx, hero, ev);
  }
}

/** Foes left far behind come back from the side the hero walks toward. */
function chase(ctx: Play, hero: Ent) {
  const h = at(hero);
  const [, , vw, vh] = ctx.camera.view;
  const far = Math.hypot(vw, vh) / 2 + RUN.despawn;   // beyond the corners of any screen
  for (const e of ctx.findAll("enemy")) {
    e.data.flash = Math.max(0, (e.data.flash as number) - ctx.dt);
    if (e.data.flash === 0) e.tint = "#ffffff";
    e.data.kb = Math.max(0, (e.data.kb as number) - ctx.dt);
    const to = vec.sub(h, at(e));
    if (vec.len(to) > far && !e.tags.includes("boss")) {
      // Well past the edge: placed right at it, they seemed to pop in next to a walking hero.
      const p = offscreen(ctx, hero, vec.angle(vec.scale(to, -1)) + 180 + ctx.random.range(-40, 40), ctx.random.range(60, 120));
      [e.pos.x, e.pos.y] = p;   // a far jump: the engine draws it there at once
      continue;
    }
    e.flipX = to[0] < 0;
    if (e.data.kb === 0) e.body.velocity = vec.scale(vec.norm(to), ENEMIES[e.data.kind as EnemyKind].speed);
  }
}

/** Touching foes hurt; a short invulnerability follows each hit. */
function contact(ctx: Play, hero: Ent, st: Stats) {
  ctx.state.iframes = Math.max(0, ctx.state.iframes - ctx.dt);
  hero.tint = ctx.state.iframes > 0 && Math.floor(ctx.state.iframes * 20) % 2 === 0 ? "#ff6060" : "#ffffff";
  if (ctx.state.iframes > 0) return;
  const touching = ctx.queryCircle(at(hero), HERO.radius + 4, { tags: ["enemy"] }).filter((e) => e.alive);
  if (!touching.length) return;
  // Every foe pressing in hurts (the five strongest): standing in a crowd is deadly.
  const hits = touching.map((e) => ENEMIES[e.data.kind as EnemyKind].dmg).sort((a, b) => b - a).slice(0, HERO.crowd);
  const raw = hits.reduce((sum, d) => sum + d, 0) * dmgScale(ctx.state.time);
  ctx.state.hp -= Math.max(1, raw - st.armor * hits.length);
  ctx.state.iframes = HERO.iframes;
  ctx.audio.play("hurt", { pan: pan(ctx, hero.pos.x), volume: 0.6 });
  ctx.camera.shake(2, 0.15);
  ctx.input.rumble(0.4, 0.15);
}

export function updateEnemies(ctx: Play, hero: Ent, st: Stats) {
  if (ctx.state.time < RUN.seconds) waveSpawns(ctx, hero);
  events(ctx, hero);
  chase(ctx, hero);
  contact(ctx, hero, st);
}
