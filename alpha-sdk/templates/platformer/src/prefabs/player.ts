import type { EntitySpec, Vec2 } from "alpha";
import { PLAYER } from "../tuning";
import type { Platformer } from "../types";

type Spec = EntitySpec<Platformer, "player">;

// A dynamic box drawn with the hero sheet (16x16 frames) that never rotates.
export function player(pos: Vec2): Spec {
  return {
    tags: ["player"],
    pos,
    z: 2,
    shape: { sheet: "hero", animation: "idle" },
    // fixedRotation: the box never tips over.
    body: { type: "dynamic", fixedRotation: true, collider: { rect: PLAYER.size } },
  };
}
