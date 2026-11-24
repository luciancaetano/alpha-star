import type { EntitySpec } from "alpha";
import type { Flappy } from "../types";
import { BIRD } from "../tuning";

/** The bird; `physics` false for the floating title-screen version. */
export function bird(physics: boolean): EntitySpec<Flappy["tags"]> {
  return {
    tags: ["bird"],
    pos: [BIRD.x, BIRD.startY],
    shape: { circle: BIRD.radius, color: "#f8d838", outline: "#000" },
    ...(physics && { body: { type: "dynamic", collider: { circle: BIRD.radius } } }),
  };
}
