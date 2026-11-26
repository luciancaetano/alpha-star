// Headless agent tests: same seed + same inputs => same result, every run.
import { simulate } from "alpha/test";
import { waveAt } from "../src/data/waves";
import { applyOffer, openChest, rollOffers } from "../src/game/offers";
import { gainXp, newRun, type RunState } from "../src/game/run";
import game from "../src/main";

const state = (run: { state: unknown }) => run.state as RunState;
const PLAY = { scene: "play" as const };
const fail = (msg: string) => { throw new Error(msg); };

export function titleShowsAndPlayStartsANight() {
  const run = simulate(game, { seconds: 1, clicks: { "title#play": [30] } });
  run.expect.noErrors();
  run.expect.scene("play");
  run.expect.shown("hud");
  run.expect.count("hero").equals(1);
}

export function enemiesComeAndTheBoltKillsThem() {
  const run = simulate(game, { ...PLAY, seconds: 20 });
  run.expect.noErrors();
  run.expect.count("enemy").atLeast(1);
  if (state(run).kills < 3) fail(`auto weapons should kill, kills ${state(run).kills}`);
}

export function walkingMovesTheHero() {
  const run = simulate(game, { ...PLAY, seconds: 1, inputs: { axes: { moveX: [[0, 1]] } } });
  run.expect.animation("hero").equals("walk");
  const [h] = run.entities("hero");
  if (h.pos[0] < 50) fail(`hero should walk right, x ${h.pos[0]}`);
}

export function gemsLevelUpAndACardCanBePicked() {
  // Gems come to the hero; the first level opens the cards (the world pauses on them).
  const run = simulate(game, { ...PLAY, seconds: 60, inputs: { pick1: [3590] } });
  run.expect.noErrors();
  if (state(run).level < 2) fail(`expected a level up, level ${state(run).level}`);
  const owned = state(run).weapons.length + Object.keys(state(run).passives).length;
  if (owned < 2) fail(`picking a card adds an item, have ${owned}`);
}

export function givingUpEndsTheNightAndBanksTheGold() {
  const run = simulate(game, { ...PLAY, seconds: 2, saves: { main: { gold: 7 } }, inputs: { pause: [30] }, clicks: { "paused#quit": [60] } });
  run.expect.noErrors();
  run.expect.scene("over");
  run.expect.shown("over");
  if ((run.saves.main.gold as number) < 7) fail("gold is kept and added to");
}

export function aLevelUpHoldsTheWorldStill() {
  // Nobody picks a card: the clock stops at the first level up.
  const run = simulate(game, { ...PLAY, seconds: 90 });
  run.expect.shown("levelup");
  if (state(run).time > 70) fail(`time should stop on the cards, at ${state(run).time}`);
}

export function offersAreDistinctAndFillSlots() {
  const s = newRun("wick", {});
  const rng = { range: () => 0.5, int: (a: number) => a, pick: <T>(xs: readonly T[]) => xs[0] };
  const offers = rollOffers(s, rng);
  if (offers.length !== 3) fail(`three offers, got ${offers.length}`);
  if (new Set(offers.map((o) => JSON.stringify(o))).size !== 3) fail("offers are distinct");
  applyOffer(s, offers[0], 100);
  if (s.weapons[0].level !== 2) fail("the owned bolt levels first (weighted double)");
}

export function chestEvolvesAMaxedWeaponWithItsPassive() {
  const s: RunState = { ...newRun("wick", {}), passives: { tome: 1 } };
  s.weapons[0].level = 6;
  const rng = { range: () => 0, int: (a: number) => a, pick: <T>(xs: readonly T[]) => xs[0] };
  const card = openChest(s, rng, (k) => k);
  if (!s.weapons[0].evo || card.name !== "weapon.bolt.evo") fail(`expected Sunfire Lance, got ${card.name}`);
}

export function xpCarriesOverLevels() {
  const s = newRun("wick", {});
  if (gainXp(s, 4 + 8 + 3) !== 2 || s.level !== 3 || s.xp !== 3) fail(`level ${s.level} xp ${s.xp}`);
}

