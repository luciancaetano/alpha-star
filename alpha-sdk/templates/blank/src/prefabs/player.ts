// A prefab is a function returning plain data (an EntitySpec).
import type { EntitySpec, Vec2 } from "alpha";
import type { Game } from "../types";

export function player(at: Vec2): EntitySpec<Game, "player"> {
  return { tags: ["player"], pos: at, shape: { image: "player" } };
}
