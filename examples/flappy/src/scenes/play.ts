import type { Context } from "alpha";
import { scene } from "../alpha";
import { bird } from "../prefabs/bird";
import { background } from "../prefabs/background";
import { ground } from "../prefabs/ground";
import { pipePair } from "../prefabs/pipes";
import { BIRD, FEATHERS, PIPE } from "../tuning";
import type { Flappy } from "../types";

type Play = Context<Flappy, { score: number }>;

// Game over with a light rumble (no-op without a gamepad).
function crash(ctx: Play) {
  ctx.input.rumble(0.4, 0.25);
  ctx.go("over", { score: ctx.state.score });
}

export default scene("play", {
  state: () => ({ score: 0 }),

  enter(ctx) {
    ctx.spawn(background(ctx.camera.view));
    ctx.spawn(ground(ctx.camera.view));
    ctx.spawn(bird(true));
    ctx.ui.show("hud");

    ctx.onHit("bird", "solid", () => crash(ctx));
    ctx.onEnter("bird", "goal", (_bird, goal) => {
      ctx.state.score += 1;
      ctx.ui.set("hud", "score", ctx.state.score);
      ctx.destroy(goal);
      ctx.audio.play("point");
    });
    ctx.every(PIPE.every, () => pipePair(ctx.random, ctx.camera.view).forEach((p) => ctx.spawn(p)));
  },

  update(ctx) {
    const b = ctx.find("bird");
    if (ctx.input.pressed("flap")) {
      b.body.velocity = [0, BIRD.flap];
      b.emit(FEATHERS.count);
      ctx.audio.play("flap");
    }
    b.rotation = Math.min(BIRD.maxTilt, Math.max(-30, b.body.velocity.y * 0.15));
    const [left, top] = ctx.camera.view;
    if (b.pos.y < top) crash(ctx);  // flew off the top of the screen

    for (const e of ctx.findAll("scrolls")) {
      if (e.pos.x < left - PIPE.width) ctx.destroy(e);
    }
  },
});
