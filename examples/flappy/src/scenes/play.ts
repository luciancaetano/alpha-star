import type { Context } from "alpha";
import { scene } from "../alpha";
import { bird } from "../prefabs/bird";
import { ground } from "../prefabs/ground";
import { pipePair } from "../prefabs/pipes";
import { BIRD, PIPE } from "../tuning";
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
    ctx.spawn(ground());
    ctx.spawn(bird(true));
    ctx.ui.show("hud");

    ctx.onHit("bird", "solid", () => crash(ctx));
    ctx.onEnter("bird", "goal", (_bird, goal) => {
      ctx.state.score += 1;
      ctx.ui.set("hud", "score", ctx.state.score);
      ctx.destroy(goal);
      ctx.audio.play("point");
    });
    ctx.every(PIPE.every, () => pipePair(ctx.random).forEach((p) => ctx.spawn(p)));
  },

  update(ctx) {
    const b = ctx.find("bird");
    if (ctx.input.pressed("flap")) {
      b.body.velocity = [0, BIRD.flap];
      ctx.audio.play("flap");
    }
    b.rotation = Math.min(BIRD.maxTilt, Math.max(-30, b.body.velocity.y * 0.15));
    if (b.pos.y < 0) crash(ctx);

    for (const e of ctx.findAll("scrolls")) {
      if (e.pos.x < -PIPE.width) ctx.destroy(e);
    }
  },
});
