# MCP for agents

`alpha mcp` is a [Model Context Protocol](https://modelcontextprotocol.io) server on stdio. It
lets an AI coding agent check, test and export your game, then **start it and play it**: send
input, step frame by frame, read the state, take screenshots, read the log and hot-reload. The
agent sees what a player sees and can prove a change works before it says "done".

## Set it up

`alpha init` writes this `.mcp.json` in the game folder (Claude Code and most MCP clients read it);
for an older project, add it by hand:

```json
{ "mcpServers": { "alpha": { "command": "npx", "args": ["alpha", "mcp"] } } }
```

Or register it from the command line in Claude Code:

```
claude mcp add alpha -- npx alpha mcp
```

Restart the agent in the game folder. **Expected:** the agent lists 14 tools whose names start
with `alpha_` and `game_`. Pass `--engine <path>` after `mcp` to use a specific engine build.

## A complete session

This is what an agent does to verify "Space makes the player grow for a moment, and moveX moves
it" in the `blank` template.
Each line is one tool call with its arguments, and what comes back:

```
alpha_check   {}                                  -> no problems
alpha_test    {}                                  -> PASS ... / 3 passed, 0 failed
game_start    { "hidden": true }                  -> first state: scene "play", frame 0, entities [...]
game_pause    {}                                  -> paused
game_input    { "kind": "press", "action": "jump" }
game_step     { "frames": 6 }                     -> state after 6 steps (frame 6)
game_screenshot {}                                -> PNG of the frame: the player is enlarged
game_input    { "kind": "axis", "axis": "moveX", "value": 1 }
game_step     { "frames": 30 }                    -> player pos x grew by ~60
game_input    { "kind": "axis", "axis": "moveX" } # no value: release the axis
game_logs     { "since": 0 }                      -> console lines + { next: 12 }
game_stop     {}
```

Then the edit loop: change the code -> `alpha_check` -> `game_reload` -> `game_step` ->
`game_state` / `game_screenshot` -> repeat. Finish with `alpha_test` and add a headless test for
what was just verified ([Testing](testing.html)), so it stays true.

## The tools

### Build and test

| Tool | Arguments | Returns |
|---|---|---|
| `alpha_check` | `entry?` (default `src/main.ts`) | `no problems`, or one `file:line:col message` per problem |
| `alpha_test` | `entry?` (default `test/main.test.ts`) | one PASS/FAIL line per test with the reason; an error result when any test fails |
| `alpha_export` | `entry?`, `out?` (default `dist`), `standalone?` | the pack path and file count, or the problems |

### Run and drive the game

| Tool | Arguments | What it does |
|---|---|---|
| `game_start` | `entry?`, `out?`, `hidden?` | exports and launches the game with the control channel, replacing any game this server started; windowed unless `hidden: true`; runs in real time until paused; returns the first state |
| `game_pause` / `game_resume` | | stop real-time play / resume it at 60 steps per second |
| `game_step` | `frames?` (1..36000, default 1) | pauses if running, advances exactly N fixed steps, returns the state |
| `game_input` | `kind`, `action?`, `axis?`, `value?`, `player?` (1..4) | `press` = down for exactly one step; `hold` = down until `release`; `axis` = hold an axis at -1..1 (omit `value` to release) |
| `game_click` | `target` (`"document#elementId"`) | clicks a UI element as a player would; its handler runs on the next step; fails if the document is hidden |
| `game_reload` | | re-exports and hot-reloads the running game (keeps the scene, its params, JSON `ctx.state` and the save); a type or load error changes nothing and comes back as the error |
| `game_stop` | | closes the game (also happens when the server exits) |

### Look at the game

| Tool | Arguments | Returns |
|---|---|---|
| `game_state` | | `scene`, `paused`, `error` (the script error that paused the game, mapped to your TypeScript source, or `null`), `frame`, `fps`, `entities` `[{ id, tags, pos, rotation, z, velocity }]`, `ui` `{ document: { shown, texts: { elementId: text } } }` |
| `game_screenshot` | | a PNG of the current frame (game + UI, at window size) |
| `game_logs` | `since?` | `console.log/warn/error` lines and script errors since the cursor; pass the returned `next` as `since` next time |

Action, axis and UI names come from the game (`config.actions`, `config.axes`, the UI
documents). An unknown name fails with the list of declared ones.

## Recipes

**Reproduce a bug exactly.** Pause first, then mix `game_input` and `game_step`: the same
sequence gives the same frames every time (fixed step, seeded random).

**Check a menu.** `game_state` lists every shown document's element ids and texts; pick a target
from there and `game_click { "target": "over#again" }`, then `game_step` once.

**Follow a script error.** A script error pauses the game; `game_state.error` holds the message
with the TypeScript file and line. Fix, `game_reload`, `game_step`.

**Watch the log only for what is new.** Keep the `next` cursor from each `game_logs` call.

## Gotchas

- `game_step` pauses a running game first: after it, the game stays paused until `game_resume`.
- A game started by `game_start` does not watch files; reload with `game_reload` when you want
  it, so an export never restarts a scene in the middle of a step sequence.
- `press` is one step long. To keep a button down across steps use `hold`, then `release`.
- Prefer `hidden: true` for automated runs; windowed shows the user what the agent does.

## Security

The control channel listens on 127.0.0.1 only and requires a per-launch token. It exists only in
the dev and release engines (`ALPHA_MCP`); the shipping engine that players get has none of it.
The Android debug APK has no MCP channel either: `alpha mcp` drives desktop engines.

## See also

- [AGENTS.md for games](agents.html): what to tell the agent about your project.
- [Testing](testing.html): the same checks, headless, as `alpha_test` runs them.
- [Hot reload](hot-reload.html): what `game_reload` keeps.
