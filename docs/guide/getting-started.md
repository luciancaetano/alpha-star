# Getting started

Make a small 2D game, run it, then change it. You write TypeScript (JavaScript with checks that
catch mistakes); alpha-star opens the window and runs your code. No C++ or AI assistant needed.

**Your path:** install → play → [the one-file tutorial](tutorial-hop.md) → [change your first
game](first-game.md) → [learn the concepts](concepts.md). Keep the API reference for looking
things up.

## 1. Check your tools

You need a code editor, **Node.js 20 or newer** and a terminal. The prebuilt engine runs on
**Windows x64 and Linux x64**.

```sh
node --version
npm --version
```

**Expected:** two version numbers, Node v20 or newer. If a command is not found, install Node.js
(it includes npm) and reopen the terminal.

## 2. Create a project

In the folder where you keep projects:

```sh
npx @alpha-star/sdk init my-game --template blank
cd my-game
npm install
```

Accept if npm asks to install the SDK. **Expected:** a `my-game` folder with `package.json`,
`src`, `assets` and `test`; `npm install` downloads the tools and the engine. Run every following
command inside `my-game`.

## 3. Play it

```sh
npm start
```

**Expected:** a character in the middle of a dark blue window. Click the window, move with the
arrow keys or WASD, press Space to hear a blip and see the character grow for a moment. Close the
window (or press Ctrl+C in the terminal) to stop.

## 4. Check that it works

```sh
npm run check
npm test
```

**Expected:** `check` prints nothing (no type errors) and `test` ends with
`3 passed, 0 failed`: the starter's tests play the game without a window and check the start
position, movement and the Space effect. See [Testing](testing.md).

## 5. Make your first change

In `src/main.ts`, change the title line:

```ts
title: "My First Game",
```

Save and run `npm start`. **Expected:** the new title in the window's title bar.

Now try hot reload: run `npx alpha run --watch`, open `src/tuning.ts`, change `speed: 120` to
`speed: 300` and save. **Expected:** the game restarts in the same window, and the character is
much faster. See [Hot reload](hot-reload.md).

Continue with [Your first game](first-game.md) for complete, commented code.

## Where to edit

| File | What you change there |
|---|---|
| `src/main.ts` | Title, background, controls and assets |
| `src/tuning.ts` | Speeds, game size and timings |
| `src/scenes/play.ts` | What happens when play starts (`enter`) and every step (`update`) |
| `src/prefabs/player.ts` | The data that creates the character |
| `src/types.ts` | Every name the game uses: controls, assets, tags, scenes |
| `src/alpha.ts` | Binds those names to the `game` and `scene` builders; leave it as generated |
| `test/main.test.ts` | Automated checks of game behavior |

## If something goes wrong

| What you see | What to do |
|---|---|
| `node`, `npm` or `npx` not found | Install Node.js, then reopen the terminal. |
| PowerShell says `npm.ps1` cannot run | Use `npm.cmd` and `npx.cmd`, or Command Prompt. |
| Folder exists and is not empty | Pick a new name, such as `my-second-game`. Existing files are kept. |
| npm cannot find `package.json` | Run `cd my-game` first. |
| Engine not found | Check your OS/architecture and run `npm install --include=optional` (`--omit=optional` skips the engine). |
| TypeScript error with a file and line | Fix the first error at that location, save, run `npm run check` again. |
| Changes do not appear | Save the file and restart `npm start`; never edit the generated `dist/`. |

On macOS or Linux arm64 the SDK checks and exports code, but there is no prebuilt desktop engine:
[build one from source](https://github.com/luciancaetano/alpha-star/blob/main/BUILDING.md) and
pass `--engine <path>` or set `ALPHA_STAR`.

## Commands to keep handy

| Goal | Command (inside your game folder) |
|---|---|
| Play | `npm start` |
| Check TypeScript | `npm run check` |
| Run tests | `npm test` |
| Play and reload on save | `npx alpha run --watch` |
| Build for players | `npx alpha export --standalone` ([Exporting](exporting.md)) |
| SDK version | `npx alpha --version` |
| Set up an AI coding assistant | `npx alpha skills` ([AGENTS.md for games](agents.md)) |

Already have a project? `npm install --save-dev @alpha-star/sdk` ([Concepts](concepts.md) lists
the files it needs). Upgrade with `npm install --save-dev @alpha-star/sdk@latest`; for a global
`alpha` command, `npm install --global @alpha-star/sdk`.

**Next:** [Tutorial: a one-file game](tutorial-hop.md), then [Your first game](first-game.md).
Browse bigger games in [Examples](examples.md) or the [showcase](index.html).
