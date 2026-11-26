# Hot reload

Keep the game open while you edit it. Every save rebuilds the game and swaps the new code into the
running engine in a moment, without closing the window.

```
npx alpha run --watch
```

## Try it: change the speed while playing

In a project made with `alpha init my-game --template blank`:

**1.** Run `npx alpha run --watch`. The game opens; move the character with the arrow keys.

**2.** Open `src/tuning.ts` and change the speed, then save:

```ts
export const PLAYER = { speed: 300, hop: 0.15 };  // was 120
```

**3. Expected:** within about half a second the scene restarts in the same window and the
character moves more than twice as fast. The terminal stays open, watching for the next save.

## What survives a reload

A reload rebuilds the world: the current scene's `enter` runs again and spawns its entities from
scratch. These are carried over:

| Kept | Condition |
|---|---|
| The current scene and its `enter` params | params are plain JSON (otherwise the game restarts at `start`) |
| `ctx.state` | plain JSON (no functions, class instances, `undefined`, NaN) and the scene still declares `state`; otherwise `state(params)` runs again |
| The save (`ctx.save`) | always |

Entity positions, entity `data`, timers and tweens are **not** kept. So keep values that must
survive in `ctx.state`:

```ts
// src/scenes/play.ts
export default scene("play", {
  // Plain JSON: survives a hot reload.
  state: () => ({ jumps: 0 }),

  enter(ctx) {
    // Runs again after every reload: the world is rebuilt from here.
    ctx.spawn(player([VIEW.width / 2, VIEW.height / 2]));
  },

  update(ctx) {
    if (ctx.input.pressed("jump")) {
      ctx.state.jumps += 1;
      console.log(`jumps: ${ctx.state.jumps}`);
    }
    // ...movement as before
  },
});
```

**Expected:** press Space a few times, edit any file and save. The player is back in the middle
(re-spawned by `enter`), and the next jump logs the count continuing from where it was.

## When the new code is broken

- **A type error:** the terminal prints `file:line:col message`, the pack is **not** rewritten,
  and the game keeps running the last good code. Fix and save again.
- **An error while the new code boots** (in `enter`, for example): the old game keeps running and
  the error is shown.
- **An error at runtime** pauses the game with the message, as always; fix the code and save to
  reload.

## What is watched

The whole project folder, except `dist/`, `build/`, `node_modules/` and `.git/`. A burst of saves
triggers one rebuild, 200 ms after the last change; the engine polls its pack and reloads once it has been quiet for 0.3 s, so one export is one reload. Assets count too: replace an image or a sound
and save, and the reload picks it up. Shader sources (`.frag`) are recompiled the same way (see
[Shaders and lighting](shaders.html)).

`--watch` combines with the other `run` options:

```
npx alpha run --watch --out build/dev          # pack somewhere other than dist/
npx alpha run --watch -- --devtools            # also open the F12 dev tools at start
```

## Recipes

- **Tune feel live:** put every gameplay number in `src/tuning.ts` (the templates do) and keep
  that file open next to the game.
- **Iterate on one scene:** make it the game's `start` while you work on it, so each reload lands
  there.
- **Agents:** an agent drives the engine through MCP and reloads on demand with `game_reload`
  (an agent-driven engine does not watch files). See [MCP for agents](mcp.html).

## Gotchas

- Shipping engines have no hot reload, and neither does a bundled pack (an Android APK or a
  standalone export): only a pack folder named on the command line, as `alpha run` passes, is
  watched.
- A value stored in a module-level variable (`let score = 0` outside the scene) resets on every
  reload. Use `ctx.state`.
- The window size and title are fixed at launch: changing `config.size` or `config.title` needs a restart.
- If `ctx.state` holds something that is not plain JSON (a class instance, a function, an
  entity), it starts fresh after the reload.

## See also

- [Concepts](concepts.html): scenes, `state`, `enter` and `update`.
- [Testing](testing.html): lock in what you tuned with a headless test.
- [MCP for agents](mcp.html): `game_reload` for agents.
