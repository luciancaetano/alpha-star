import type { Context, Entity, Vec2 } from "alpha";
import { vec } from "alpha";
import { scene } from "../alpha";
import { bullet, enemy, hero, walls } from "../prefabs/actors";
import { blast, label } from "../prefabs/effects";
import { BOMB, BULLET, ENEMY, HERO, LABEL, WALL } from "../tuning";
import type { Arena } from "../types";

type State = { score: number };
type Play = Context<Arena, State>;
type Hero = Entity<Arena, "hero">;

const at = (e: Entity<Arena>): Vec2 => [e.pos.x, e.pos.y];
// Left speaker .. right speaker across the visible world.
const pan = (ctx: Play, x: number) => {
  const [vx, , vw] = ctx.camera.view;
  return ((x - vx) / vw) * 2 - 1;
};

function hud(ctx: Play) {
  const heroes = ctx.findAll("hero");
  ctx.ui.set("hud", "score", `SCORE ${ctx.state.score}`);
  ctx.ui.set("hud", "hp", `HP ${heroes.map((h) => h.data.hp).join(" / ")}`);
  ctx.ui.set("hud", "bombs", `BOMBS ${heroes.map((h) => h.data.bombs).join(" / ")}`);
}

// A random point just inside one of the four walls (which line the visible world).
function edgePoint(ctx: Play): Vec2 {
  const m = WALL + ENEMY.radius + 2;
  const [vx, vy, vw, vh] = ctx.camera.view;
  const side = ctx.random.int(0, 3);
  const x = ctx.random.range(vx + m, vx + vw - m);
  const y = ctx.random.range(vy + m, vy + vh - m);
  return [[x, vy + m], [x, vy + vh - m], [vx + m, y], [vx + vw - m, y]][side] as Vec2;
}

// Player n joins when a device for them exists (player 2 = the second gamepad).
function joinPlayers(ctx: Play) {
  const have = new Set(ctx.findAll("hero").map((h) => h.data.player));
  for (const n of [1, 2] as const) {
    if (n <= ctx.input.players() && !have.has(n) && ctx.time < 0.5) ctx.spawn(hero(n, HERO.starts[n - 1]));
  }
}

function kill(ctx: Play, e: Entity<Arena, "enemy">, points: number) {
  ctx.spawn(blast(at(e)));
  ctx.audio.play("boom", { pan: pan(ctx, e.pos.x), volume: 0.6 });
  const l = ctx.spawn(label([e.pos.x, e.pos.y - 8], `+${points}`));
  ctx.tween(l, { to: { y: e.pos.y - 8 - LABEL.rise, alpha: 0 }, seconds: LABEL.seconds, onDone: () => ctx.destroy(l) });
  ctx.destroy(e);
  ctx.state.score += points;
}

// Every enemy in the circle goes (a physics area query).
function bomb(ctx: Play, h: Hero) {
  h.data.bombs -= 1;
  for (const e of ctx.queryCircle(at(h), BOMB.radius, { tags: ["enemy"] })) kill(ctx, e, 1);
  ctx.spawn(blast(at(h), true));
  ctx.camera.shake(BOMB.shake, BOMB.seconds);
  ctx.input.rumble(0.6, 0.2, h.data.player);
}

// Auto-aim: toward the nearest enemy, or where the hero last walked when there is none.
function aimFrom(ctx: Play, h: Hero): Vec2 {
  const enemies = ctx.findAll("enemy");
  if (!enemies.length) return h.data.aim;
  const near = enemies.reduce((a, b) => (vec.dist(at(a), at(h)) <= vec.dist(at(b), at(h)) ? a : b));
  return vec.norm(vec.sub(at(near), at(h)));
}

function steer(ctx: Play, h: Hero) {
  const input = ctx.input.player(h.data.player);
  const move: Vec2 = [input.axis("moveX"), input.axis("moveY")];
  const amount = Math.min(1, vec.len(move));
  h.body.velocity = vec.scale(vec.norm(move), amount * HERO.speed);
  if (amount > 0) h.data.aim = vec.norm(move);
  if (move[0] !== 0) h.flipX = move[0] < 0;
  h.animation = amount > 0 ? "walk" : "idle";
  h.data.cooldown = Math.max(0, h.data.cooldown - ctx.dt);
  if (input.down("fire") && h.data.cooldown === 0) {
    ctx.spawn(bullet(at(h), aimFrom(ctx, h)));
    ctx.audio.play("shot", { pan: pan(ctx, h.pos.x), volume: 0.4 });
    h.data.cooldown = BULLET.cooldown;
  }
  if (input.pressed("bomb") && h.data.bombs > 0) bomb(ctx, h);
}

// An enemy walks toward the nearest hero.
function chase(ctx: Play, e: Entity<Arena, "enemy">) {
  const heroes = ctx.findAll("hero");
  if (!heroes.length) return;
  const target = heroes.reduce((a, b) => (vec.dist(at(a), at(e)) <= vec.dist(at(b), at(e)) ? a : b));
  e.body.velocity = vec.scale(vec.norm(vec.sub(at(target), at(e))), ENEMY.speed);
}

function age(ctx: Play, b: Entity<Arena, "bullet">) {
  b.data.life -= ctx.dt;
  if (b.data.life <= 0) ctx.destroy(b);
}

// Several contacts can arrive in one step: an enemy already used up hurts no one.
function hurt(ctx: Play, h: Hero, e: Entity<Arena, "enemy">) {
  if (!e.alive || !h.alive) return;
  ctx.spawn(blast(at(e)));
  ctx.destroy(e);
  ctx.audio.play("hit");
  ctx.camera.shake(2, 0.15);
  h.data.hp -= 1;
  if (h.data.hp <= 0) ctx.destroy(h);
}

export default scene("play", {
  state: () => ({ score: 0 }),

  enter(ctx) {
    for (const w of walls(ctx.camera.view)) ctx.spawn(w);
    ctx.spawn(hero(1, HERO.starts[0]));
    ctx.every(ENEMY.every, () => {
      if (ctx.findAll("enemy").length < ENEMY.max) ctx.spawn(enemy(edgePoint(ctx)));
    });
    ctx.onEnter("bullet", "enemy", (b, e) => {
      if (!e.alive || !b.alive) return;
      ctx.destroy(b);
      kill(ctx, e, 1);
    });
    ctx.onHit("hero", "enemy", (h, e) => hurt(ctx, h, e));
    ctx.ui.show("hud");
  },

  update(ctx) {
    joinPlayers(ctx);
    if (!ctx.findAll("hero").length) ctx.go("over", { score: ctx.state.score });
    if (ctx.input.pressed("restart")) ctx.go("play");
  },

  // Run in this order after update, every step; a tag system runs once per living entity.
  systems: [
    { tag: "hero", each: steer },
    { tag: "enemy", each: chase },
    { tag: "bullet", each: age },
    { name: "hud", run: hud },
  ],
});
