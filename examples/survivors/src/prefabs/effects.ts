// Effects are data: one-shot emitters (gone with their last particle) and floating world text.
import type { Vec2 } from "alpha";
import type { Color, Spec } from "../types";

export function burst(at: Vec2, color: Color, count: number, speed = 70): Spec {
  return {
    tags: ["fx"], pos: at, z: 9, layer: "fx",
    emitter: {
      burst: count, oneShot: true, lifetime: [0.2, 0.55], speed: [speed * 0.3, speed], size: [3, 0],
      color: [`${color}ff` as Color, `${color}00` as Color], blend: "add",
    },
  };
}

/** A ring of sparks for level-ups, chests and evolutions. */
export function nova(at: Vec2, color: Color): Spec {
  return {
    tags: ["fx"], pos: at, z: 9, layer: "fx",
    emitter: { burst: 90, oneShot: true, lifetime: [0.5, 0.9], speed: [140, 180], size: [4, 0], color: [`${color}ff` as Color, `${color}00` as Color], blend: "add", max: 90 },
  };
}

export function label(at: Vec2, text: string, color: Color, size = 8): Spec {
  return { tags: ["label"], pos: at, z: 10, layer: "fx", shape: { text, font: "pixel", size, color } };
}
