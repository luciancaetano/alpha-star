# Your first game

Turn the `blank` starter into a movement playground you understand. Learn where to change speed, how input moves a character, and how to check that movement automatically.

**Before you start:** finish [Getting started](getting-started.md) and open `my-game` in your editor. Use the generated `blank` project; keep its other files and assets. No extra artwork is needed.

Follow the steps in order. **Replace the file** means the code block is complete. Other blocks show only the part to edit.

## 1. Change one number and feel the difference

**Replace the file** `src/tuning.ts` with:

```ts
// Our play area, measured in game pixels.
export const VIEW = { width: 320, height: 180 };

// Keep adjustable numbers together so they are easy to find.
export const PLAYER = {
  speed: 180, // Pixels per second. The starter used 120: this is faster.
  hop: 0.15, // Seconds for the growing part of the Space animation.
};
```

Save, run `npm start`, and hold Right. **Expected:** the character moves faster. Close the window when you finish. Try `speed: 60`, then restore `180` for the rest of the tutorial.

## 2. Understand movement

A **scene** is one part of a game, such as play or a menu. This project starts in `play`. Its `enter` function runs when the scene starts; `update` runs repeatedly while you play.

**Replace the file** `src/scenes/play.ts` with this commented version:

```ts
import { vec } from "alpha";
import { scene } from "../alpha";
import { player } from "../prefabs/player";
import { PLAYER, VIEW } from "../tuning";

export default scene("play", {
  enter(ctx) {
    // ctx is the scene's toolbox. spawn creates a game object.
    // player(...) supplies its picture, position and "player" tag.
    ctx.spawn(player([VIEW.width / 2, VIEW.height / 2]));
  },

  update(ctx) {
    // Find the object created in enter, using its tag.
    const p = ctx.find("player");

    // Keyboard axes give -1, 0 or +1; sticks allow values between.
    const horizontal = ctx.input.axis("moveX");
    const vertical = ctx.input.axis("moveY");

    // Normalize so diagonal movement is not faster.
    // No input gives [0, 0], so the character stays still.
    const direction = vec.norm([horizontal, vertical]);

    // Pixels/second times seconds/step gives pixels to move this step.
    const movement = vec.scale(direction, PLAYER.speed * ctx.dt);

    // x increases rightward; y increases downward.
    // Keep the character's center inside our 320 by 180 play area.
    p.pos.x = Math.max(0, Math.min(VIEW.width, p.pos.x + movement[0]));
    p.pos.y = Math.max(0, Math.min(VIEW.height, p.pos.y + movement[1]));

    // pressed is true only when the button first goes down.
    if (ctx.input.pressed("jump")) {
      ctx.audio.play("blip");
      // A tween changes a value over time; yoyo returns to its old value.
      ctx.tween(p, {
        to: { scale: 1.4 },
        seconds: PLAYER.hop,
        ease: "quadOut", // Slow down as the character reaches its big size.
        yoyo: true,
      });
    }
  },
});
```

Run `npm run check`, then `npm start`. **Expected:** arrows and WASD still move the character. Space makes it grow and return to normal with a sound. Holding Space does not repeat the effect; release and press again.

The limit applies to the character's **center**, so part of its image can cross the edge. This lesson keeps the fixed play area even if you resize the window. [Window concepts](concepts.md#later-topics) explain the full visible area.

## 3. Connect a key to an action

In `src/main.ts`, replace the existing `actions` line with:

```ts
// Several physical buttons can trigger the same named action.
actions: { jump: ["Space", "J", "GamepadA"] },
```

Save and restart. **Expected:** J and Space trigger the same sound and animation. The scene reads `jump`, so it does not need to know which key you chose.

`jump` is already listed in `src/types.ts`. To invent an action name later, add it there and in `config.actions`. See [Input](input.md) for more controls.

## 4. Make the feedback your own

In `src/scenes/play.ts`, change `to: { scale: 1.4 }` to `to: { scale: 1.8 }`. In `src/tuning.ts`, change `hop: 0.15` to `hop: 0.25`.

Run `npm start` and press J. **Expected:** the character grows larger and takes longer to return. `scale: 1` is the original size; `1.8` is 180% of that size. `yoyo: true` handles the return trip.

To see changes as you save, close the game and run:

```sh
npx alpha run --watch
```

Save a change to the scale, then press J again. If the game is in an unexpected state, stop with Ctrl+C and restart. See [Hot reload](hot-reload.md) for what is preserved.

## 5. Check movement automatically

A test runs the game with chosen inputs and checks the result. Keep the starter tests. **Append** this function to `test/main.test.ts`; that file already imports `simulate` and `game`.

```ts
export function holdingLeftMovesThePlayer() {
  const run = simulate(game, {
    seconds: 0.5,
    // From simulation frame 0, hold the horizontal axis at -1 (left).
    inputs: { axes: { moveX: [[0, -1]] } },
  });

  // Detect script errors and a missing or duplicated player first.
  run.expect.noErrors();
  run.expect.count("player").equals(1);
  const [p] = run.entities("player");

  // Start x = 160. At 180 pixels/second, half a second moves 90 pixels.
  // Expected x = 70; allow a small rounding difference.
  if (Math.abs(p.pos[0] - 70) > 1) {
    throw new Error(`Expected x near 70 after moving left, got ${p.pos[0]}`);
  }
}
```

Run `npm run check`, then `npm test`. **Expected:** the starter tests and your new test pass.

To see why the test matters, temporarily set `speed` to `120` and run `npm test` again. Your new test should fail: the player ends near x = 100. Restore `180` and run the tests again; they should pass.

If you deliberately choose another speed later, update the expected position using `160 - speed * 0.5`. Tests should describe the behavior you want.

## 6. Try a small challenge

- Change the background color in `src/main.ts`, save and check the window.
- Make the Space effect twice the character's original size. Hint: `scale: 2`.
- Add a test for upward movement. Hint: use `moveY`, start at y = 90, and check `p.pos[1]`.

You now have configurable movement, keyboard/gamepad controls, sound, animated feedback and a movement test. No physics setup is needed because this example changes position directly.

**Next:** [Concepts](concepts.md) explains these pieces. Choose an [example game](examples.md), or follow [Exporting](exporting.md) to share your game.
