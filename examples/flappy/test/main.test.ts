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

export function secondPlayerCanFlapToo() {
  // Player 2's gamepad (e.g. its right trigger) flaps like player 1's keys.
  const run = simulate(game, { scene: "play", seconds: 1, inputs: [{}, { flap: [0, 20, 40] }] });
  run.expect.scene("play");
}

export function pipesSpawnOnSchedule() {
  // Two pairs by t=3.1s; each pair is 2 pipes + 1 goal.
  const run = simulate(game, { scene: "play", seconds: 3.1, inputs: { flap: FLAP_STEADILY } });
  run.expect.scene("play");
  run.expect.count("scrolls").atLeast(6);
}

export function titleShowsItsScreen() {
  simulate(game, { seconds: 0.1 }).expect.shown("title");
}

export function hudStartsAtZero() {
  const run = simulate(game, { scene: "play", seconds: 0.5 });
  run.expect.shown("hud");
  run.expect.text("hud", "score").equals("0");
}

export function gameOverShowsScoreAndBest() {
  const run = simulate(game, { scene: "play", seconds: 3 });
  run.expect.scene("over");
  run.expect.text("over", "score").equals("0");
  run.expect.text("over", "best").equals("0");
}

export function playAgainButtonRestarts() {
  // The bird hits the ground well before 3 s; the click lands on the game-over screen.
  const run = simulate(game, { scene: "play", seconds: 3.1, clicks: { "over#again": [180] } });
  run.expect.scene("play");
  run.expect.shown("hud");
}

export function flappingThrowsFeathers() {
  // One flap on frame 0: a burst of 8 feathers that live 0.3 s or more.
  const run = simulate(game, { scene: "play", seconds: 0.2, inputs: { flap: [0] } });
  run.expect.particles().equals(8);
  simulate(game, { scene: "play", seconds: 1 }).expect.particles().equals(0);
}

export function gameOverFadesTheWorld() {
  const run = simulate(game, { scene: "play", seconds: 3.5 });
  run.expect.scene("over");
  run.expect.count("fade").equals(1);
  run.expect.noErrors();
}

export function themeMusicStartsOnTheTitle() {
  simulate(game, { seconds: 0.1 }).expect.music("theme");
}

export function titleSpeaksThePlayersLanguage() {
  simulate(game, { seconds: 0.1 }).expect.text("title", "hint").equals("press flap");
  const pt = simulate(game, { seconds: 0.1, settings: { locale: "pt" } });
  pt.expect.text("title", "hint").equals("aperte para voar");
}

export function settingsMenuSetsVolumeAndLanguage() {
  // S / Start opens the menu; the clicks land on its buttons (gamepad A on focus does the same).
  const run = simulate(game, {
    seconds: 0.3,
    inputs: { settings: [0] },
    clicks: { "settings#music-down": [5, 6], "settings#sfx-up": [7], "settings#language": [8] },
  });
  run.expect.scene("settings");
  run.expect.text("settings", "music").equals("80%");
  run.expect.text("settings", "lang").equals("PORTUGUÊS");
  if (run.settings.volumes.music !== 0.8 || run.locale !== "pt") throw new Error(`settings not kept: ${JSON.stringify(run.settings)}`);
}

export function settingsBackReturnsToTitle() {
  const run = simulate(game, { seconds: 0.3, inputs: { settings: [0] }, clicks: { "settings#back": [5] } });
  run.expect.scene("title");
}

export function bestScoreIsKeptInTheSave() {
  const run = simulate(game, { scene: "play", seconds: 3, saves: { main: { best: 7 } } });
  run.expect.text("over", "best").equals("7");
  if (run.saves.main.best !== 7) throw new Error(JSON.stringify(run.saves));
}
