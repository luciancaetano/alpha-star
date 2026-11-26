# Exporting

> summary: alpha export packs, standalone builds with the shipping engine and game.jsbc bytecode, signing, other targets and Android APKs; read when building something a player will run.

## Commands

```
npx alpha export                                  # dist/: main.js + assets (the pack)
npx alpha export --standalone --out build/game    # + the shipping engine: runs by itself
npx alpha export --standalone --target linux-x64  # another platform the SDK has an engine for
npx alpha export --no-atlas                       # images as separate files
npx alpha export --standalone --no-bytecode       # ship main.js (readable stack traces)
npx alpha export --standalone --sign              # runs $ALPHA_SIGN_CMD ({file} = engine path)
npx alpha export --target android-arm64 --out build/android   # Gradle project (+ APK with toolchain)
```

## Pack

`alpha export` type-checks `src/main.ts`, bundles it into one ES module (`dist/main.js`, inline source map) and copies exactly the declared assets (plus files .rml/.rcss/maps reference). PNGs go into atlas pages. `THIRD_PARTY_NOTICES.txt` gets engine licenses plus each font's `license` file.

## Standalone layout

```
build/game/
  alpha_star.exe          shipping engine, byte for byte (same for every game)
  game.jsbc               the game as encrypted QuickJS bytecode (no source, no source map)
  alpha-atlas-*.png, assets/..., THIRD_PARTY_NOTICES.txt
```

## Android

```ts
config: { title: "Flappy Star", size: [360, 640],   // taller than wide = portrait
  android: { package: "com.studio.flappy", icon: "art/icon.png", versionCode: 3, version: "1.2" } }
```
All `android` fields optional (package defaults to `org.alphastar.<title letters>`). Needs JDK 17+, Android SDK, `sdkmanager "platforms;android-35" "build-tools;36.0.0"`. The engine is prebuilt (`@alpha-star/engine-android-arm64`, optional dependency of the SDK, copied to `app/src/{debug,release}/jniLibs`); `--engine-src <checkout>` / `ALPHA_ENGINE_SRC` builds it from source instead (then also `"ndk;27.2.12479018" "cmake;3.31.6"`). Error "no Android engine": reinstall without `--omit=optional`. (newer sdkmanager: `/` instead of `;`). Variants: `--variant debug` (APK with dev tools: shake the phone or `adb shell input keyevent 82`, as in React Native), `release` (APK, shipping), `bundle` (AAB for Google Play, shipping); `--no-build` = project only. `alpha run --target android-arm64 --out android [--device SERIAL]` builds debug, installs on the USB phone, starts it and follows logcat (phone must be unlocked). `.so` files are 16 KB aligned (Play requirement). The project may be committed (`--out android` in the game; engine path is relative; ignore `.gradle/ build/ local.properties app/.cxx/ app/src/main/assets/ app/src/debug/jniLibs/ app/src/release/jniLibs/`; re-export rewrites template files, keeps Gradle caches, replaces assets). Release keystore via `ALPHA_ANDROID_KEYSTORE`, `ALPHA_ANDROID_KEYSTORE_PASSWORD`, `ALPHA_ANDROID_KEY_ALIAS`, `ALPHA_ANDROID_KEY_PASSWORD` (debug key otherwise). arm64-v8a only. The manifest asks for INTERNET only when the game's sources import `alpha:net`. Touch: `Touch`, `TouchLeft`, `TouchRight`, `Back`, on-screen `config.touchSticks` / `touchButtons` (input.md).

## Rules and gotchas

- Shipping engine has no dev tools, MCP, test runner, headless flags, hot reload or console; logs go to the player's data folder.
- Version lock: `game.jsbc` runs only on the engine build that compiled it; ship both from the same export. A mismatch stops at start naming both versions.
- Export refuses if the SDK's release and shipping engines are different builds ("restage both with tools/stage_engine.py").
- `game.jsbc` is encrypted with a key inside the engine: it stops casual extraction (strings, decompilers on the file), not a determined attacker who reverse-engineers the engine. Never put secrets (API keys) in a game.
- `game.jsbc failed authentication; re-export with this engine`: the file was changed or damaged, or comes from another engine build; re-export.
- `--sign` without `ALPHA_SIGN_CMD` fails; a failed signing deletes the pack. The SDK never stores keys or certificates.
- Errors from `game.jsbc` have no line numbers; use `--no-bytecode` to debug a standalone build.
- In the alpha-star repo: `python tools/check_shipping.py --pack build/game [--signed]` verifies a pack.
- Engines come from `@alpha-star/engine-<platform>-<arch>` (or `alpha-sdk/engine/` in a checkout); override with `--engine <path>` or `$ALPHA_STAR`.
