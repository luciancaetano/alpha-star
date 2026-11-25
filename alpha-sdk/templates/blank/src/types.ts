// Every name the game uses. The compiler rejects anything not listed here: add names here first.
export interface Game {
  actions: "jump";
  axes: "moveX" | "moveY";
  sounds: "blip";
  images: "player";
  tags: "player";
  /** UI documents -> ids scripts touch. `{}` when the game has no UI. */
  ui: {};
  /** Scene name -> params its enter() receives (`undefined` = none). */
  scenes: { play: undefined };
}
