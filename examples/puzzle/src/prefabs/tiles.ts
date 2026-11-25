// One sprite per grid cell, from the "tiles" sheet (each animation is one frame).
import type { EntitySpec, Vec2 } from "alpha";
import type { Cell } from "../rules";
import { TILE, VIEW } from "../tuning";
import type { Crates } from "../types";

type Spec = EntitySpec<Crates["tags"], never, Crates["sheets"]>;
type Kind = "wall" | "floor" | "goal" | "box" | "player";

/** Pixel center of a cell, with the level centered on screen. */
export function toPixels(cell: Cell, size: Cell): Vec2 {
  const left = (VIEW.width - size[0] * TILE) / 2;
  const top = (VIEW.height - size[1] * TILE) / 2 + 8;
  return [left + (cell[0] + 0.5) * TILE, top + (cell[1] + 0.5) * TILE];
}

const Z: Record<Kind, number> = { floor: 0, goal: 1, wall: 1, box: 2, player: 3 };

export function tile(kind: Kind, cell: Cell, size: Cell, data = {}): Spec {
  return { tags: [kind], pos: toPixels(cell, size), z: Z[kind], shape: { sheet: "tiles", animation: kind }, data };
}
