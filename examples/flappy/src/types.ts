// Every name the game uses. The compiler rejects anything not listed here.
export interface Flappy {
  actions: "flap" | "restart";
  sounds: "flap" | "point";
  images: never;
  tags: "bird" | "solid" | "goal" | "scrolls";
  scenes: {
    title: undefined;
    play: undefined;
    over: { score: number };
  };
}
