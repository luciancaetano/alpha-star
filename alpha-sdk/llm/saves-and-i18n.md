# Saves, Settings and i18n

> summary: Persistent save slots with versioning/migration, player settings (volumes, locale, rebinds, accessibility flags) and localization with ctx.t, plurals and data-t; read when persisting progress, adding options menus or translating text.

## Saves API

```ts
ctx.save.get(key, fallback); ctx.save.set(key, value);   // JSON-safe values (SaveValue), copy stored
ctx.save.has(key); ctx.save.remove(key); ctx.save.clear(); ctx.save.keys();
ctx.save.slot("slot2").set("level", 3);   // named slots: 1..32 letters, digits, '-', '_'; slot("main") = ctx.save
config: { save: { version: 2, migrate: (data, fromVersion) => ({ ...data, gold: data.coins ?? 0 }) } }
```
Each slot is a file in the player's data folder (`save.json`, `save-<slot>.json`), written atomically (temp + rename, previous kept as `.bak`; a damaged file falls back to its `.bak` with a warning). Files before versioning are version 1; `migrate` runs once per older slot when read.

## Settings API

```ts
ctx.settings.uiScale = 1.5;          // 0.5..3, RmlUi dp ratio
ctx.settings.highContrast = true;    // class "high-contrast" on every UI body
ctx.settings.reducedMotion = true;   // shake and particle counts to 1/4
ctx.settings.volumes; ctx.settings.locale; ctx.settings.fullscreen; ctx.settings.bindings;  // read-only
ctx.settings.reset();                // volumes 1, config.locale, config.actions, flags off
// set where they live (persisted too):
ctx.audio.bus("music").volume = 0.5; ctx.window.fullscreen = true; ctx.input.rebind("jump", ["W", "GamepadA"]); ctx.locale = "pt";
```
Stored in `settings.json`, apart from saves, applied at boot; a value the game can no longer use is skipped with a warning.

## Localization API

```ts
// types.ts: locales: "en" | "pt";
assets: { locales: { en: "assets/locales/en.json", pt: "assets/locales/pt.json" } },
config: { locale: "en" },            // default + fallback (default: first of assets.locales)
ctx.t(key, params?): string          // nested keys with dots; {name} placeholders
ctx.locale = "pt";                   // re-translates data-t elements, persisted
```
```json
{ "hud": { "level": "LEVEL {n}/{of}", "moves": { "zero": "NO MOVES", "one": "1 MOVE", "other": "{count} MOVES" } } }
```
```xml
<p data-t="hud.keys">ARROWS  Z UNDO</p>
```

## Example (examples/puzzle)

```ts
// start.ts: resume at the saved level
ctx.go("play", { level: Math.min(ctx.save.get("level", 0), LEVELS.length - 1) });
// solved.ts: keep the best score per level
const best = ctx.save.get(key, 0);
if (!best || moves < best) ctx.save.set(key, moves);
ctx.save.set("level", next);
// play.ts
ctx.ui.set("hud", "moves", ctx.t("hud.moves", { count: ctx.state.moves }));
if (ctx.input.pressed("language")) ctx.locale = ctx.locale === "en" ? "pt" : "en";
```

## Rules and gotchas

- Only `SaveValue` (number, string, boolean, null, arrays, plain objects). Every `set` writes the file.
- Plural entries `{ zero?, one, other }` are picked by `params.count`.
- Missing key: shows `"[key]"` (logged once in dev builds); falls back to `config.locale`'s text first.
- Locale texts are UTF-8, any script: give the font `fallbacks` for scripts it lacks (text-and-fonts.md); right-to-left languages order themselves.
- `ctx.locale` is null without `assets.locales`.
- Save in `ctx.onPause` if progress must survive the OS killing a backgrounded game.
- Bump `config.save.version` when the data shape changes. Keep data-shape rules in the game, not in engine code.
- Tests start with empty in-memory saves/settings unless seeded: `simulate(game, { seconds: 1, saves: { main: { level: 1 } }, settings: { locale: "pt", reducedMotion: true } })`; read `run.saves.main`, `run.locale`, `run.settings`.
