import type { EntitySpec, Vec2 } from "alpha";
import type { Flappy } from "../types";
import { WORLD } from "../tuning";

/** The deck: its top stays at the design floor; it reaches down to the bottom of the visible world (ctx.camera.view). */
export function ground([, y, , h]: readonly number[]): EntitySpec<Flappy, "solid"> {
  const top = WORLD.height - WORLD.groundHeight;
  const size: Vec2 = [WORLD.width, Math.max(WORLD.groundHeight, y + h - top)];
  return {
    tags: ["solid"],
    pos: [WORLD.width / 2, top + size[1] / 2],
    shape: { image: "deck", size },
    body: { type: "static", collider: { rect: size } },
    z: 10,
  };
}
