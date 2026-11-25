// Headless agent tests. The rules are pure functions, tested directly; the game through simulate().
import { simulate } from "alpha/test";
import { LEVELS } from "../src/levels";
import { move, parse, solved } from "../src/rules";
import game from "../src/main";

function check(ok: boolean, what: string) {
  if (!ok) throw new Error(what);
}

export function rulesPushOneBoxButNeverTwoOrIntoAWall() {
  const { level, start } = parse(["#####", "#@$$#", "#####"]);
  check(move(level, start, "right") === null, "two boxes in a row cannot be pushed");
  check(move(level, start, "up") === null, "walls block");
  const { level: open, start: s } = parse(["######", "#@$ .#", "######"]);
  const pushed = move(open, s, "right")!;
  check(pushed.player[0] === 2 && pushed.boxes[0][0] === 3, `pushed: ${JSON.stringify(pushed)}`);
  check(!solved(open, pushed) && solved(open, move(open, pushed, "right")!), "solved when every box is on a goal");
}

export function everyLevelHasAsManyBoxesAsGoalsAndOnePlayer() {
  LEVELS.forEach((rows, i) => {
    const { level, start } = parse(rows);
    check(level.goals.length === start.boxes.length, `level ${i + 1}: ${level.goals.length} goals, ${start.boxes.length} boxes`);
    check(rows.join("").split("").filter((c) => "@+".includes(c)).length === 1, `level ${i + 1}: one player`);
  });
}

export function startsOnTheSavedLevel() {
  simulate(game, { seconds: 0.2 }).expect.text("hud", "level").equals("LEVEL 1/3");
  simulate(game, { seconds: 0.2, saves: { main: { level: 1 } } }).expect.text("hud", "level").equals("LEVEL 2/3");
}

export function onePushSolvesLevelOneAndSavesTheBest() {
  const run = simulate(game, { seconds: 1, inputs: { right: [5] } });
  run.expect.noErrors();
  run.expect.scene("solved");
  run.expect.text("solved", "moves").equals("IN 1 MOVE");
  check(run.saves.main.best0 === 1 && run.saves.main.level === 1, `saves: ${JSON.stringify(run.saves)}`);
}

export function levelTwoInFourMovesThenNextGoesToLevelThree() {
  const solve = { right: [5, 15], left: [25], up: [35] };
  const run = simulate(game, { seconds: 1.5, saves: { main: { level: 1 } }, inputs: solve });
  run.expect.scene("solved");
  run.expect.text("solved", "moves").equals("IN 4 MOVES");
  const next = simulate(game, { seconds: 1.6, saves: { main: { level: 1 } }, inputs: solve, clicks: { "solved#next": [80] } });
  next.expect.text("hud", "level").equals("LEVEL 3/3");
}

export function undoPutsTheBoxBackAndCountsAMove() {
  const run = simulate(game, { seconds: 1, saves: { main: { level: 1 } }, inputs: { right: [5], undo: [15] } });
  const boxes = run.entities("box").map((b) => b.pos[0]);
  const fresh = simulate(game, { seconds: 1, saves: { main: { level: 1 } } }).entities("box").map((b) => b.pos[0]);
  check(JSON.stringify(boxes) === JSON.stringify(fresh), `boxes back: ${boxes} vs ${fresh}`);
  run.expect.text("hud", "moves").equals("2 MOVES");
}

export function walkingIntoAWallIsNotAMove() {
  // One step left reaches the wall; the second press bumps it.
  simulate(game, { seconds: 0.5, inputs: { left: [5, 10] } }).expect.text("hud", "moves").equals("1 MOVE");
}

export function lTogglesPortuguese() {
  const run = simulate(game, { seconds: 0.5, inputs: { language: [5] } });
  run.expect.text("hud", "level").equals("NÍVEL 1/3");
  if (run.settings.locale !== "pt") throw new Error(`locale persisted: ${run.settings.locale}`);
}

export function reducedMotionSnapsInsteadOfGliding() {
  // Frame 0 is the start scene; the press on frame 3 moves, measured two steps later.
  const glide = simulate(game, { seconds: 5 / 60, inputs: { right: [3] }, saves: { main: { level: 1 } } }).entities("player")[0].pos[0];
  const snap = simulate(game, { seconds: 5 / 60, inputs: { right: [3] }, saves: { main: { level: 1 } }, settings: { reducedMotion: true } })
    .entities("player")[0].pos[0];
  check(snap > glide, `snapped ${snap} should be ahead of gliding ${glide}`);
}
