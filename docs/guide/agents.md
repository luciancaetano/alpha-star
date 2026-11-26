# AGENTS.md for game projects

`AGENTS.md` is the file AI coding agents (Claude Code, Codex, Cursor, Copilot, Gemini CLI...)
read first when they open your project. A good one tells the agent, in one screen, how to check
its work and where each decision lives. `alpha init` writes one for you; keep it current as the
game grows.

## A complete AGENTS.md

This is the one `alpha init my-game --template blank` writes. Every template and example starts
from it:

```md
# AGENTS.md - alpha-star game

You are editing a game for the alpha-star engine. Write TypeScript in `src/`; `alpha export`
type-checks it and bundles it into one JS module (`dist/main.js`) that the engine runs. The API
types live in `node_modules/@alpha-star/sdk/types/alpha.d.ts`: read that file when unsure, it is
the source of truth (the SDK README and docs site explain each feature).

## Commands

    npm install                 # once
    npm run check               # type-check only (fast)
    npm test                    # headless simulate() tests (test/main.test.ts)
    npm start                   # export, then play
    npx alpha run --watch       # every save re-exports and the game hot-reloads
    npx alpha run -- --frames 120 --hidden --screenshot s.bmp   # headless screenshot
    npx alpha mcp               # MCP server: start, drive and observe the game as an agent
    npx alpha export --standalone --out build/game              # pack + player engine: runs by itself

(In the real file the commands sit in their own code fence.)

A change is done only when `npm run check` is clean and `npm test` passes.

## Layout (one way to do each thing)

| File | Holds |
|---|---|
| `src/types.ts` | Every name the game uses (actions, axes, sounds, images, tags, UI ids, scenes). Edit first. |
| `src/alpha.ts` | `define<Game>()`: the `scene` and `game` builders bound to those names. |
| `src/tuning.ts` | Numbers that set the feel (speeds, sizes, timings). |
| `src/prefabs/*.ts` | Functions returning `EntitySpec` data (shape, body, emitter, data). |
| `src/scenes/*.ts` | `scene(name, { state, enter, update, exit })`. |
| `src/main.ts` | `game({ config, start, scenes, assets })`. |
| `test/main.test.ts` | Exported functions; each runs `simulate(game, {...})` and checks with `expect`. |
| `assets/` | Images, sounds, fonts, maps, UI (`.rml`/`.rcss`), locales. Declared in `main.ts`. |

## Rules

- Declare a name in `types.ts` before using it; misspelled names are compile errors.
- Randomness only through `ctx.random` (seeded by `config.seed`); never `Math.random`.
- Write the failing test first (`simulate` with `inputs`, then `run.expect...` or `run.entities`).
- A script error pauses the game with a message naming what failed and what exists; fix the cause.
- F12 opens the dev tools while the game runs (not in exported standalone games).
```

## Make it yours, step by step

### 1. Say what the game is

Replace the title and add one line on the game and its controls:

```md
# AGENTS.md - Star Hop (alpha-star game)

A one-screen platformer: collect every star before the timer ends. Arrows/A-D move, Space jumps.
```

### 2. Add how things work here

The generic rules cover the engine. Add the few patterns specific to your game, so the agent
follows them instead of inventing new ones. Lines in the spirit of the examples' `AGENTS.md`:

```md
## How things work here
- Rules are pure functions in `src/rules.ts`; the scene only draws what they return (puzzle).
- The 120 stars move through `ctx.positions("star")`, not one entity at a time (shmup).
- Two players: each hero's `data.player` picks `ctx.input.player(n)` (topdown).
```

### 3. Grow the layout table with the game

When you add `src/levels.ts` or a new scene, add its row. The table is the agent's map: a file
missing from it is a file the agent will not think of.

### 4. Keep the definition of done

Leave the line "A change is done only when `npm run check` is clean and `npm test` passes". It is
what makes an agent run the tests instead of declaring victory.

## Give the agent the engine skills

```
npx alpha skills
```

It lists the agents and asks which ones to set up; press Enter for the defaults, Claude Code
(`.claude/skills`) and the shared `.agents/skills` folder (Codex, Gemini CLI, Cursor, Copilot,
OpenCode, Amp). `--agents claude,cursor` skips the question and `--list` shows what is
available. Rerun it after updating the SDK to update the skills.

Then ask the agent to run the **`alpha-harness`** skill. It copies the SDK's API docs for LLMs into
`docs/llm/` and writes an index of them, plus code-size rules, into your `AGENTS.md`.

## Let the agent play the game

Give the agent the MCP server ([MCP for agents](mcp.html)): it can start the game hidden, send
input, step frames, read the state and take screenshots. Determinism makes its tests exact, and
script errors pause the game with messages written for it to fix.

A request that works well:

```
Add a double jump. Write a failing test in test/main.test.ts first, make it pass,
then verify it in the running game with the alpha MCP tools and show me a screenshot.
```

## Gotchas

- **Stale is worse than missing.** An `AGENTS.md` that names files that no longer exist sends the agent
  the wrong way. Update it in the same change as the code.
- **Keep it short.** One screen. Point to `alpha.d.ts` and these docs for details instead of
  copying them.
- **The types are the truth.** If the doc and `alpha.d.ts` disagree, the agent should trust the
  types; say so in the file (the template does).

## See also

- [MCP for agents](mcp.html): the tools the agent uses to play.
- [Testing](testing.html): what "`npm test` passes" means.
- [Examples](examples.html): six projects, each with its own `AGENTS.md`.
