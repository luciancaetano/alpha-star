// Gameplay numbers. Change the feel here, not in the scenes.
export const VIEW = { width: 240, height: 320 };
export const SHIP = { speed: 140, lives: 3, start: [120, 280] as [number, number], safeSeconds: 1.2, margin: 10 };
export const SHOT = { speed: 320, cooldown: 0.12 };
export const ENEMY = { every: 0.7, fall: [45, 80] as [number, number], sway: 40, swayHz: 0.8, points: 10 };
export const STARS = { count: 120, speeds: [20, 45, 90] };  // three depths, px/s
