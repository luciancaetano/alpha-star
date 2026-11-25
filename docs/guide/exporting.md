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

The pack includes `THIRD_PARTY_NOTICES.txt` (the engine's dependencies' licenses plus the license
files your fonts name with `{ file, license }`). The font's own license is what matters for a font
(OFL is fine).

Engines live in the SDK under `engine/<platform>-<arch>/` (release: run/test/mcp) and
`engine/<platform>-<arch>/shipping/` (players). Override with `--engine <path>` or `$ALPHA_STAR`.

Android: `alpha export --target android-arm64` writes a Gradle project (and builds the APK when the
toolchain is installed); see [Android](android.html).
