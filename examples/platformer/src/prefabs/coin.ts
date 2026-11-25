import type { EntitySpec, MapObject } from "alpha";
import type { Platformer } from "../types";

type Spec = EntitySpec<Platformer["tags"], never, Platformer["sheets"]>;

// A spinning coin at a Tiled point object; a sensor, so the player passes through it.
export function coin(at: MapObject): Spec {
  return {
    tags: ["coin"],
    pos: [at.x, at.y],
    z: 1,
    shape: { sheet: "coin", animation: "spin" },
    body: { type: "static", collider: { rect: [8, 8], sensor: true } },
  };
}
