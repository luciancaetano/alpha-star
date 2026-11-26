// Type test for phase 4: live vectors, Rect, Color, postfx.set, audio.music.*.
import { define, type Rect } from "alpha";

interface G {
  actions: "go"; sounds: never; images: never; music: "theme" | "boss";
  tags: { hero: {} }; ui: {}; scenes: { play: undefined };
}
export const { scene, game } = define<G>();
const play = scene("play", {
  enter(ctx) {
    const e = ctx.spawn({ tags: ["hero"], pos: [0, 0], body: { type: "dynamic", collider: { circle: 4 } }, light: { radius: 40, color: "#ffd9a0" } });
    e.pos.x += 1;
    e.pos = [10, 20];
    e.body.velocity.y = 0;
    e.body.velocity = [1, 2];
    ctx.camera.pos = [5, 5];
    ctx.camera.pos.x += 1;
    const r: Rect = [0, 0, 10, 10];
    ctx.camera.bounds(r);
    ctx.queryRect(r);
    const [, , w] = ctx.camera.view;
    void w;
    ctx.postfx.set("bloom", { strength: 1 });
    ctx.postfx.set("tint", { color: "#ff0000" });
    ctx.audio.music.play("theme", { fadeIn: 1 });
    ctx.audio.music.crossfade("boss", 2);
    if (ctx.audio.music.playing === "boss") ctx.audio.music.stop(1);
    // @ts-expect-error pos takes [x, y], not an object
    e.pos = { x: 1, y: 2 };
    // @ts-expect-error colors are "#rrggbb"
    ctx.postfx.set("tint", { color: "red" });
    // @ts-expect-error postfx.add is gone: set adds or replaces
    ctx.postfx.add("crt");
    // @ts-expect-error music is an object: music.play
    ctx.audio.music("theme");
    // @ts-expect-error unknown music
    ctx.audio.music.play("title");
    // @ts-expect-error a light color is a Color
    ctx.spawn({ tags: ["hero"], pos: [0, 0], light: { radius: 4, color: "warm" } });
  },
});
export default game({ config: { title: "t", size: [10, 10], seed: 1, actions: { go: ["Space"] } }, start: "play", scenes: { play },
  assets: { sounds: {}, music: { theme: "a.ogg", boss: "b.ogg" } } });
