import type { Context, Entity } from "alpha";
import { vec } from "alpha";
import { scene } from "../alpha";
import { boom, enemy, ship, shot, star } from "../prefabs/actors";
import { ENEMY, SHIP, SHOT, STARS, VIEW } from "../tuning";
import type { Starfighter } from "../types";

type State = { score: number };
type Play = Context<Starfighter, State>;
type Thing = Entity<Starfighter["tags"], any>;

const pan = (x: number) => (x / VIEW.width) * 2 - 1;
const clamp = (v: number, lo: number, hi: number) => Math.max(lo, Math.min(hi, v));

// Every star moves in one batch: read the arrays, change them, write() once.
function scrollStars(ctx: Play) {
  const stars = ctx.positions("star");
  for (let i = 0; i < stars.count; i++) {
    stars.y[i] += STARS.speeds[i % 3] * ctx.dt;
    if (stars.y[i] > VIEW.height) stars.y[i] -= VIEW.height;
  }
  stars.write();
}

function fly(ctx: Play, s: Thing) {
  const move = vec.scale(vec.norm([ctx.input.axis("moveX"), ctx.input.axis("moveY")]), SHIP.speed);
  // Keep the ship on screen: stop pushing outward at the edges.
  const m = SHIP.margin;
  if ((s.pos.x <= m && move[0] < 0) || (s.pos.x >= VIEW.width - m && move[0] > 0)) move[0] = 0;
  if ((s.pos.y <= m && move[1] < 0) || (s.pos.y >= VIEW.height - m && move[1] > 0)) move[1] = 0;
  s.body.velocity = move;
  s.data.cooldown = Math.max(0, (s.data.cooldown as number) - ctx.dt);
  if (ctx.input.down("fire") && s.data.cooldown === 0) {
    ctx.spawn(shot([s.pos.x, s.pos.y - 10]));
    ctx.audio.play("shot", { volume: 0.3, pan: pan(s.pos.x) });
    s.data.cooldown = SHOT.cooldown;
  }
}

// Enemies sway sideways as they fall (their own phase), and leave at the bottom.
function steerEnemies(ctx: Play) {
  for (const e of ctx.findAll("enemy")) {
    const vx = ENEMY.sway * Math.cos((ctx.time * ENEMY.swayHz + (e.data.phase as number) / 50) * Math.PI * 2);
    e.body.velocity = [e.pos.x < 12 ? Math.abs(vx) : e.pos.x > VIEW.width - 12 ? -Math.abs(vx) : vx, e.data.fall as number];
    if (e.pos.y > VIEW.height + 16) ctx.destroy(e);
  }
  for (const s of ctx.findAll("shot")) if (s.pos.y < -8) ctx.destroy(s);
}

function explode(ctx: Play, e: Thing) {
  ctx.spawn(boom([e.pos.x, e.pos.y]));
  ctx.audio.play("boom", { volume: 0.5, pan: pan(e.pos.x) });
  ctx.destroy(e);
}

// Hit: lose a life, blink while safe (a yoyo tween on alpha), rumble the pad.
function hurt(ctx: Play, s: Thing, e: Thing) {
  if (!e.alive) return;
  explode(ctx, e);
  if (ctx.time < (s.data.safeUntil as number)) return;
  s.data.lives = (s.data.lives as number) - 1;
  s.data.safeUntil = ctx.time + SHIP.safeSeconds;
  ctx.audio.play("hurt");
  ctx.camera.shake(3, 0.3);
  ctx.input.rumble(0.8, 0.25);
  ctx.tween(s, { to: { alpha: 0.2 }, seconds: SHIP.safeSeconds / 8, yoyo: true, repeat: 3 });
  if ((s.data.lives as number) <= 0) ctx.after(0.5, () => ctx.go("over", { score: ctx.state.score }));
}

export default scene("play", {
  state: () => ({ score: 0 }),

  enter(ctx) {
    for (let i = 0; i < STARS.count; i++) {
      ctx.spawn(star([ctx.random.range(0, VIEW.width), ctx.random.range(0, VIEW.height)], i % 3));
    }
    ctx.spawn(ship());
    ctx.every(ENEMY.every, () => ctx.spawn(enemy([ctx.random.range(16, VIEW.width - 16), -12], ctx.random.range(...ENEMY.fall))));
    ctx.onEnter("shot", "enemy", (s, e) => {
      if (!s.alive || !e.alive) return;
      ctx.destroy(s);
      explode(ctx, e);
      ctx.state.score += ENEMY.points;
    });
    ctx.onHit("ship", "enemy", (s, e) => hurt(ctx, s, e));
    if (ctx.audio.musicPlaying !== "theme") ctx.audio.music("theme", { fadeIn: 1, volume: 0.6 });
    ctx.ui.show("hud");
  },

  update(ctx) {
    scrollStars(ctx);
    const s = ctx.find("ship");
    if ((s.data.lives as number) > 0) fly(ctx, s);
    else s.body.velocity = [0, 0];
    steerEnemies(ctx);
    ctx.ui.set("hud", "score", `${ctx.state.score}`.padStart(6, "0"));
    ctx.ui.set("hud", "lives", "A".repeat(Math.max(0, s.data.lives as number)));
    if (ctx.input.pressed("restart")) ctx.go("play");
  },
});
