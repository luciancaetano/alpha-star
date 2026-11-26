// Every name the game uses. The compiler rejects anything not listed here.
export interface Starfighter {
  actions: "fire" | "restart" | "crt";
  axes: "moveX" | "moveY";
  sounds: "shot" | "boom" | "hurt";
  /** Streamed music (assets.music): a generated WAV loop. */
  music: "theme";
  images: "ship" | "enemy";
  tags: { ship: { lives: number; cooldown: number; safeUntil: number }; enemy: { fall: number; phase: number }; shot: {}; star: {}; boom: {} };
  /** config.layers: stars far behind (parallax), effects on top. */
  layers: "stars" | "fx";
  /** config.collision: shots hit enemies only, enemies hit the ship and shots, never each other. */
  categories: "ship" | "enemy" | "shot";
  ui: { hud: "score" | "lives"; over: "score" | "again" };
  scenes: { play: undefined; over: { score: number } };
}
