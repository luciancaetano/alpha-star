import type { Context, Entity } from "alpha";
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

// Flap on input, tilt with the fall, crash when flying off the top of the screen.
function fly(ctx: Play, b: Entity<Flappy, "bird">) {
  if (ctx.input.pressed("flap")) {
    b.body.velocity = [0, BIRD.flap];
    b.emit(FEATHERS.count);
    ctx.audio.play("flap");
  }
  b.rotation = Math.min(BIRD.maxTilt, Math.max(-30, b.body.velocity.y * 0.15));
  if (b.pos.y < ctx.camera.view[1]) crash(ctx);
}

// Pipes and goals that scrolled past the left edge are gone for good.
function recycle(ctx: Play, e: Entity<Flappy, "scrolls">) {
  if (e.pos.x < ctx.camera.view[0] - PIPE.width) ctx.destroy(e);
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
      ctx.destroy(goal);
      ctx.state.score += 1;
      ctx.emit("scored", { score: ctx.state.score });
    });
    // What a point looks and sounds like, apart from how it is earned.
    ctx.on("scored", ({ score }) => {
      ctx.ui.set("hud", "score", score);
      ctx.audio.play("point");
    });
    ctx.every(PIPE.every, () => pipePair(ctx.random, ctx.camera.view).forEach((p) => ctx.spawn(p)));
  },

  // Run in this order every step; a tag system runs once per living entity.
  systems: [
    { tag: "bird", each: fly },
    { tag: "scrolls", each: recycle },
  ],
});
