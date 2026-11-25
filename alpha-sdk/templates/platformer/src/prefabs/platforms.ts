import type { EntitySpec, Vec2 } from "alpha";
import { LIFT } from "../tuning";
import type { Platformer } from "../types";

type Spec = EntitySpec<Platformer["tags"], never, Platformer["sheets"]>;

// A wooden plank that is solid only from above: the hero jumps up through it and lands on it.
export function ledge(pos: Vec2, width: number): Spec {
  return {
    tags: ["ledge"],
    pos,
    shape: { rect: [width, 6], color: "#8b5a2b", outline: "#5c3a1a" },
    body: { type: "static", collider: { rect: [width, 6], oneWay: true } },
  };
}

// A lift over the pit; play.ts tweens it back and forth (kinematic: it carries the hero).
export function lift(): Spec {
  return {
    tags: ["lift"],
    pos: LIFT.from,
    shape: { rect: LIFT.size, color: "#c0c0c8", outline: "#606070" },
    body: { type: "kinematic", collider: { rect: LIFT.size, friction: 1 } },
  };
}
