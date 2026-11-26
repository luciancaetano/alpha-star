// Every name the game uses. The compiler rejects anything not listed here.
export interface Platformer {
  actions: "jump" | "restart";
  axes: "moveX";
  sounds: "jump" | "coin" | "land";
  images: never;
  /** "walls": colliders made from the LDtk IntGrid layer "Walls" (spawnLevel collide). */
  tags: { player: {}; coin: { taken?: boolean }; walls: {}; dust: {}; popup: {}; scenery: {}; ledge: {}; lift: {} };
  /** Sheet -> its animation names (assets.sheets in main.ts). */
  sheets: { hero: "idle" | "run" | "jump" | "fall"; coin: "spin" };
  worlds: "main";
  /** config.layers: far scenery (parallax), the level, floating effects on top. */
  layers: "far" | "fx";
  fonts: "pixel";
  ui: { hud: "coins"; won: never };
  scenes: { play: undefined; won: undefined };
  /** Game events (ctx.emit / ctx.on), delivered at the end of the step. */
  events: { coinTaken: undefined };
}
