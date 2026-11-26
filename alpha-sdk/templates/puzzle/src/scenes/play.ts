import type { Context } from "alpha";
import { scene } from "../alpha";
import { LEVELS } from "../levels";
import { tile, toPixels } from "../prefabs/tiles";
import { move, parse, solved, type Dir, type Level, type Position } from "../rules";
import { LANGUAGES, SLIDE, VIEW } from "../tuning";
import type { Crates } from "../types";

type State = { level: number; pos: Position; history: Position[]; moves: number; done: boolean };
type Play = Context<Crates, State>;

const DIRS: Dir[] = ["up", "down", "left", "right"];
const levelOf = (ctx: Play): Level => parse(LEVELS[ctx.state.level]).level;

function hud(ctx: Play) {
  const best = ctx.save.get(`best${ctx.state.level}`, 0);
  ctx.ui.set("hud", "level", ctx.t("hud.level", { n: ctx.state.level + 1, of: LEVELS.length }));
  ctx.ui.set("hud", "moves", ctx.t("hud.moves", { count: ctx.state.moves }));
  ctx.ui.set("hud", "best", best ? ctx.t("hud.best", { n: best }) : "");
  ctx.find("language").text = ctx.t("hud.language");  // world text: the language's own name
}

// Puts every sprite where the position says; `glide` tweens instead of jumping.
function place(ctx: Play, glide: boolean) {
  const level = levelOf(ctx);
  const seconds = ctx.settings.reducedMotion ? 0 : SLIDE;
  const put = (e: ReturnType<Play["find"]>, cell: [number, number]) => {
    const [x, y] = toPixels(cell, level.size);
    if (glide && seconds > 0) ctx.tween(e, { to: { x, y }, seconds, ease: "quadOut" });
    else [e.pos.x, e.pos.y] = [x, y];
  };
  put(ctx.find("player"), ctx.state.pos.player);
  for (const box of ctx.findAll("box")) {
    const cell = ctx.state.pos.boxes[box.data.i as number];
    put(box, cell);
    box.animation = level.goals.some((g) => g[0] === cell[0] && g[1] === cell[1]) ? "boxOnGoal" : "box";
  }
}

function tryMove(ctx: Play, dir: Dir) {
  const next = move(levelOf(ctx), ctx.state.pos, dir);
  if (!next) return void ctx.audio.play("bump", { volume: 0.5 });
  const pushed = next.boxes !== ctx.state.pos.boxes;
  ctx.state.history.push(ctx.state.pos);
  ctx.state.pos = next;
  ctx.state.moves += 1;
  ctx.audio.play(pushed ? "push" : "step");
  place(ctx, true);
  if (solved(levelOf(ctx), next)) {
    ctx.state.done = true;
    ctx.audio.play("solved");
    ctx.after(0.4, () => ctx.go("solved", { level: ctx.state.level, moves: ctx.state.moves }));
  }
}

function undo(ctx: Play) {
  const previous = ctx.state.history.pop();
  if (!previous) return;
  ctx.state.pos = previous;
  ctx.state.moves += 1;  // undoing counts as a move, like most Sokoban scoring
  place(ctx, false);
}

export default scene("play", {
  state: ({ level }) => ({ level, pos: parse(LEVELS[level]).start, history: [], moves: 0, done: false }),

  enter(ctx) {
    const { level } = parse(LEVELS[ctx.state.level]);
    for (let y = 0; y < level.size[1]; y++) for (let x = 0; x < level.size[0]; x++) ctx.spawn(tile("floor", [x, y], level.size));
    for (const w of level.walls) ctx.spawn(tile("wall", w, level.size));
    for (const g of level.goals) ctx.spawn(tile("goal", g, level.size));
    ctx.state.pos.boxes.forEach((b, i) => ctx.spawn(tile("box", b, level.size, { i })));
    ctx.spawn(tile("player", ctx.state.pos.player, level.size));
    place(ctx, false);
    ctx.spawn({ tags: ["language"], pos: [VIEW.width / 2, VIEW.height - 24],
                shape: { text: ctx.t("hud.language"), font: "main", size: 10, color: "#ffd166" } });
    ctx.ui.show("hud");
  },

  update(ctx) {
    if (ctx.input.pressed("language")) ctx.locale = LANGUAGES[(LANGUAGES.indexOf(ctx.locale ?? "en") + 1) % LANGUAGES.length];
    if (ctx.input.pressed("restart")) return ctx.go("play", { level: ctx.state.level });
    if (ctx.state.done) return;
    if (ctx.input.pressed("undo")) undo(ctx);
    const dir = DIRS.find((d) => ctx.input.pressed(d));
    if (dir) tryMove(ctx, dir);
    hud(ctx);
  },
});