export function shopUpgradesCostGold() {
  const run = simulate(game, { seconds: 1, saves: { main: { gold: 100 } }, clicks: { "title#shop": [10], "shop#might": [30] } });
  run.expect.noErrors();
  if (run.saves.main.gold !== 40) fail(`might costs 60, gold ${run.saves.main.gold}`);
  run.expect.text("shop", "might").equals("Força +5%  1/5  120");
}

export function aLongNightWithManyWeaponsRunsWithoutErrors() {
  // Regression: chain spark once struck a foe another strike had just killed ("entity is not alive").
  const every = (n: number, from: number) => Array.from({ length: 60 }, (_, i) => from + i * n);
  const circle = Array.from({ length: 60 }, (_, i) => [i * 300, Math.cos(i) ] as [number, number]);
  const run = simulate(game, {
    ...PLAY, seconds: 300,
    inputs: { pick1: every(300, 10), pick2: every(300, 110), pick3: every(300, 210), axes: { moveX: circle, moveY: circle.map(([f, v]) => [f, Math.sin(v * 3)] as [number, number]) } },
  });
  run.expect.noErrors();
}

export function portugueseByDefaultAndTheButtonSwitchesToEnglish() {
  const pt = simulate(game, { seconds: 0.5 });
  if (pt.locale !== "pt") fail(`default should be pt, got ${pt.locale}`);
  pt.expect.text("title", "best").equals("Sobreviva até o amanhecer");
  const en = simulate(game, { seconds: 0.5, clicks: { "title#lang": [10] } });
  if (en.locale !== "en") fail(`lang button should switch to en, got ${en.locale}`);
  en.expect.text("title", "best").equals("Survive until dawn");
  // Powers on the level-up cards are translated too (the first cards of this seed).
  const cards = simulate(game, { ...PLAY, seconds: 90 });
  cards.expect.shown("levelup");
  cards.expect.text("levelup", "c1tag").equals("NOVO!");
}

export function standingStillInTheCrowdIsDeadly() {
  // Even taking every card, a hero who never moves is overrun within a few minutes.
  const picks = Array.from({ length: 80 }, (_, i) => 10 + i * 150);
  const run = simulate(game, { ...PLAY, seconds: 200, inputs: { pick1: picks } });
  run.expect.noErrors();
  run.expect.scene("over");
}

export function theGroundCoversEveryScreenSize() {
  // Desktop, 4:3, ultrawide, 4K and a tall phone: the visible world differs, the floor must fill it.
  const sizes: Array<[number, number]> = [[768, 432], [1024, 768], [3440, 1440], [3840, 2160], [1080, 2340]];
  for (const size of sizes) {
    const run = simulate(game, { ...PLAY, seconds: 5, size, inputs: { pick1: [200], axes: { moveX: [[0, 1]], moveY: [[0, 0.6]] } } });
    run.expect.noErrors();
    const [vx, vy, vw, vh] = state(run).view;
    const tiles = run.entities("ground").filter((t) => t.alpha > 0).map((t) => t.pos);
    const covered = (x: number, y: number) => tiles.some(([tx, ty]) => Math.abs(tx - x) <= 64 && Math.abs(ty - y) <= 64);
    for (const [x, y] of [[vx, vy], [vx + vw, vy], [vx, vy + vh], [vx + vw, vy + vh], [vx + vw / 2, vy + vh / 2]]) {
      if (!covered(x, y)) fail(`${size}: view ${state(run).view.map(Math.round)} has no ground at ${Math.round(x)},${Math.round(y)}`);
    }
  }
}

export function theHordeGrowsGraduallyWithoutJumps() {
  // Rate and cap never jump at a minute mark, and only ever go up.
  for (let t = 1; t < 15 * 60; t++) {
    const [a, b] = [waveAt(t - 1), waveAt(t)];
    if (b.rate < a.rate || b.rate - a.rate > 0.05) fail(`rate jumps at ${t}s: ${a.rate} -> ${b.rate}`);
    if (b.cap < a.cap || b.cap - a.cap > 1) fail(`cap jumps at ${t}s: ${a.cap} -> ${b.cap}`);
  }
  // In play: few foes early, many more a few minutes in.
  const early = simulate(game, { ...PLAY, seconds: 5 }).entities("enemy").length;
  if (early > 6) fail(`the first seconds should be calm, ${early} foes at 5s`);
}
