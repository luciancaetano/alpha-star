import type { Context, Entity } from "alpha";
import { scene } from "../alpha";
import { coin } from "../prefabs/coin";
import { player } from "../prefabs/player";
import { CAMERA, PLAYER } from "../tuning";
import type { Platformer } from "../types";

type State = { coins: number; total: number; grounded: boolean; fallSpeed: number };
type Play = Context<Platformer, State>;
type Hero = Entity<Platformer["tags"], "idle" | "run" | "jump" | "fall" | "spin">;

// Idle / run on the ground, jump / fall in the air; faces where it moves.
function animate(p: Hero, ctx: Play, move: number) {
  if (move !== 0) p.flipX = move < 0;
  if (!ctx.state.grounded) p.animation = p.body.velocity.y < 0 ? "jump" : "fall";
  else p.animation = move !== 0 ? "run" : "idle";
}

// Touching ground; a hard landing shakes the camera.
function land(ctx: Play) {
  if (!ctx.state.grounded && ctx.state.fallSpeed > PLAYER.hardLanding) {
    ctx.camera.shake(CAMERA.shake, CAMERA.shakeSeconds);
    ctx.audio.play("land");
  }
  ctx.state.grounded = true;
}

export default scene("play", {
  state: () => ({ coins: 0, total: 0, grounded: false, fallSpeed: 0 }),

  enter(ctx) {
    const map = ctx.spawnMap("level1", { collide: ["walls"] });
    const spawns = map.objects("spawns");
    const start = spawns.find((o) => o.name === "player") ?? spawns[0];
    const hero = ctx.spawn(player([start.x, start.y]));
    for (const o of spawns.filter((s) => s.type === "coin")) ctx.spawn(coin(o));
    ctx.state.total = ctx.findAll("coin").length;

    ctx.camera.bounds([0, 0, map.size[0], map.size[1]]);
    ctx.camera.follow(hero, { lerp: CAMERA.lerp, deadzone: CAMERA.deadzone });
    ctx.ui.show("hud");
    ctx.ui.set("hud", "coins", `coins 0/${ctx.state.total}`);

    ctx.onHit("player", "walls", (p, wall) => {
      if (wall.pos.y > p.pos.y) land(ctx);  // the wall's center is below: standing on it
    });
    ctx.onEnter("player", "coin", (_p, c) => {
      ctx.destroy(c);
      ctx.state.coins += 1;
      ctx.audio.play("coin");
      ctx.ui.set("hud", "coins", `coins ${ctx.state.coins}/${ctx.state.total}`);
      if (ctx.state.coins === ctx.state.total) ctx.go("won");
    });
  },

  update(ctx) {
    const p = ctx.find("player");
    const move = Math.sign(Math.round(ctx.input.axis("moveX") * 10));
    let vy = p.body.velocity.y;
    if (ctx.input.pressed("jump") && ctx.state.grounded) {
      vy = PLAYER.jump;
      ctx.state.grounded = false;
      ctx.audio.play("jump");
    }
    if (vy > 60) ctx.state.grounded = false;  // walked off an edge
    ctx.state.fallSpeed = Math.max(0, vy);
    p.body.velocity = [move * PLAYER.speed, vy];
    p.rotation = 0;  // the box never tips over
    animate(p, ctx, move);
    if (p.pos.y > 400 || ctx.input.pressed("restart")) ctx.go("play");  // fell into the pit
  },
});
