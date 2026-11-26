# Getting started

Make a small 2D game, run it, then change how it plays. You write TypeScript (JavaScript with checks that catch mistakes); alpha-star opens the game window and runs your code. You do not need C++ or an AI assistant to follow this guide.

**Your path:** install → play → [change your first game](first-game.md) → [learn the concepts](concepts.md). Leave the API reference for when you need to look something up.

## 1. Check your tools

You need a code editor, **Node.js 20 or newer**, and a terminal (the window where you type commands). This guide uses the prebuilt engine for **Windows x64 or Linux x64**.

Open a terminal and run these commands one at a time:

```sh
node --version
npm --version
```

**Expected:** two version numbers. Node must be v20 or newer. If a command is not found, install Node.js with npm, then reopen the terminal.

`npm` installs project tools. `npx` runs a tool without a global installation. The SDK is the package containing alpha-star's commands and TypeScript definitions; npm also downloads the engine packages.

## 2. Create a project

In the folder where you keep projects, run:

```sh
npx @alpha-star/sdk init my-game --template blank
```

If npm asks to install the SDK, accept. This creates a folder called `my-game`. The `blank` template already has a movable character, a sound and tests.

Enter that folder and install its dependencies:

```sh
cd my-game
npm install
```

**Expected:** `package.json`, `src`, `assets` and `test` inside `my-game`. Run all following commands in that folder. Keep optional dependencies enabled: `--omit=optional` or `--no-optional` skips the engine.

## 3. Open your game

```sh
npm start
```

**Expected:** a character in the middle of a dark blue window. Click the window to focus it. Move with arrow keys or WASD. Press Space to hear a blip and briefly enlarge the character. The action called `jump` here is a visual effect, not a physics jump.

Close the game window to return to the terminal. If a command is still running, press Ctrl+C in the terminal.

## 4. Check that it works

```sh
npm run check
npm test
```

The first command checks TypeScript for mistakes. The second runs the starter's tests: programs that check starting position, movement and the Space effect without opening a game window. Both should finish without errors.

## 5. Make your first change

Open `my-game` in your editor. In `src/main.ts`, find:

```ts
title: "My Game",
```

Replace that line with:

```ts
title: "My First Game",
```

Save and run `npm start` again. **Expected:** your new title in the window's title bar.

Continue with [Your first game](first-game.md) to change speed, understand movement, add feedback and test the result using complete, commented code.

## Where to edit

Start with the first two rows; you do not need to understand every file today.

| File | What you change there |
|---|---|
| `src/main.ts` | Title, background, controls and assets |
| `src/tuning.ts` | Speed, game size and animation duration |
| `src/scenes/play.ts` | What happens when play starts and while it runs |
| `src/prefabs/player.ts` | Data used to create the character |
| `src/types.ts` | Allowed names for controls, assets, tags and scenes |
| `src/alpha.ts` | Connects those names to the game and scene builders; leave it as generated for now |
| `test/main.test.ts` | Automated checks for game behavior |

## If something goes wrong

| What you see | What to do next |
|---|---|
| `node`, `npm` or `npx` not found | Install Node.js with npm, then reopen the terminal. |
| PowerShell says `npm.ps1` cannot run | Use `npm.cmd` and `npx.cmd`, or use Command Prompt. |
| Folder exists and is not empty | Pick a new name, such as `my-second-game`. Existing files are kept. |
| npm cannot find `package.json` | Run `cd my-game` before project commands. |
| Engine not found | Check your OS/architecture and reinstall with `npm install --include=optional`. |
| TypeScript error with a file and line | Open that location, fix the first error, save and run `npm run check` again. |
| Changes do not appear | Save the source file and restart with `npm start`; do not edit generated files in `dist`. |

On macOS or Linux arm64, the SDK checks and exports code, but there is no prebuilt desktop engine for those platforms. See [building from source](https://github.com/luciancaetano/alpha-star/blob/main/BUILDING.md), then pass `--engine <path>` or set `ALPHA_STAR`.

## Commands to keep handy

| Goal | Command inside your game folder |
|---|---|
| Play | `npm start` |
| Check TypeScript | `npm run check` |
| Run tests | `npm test` |
| Play and reload after saving | `npx alpha run --watch` |
| Prepare a game for players | `npx alpha export --standalone` — see [Exporting](exporting.md) |
| See SDK version | `npx alpha --version` |
| Set up an AI coding assistant | `npx alpha skills` — see [Agent instructions](agents.md) |

Already have a project? Use `npm install --save-dev @alpha-star/sdk`; [Concepts](concepts.md) explains the files it needs. Upgrade with `npm install --save-dev @alpha-star/sdk@latest`. For a global `alpha` command, use `npm install --global @alpha-star/sdk`.

**Next:** [Your first game](first-game.md). Explore larger games later in [Examples](examples.md) or the [showcase](index.html).
