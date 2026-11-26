import type { EntitySpec } from "alpha";
import type { Flappy } from "../types";

type View = readonly [number, number, number, number];

/** Distant orbital sky over the whole visible world (ctx.camera.view), behind every scene and all colliders. */
export function background([x, y, w, h]: View): EntitySpec<Flappy["tags"], Flappy["images"]> {
  return {
    tags: [],
    pos: [x + w / 2, y + h / 2],
    z: -100,
    shape: { image: "nebula", size: [w, h] },
  };
}
