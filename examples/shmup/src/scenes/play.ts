import type { Context, Entity } from "alpha";
import { vec } from "alpha";
import { scene } from "../alpha";
import { boom, enemy, ship, shot, star } from "../prefabs/actors";
import { ENEMY, SHIP, SHOT, STARS } from "../tuning";
import type { Starfighter } from "../types";

type State = { score: number; crt: boolean };
type Play = Context<Starfighter, State>;
type Ship = Entity<Starfighter, "ship">;

// The play field is the whole visible world (ctx.camera.view): taller or wider screens show more of it.
const pan = (ctx: Play, x: number) => {
  const [vx, , vw] = ctx.camera.view;
  return ((x - vx) / vw) * 2 - 1;
};
const clamp = (v: number, lo: number, hi: number) => Math.max(lo, Math.min(hi, v));

// Every star moves in one batch: read the arrays, change them, write() once.
function scrollStars(ctx: Play) {
  const [, vy, , vh] = ctx.camera.view;
  const stars = ctx.positions("star");
  for (let i = 0; i < stars.count; i++) {
    stars.y[i] += STARS.speeds[i % 3] * ctx.dt;
    if (stars.y[i] > vy + vh) stars.y[i] -= vh;
  }
  stars.write();
}

function toggleCrt(ctx: Play) {
  ctx.state.crt = !ctx.state.crt;
  if (ctx.state.crt) ctx.postfx.add("crt", { strength: 0.6 });
  else ctx.postfx.remove("crt");
}

function fly(ctx: Play, s: Ship) {
  const move = vec.scale(vec.norm([ctx.input.axis("moveX"), ctx.input.axis("moveY")]), SHIP.speed);
  // Keep the ship on screen: stop pushing outward at the edges.
  const m = SHIP.margin;
  const [vx, vy, vw, vh] = ctx.camera.view;
  if ((s.pos.x <= vx + m && move[0] < 0) || (s.pos.x >= vx + vw - m && move[0] > 0)) move[0] = 0;
  if ((s.pos.y <= vy + m && move[1] < 0) || (s.pos.y >= vy + vh - m && move[1] > 0)) move[1] = 0;
  s.body.velocity = move;
  s.data.cooldown = Math.max(0, s.data.cooldown - ctx.dt);
  if (ctx.input.down("fire") && s.data.cooldown === 0) {
    ctx.spawn(shot([s.pos.x, s.pos.y - 10]));
    ctx.audio.play("shot", { volume: 0.3, pan: pan(ctx, s.pos.x) });
    s.data.cooldown = SHOT.cooldown;
  }
}

// Enemies sway sideways as they fall (their own phase), and leave at the bottom.
function steerEnemies(ctx: Play) {
  const [vx, vy, vw, vh] = ctx.camera.view;
  for (const e of ctx.findAll("enemy")) {
    const sway = ENEMY.sway * Math.cos((ctx.time * ENEMY.swayHz + e.data.phase / 50) * Math.PI * 2);
    e.body.velocity = [e.pos.x < vx + 12 ? Math.abs(sway) : e.pos.x > vx + vw - 12 ? -Math.abs(sway) : sway, e.data.fall];
    if (e.pos.y > vy + vh + 16) ctx.destroy(e);
  }
  for (const s of ctx.findAll("shot")) if (s.pos.y < vy - 8) ctx.destroy(s);
}

function explode(ctx: Play, e: Entity<Starfighter>) {
  ctx.spawn(boom([e.pos.x, e.pos.y]));
  ctx.audio.play("boom", { volume: 0.5, pan: pan(ctx, e.pos.x) });
  ctx.destroy(e);
}

// Hit: lose a life, blink while safe (a yoyo tween on alpha), rumble the pad.
function hurt(ctx: Play, s: Ship, e: Entity<Starfighter, "enemy">) {
  if (!e.alive) return;
  explode(ctx, e);
  if (ctx.time < s.data.safeUntil) return;
  s.data.lives -= 1;
  s.data.safeUntil = ctx.time + SHIP.safeSeconds;
  ctx.audio.play("hurt");
  ctx.camera.shake(3, 0.3);
  ctx.input.rumble(0.8, 0.25);
  ctx.tween(s, { to: { alpha: 0.2 }, seconds: SHIP.safeSeconds / 8, yoyo: true, repeat: 3 });
  if (s.data.lives <= 0) ctx.after(0.5, () => ctx.go("over", { score: ctx.state.score }));
}

export default scene("play", {
  state: () => ({ score: 0, crt: false }),

  enter(ctx) {
    const [vx, vy, vw, vh] = ctx.camera.view;
    for (let i = 0; i < STARS.count; i++) {
      ctx.spawn(star([ctx.random.range(vx, vx + vw), ctx.random.range(vy, vy + vh)], i % 3));
    }
    ctx.spawn(ship([vx + vw / 2, vy + vh - SHIP.fromBottom]));
    ctx.every(ENEMY.every, () => {
      const [x, y, w] = ctx.camera.view;
      ctx.spawn(enemy([ctx.random.range(x + 16, x + w - 16), y - 12], ctx.random.range(...ENEMY.fall)));
    });
    ctx.onEnter("shot", "enemy", (s, e) => {
      if (!s.alive || !e.alive) return;
      ctx.destroy(s);
      explode(ctx, e);
      ctx.state.score += ENEMY.points;
    });
    ctx.onHit("ship", "enemy", (s, e) => hurt(ctx, s, e));
    if (ctx.audio.musicPlaying !== "theme") ctx.audio.music("theme", { fadeIn: 1, volume: 0.6 });
    ctx.ui.show("hud");
    // Render-only looks: glow on the bright shots and stars; C toggles a CRT filter.
    ctx.postfx.add("bloom", { strength: 0.9, radius: 3 });
    if (ctx.state.crt) ctx.postfx.add("crt", { strength: 0.6 });
  },

  update(ctx) {
    scrollStars(ctx);
    const s = ctx.find("ship");
    if (s.data.lives > 0) fly(ctx, s);
    else s.body.velocity = [0, 0];
    steerEnemies(ctx);
    ctx.ui.set("hud", "score", `${ctx.state.score}`.padStart(6, "0"));
    ctx.ui.set("hud", "lives", "A".repeat(Math.max(0, s.data.lives)));
    if (ctx.input.pressed("crt")) toggleCrt(ctx);
    if (ctx.input.pressed("restart")) ctx.go("play");
  },
});
