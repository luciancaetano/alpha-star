import type { EntitySpec } from "alpha";
import type { Flappy } from "../types";
import { ART, BIRD, WORLD } from "../tuning";

/** The bird; `physics` false for the floating title-screen version. */
export function bird(physics: boolean): EntitySpec<Flappy["tags"], Flappy["images"]> {
  const size = physics ? ART.playerSize : ART.titleSize;
  return {
    tags: ["bird"],
    pos: physics ? [BIRD.x, BIRD.startY] : [WORLD.width / 2, ART.titleY],
    shape: { image: "player", size: [size, size] },
    z: 5,
    // Ion trail: play.ts bursts it on every flap.
    emitter: {
      lifetime: [0.3, 0.6], speed: [60, 140], angle: [150, 210], gravity: [0, 30],
      size: [4, 1], color: ["#88ffffff", "#a36bff00"], shape: "circle", max: 64,
    },
    ...(physics && { body: { type: "dynamic", collider: { circle: BIRD.radius } } }),
  };
}
