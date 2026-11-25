// Every name the game uses. The compiler rejects anything not listed here.
export interface Platformer {
  actions: "jump" | "restart";
  axes: "moveX";
  sounds: "jump" | "coin" | "land";
  images: never;
  /** "walls" is the Tiled layer whose tiles become colliders (spawnMap collide). */
  tags: "player" | "coin" | "walls" | "dust" | "popup" | "scenery";
  /** Sheet -> its animation names (assets.sheets in main.ts). */
  sheets: { hero: "idle" | "run" | "jump" | "fall"; coin: "spin" };
  maps: "level1";
  /** config.layers: far scenery (parallax), the level, floating effects on top. */
  layers: "far" | "fx";
  fonts: "pixel";
  ui: { hud: "coins"; won: never };
  scenes: { play: undefined; won: undefined };
}
