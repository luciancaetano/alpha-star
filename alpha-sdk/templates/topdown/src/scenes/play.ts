import type { Context, Entity, Player, Vec2 } from "alpha";
import { vec } from "alpha";
import { scene } from "../alpha";
import { bullet, enemy, hero, walls } from "../prefabs/actors";
import { blast, label } from "../prefabs/effects";
import { BOMB, BULLET, ENEMY, HERO, LABEL, VIEW, WALL } from "../tuning";
import type { Arena } from "../types";

type State = { score: number };
type Play = Context<Arena, State>;
type Hero = Entity<Arena["tags"], any>;

const at = (e: Hero): Vec2 => [e.pos.x, e.pos.y];
const pan = (x: number) => (x / VIEW.width) * 2 - 1;  // left speaker .. right speaker

function hud(ctx: Play) {
  const heroes = ctx.findAll("hero");
  ctx.ui.set("hud", "score", `SCORE ${ctx.state.score}`);
  ctx.ui.set("hud", "hp", `HP ${heroes.map((h) => h.data.hp).join(" / ")}`);
  ctx.ui.set("hud", "bombs", `BOMBS ${heroes.map((h) => h.data.bombs).join(" / ")}`);
}

// A random point just inside one of the four walls.
function edgePoint(ctx: Play): Vec2 {
  const m = WALL + ENEMY.radius + 2;
  const side = ctx.random.int(0, 3);
  const x = ctx.random.range(m, VIEW.width - m);
  const y = ctx.random.range(m, VIEW.height - m);
  return [[x, m], [x, VIEW.height - m], [m, y], [VIEW.width - m, y]][side] as Vec2;
}

// Player n joins when a device for them exists (player 2 = the second gamepad).
function joinPlayers(ctx: Play) {
  const have = new Set(ctx.findAll("hero").map((h) => h.data.player));
  for (let n = 1; n <= Math.min(2, ctx.input.players()); n++) {
    if (!have.has(n) && ctx.time < 0.5) ctx.spawn(hero(n, HERO.starts[n - 1]));
  }
}

function kill(ctx: Play, e: Hero, points: number) {
  ctx.spawn(blast(at(e)));
  ctx.audio.play("boom", { pan: pan(e.pos.x), volume: 0.6 });
  const l = ctx.spawn(label([e.pos.x, e.pos.y - 8], `+${points}`));
  ctx.tween(l, { to: { y: e.pos.y - 8 - LABEL.rise, alpha: 0 }, seconds: LABEL.seconds, onDone: () => ctx.destroy(l) });
  ctx.destroy(e);
  ctx.state.score += points;
}

// Every enemy in the circle goes (a physics area query).
function bomb(ctx: Play, h: Hero) {
  h.data.bombs = (h.data.bombs as number) - 1;
  for (const e of ctx.queryCircle(at(h), BOMB.radius, { tags: ["enemy"] })) if (e.alive) kill(ctx, e, 1);
  ctx.spawn(blast(at(h), true));
  ctx.camera.shake(BOMB.shake, BOMB.seconds);
  ctx.input.rumble(0.6, 0.2, h.data.player as Player);
}

// Auto-aim: toward the nearest enemy, or where the hero last walked when there is none.
function aimFrom(ctx: Play, h: Hero): Vec2 {
  const enemies = ctx.findAll("enemy");
  if (!enemies.length) return h.data.aim as Vec2;
  const near = enemies.reduce((a, b) => (vec.dist(at(a), at(h)) <= vec.dist(at(b), at(h)) ? a : b));
  return vec.norm(vec.sub(at(near), at(h)));
}

function steer(ctx: Play, h: Hero) {
  const input = ctx.input.player(h.data.player as Player);
  const move: Vec2 = [input.axis("moveX"), input.axis("moveY")];
  const amount = Math.min(1, vec.len(move));
  h.body.velocity = vec.scale(vec.norm(move), amount * HERO.speed);
  if (amount > 0) h.data.aim = vec.norm(move);
  if (move[0] !== 0) h.flipX = move[0] < 0;
  h.animation = amount > 0 ? "walk" : "idle";
  h.data.cooldown = Math.max(0, (h.data.cooldown as number) - ctx.dt);
  if (input.down("fire") && h.data.cooldown === 0) {
    ctx.spawn(bullet(at(h), aimFrom(ctx, h)));
    ctx.audio.play("shot", { pan: pan(h.pos.x), volume: 0.4 });
    h.data.cooldown = BULLET.cooldown;
  }
  if (input.pressed("bomb") && (h.data.bombs as number) > 0) bomb(ctx, h);
}

// Enemies walk toward the nearest hero.
function chase(ctx: Play, heroes: Hero[]) {
  for (const e of ctx.findAll("enemy")) {
    const target = heroes.reduce((a, b) => (vec.dist(at(a), at(e)) <= vec.dist(at(b), at(e)) ? a : b));
    e.body.velocity = vec.scale(vec.norm(vec.sub(at(target), at(e))), ENEMY.speed);
  }
}

function ageBullets(ctx: Play) {
  for (const b of ctx.findAll("bullet")) {
    b.data.life = (b.data.life as number) - ctx.dt;
    if ((b.data.life as number) <= 0) ctx.destroy(b);
  }
}

// Several contacts can arrive in one step: an enemy already used up hurts no one.
function hurt(ctx: Play, h: Hero, e: Hero) {
  if (!e.alive || !h.alive) return;
  ctx.spawn(blast(at(e)));
  ctx.destroy(e);
  ctx.audio.play("hit");
  ctx.camera.shake(2, 0.15);
  h.data.hp = (h.data.hp as number) - 1;
  if ((h.data.hp as number) <= 0) ctx.destroy(h);
}

export default scene("play", {
  state: () => ({ score: 0 }),

  enter(ctx) {
    for (const w of walls()) ctx.spawn(w);
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
    const heroes = ctx.findAll("hero").filter((h) => h.alive);
    if (!heroes.length) return ctx.go("over", { score: ctx.state.score });
    for (const h of heroes) steer(ctx, h);
    chase(ctx, heroes);
    ageBullets(ctx);
    hud(ctx);
    if (ctx.input.pressed("restart")) ctx.go("play");
  },
});
