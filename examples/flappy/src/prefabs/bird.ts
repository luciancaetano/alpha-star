import type { EntitySpec } from "alpha";
import type { Flappy } from "../types";
import { BIRD } from "../tuning";

/** The bird; `physics` false for the floating title-screen version. */
export function bird(physics: boolean): EntitySpec<Flappy["tags"]> {
  return {
    tags: ["bird"],
    pos: [BIRD.x, BIRD.startY],
    shape: { circle: BIRD.radius, color: "#f8d838", outline: "#000" },
    // Feathers: nothing by itself (rate 0); play.ts bursts them on every flap.
    emitter: {
      lifetime: [0.3, 0.6], speed: [60, 140], angle: [110, 250], gravity: [0, 300],
      size: [6, 2], color: ["#ffffffff", "#f8d83800"], shape: "rect", max: 64,
    },
    ...(physics && { body: { type: "dynamic", collider: { circle: BIRD.radius } } }),
  };
}
