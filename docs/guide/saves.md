# Saves, settings, i18n

## Saves

```ts
ctx.save.set("best", 12);
ctx.save.get("best", 0);
ctx.save.slot("slot2").set("level", 3);   // named slots, one file each
config: { save: { version: 2, migrate: (data, from) => ({ ...data, gold: data.coins ?? 0 }) } }
```

Values are JSON. Each slot is a file in the player's data folder, written atomically (temp file +
rename, the previous one kept as `.bak`; a damaged file falls back to it).

## Settings

```ts
ctx.settings.uiScale = 1.5;          // RmlUi dp ratio
ctx.settings.highContrast = true;    // .high-contrast on every UI body
ctx.settings.reducedMotion = true;   // camera shake and particles to 1/4; games may snap tweens
ctx.audio.bus("music").volume = 0.5; ctx.window.fullscreen = true; ctx.input.rebind(...); ctx.locale = "pt";
ctx.settings.reset();
```

All persisted in `settings.json`, apart from saves, and applied at boot.

## Localization

```ts
// types.ts
locales: "en" | "pt";
// main.ts
assets: { locales: { en: "assets/locales/en.json", pt: "assets/locales/pt.json" } }, config: { locale: "en" }
```

```json
{ "hud": { "moves": { "zero": "NO MOVES", "one": "1 MOVE", "other": "{count} MOVES" } } }
```

```ts
ctx.t("hud.moves", { count: 3 });   // "3 MOVES"; plural form picked by count
ctx.locale = "pt";                  // re-translates every data-t element, persisted
```

```xml
<p data-t="hud.keys">ARROWS  Z UNDO</p>
```

## In tests

```ts
const run = simulate(game, { seconds: 1, saves: { main: { level: 1 } }, settings: { locale: "pt", reducedMotion: true } });
run.saves.main.best; run.locale; run.settings.volumes;
```

Each `simulate` starts from empty in-memory saves and settings unless seeded. See `examples/puzzle`.
