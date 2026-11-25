// Every name the game uses. The compiler rejects anything not listed here.
export interface Arena {
  actions: "fire" | "bomb" | "restart";
  /** Per player: player 1 = WASD/arrows/first pad, player 2 = second pad. */
  axes: "moveX" | "moveY";
  sounds: "shot" | "hit" | "boom";
  images: never;
  tags: "hero" | "enemy" | "bullet" | "wall" | "blast" | "label";
  /** Sheet -> its animation names (assets.sheets in main.ts). */
  sheets: { hero: "idle" | "walk" };
  /** No fonts: UI and world text use the engine's built-in 8x8 font. */
  ui: { hud: "score" | "hp" | "bombs"; over: "score" | "best" | "again" };
  scenes: { play: undefined; over: { score: number } };
}
