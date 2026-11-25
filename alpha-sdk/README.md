# @alpha-star/sdk

Types and the `alpha` CLI for alpha-star games (TypeScript, bundled to one JS
module the engine runs). Run every command from the game project folder.

```
alpha check                         type-check only
alpha export [--out dist]           type-check, bundle, copy assets into a pack
alpha export --standalone           ... plus the player engine (runs by itself)
alpha run [-- engine args]          export, then open the pack in the engine
alpha test                          run test/main.test.ts headlessly (simulate + expect)
alpha mcp                           MCP server for AI agents (below)
```

Engines are bundled per platform in `engine/<platform>-<arch>/` (release:
`run`/`test`/`mcp`) and `engine/<platform>-<arch>/shipping/` (players). Override
with `--engine <path>` or `$ALPHA_STAR`.

## Using alpha-star with an AI agent (MCP)

`alpha mcp` is a [Model Context Protocol](https://modelcontextprotocol.io) server
on stdio. It lets an agent build and test the game, then start it and play it:
send input, step frame by frame, read the state, take screenshots, read logs.

Claude Code: add a `.mcp.json` to the game folder (or `claude mcp add alpha -- npx alpha mcp`):

```json
{
  "mcpServers": {
    "alpha": { "command": "npx", "args": ["alpha", "mcp"] }
  }
}
```

Any other MCP client: run `npx alpha mcp` with the game folder as the working
directory (`{"command": "npx", "args": ["alpha", "mcp"], "cwd": "<game folder>"}`).

| Tool | What it does |
|---|---|
| `alpha_check` | Type-check; returns `no problems` or `file:line:col message` lines |
| `alpha_export` | Build the pack (`out`, `standalone`) |
| `alpha_test` | Run the headless tests; PASS/FAIL per test |
| `game_start` | Export + launch the game with the control channel (`hidden` for no window); returns the state |
| `game_state` | Scene, paused, script error (source-mapped), frame, fps, entities (id, tags, pos, rotation, z, velocity), UI documents and their texts |
| `game_step` | Pause and advance exactly N fixed steps (1/60 s), return the state |
| `game_pause` / `game_resume` | Stop / restart real-time play |
| `game_input` | `press` (one step), `hold`, `release` an action, or set an `axis` (-1..1), per player 1..4 |
| `game_click` | Click a UI element: `"document#elementId"` |
| `game_screenshot` | PNG of the current frame |
| `game_logs` | console lines and script errors since a cursor |
| `game_stop` | Close the game (also done when the server exits) |

A typical loop: edit code → `alpha_check` → `alpha_test` → `game_start` →
`game_pause` → `game_input` + `game_step` → `game_state` / `game_screenshot` →
fix → repeat. Input goes through the game's own actions, exactly like a device,
and `game_step` is deterministic (fixed step, seeded random).

### How it works (and why it is safe)

`game_start` launches the release engine as `alpha_star <pack> --control <port>`.
The engine serves a small line-delimited JSON protocol (`alpha-control`, see
`src/mcp/dispatch.hpp` in the engine repo) on **127.0.0.1 only**: connections
from any other address are closed, one client at a time, and every connection
must first present a random token that `alpha mcp` passes to the engine in
`ALPHA_CONTROL_TOKEN` (so other local programs cannot drive the game). Requests
are served on the engine's main thread between frames. The control channel
exists only in dev and release engines; the shipping engine players get
(`alpha export --standalone`) contains none of it.
