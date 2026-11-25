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
export const CAMERA = { lerp: 0.15, deadzone: [48, 32] as Vec2, shake: 3, shakeSeconds: 0.25 };
