// Headless tests (`npm test`): every exported function runs the game with simulate().
// Same seed + same inputs => same result, every run.
import { simulate } from "alpha/test";
import game from "../src/main";

export function startsWithThePlayerInTheMiddle() {
  const run = simulate(game, { seconds: 0.5 });
  run.expect.noErrors();
  run.expect.count("player").equals(1);
  const [p] = run.entities("player");
  if (p.pos[0] !== 160 || p.pos[1] !== 90) throw new Error(`player at ${p.pos}`);
}

export function holdingRightMovesThePlayer() {
  // moveX held at +1 from frame 0 (D, Right or the stick).
  const run = simulate(game, { seconds: 0.5, inputs: { axes: { moveX: [[0, 1]] } } });
  const [p] = run.entities("player");
  if (p.pos[0] < 210) throw new Error(`expected the player to move right, at x ${p.pos[0]}`);
}

export function jumpSquashesAndSettles() {
  const mid = simulate(game, { seconds: 0.1, inputs: { jump: [0] } }).entities("player")[0];
  if (mid.scale <= 1) throw new Error(`scale should grow, ${mid.scale}`);
  const after = simulate(game, { seconds: 1, inputs: { jump: [0] } }).entities("player")[0];
  if (after.scale !== 1) throw new Error(`scale should settle at 1, ${after.scale}`);
}
