import type { EntitySpec } from "alpha";
import type { Flappy } from "../types";
import { WORLD } from "../tuning";

/** Distant orbital sky, behind every scene and all colliders. */
export function background(): EntitySpec<Flappy["tags"], Flappy["images"]> {
  return {
    tags: [],
    pos: [WORLD.width / 2, WORLD.height / 2],
    z: -100,
    shape: { image: "nebula", size: [WORLD.width, WORLD.height] },
  };
}
