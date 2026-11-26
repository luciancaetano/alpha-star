// Gameplay numbers. Change the feel here, not in the scenes.
export const VIEW = { width: 320, height: 240 };
export const WALL = 8;                                   // arena border thickness (px)
export const HERO = { speed: 90, hp: 3, bombs: 2, radius: 6, starts: [[140, 120], [180, 120]] as Array<[number, number]> };
export const BULLET = { speed: 260, life: 0.8, cooldown: 0.18, radius: 2 };
export const ENEMY = { speed: 38, every: 1.1, max: 14, radius: 6 };
export const BOMB = { radius: 72, shake: 4, seconds: 0.3 };
export const LABEL = { rise: 16, seconds: 0.6 };         // "+1" floats up and fades
