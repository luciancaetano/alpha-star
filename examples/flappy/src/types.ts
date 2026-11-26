// Every name the game uses. The compiler rejects anything not listed here.
export interface Flappy {
  actions: "flap" | "restart" | "settings";
  sounds: "flap" | "point";
  images: "player" | "nebula" | "column" | "deck";
  tags: { bird: {}; solid: {}; goal: {}; scrolls: {}; fade: {} };
  /** Streamed music (assets.music). */
  music: "theme";
  /** Languages (assets.locales); texts in assets/locales/*.json. */
  locales: "en" | "pt";
  /** UI documents (assets/ui/*.rml) -> ids of the elements scripts touch. */
  ui: {
    title: "hint" | "settings";
    hud: "score";
    over: "score" | "best" | "again";
    settings: "music" | "sfx" | "music-down" | "music-up" | "sfx-down" | "sfx-up" | "language" | "lang" | "back";
  };
  scenes: {
    title: undefined;
    play: undefined;
    over: { score: number };
    settings: undefined;
  };
}
