import type { EntitySpec, Vec2 } from "alpha";
import { DUST } from "../tuning";
import type { Platformer } from "../types";

type Spec = EntitySpec<Platformer["tags"], never, Platformer["sheets"], Platformer["layers"], Platformer["fonts"]>;

// A one-shot puff at the feet: its entity goes away with its last particle.
export function dust(feet: Vec2): Spec {
  return {
    tags: ["dust"],
    pos: feet,
    layer: "fx",
    emitter: {
      burst: DUST.count, oneShot: true, lifetime: [0.3, 0.5], speed: [20, 50], angle: [190, 350],
      gravity: [0, 60], size: [5, 2], color: ["#b89a6cff", "#b89a6c00"], shape: "rect",
    },
  };
}

// "+1" in the pixel font; play.ts floats it up and fades it out.
export function popup(at: Vec2): Spec {
  return {
    tags: ["popup"],
    pos: [at[0], at[1] - 10],
    layer: "fx",
    shape: { text: "+1", font: "pixel", size: 8, color: "#ffe14d" },
  };
}

// Far hills on the parallax layer: soft circles along the bottom of the view.
export function hill(x: number, radius: number): Spec {
  return {
    tags: ["scenery"],
    pos: [x, 150 + radius * 0.4],
    layer: "far",
    shape: { circle: radius, color: "#5f9e6e", alpha: 0.85 },
  };
}
