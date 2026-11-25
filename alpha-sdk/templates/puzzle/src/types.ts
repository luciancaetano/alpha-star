// Every name the game uses. The compiler rejects anything not listed here.
export interface Crates {
  actions: "up" | "down" | "left" | "right" | "undo" | "restart" | "language";
  sounds: "step" | "push" | "bump" | "solved";
  images: never;
  tags: "wall" | "floor" | "goal" | "box" | "player";
  /** One sheet of 16x16 tiles; each "animation" is a single frame. */
  sheets: { tiles: "wall" | "floor" | "goal" | "box" | "boxOnGoal" | "player" };
  /** Texts in assets/locales/*.json: ctx.t(key) in scripts, data-t="key" in .rml. */
  locales: "en" | "pt";
  ui: { hud: "level" | "moves" | "best"; solved: "moves" | "next" };
  /** Levels are 0-based indexes into LEVELS. */
  scenes: { play: { level: number }; solved: { level: number; moves: number }; start: undefined };
}
