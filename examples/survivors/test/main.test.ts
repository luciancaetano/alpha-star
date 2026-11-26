// Headless agent tests: same seed + same inputs => same result, every run.
import { simulate } from "alpha/test";
import { waveAt } from "../src/data/waves";
import { applyOffer, openChest, rollOffers } from "../src/game/offers";
import { gainXp, newRun, type RunState } from "../src/game/run";
import { PASSIVE_IDS, PASSIVES, WEAPON_IDS, WEAPONS, weaponLevel } from "../src/data/items";
import game from "../src/main";
import { HEROES } from "../src/tuning";
import type { HeroId } from "../src/types";

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

export function theTouchStickMovesTheHeroOnAPhone() {
  // A finger on the left half grows the stick where it lands; dragging right walks right.
  const run = simulate(game, { ...PLAY, seconds: 1, platform: "android",
    touches: [{ id: 1, path: [[0, [80, 120]], [2, [120, 120]]] }] });
  run.expect.noErrors();
  const [h] = run.entities("hero");
  if (h.pos[0] < 50) fail(`hero should walk right with the touch stick, x ${h.pos[0]}`);
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
  if (s.weapons.length !== 2 || s.weapons[1].level !== 1) fail("a new weapon joins at level 1");
  applyOffer(s, { kind: "weapon", id: s.weapons[0].id }, 100);
  if (s.weapons[0].level !== 2) fail("an owned weapon levels up");
}

export function chestEvolvesAMaxedWeaponWithItsPassive() {
  const s: RunState = { ...newRun("wick", {}), passives: { ember: 1 } };
  s.weapons[0].level = 6;
  const rng = { range: () => 0, int: (a: number) => a, pick: <T>(xs: readonly T[]) => xs[0] };
  const card = openChest(s, rng, (k) => k);
  if (!s.weapons[0].evo || card.name !== "weapon.flare.evo") fail(`expected Solar Flare, got ${card.name}`);
}

export function fiftyNewPowersEachWithAUniqueIcon() {
  if (WEAPON_IDS.length + PASSIVE_IDS.length !== 5 + 7 + 50) fail(`expected 50 new powers, ${WEAPON_IDS.length} weapons ${PASSIVE_IDS.length} passives`);
  const icons = [...WEAPON_IDS.flatMap((id) => [WEAPONS[id].icon, WEAPONS[id].evoIcon]), ...PASSIVE_IDS.map((id) => PASSIVES[id].icon)];
  if (new Set(icons).size !== icons.length) fail("every power has its own icon");
}

export function eachHeroStartsWithASignatureNoOneElseIsOffered() {
  const always = { range: () => 0.5, int: (a: number) => a, pick: <T>(xs: readonly T[]) => xs[0] };
  for (const hero of Object.keys(HEROES) as HeroId[]) {
    const s = newRun(hero, {});
    const sig = HEROES[hero].weapon;
    if (s.weapons[0].id !== sig || WEAPONS[sig].hero !== hero) fail(`${hero} starts with its signature ${sig}`);
    s.weapons = [];
    for (let i = 0; i < 40; i++) {
      const offered = rollOffers(s, { ...always, pick: <T>(xs: readonly T[]) => xs[i % xs.length] });
      const foreign = offered.find((o) => o.kind === "weapon" && WEAPONS[o.id].hero && WEAPONS[o.id].hero !== hero);
      if (foreign) fail(`${hero} was offered another hero's signature ${JSON.stringify(foreign)}`);
    }
  }
}

export function signaturesStartStrongerThanTheBasicWeapon() {
  const lash = weaponLevel("lash", 1, false), whip = weaponLevel("whip", 1, false);
  if (lash.dmg <= whip.dmg || lash.count < 2) fail("Sister Taper's lash starts stronger and on both sides");
}

export function everyHeroAndManyPowersSurviveAMinuteWithoutErrors() {
  // Every hero, taking every card: all weapon kinds and status effects get exercised.
  const picks = Array.from({ length: 60 }, (_, i) => 10 + i * 40);
  for (const hero of Object.keys(HEROES) as HeroId[]) {
    const run = simulate(game, {
      seconds: 60, saves: { main: { hero, heroes: Object.keys(HEROES) } }, clicks: { "title#play": [30] },
      inputs: { pick1: picks, pick2: picks.map((f) => f + 20) },
    });
    run.expect.noErrors();
    if (!run.state) fail(`${hero} died within a minute: ${JSON.stringify(run.saves.main)}`);
    if (state(run).hero !== hero) fail(`expected to play ${hero}, got ${state(run).hero}`);
    run.expect.noErrors();
    if (state(run).kills < 3) fail(`${hero} should kill with its signature, kills ${state(run).kills}`);
  }
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
