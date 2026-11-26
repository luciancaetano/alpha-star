// One night in the bog: walk, survive, grow. The per-step logic lives in src/game/; SYSTEMS orders it.
import type { Player, Vec2, When } from "alpha";
import { vec } from "alpha";
import { scene } from "../alpha";
import { chase, contact, spawnHorde } from "../game/enemies";
import { boltHit, updateProjectiles } from "../game/projectiles";
import { choose, hud, nextMenu, pause, wireBanners, wireMenus } from "../game/menus";
import { updatePickups } from "../game/pickups";
import { newRun, type Play, type PlaySystem, type RunState, stats } from "../game/run";
import { fireWeapons } from "../game/weapons";
import { buildWorld, updateWorld } from "../game/world";
import { autopilot, demoRun } from "../game/demo";
import { hero, HP_SEGMENTS, hpBar } from "../prefabs/actors";
import { RUN } from "../tuning";
import type { Ent, HeroId, RunResult } from "../types";

const result = (ctx: Play, dawn: boolean): RunResult => {
  const s = ctx.state;
  return { hero: s.hero, seconds: Math.floor(s.time), level: s.level, kills: s.kills, gold: s.gold + Math.floor(s.kills / 10), dawn };
};

function walk(ctx: Play, h: Ent<"hero">, speed: number) {
  const input = ctx.input.player(1 as Player);
  const move: Vec2 = ctx.state.demo ? autopilot(ctx.state.time) : [input.axis("moveX"), input.axis("moveY")];
  const amount = Math.min(1, vec.len(move));
  h.body.velocity = vec.scale(vec.norm(move), amount * speed);
  if (move[0] !== 0) {
    ctx.state.facing = move[0] < 0 ? -1 : 1;
    h.flipX = move[0] < 0;
  }
  h.animation = amount > 0 ? "walk" : "idle";
}

function showHp(ctx: Play, maxHp: number) {
  const on = Math.ceil((Math.max(0, ctx.state.hp) / maxHp) * HP_SEGMENTS);
  for (const seg of ctx.findAll("hpbar")) if (seg.data.seg >= 0) seg.alpha = seg.data.seg < on ? 1 : 0;
}

/** A system that needs the hero (none without one). Once the night is over (the step the hero died),
 *  only the "always" ones still run. */
const onHero = (name: string, fn: (ctx: Play, h: Ent<"hero">) => void, when: When = "running"): PlaySystem => ({
  name, when, tag: "hero", each: (ctx, h) => void ((when !== "always" && ctx.state.over) || fn(ctx, h)),
});

function tick(ctx: Play, h: Ent<"hero">) {
  const s = ctx.state;
  const st = stats(s);
  s.time += ctx.dt;
  s.hp = Math.min(st.maxHp, s.hp + st.regen * ctx.dt);
  walk(ctx, h, st.speed);
}

function life(ctx: Play) {
  const s = ctx.state;
  const st = stats(s);
  if (s.demo) [s.hp, s.pending] = [st.maxHp, 0];
  showHp(ctx, st.maxHp);
  if (s.hp > 0) return;
  ctx.audio.play("death");
  ctx.go("over", result(ctx, s.time >= RUN.seconds));
  s.over = true;
}

function menus(ctx: Play, h: Ent<"hero">) {
  if (ctx.input.pressed("pause")) pause(ctx);
  else nextMenu(ctx, h);
}

function menuKeys(ctx: Play) {
  if (ctx.state.mode === "levelup") (["pick1", "pick2", "pick3"] as const).forEach((a, i) => ctx.input.pressed(a) && choose(ctx, i));
}

/** Menus hold the world still: bodies, contacts, timers, tweens and particles wait. */
const syncFrozen = (ctx: Play) => void (ctx.frozen = ctx.state.mode !== "play");

/** One step of the night, in order. "running" systems skip while a menu is up (ctx.frozen). */
const SYSTEMS: PlaySystem[] = [
  onHero("tick", tick),
  onHero("weapons", (ctx, h) => fireWeapons(ctx, h, stats(ctx.state))),
  onHero("projectiles", (ctx, h) => updateProjectiles(ctx, h, stats(ctx.state))),
  onHero("horde", (ctx, h) => spawnHorde(ctx, h, stats(ctx.state))),
  chase,
  onHero("contact", (ctx, h) => contact(ctx, h, stats(ctx.state))),
  onHero("pickups", (ctx, h) => updatePickups(ctx, h, stats(ctx.state))),
  onHero("life", (ctx) => life(ctx)),
  onHero("menus", menus),
  onHero("menuKeys", (ctx) => menuKeys(ctx), "frozen"),
  // Everything below runs every step: a menu opened or closed above takes effect this step.
  onHero("frozen", syncFrozen, "always"),
  onHero("view", (ctx) => void (ctx.state.view = [...ctx.camera.view]), "always"),
  onHero("world", (ctx) => updateWorld(ctx), "always"),
  onHero("hud", (ctx) => hud(ctx), "always"),
];

export default scene("play", {
  state: (p?: { hero: HeroId; demo?: boolean }): RunState => (p?.demo ? demoRun(p.hero) : newRun(p?.hero ?? "wick", {})),

  enter(ctx) {
    ctx.state.shop = ctx.save.get("shop", {});
    if (ctx.state.time === 0) ctx.state.hp = stats(ctx.state).maxHp;
    buildWorld(ctx);
    const h = ctx.spawn(hero(ctx.state.hero, [0, 0]));
    ctx.spawn({ ...hpBar(-1), parent: h });
    for (let i = 0; i < HP_SEGMENTS; i++) ctx.spawn({ ...hpBar(i), parent: h });
    // Locked on the hero (no lag): camera and hero are drawn with the same interpolation, so the hero
    // holds still on screen. Rounding the camera made the hero shake half a pixel while walking.
    ctx.camera.follow(h, { lerp: 1 });
    ctx.onEnter("shot", "enemy", (s, e) => boltHit(ctx, s, e));
    wireBanners(ctx);
    wireMenus(ctx, () => ctx.go("over", result(ctx, false)));
    ctx.ui.show("hud");
    ctx.postfx.set("bloom", { strength: 0.7, radius: 4 });
    ctx.postfx.set("vignette", { strength: 0.55, radius: 0.55 });
    // Both layers start in the same step and have the same length, so they loop together.
    ctx.audio.play("theme", { loop: true, volume: 0.35 });
    ctx.audio.play("themeBass", { loop: true, volume: 0.35 });
  },

  // First, so a menu button clicked at the start of this step (ui.on) decides which systems run.
  update: syncFrozen,
  systems: SYSTEMS,

  exit(ctx) {
    ctx.audio.stopAll();
  },
});
