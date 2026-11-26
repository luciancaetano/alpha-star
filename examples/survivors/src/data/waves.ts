// The night's script: who spawns each minute, how often, and the timed events.
import type { EnemyKind } from "../types";

/** At the start of a minute: who comes, how many per second, how many may be alive. */
export interface Wave { kinds: EnemyKind[]; rate: number; cap: number }

/** Index = minute of the run (0..14). Rate and cap glide from one minute to the next (see `waveAt`). */
export const WAVES: Wave[] = [
  { kinds: ["moth"], rate: 0.7, cap: 12 },
  { kinds: ["moth", "shade"], rate: 1.4, cap: 30 },
  { kinds: ["moth", "shade", "shade"], rate: 2.1, cap: 50 },
  { kinds: ["shade", "wisp"], rate: 2.8, cap: 70 },
  { kinds: ["shade", "ghoul", "moth"], rate: 3.5, cap: 90 },
  { kinds: ["ghoul", "wisp", "moth"], rate: 4.3, cap: 110 },
  { kinds: ["ghoul", "shade"], rate: 5.1, cap: 130 },
  { kinds: ["wisp", "wisp", "ghoul"], rate: 6, cap: 150 },
  { kinds: ["moth", "ghoul", "shade"], rate: 7, cap: 170 },
  { kinds: ["ghoul", "ghoul", "wisp"], rate: 8, cap: 190 },
  { kinds: ["shade", "wisp", "ghoul"], rate: 9, cap: 210 },
  { kinds: ["ghoul", "moth", "wisp"], rate: 10, cap: 225 },
  { kinds: ["ghoul", "ghoul", "shade"], rate: 11, cap: 240 },
  { kinds: ["wisp", "ghoul", "shade"], rate: 12, cap: 250 },
  { kinds: ["ghoul", "ghoul", "wisp", "shade"], rate: 13, cap: 260 },
];

/** The wave at `seconds`: this minute's foes, rate and cap blended toward the next minute's. */
export function waveAt(seconds: number): Wave {
  const m = Math.min(WAVES.length - 1, Math.floor(seconds / 60));
  const next = WAVES[Math.min(WAVES.length - 1, m + 1)];
  const t = m === WAVES.length - 1 ? 0 : (seconds - m * 60) / 60;
  const w = WAVES[m];
  return { kinds: w.kinds, rate: w.rate + (next.rate - w.rate) * t, cap: Math.round(w.cap + (next.cap - w.cap) * t) };
}

/** Enemy hit points grow through the night. */
export const hpScale = (seconds: number) => 1 + seconds / 60 * 0.16;
/** Contact damage grows too: a build that stops moving falls behind. */
export const dmgScale = (seconds: number) => 1 + seconds / 60 * 0.12;

export type EventKind = "swarm" | "ring" | "elite" | "boss" | "snuffer";
export interface NightEvent { at: number; kind: EventKind; enemy: EnemyKind; count: number }
/** Banner text: locale key `event.<at>`. */

export const EVENTS: NightEvent[] = [
  { at: 75, kind: "swarm", enemy: "moth", count: 12 },
  { at: 150, kind: "elite", enemy: "gargoyle", count: 1 },
  { at: 210, kind: "ring", enemy: "shade", count: 28 },
  { at: 300, kind: "boss", enemy: "queen", count: 1 },
  { at: 390, kind: "swarm", enemy: "wisp", count: 30 },
  { at: 450, kind: "elite", enemy: "gargoyle", count: 2 },
  { at: 540, kind: "ring", enemy: "ghoul", count: 30 },
  { at: 600, kind: "boss", enemy: "queen", count: 2 },
  { at: 690, kind: "swarm", enemy: "moth", count: 50 },
  { at: 750, kind: "elite", enemy: "gargoyle", count: 3 },
  { at: 810, kind: "ring", enemy: "wisp", count: 40 },
  { at: 840, kind: "elite", enemy: "gargoyle", count: 4 },
  { at: 900, kind: "snuffer", enemy: "snuffer", count: 1 },
];
