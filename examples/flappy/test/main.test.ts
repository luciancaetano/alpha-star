// Headless agent tests: same seed + same inputs => same result, every run.
import { simulate } from "alpha/test";
import game from "../src/main";

// One flap every 40 frames roughly holds altitude (flap -320 px/s, gravity 900 px/s^2).
// Flapping every 20 frames climbs ~57 px per flap and leaves the top of the screen at ~1.4 s.
const FLAP_STEADILY = Array.from({ length: 5 }, (_, i) => i * 40);

export function birdDiesWithoutInput() {
  simulate(game, { scene: "play", seconds: 3 }).expect.scene("over");
}

export function flappingKeepsBirdAliveBeforeFirstPipe() {
  const run = simulate(game, { scene: "play", seconds: 1, inputs: { flap: [0, 20, 40] } });
  run.expect.scene("play");
  run.expect.noErrors();
}

export function pipesSpawnOnSchedule() {
  // Two pairs by t=3.1s; each pair is 2 pipes + 1 goal.
  const run = simulate(game, { scene: "play", seconds: 3.1, inputs: { flap: FLAP_STEADILY } });
  run.expect.scene("play");
  run.expect.count("scrolls").atLeast(6);
}
