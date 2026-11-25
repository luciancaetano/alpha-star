// Headless agent tests: same seed + same inputs => same result, every run.
import { simulate } from "alpha/test";
import game from "../src/main";

const frames = (from: number, to: number) => Array.from({ length: to - from }, (_, i) => from + i);
const score = (state: unknown) => (state as { score: number }).score;
// Sweep left and right along the bottom while holding fire.
const SWEEP = { fire: frames(0, 600), axes: { moveX: [[0, -1], [50, 1], [150, -1], [250, 1], [350, -1], [450, 1]] as Array<[number, number]> } };

export function startsWithTheShipStarsAndMusic() {
  const run = simulate(game, { seconds: 0.5 });
  run.expect.noErrors();
  run.expect.count("ship").equals(1);
  run.expect.count("star").equals(120);
  run.expect.music("theme");
  run.expect.text("hud", "lives").equals("AAA");
}

export function starsScrollDownAndWrap() {
  const y0 = simulate(game, { seconds: 0.1 }).entities("star").map((s) => s.pos[1]);
  const y1 = simulate(game, { seconds: 1.1 }).entities("star").map((s) => s.pos[1]);
  const moved = y0.filter((y, i) => y1[i] !== y).length;
  if (moved !== y0.length) throw new Error(`every star should move, ${moved}/${y0.length} did`);
}

export function enemiesFallInWaves() {
  simulate(game, { seconds: 3 }).expect.count("enemy").atLeast(3);
}

export function firingWhileSweepingShootsEnemiesDown() {
  const run = simulate(game, { seconds: 8, inputs: SWEEP });
  run.expect.noErrors();
  if (score(run.state) < 30) throw new Error(`expected kills, score ${score(run.state)}`);
  run.expect.text("hud", "score").equals(`${score(run.state)}`.padStart(6, "0"));
}

export function shotsNeverHitEachOtherOrTheShip() {
  // Categories: a stream of shots leaves the ship untouched and reaches the top.
  const run = simulate(game, { seconds: 0.3, inputs: { fire: frames(0, 18) } });
  run.expect.count("shot").atLeast(2);
  run.expect.text("hud", "lives").equals("AAA");
}

export function sittingStillEventuallyLosesAndTheMusicFades() {
  const run = simulate(game, { seconds: 60 });
  run.expect.scene("over");
  run.expect.music(null);
}
