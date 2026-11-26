# AGENTS.md — Crates (alpha-star game)

Sokoban: push every crate onto a goal. Arrows / D-pad move, Z undoes, R restarts the level,
L cycles the language: English, Portuguese, Russian, Arabic, Hebrew, Hindi, Chinese (Noto Sans
subsets in `assets/fonts/`, `main` falling back to one font per script). Progress and best move counts are saved.

You are editing a game for the alpha-star engine. Write TypeScript in `src/`; `alpha export`
type-checks it and bundles it into one JS module (`dist/main.js`) that the engine runs. The API
types live in `node_modules/@alpha-star/sdk/types/alpha.d.ts`: read that file when unsure, it
is the source of truth.

## Commands

```
npm install                 # once
npm run check               # type-check only (fast)
npm test                    # rules tests + headless simulate() tests (test/main.test.ts)
npm start                   # export, then play
npx alpha run --watch       # every save re-exports and the game hot-reloads
npx alpha mcp               # MCP server: start, drive and observe the game as an agent
node tools/make-assets.mjs  # regenerate the tile sheet (committed; CC0, made here)
```

A change is done only when `npm run export` prints `exported dist (...)` and `npm test` passes.

## Layout

| File | Holds |
|---|---|
| `src/rules.ts` | The rules as pure functions: `parse`, `move`, `solved`. No engine calls. |
| `src/levels.ts` | Levels as text rows (`#` wall, `.` goal, `$` crate, `*` crate on goal, `@` player). |
| `src/types.ts` | Every name: actions, sounds, tags, the tile sheet's frames, locales, UI ids, scenes. |
| `src/prefabs/tiles.ts` | One sheet sprite per cell; `toPixels` centers the level. |
| `src/scenes/start.ts` | Continues at the saved level. |
| `src/scenes/play.ts` | Input -> `move` -> state; sprites follow the state (tweened). Undo history. |
| `src/scenes/solved.ts` | Saves the best move count and the next level; "next" button. |
| `assets/locales/*.json` | Texts: `ctx.t("hud.moves", { count })` (plural forms), `data-t` in `.rml`. |
| `assets/fonts/` | Noto Sans subsets (OFL). A new language: add its locale, then cut its script's Noto font to the locale's characters with the engine repo's `tools/subset_fonts.py` and list it in `assets.fonts.main.fallbacks`. |

## How things work here

- **Rules first:** game logic lives in pure functions on JSON data; the scene draws what they
  return. Test rules directly in `test/main.test.ts` (plain function calls), the game with
  `simulate`. New level: add rows to `LEVELS`; the "boxes = goals" test checks it.
- **State:** `ctx.state` holds the position and undo history as plain JSON, so hot reload keeps
  them. Scene params carry the level: `ctx.go("play", { level })`.
- **Saves:** `ctx.save.get("level", 0)` / `set`; tests seed them with `saves: { main: {...} }`
  and read `run.saves`.
- **Text:** `ctx.locale = "pt"` re-translates every `data-t` element and is persisted;
  `ctx.t(key, params)` for texts set from scripts.
- **Accessibility:** `ctx.settings.reducedMotion` makes moves snap instead of glide;
  `.high-contrast` styles in `crates.rcss` apply when `ctx.settings.highContrast` is on.

F12 opens the dev tools while the game runs (not in exported standalone games).
