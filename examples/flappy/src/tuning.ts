// All tunable numbers in one place.
export const WORLD = { width: 288, height: 512, groundHeight: 40 };
export const BIRD = { x: 80, startY: 240, radius: 12, flap: -320, maxTilt: 90 };
// Feathers thrown on every flap; the game-over screen darkens to FADE.alpha in FADE.seconds.
export const FEATHERS = { count: 8 };
export const FADE = { alpha: 0.55, seconds: 0.6 };
export const PIPE = { width: 52, gap: 130, speed: 120, every: 1.5, margin: 60 };
