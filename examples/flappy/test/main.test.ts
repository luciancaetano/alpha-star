// Headless agent tests: same seed + same inputs => same result, every run.
import { simulate } from "alpha/test";
import game from "../src/main";

const FLAP_EVERY_20 = Array.from({ length: 10 }, (_, i) => i * 20);

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
  const run = simulate(game, { scene: "play", seconds: 3.1, inputs: { flap: FLAP_EVERY_20 } });
  run.expect.count("scrolls").atLeast(6);
}
