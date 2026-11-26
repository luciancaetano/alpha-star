// Numbers that set the look and feel. The shader list lives here: one place for its order.
import type { ShaderLab } from "./types";

export type ShaderName = keyof ShaderLab["shaders"];

export const VIEW = { width: 320, height: 180 };

/** Cycle order: next/prev walk it and wrap around. */
export const SHADERS: readonly ShaderName[] = ["wave", "chroma", "sepia", "posterize", "scanlines", "pulse"];

/** intensity uniform: starts at `start`, moves by `step` per press, stays in [0, 1]. */
export const INTENSITY = { start: 0.75, step: 0.25 };

export const ORBS = { count: 5, radius: 9, orbitX: 84, orbitY: 52, speed: 0.6, light: 90 };
