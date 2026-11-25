import type { EntitySpec, Vec2 } from "alpha";
import type { Flappy } from "../types";
import { WORLD } from "../tuning";

export function ground(): EntitySpec<Flappy["tags"], Flappy["images"]> {
  const size: Vec2 = [WORLD.width, WORLD.groundHeight];
  return {
    tags: ["solid"],
    pos: [WORLD.width / 2, WORLD.height - WORLD.groundHeight / 2],
    shape: { image: "deck", size },
    body: { type: "static", collider: { rect: size } },
    z: 10,
  };
}
