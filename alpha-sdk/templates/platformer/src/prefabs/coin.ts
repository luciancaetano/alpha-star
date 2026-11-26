import type { EntitySpec, LevelEntity } from "alpha";
import type { Platformer } from "../types";


// A spinning coin at an LDtk "Coin" entity; a sensor, so the player passes through it.
export function coin(at: LevelEntity): EntitySpec<Platformer, "coin"> {
  return {
    tags: ["coin"],
    pos: at.pos,
    z: 1,
    shape: { sheet: "coin", animation: "spin" },
    body: { type: "static", collider: { rect: [8, 8], sensor: true } },
  };
}
