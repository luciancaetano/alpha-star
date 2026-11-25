// Every name the game uses. The compiler rejects anything not listed here.
export interface Flappy {
  actions: "flap" | "restart";
  sounds: "flap" | "point";
  images: never;
  tags: "bird" | "solid" | "goal" | "scrolls";
  /** UI documents (assets/ui/*.rml) -> ids of the elements scripts touch. */
  ui: {
    title: never;
    hud: "score";
    over: "score" | "best" | "again";
  };
  scenes: {
    title: undefined;
    play: undefined;
    over: { score: number };
  };
}
