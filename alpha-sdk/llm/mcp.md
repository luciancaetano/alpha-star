# MCP

> summary: The alpha mcp server's tools for agents (check, test, export, start, step, input, state, screenshot, logs, reload) and the play-test loop; read when driving a running game as an AI agent.

## Setup

```json
{ "mcpServers": { "alpha": { "command": "npx", "args": ["alpha", "mcp"] } } }
```
`alpha init` writes it to `.mcp.json` in the game folder; older projects add it by hand (or `claude mcp add alpha -- npx alpha mcp`). Other clients: run `npx alpha mcp` with the game folder as cwd.

## Tools

| Tool | What it does |
|---|---|
| `alpha_check` | Type-check: `no problems` or `file:line:col message` lines |
| `alpha_test` | Headless tests: PASS/FAIL per test |
| `alpha_export` | Build the pack (`out`, `standalone`) |
| `game_start` | Export + launch with the control channel (`hidden` = no window); returns the state |
| `game_state` | Scene, paused, script error (source-mapped), frame, fps, entities (id, tags, pos, rotation, z, velocity, layer, alpha, text, particles, data), joints, UI docs and texts, locale, music, input.touchSticks (value, finger), net (alpha:net sockets: id, kind, endpoint, state, error, stats) |
| `game_step` | Pause and advance exactly N fixed steps (1/60 s), return the state |
| `game_pause` / `game_resume` | Stop / restart real-time play |
| `game_input` | `press` (one step), `hold`, `release` an action, or set an `axis` (-1..1), per player 1..4 |
| `game_click` | Click a UI element: `"document#elementId"` |
| `game_screenshot` | PNG of the current frame (includes lighting/postfx) |
| `game_logs` | console lines and script errors since a cursor |
| `game_reload` | Re-export and hot-reload (keeps scene, JSON params, JSON `ctx.state`, save) |
| `game_stop` | Close the game (also when the server exits) |

## Loop

edit -> `alpha_check` -> `alpha_test` -> `game_start` (hidden) -> `game_pause` -> `game_input` + `game_step` -> `game_state` / `game_screenshot` -> fix -> `game_reload` -> repeat.

Example: verify a jump
```
game_start { hidden: true }
game_pause
game_input { kind: "press", action: "jump" }    // kind: press | hold | release | axis (axis + value)
game_step { frames: 20 }                         // 1..36000, default 1; then read pos in the state
```
Input takes effect on the next step. `game_start` is windowed unless `hidden: true`.

## Rules and gotchas

- Input goes through the game's own actions, like a device; steps are deterministic (fixed step, seeded random).
- Under `--control` the engine does not watch files: reload only via `game_reload`, so a scene never restarts mid-`game_step` sequence.
- A failing reload (type/load error) changes nothing and returns the error; the old game keeps running.
- `no game is running: call game_start first` -> call `game_start`.
- Script errors pause the game; read them in `game_state` or `game_logs`.
- Entity `data` is visible in `game_state`: put debug-relevant game values there.
- The channel listens on 127.0.0.1 with a per-launch token; it exists only in dev/release engines, never in shipped games.
