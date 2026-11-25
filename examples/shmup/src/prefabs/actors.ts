// Entity specs: plain data built by functions.
import type { EntitySpec, Vec2 } from "alpha";
import { SHIP, SHOT } from "../tuning";
import type { Starfighter } from "../types";

type G = Starfighter;
type Spec = EntitySpec<G["tags"], G["images"], {}, G["layers"], never, G["categories"]>;

export function ship(): Spec {
  return {
    tags: ["ship"], pos: SHIP.start, z: 2, shape: { image: "ship" },
    body: { type: "dynamic", fixedRotation: true, category: "ship", collidesWith: ["enemy"], collider: { rect: [12, 12] } },
    data: { lives: SHIP.lives, cooldown: 0, safeUntil: 0 },
  };
}

// Kinematic: moved by its velocity only (nothing pushes it).
export function enemy(at: Vec2, fall: number): Spec {
  return {
    tags: ["enemy"], pos: at, z: 1, shape: { image: "enemy" },
    body: { type: "kinematic", velocity: [0, fall], category: "enemy", collidesWith: ["ship", "shot"], collider: { rect: [14, 12] } },
    data: { fall, phase: at[0] },
  };
}

// A fast sensor: `bullet` checks continuously so it never skips an enemy between steps.
export function shot(at: Vec2): Spec {
  return {
    tags: ["shot"], pos: at, shape: { rect: [2, 6], color: "#9bf6ff" },
    body: { type: "dynamic", bullet: true, velocity: [0, -SHOT.speed], category: "shot", collidesWith: ["enemy"],
            collider: { rect: [2, 6], sensor: true } },
  };
}

// The far layer: tiny dots; the play scene scrolls them in bulk (ctx.positions).
export function star(at: Vec2, depth: number): Spec {
  const shade = ["#555c7a", "#8a90b5", "#e0e6ff"][depth];
  return { tags: ["star"], pos: at, layer: "stars", shape: { rect: [1 + (depth >> 1), 1 + (depth >> 1)], color: shade as `#${string}` } };
}

export function boom(at: Vec2): Spec {
  return {
    tags: ["boom"], pos: at, layer: "fx",
    emitter: { burst: 24, oneShot: true, lifetime: [0.25, 0.6], speed: [30, 120], size: [3, 1], blend: "add",
               color: ["#ffd166ff", "#ef476f00"], shape: "rect" },
  };
}
