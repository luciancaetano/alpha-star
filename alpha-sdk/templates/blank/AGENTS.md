# AGENTS.md — alpha-star game

You are editing a game for the alpha-star engine. Write TypeScript in `src/`; `alpha export`
type-checks it and bundles it into one JS module (`dist/main.js`) that the engine runs. The API
types live in `node_modules/@alpha-star/sdk/types/alpha.d.ts`: read that file when unsure, it is
the source of truth (the SDK README and docs site explain each feature).

## Commands

```
npm install                 # once
npm run check               # type-check only (fast)
npm test                    # headless simulate() tests (test/main.test.ts)
npm start                   # export, then play
npx alpha run --watch       # every save re-exports and the game hot-reloads
npx alpha run -- --frames 120 --hidden --screenshot s.bmp   # headless screenshot
npx alpha mcp               # MCP server: start, drive and observe the game as an agent
npx alpha export --standalone --out build/game              # pack + player engine: runs by itself
node tools/make-assets.mjs  # regenerate the placeholder art
```

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
