// One night in the bog: walk, survive, grow. Systems live in src/game/; this wires them per step.
import type { Player, Vec2 } from "alpha";
import { vec } from "alpha";
import { scene } from "../alpha";
import { updateEnemies } from "../game/enemies";
import { boltHit, freeze, updateProjectiles } from "../game/projectiles";
import { choose, hud, nextMenu, pause, wireMenus } from "../game/menus";
import { updatePickups } from "../game/pickups";
import { newRun, type Play, type RunState, stats } from "../game/run";
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

function walk(ctx: Play, h: Ent, speed: number) {
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
  for (const seg of ctx.findAll("hpbar")) if (seg.data.seg !== undefined) seg.alpha = (seg.data.seg as number) < on ? 1 : 0;
}

function menuKeys(ctx: Play) {
  if (ctx.state.mode === "levelup") (["pick1", "pick2", "pick3"] as const).forEach((a, i) => ctx.input.pressed(a) && choose(ctx, i));
}

function step(ctx: Play, h: Ent) {
  const s = ctx.state;
  const st = stats(s);
  s.time += ctx.dt;
  s.hp = Math.min(st.maxHp, s.hp + st.regen * ctx.dt);
  walk(ctx, h, st.speed);
  fireWeapons(ctx, h, st);
  updateProjectiles(ctx, h, st);
  updateEnemies(ctx, h, st);
  updatePickups(ctx, h, st);
  if (s.demo) [s.hp, s.pending] = [st.maxHp, 0];
  showHp(ctx, st.maxHp);
  if (s.hp <= 0) {
    ctx.audio.play("death");
    ctx.go("over", result(ctx, s.time >= RUN.seconds));
    s.over = true;
    return;
  }
  if (ctx.input.pressed("pause")) pause(ctx);
  else nextMenu(ctx, h);
}

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
    wireMenus(ctx, () => ctx.go("over", result(ctx, false)));
    ctx.ui.show("hud");
    ctx.postfx.add("bloom", { strength: 0.7, radius: 4 });
    ctx.postfx.add("vignette", { strength: 0.55, radius: 0.55 });
    ctx.audio.play("theme", { loop: true, volume: 0.35 });
  },

  update(ctx) {
    const h = ctx.findAll("hero")[0];
    if (!h || ctx.state.over) return;
    if (ctx.state.mode === "play") step(ctx, h);
    else {
      h.body.velocity = [0, 0];
      freeze(ctx);
      menuKeys(ctx);
    }
    ctx.state.view = [...ctx.camera.view];
    updateWorld(ctx);
    hud(ctx);
  },

  exit(ctx) {
    ctx.audio.stopAll();
  },
});
