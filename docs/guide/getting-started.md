# Getting started

Explore the [alpha-star showcase](index.html) for real screenshots of the five example games.

alpha-star is a small 2D engine for games written by AI agents and the people working with them.
You write TypeScript against one typed API; the engine (C++, SDL3, Box2D, QuickJS) runs it with a
fixed step and a seeded random, so every run with the same inputs is the same run, and headless
tests are never flaky.

## Make a game

```
npx @alpha-star/sdk init my-game --template platformer
cd my-game
npm install
npm test          # headless tests (simulate + expect)
npm start         # export the pack and play
```

Templates: `blank` (one scene, one sprite), `platformer`, `topdown`, `puzzle`, `shmup`. Each is a
complete small game with tests, generated placeholder art (CC0) and an `AGENTS.md` that tells an
AI agent how the project works.

## The project

| File | Holds |
|---|---|
| `src/types.ts` | Every name the game uses: actions, axes, sounds, images, tags, UI ids, scenes |
| `src/alpha.ts` | `export const { scene, game } = define<MyGame>()` |
| `src/tuning.ts` | The numbers that set the feel |
| `src/prefabs/*.ts` | Functions returning entity specs (plain data) |
| `src/scenes/*.ts` | `scene("play", { state, enter, update, exit })` |
| `src/main.ts` | `game({ config, start, scenes, assets })` |
| `test/main.test.ts` | Exported test functions run by `alpha test` |

A misspelled name anywhere (an action, a tag, a sound, a scene) is a compile error: the names in
`types.ts` narrow the whole API.

## Commands

```
alpha check                      type-check only
alpha test                       run the tests headlessly
alpha run [--watch]              export and play; --watch hot-reloads on every save
alpha export [--standalone]      build the pack (plus the player engine with --standalone)
alpha mcp                        MCP server: an agent builds, runs and plays the game
alpha init <dir> --template t    new project
alpha --version
```

Next: [Concepts](concepts.md), then the guides. The [API reference](reference.md) is generated
from the same type file the compiler uses.
