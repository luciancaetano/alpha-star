// An endless bog: ground tiles and scenery are recycled around the camera.
// Each cell always uses the same slot (cell mod grid), so a slot only moves when its cell has left
// the view; scenery per cell comes from a hash, so the same place always looks the same.
import { groundTile, prop, SCENERY } from "../prefabs/pickups";
import { GROUND } from "../tuning";
import type { Ent } from "../types";
import type { Play } from "./run";

/** Slots per axis to cover `span` world px with whole cells, plus one spare cell on each side. */
const slots = (span: number, cell: number) => Math.ceil(span / cell) + 2;

const mod = (a: number, n: number) => ((a % n) + n) % n;

function hash(x: number, y: number): number {
  let h = (x * 374761393 + y * 668265263) | 0;
  h = Math.imul(h ^ (h >>> 13), 1274126177);
  return ((h ^ (h >>> 16)) >>> 0) / 4294967296;
}

interface Grid { tag: "ground" | "prop"; cell: number; cols: number; rows: number }

function grids(ctx: Play): Grid[] {
  const [, , vw, vh] = ctx.camera.view;
  return [
    { tag: "ground", cell: GROUND.tile, cols: slots(vw, GROUND.tile), rows: slots(vh, GROUND.tile) },
    { tag: "prop", cell: GROUND.cell, cols: slots(vw, GROUND.cell), rows: slots(vh, GROUND.cell) },
  ];
}

/** (Re)builds a grid's slots when the visible world changed size (window resized, phone rotated). */
function fit(ctx: Play, g: Grid) {
  const have = ctx.findAll(g.tag);
  if (have.length === g.cols * g.rows && have[0]?.data.cols === g.cols) return;
  for (const e of have) ctx.destroy(e);
  for (let i = 0; i < g.cols * g.rows; i++) {
    const slot = { i, cell: "", cols: g.cols };
    ctx.spawn(g.tag === "ground" ? groundTile([0, 0], slot) : prop([0, 0], "reeds", false, slot));
  }
}

export function buildWorld(ctx: Play) {
  updateWorld(ctx);
}

/** The cell of slot `i` near the camera: the one in the camera's window with that cell mod grid. */
function cellOf(i: number, cols: number, rows: number, bx: number, by: number): [number, number] {
  const gx = bx + mod((i % cols) - bx, cols);
  const gy = by + mod(Math.floor(i / cols) - by, rows);
  return [gx, gy];
}

/** First cell of the window: one spare cell before the visible world's left/top edge. */
function origin(ctx: Play, cell: number): [number, number] {
  const [vx, vy] = ctx.camera.view;
  return [Math.floor(vx / cell) - 1, Math.floor(vy / cell) - 1];
}

function placeGround(ctx: Play, g: Grid) {
  const t = g.cell;
  const [bx, by] = origin(ctx, t);
  for (const tile of ctx.findAll("ground")) {
    const [gx, gy] = cellOf(tile.data.i, g.cols, g.rows, bx, by);
    const key = `${gx},${gy}`;
    if (tile.data.cell === key) continue;
    tile.data.cell = key;
    [tile.pos.x, tile.pos.y] = [gx * t + t / 2, gy * t + t / 2];
  }
}

function placeProp(p: Ent<"prop">, gx: number, gy: number) {
  const key = `${gx},${gy}`;
  if (p.data.cell === key) return;
  p.data.cell = key;
  const c = GROUND.cell;
  p.animation = SCENERY[Math.floor(hash(gy, gx) * SCENERY.length)];
  p.flipX = hash(gx + 7, gy) < 0.5;
  const x = Math.round(gx * c + 16 + hash(gx, gy + 3) * (c - 32));
  const y = Math.round(gy * c + 16 + hash(gx + 5, gy) * (c - 32));
  [p.pos.x, p.pos.y] = [x, y];
  p.alpha = hash(gx, gy) < 0.55 ? 1 : 0;   // not every cell has scenery
}

function placeProps(ctx: Play, g: Grid) {
  const [bx, by] = origin(ctx, g.cell);
  for (const p of ctx.findAll("prop")) {
    const [gx, gy] = cellOf(p.data.i, g.cols, g.rows, bx, by);
    placeProp(p, gx, gy);
  }
}

export function updateWorld(ctx: Play) {
  const [ground, props] = grids(ctx);
  fit(ctx, ground);
  fit(ctx, props);
  placeGround(ctx, ground);
  placeProps(ctx, props);
}
