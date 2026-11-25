# AGENTS.md for game projects

Every template and example has an `AGENTS.md` at its root: the file AI coding agents read first.
It should tell an agent, in one screen, how to build and check its work and where each decision
lives. `alpha init` writes one; keep it current as the game grows.

## What to put in it

- **One line on the game** and its controls.
- **Where the truth is:** `node_modules/@alpha-star/sdk/types/alpha.d.ts` (the API) and this site.
- **Commands** and the definition of done: `npm run check` clean and `npm test` passing.
- **Layout:** a table of files and what each holds (`types.ts` first: every name).
- **How things work here:** the few patterns specific to this game (e.g. "rules are pure
  functions in rules.ts", "stars move through ctx.positions").
- **Rules:** `ctx.random` only, write the failing test first, F12 dev tools are not in shipped games.

## Starting point

```
# AGENTS.md — <Game> (alpha-star game)

<One line: what the game is, controls.>

You are editing a game for the alpha-star engine. Write TypeScript in `src/`; `alpha export`
type-checks it and bundles it into one JS module that the engine runs. The API types live in
`node_modules/@alpha-star/sdk/types/alpha.d.ts`: read that file when unsure.

## Commands
npm run check / npm test / npm start / npx alpha run --watch / npx alpha mcp

A change is done only when `npm run check` is clean and `npm test` passes.

## Layout
| File | Holds |
|---|---|
| src/types.ts | Every name. Edit first. |
| src/tuning.ts | Numbers that set the feel. |
| src/prefabs/*.ts | Entity specs (plain data). |
| src/scenes/*.ts | Scenes. |
| test/main.test.ts | simulate() tests. |

## How things work here
- ...
```

## Working with an agent

Give the agent the MCP server (`npx alpha mcp`, see [MCP for agents](mcp.md)) so it can play
what it builds: start the game hidden, step it, read the state, take screenshots. Determinism
makes its tests exact, and script errors pause the game with messages written for it to fix.

Install the SDK's agent skills with `npx alpha skills`. It lists the agents and asks which ones to
set up; press Enter for the defaults, Claude Code (`.claude/skills`) and the shared `.agents/skills`
folder (Codex, Gemini CLI, Cursor, Copilot, OpenCode, Amp). `--agents claude,cursor` skips the
question, `--list` shows what is available. Rerun it after updating the SDK to update the skills.
Then ask the agent to run the `alpha-harness` skill: it copies the API docs for LLMs into
`docs/llm/` and writes an index of them, plus code-size rules, into the game's `AGENTS.md`.
