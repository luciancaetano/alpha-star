// Effects are data: a one-shot emitter (its entity goes away with the last particle) and world text.
import type { EntitySpec, Vec2 } from "alpha";
import type { Arena } from "../types";

type Spec = EntitySpec<Arena, "blast" | "label">;

export function blast(at: Vec2, big = false): Spec {
  return {
    tags: ["blast"],
    pos: at,
    z: 3,
    emitter: {
      burst: big ? 60 : 14, oneShot: true, lifetime: [0.2, 0.5], speed: big ? [60, 180] : [30, 90],
      size: [4, 1], color: ["#ffb347ff", "#e74c3c00"], blend: "add",
    },
  };
}

// No font named: the built-in 8x8 font, sharp at any window size.
export function label(at: Vec2, text: string): Spec {
  return { tags: ["label"], pos: at, z: 4, shape: { text, size: 8, color: "#ffe14d" } };
}
