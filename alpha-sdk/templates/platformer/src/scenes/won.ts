import { scene } from "../alpha";

export default scene("won", {
  enter(ctx) {
    ctx.ui.show("won");
  },
  update(ctx) {
    if (ctx.input.pressed("restart") || ctx.input.pressed("jump")) ctx.go("play");
  },
});
