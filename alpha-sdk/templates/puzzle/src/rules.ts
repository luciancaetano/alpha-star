// The puzzle's rules as pure functions on plain data: no engine calls, so tests check them directly
// and the scene only draws what they return. Grid cells are [column, row].
export type Cell = [number, number];
export type Dir = "up" | "down" | "left" | "right";

/** A parsed level: what never moves. */
export interface Level {
  size: Cell;
  walls: Cell[];
  goals: Cell[];
}

/** What moves: JSON-safe, so it lives in ctx.state (kept by hot reload) and undo keeps copies. */
export interface Position {
  player: Cell;
  boxes: Cell[];
}

const STEP: Record<Dir, Cell> = { up: [0, -1], down: [0, 1], left: [-1, 0], right: [1, 0] };
const same = (a: Cell, b: Cell) => a[0] === b[0] && a[1] === b[1];
const has = (cells: Cell[], c: Cell) => cells.some((x) => same(x, c));

/** Rows of text: `#` wall, `.` goal, `$` box, `*` box on a goal, `@` player, `+` player on a goal. */
export function parse(rows: string[]): { level: Level; start: Position } {
  const level: Level = { size: [Math.max(...rows.map((r) => r.length)), rows.length], walls: [], goals: [] };
  const start: Position = { player: [0, 0], boxes: [] };
  rows.forEach((row, y) => [...row].forEach((ch, x) => {
    if (ch === "#") level.walls.push([x, y]);
    if (".*+".includes(ch)) level.goals.push([x, y]);
    if ("$*".includes(ch)) start.boxes.push([x, y]);
    if ("@+".includes(ch)) start.player = [x, y];
  }));
  return { level, start };
}

/** The position after moving one cell, pushing at most one box; null when blocked. */
export function move(level: Level, pos: Position, dir: Dir): Position | null {
  const [dx, dy] = STEP[dir];
  const next: Cell = [pos.player[0] + dx, pos.player[1] + dy];
  if (has(level.walls, next)) return null;
  const pushed = pos.boxes.findIndex((b) => same(b, next));
  if (pushed < 0) return { player: next, boxes: pos.boxes };
  const beyond: Cell = [next[0] + dx, next[1] + dy];
  if (has(level.walls, beyond) || has(pos.boxes, beyond)) return null;
  return { player: next, boxes: pos.boxes.map((b, i) => (i === pushed ? beyond : b)) };
}

/** Every box on a goal. */
export function solved(level: Level, pos: Position): boolean {
  return pos.boxes.every((b) => has(level.goals, b));
}
