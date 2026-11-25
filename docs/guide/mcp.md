# MCP for agents

`alpha mcp` is a [Model Context Protocol](https://modelcontextprotocol.io) server on stdio. An
agent uses it to check, test and export the game, then start it and play it: send input, step
frame by frame, read the state, take screenshots, read logs, hot-reload.

Claude Code: add `.mcp.json` to the game folder (or `claude mcp add alpha -- npx alpha mcp`):

```json
{ "mcpServers": { "alpha": { "command": "npx", "args": ["alpha", "mcp"] } } }
```

| Tool | What it does |
|---|---|
| `alpha_check` | Type-check; `no problems` or `file:line:col message` lines |
| `alpha_test` | Run the headless tests; PASS/FAIL per test |
| `alpha_export` | Build the pack (`out`, `standalone`) |
| `game_start` | Export and launch with the control channel (`hidden` for no window) |
| `game_state` | Scene, paused, script error, frame, entities (tags, pos, velocity, data...), UI texts |
| `game_step` | Pause and advance exactly N fixed steps |
| `game_pause` / `game_resume` | Stop / restart real-time play |
| `game_input` | `press`, `hold`, `release` an action, or set an `axis`, per player |
| `game_click` | Click a UI element: `"document#elementId"` |
| `game_screenshot` | PNG of the current frame |
| `game_logs` | Console lines and script errors since a cursor |
| `game_reload` | Re-export and hot-reload the running game |
| `game_stop` | Close the game |

A typical loop: edit -> `alpha_check` -> `alpha_test` -> `game_start` -> `game_pause` ->
`game_input` + `game_step` -> `game_state` / `game_screenshot` -> fix -> `game_reload`.
Input goes through the game's own actions, like a device, and steps are deterministic.

The control channel listens on 127.0.0.1 only and requires a per-launch token. It exists only in
dev and release engines; the shipping engine has none of it.
