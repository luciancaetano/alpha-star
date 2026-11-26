// The lab's rules as pure functions: tested directly in test/main.test.ts.
import { INTENSITY, SHADERS } from "./tuning";

/** Index `step` places after `index` in SHADERS, wrapping both ways. */
export function cycle(index: number, step: number): number {
  const n = SHADERS.length;
  return (((index + step) % n) + n) % n;
}

/** Intensity moved by `steps` INTENSITY.step, kept in [0, 1]. */
export function adjust(intensity: number, steps: number): number {
  return Math.min(1, Math.max(0, Math.round((intensity + steps * INTENSITY.step) * 100) / 100));
}

/** "WAVE 1/6" for the HUD. */
export function label(index: number): string {
  return `${SHADERS[index].toUpperCase()} ${index + 1}/${SHADERS.length}`;
}

/** "INTENSITY 75%" for the HUD. */
export function level(intensity: number): string {
  return `INTENSITY ${Math.round(intensity * 100)}%`;
}
