# Exporting

```
npx alpha export                                  # dist/: main.js + assets (the pack)
npx alpha export --standalone --out build/game    # + the shipping engine: runs by itself
npx alpha export --standalone --target linux-x64  # another platform the SDK has an engine for
npx alpha export --no-atlas                       # keep images as separate files
```

A standalone export copies the **shipping** engine next to the pack: no dev tools, no MCP, no
test runner or headless flags, no console window, logs only to the player's data folder. Run the
executable in the folder to play.

## Standalone layout: bytecode next to an unmodified engine

```
build/game/
  alpha_star.exe          the SDK's shipping engine, byte for byte (signable, same for every game)
  game.jsbc               your game, compiled to QuickJS bytecode
  alpha-atlas-*.png ...   assets as plain files
  assets/...
  THIRD_PARTY_NOTICES.txt
```

Nothing is appended to or embedded in the executable; the layout is the same on every platform.
The SDK's release engine compiles the bundle (`alpha_star --compile main.js game.jsbc`) without
source text or debug info, so the pack has no `main.js` and no source map.

**Version lock.** `game.jsbc` carries a stamp: engine version, QuickJS version and a build id.
The engine runs only bytecode stamped with exactly its own build; anything else (or a truncated or
damaged file) stops at start with a message naming both versions, shown on screen and in the
log, telling the player to update the game. Ship the engine and `game.jsbc` from the same export.
The export refuses to write a pack if the SDK's release and shipping engines are different builds
(restage both).

**What this protects, and what it doesn't.** Players no longer get readable source, comments or
your source map. Bytecode is **not encryption**: property names, strings and the program's
structure are still in the file, and a determined person can decompile it. Do not put secrets
(API keys) in a game.

**Debugging a standalone build:** `--no-bytecode` ships `main.js` instead (readable stack traces
with line numbers). Error messages from `game.jsbc` have no line numbers. `alpha run`, `alpha test`,
hot reload and the MCP tools always use JS + source maps.

Check a pack before shipping it: `python tools/check_shipping.py --pack build/game` (in the
alpha-star repo) fails on any JS source or source map, and when the engine differs from the staged
shipping one.

The pack includes `THIRD_PARTY_NOTICES.txt` (the engine's dependencies' licenses plus the license
files your fonts name with `{ file, license }`). The font's own license is what matters for a font
(OFL is fine).

Engines live in the SDK under `engine/<platform>-<arch>/` (release: run/test/mcp) and
`engine/<platform>-<arch>/shipping/` (players). Override with `--engine <path>` or `$ALPHA_STAR`.

Android: `alpha export --target android-arm64` writes a Gradle project (and builds the APK when the
toolchain is installed); see [Android](android.html).
