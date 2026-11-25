// Gameplay numbers. Change the feel here, not in the scenes.
import type { Vec2 } from "alpha";

export const VIEW = { width: 320, height: 180 };
export const GRAVITY = 900;
export const PLAYER = {
  size: [12, 16] as Vec2,
  speed: 110,        // px/s at full stick / key
  jump: -330,        // px/s upward on jump
  hardLanding: 300,  // landing faster than this (px/s) shakes the camera
};
// Dust puffed on landing, the "+1" floating over a picked coin.
export const DUST = { count: 10, landing: 120 };  // landing faster than DUST.landing px/s raises dust
export const POPUP = { rise: 24, fade: 1.6 };      // px/s up, alpha lost per second
export const CAMERA = { lerp: 0.15, deadzone: [48, 32] as Vec2, shake: 3, shakeSeconds: 0.25 };
// A one-way plank over the first coins (jump up through it, stand on it) and a lift
// gliding over the pit (a tweened kinematic body: it carries the hero).
export const LEDGES: Array<{ pos: Vec2; width: number }> = [{ pos: [120, 124], width: 48 }];
export const LIFT = { from: [340, 128] as Vec2, toX: 470, size: [40, 6] as Vec2, seconds: 2.5 };
// The ground check: rays this far below the hero's feet, at its center and both edges.
export const GROUND_RAY = { reach: 3, edge: 5 };
export const COIN_POP = { seconds: 0.25, scale: 2.5 };  // picked coins grow and fade out
