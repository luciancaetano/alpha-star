# Exporting

`alpha export` turns your project into something that runs without Node, TypeScript or the
SDK: a **pack** (your game's code + assets). With `--standalone` it adds the player engine, so
the folder is a finished game you can zip and give away.

## Ship a game in one command

From the game folder:

```
npx alpha export --standalone --out build/game
```

**Expected:**

```
exported build\game (game.jsbc + 1 asset file(s), engine win32-x64)
```

```
build/game/
  alpha_star.exe            the SDK's shipping engine, byte for byte (alpha_star on Linux)
  game.jsbc                 your game, compiled to QuickJS bytecode and encrypted
  alpha-atlas-0.png         images packed into atlas pages
  alpha-atlas.json
  assets/...                sounds, music, fonts, UI documents (as your game declares them)
  THIRD_PARTY_NOTICES.txt   licenses the pack must carry
```

Double-click `alpha_star.exe` (or run `./alpha_star` on Linux) to play. Zip the whole folder to
distribute it. Nothing is installed; saves go to the player's data folder.

## Step by step

### 1. A plain pack: `alpha export`

```
npx alpha export
```

```
exported dist (main.js + 1 asset file(s))
```

`dist/` holds `main.js` (your TypeScript, type-checked and bundled into one module) plus the
assets. This is what `alpha run` plays with the release engine. It is for development and for
tools, not for players: it has no engine and its code is readable.

A type error stops the export and prints `file:line:col message`; nothing is written.

### 2. Choose where: `--out`

```
npx alpha export --out build/pack
npx alpha export --standalone --out build/game
```

The default is `dist/`.

### 3. A standalone game: `--standalone`

`--standalone` copies the **shipping** engine next to the pack: no dev tools, no MCP, no test
runner or headless flags, no console window, logs only to the player's data folder. The game
ships as `game.jsbc` instead of `main.js` (see [What the bytecode protects](#what-the-bytecode-protects)).

### 4. Another platform: `--target`

```
npx alpha export --standalone --target linux-x64 --out build/game-linux
npx alpha export --standalone --target win32-x64 --out build/game-win
npx alpha export --target android-arm64 --out build/android        # see the Android page
```

Desktop targets need that platform's engine package installed next to the SDK
(`@alpha-star/engine-win32-x64`, `@alpha-star/engine-linux-x64`); npm installs the one for your
machine. Android is its own flow: [Android](android.html).

### 5. Separate image files: `--no-atlas`

By default every image is packed into `alpha-atlas-N.png` pages (fewer textures, faster
drawing). `--no-atlas` keeps them as separate files, which is easier to inspect.

### 6. Readable code for debugging: `--no-bytecode`

```
npx alpha export --standalone --no-bytecode --out build/game-debug
```

Ships `main.js` instead of `game.jsbc`, so errors in the shipping build carry line numbers.
Errors from `game.jsbc` have none. `alpha run`, `alpha test`, hot reload and the MCP tools always
use JS with source maps.

## Recipes

**An npm script per build.** In `package.json`:

```json
"scripts": {
  "export": "alpha export",
  "dist:win": "alpha export --standalone --target win32-x64 --out build/win",
  "dist:linux": "alpha export --standalone --target linux-x64 --out build/linux"
}
```

**Sign the Windows executable.** `--sign` runs **your** signing command on the engine copy in
the pack; `{file}` becomes its quoted path (appended when missing):

```
set ALPHA_SIGN_CMD=signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /a {file}
npx alpha export --standalone --sign --out build/game
```

The SDK's own engine stays unmodified, and the SDK never creates, downloads or stores
certificates or keys. If the command fails, the pack is deleted, so an unsigned build never looks
signed. `--sign` requires `--standalone`.

- **Windows (Authenticode):** `signtool` from the Windows SDK with your code-signing certificate
  (a cloud HSM / Azure Trusted Signing command works the same way). Timestamp it (`/tr`) so the
  signature outlives the certificate. SmartScreen reputation builds up per certificate.
- **macOS (not verified in this release):** `codesign --force --options runtime --timestamp
  --sign "Developer ID Application: ..." {file}`, then notarize the zipped folder or `.app` with
  `xcrun notarytool submit --wait` and `xcrun stapler staple`. Needs an Apple Developer ID.
- **Android:** Gradle signs the APK with your release keystore from the environment; see
  [Android](android.html#build-and-install).

**Verify a pack before release** (in an alpha-star checkout):

```
python tools/check_shipping.py --pack build/game            # no JS source or source map; engine = staged shipping engine
python tools/check_shipping.py --pack build/game --signed   # same, once the Authenticode signature is removed
```

**Use a specific engine.** `--engine <path>` or `$ALPHA_STAR` overrides the engine lookup (the
platform package next to the SDK; in an alpha-star checkout, `alpha-sdk/engine/<platform>-<arch>/`).

## What the bytecode protects

The SDK's release engine compiles the bundle (`alpha_star --compile main.js game.jsbc`) without
source text or debug info, then encrypts it (XChaCha20-Poly1305, Monocypher) with a key of that
engine build, under a fresh random nonce per export. The pack has no `main.js` and no source map.
The shipping engine decrypts it in memory at start and wipes the plain bytecode right after it
loads; it never touches the disk.

- **Version lock.** `game.jsbc` carries a stamp: engine version, QuickJS version and a build id.
  The engine runs only bytecode stamped with its own build; anything else (or a truncated or
  damaged file) stops at start with a message naming both versions, on screen and in the log,
  telling the player to update. Ship the engine and `game.jsbc` from the same export. The export
  refuses to write a pack if the SDK's release and shipping engines are different builds.
- **What it stops:** casual extraction. There is no readable source, comment or source map, and
  `strings` or a bytecode decompiler run on the file gets nothing.
- **What it does not stop:** a determined attacker. The key ships inside the engine (masked),
  so someone who reverse-engineers the executable or dumps memory while the game runs can still
  get the bytecode. **Never put secrets (API keys) in a game.**
- **Trust model: the pack folder is trusted.** Any edited byte, truncated download or file from
  another version fails authentication at start (`game.jsbc failed authentication; re-export with
  this engine`). Whoever can write to the game's folder can replace the engine too, as with any
  native game; the engine only loads bytecode from its own pack (never from saves, downloads or
  the network). Protect the folder (installer permissions, store packages) and sign the
  executable so players can tell your build from a modified one.

## Gotchas

- **Fonts carry licenses.** `THIRD_PARTY_NOTICES.txt` holds the engine's dependency licenses plus
  the license files your fonts name with `{ file, license }`. OFL fonts are fine.
- **Re-export after updating the SDK.** An old `game.jsbc` does not run on a new engine (version
  lock above).
- **No prebuilt engine for macOS or Linux arm64** yet: build one from source and pass `--engine`.
- **Do not edit `dist/` by hand.** Every export rewrites it.

## See also

- [Android](android.html): APKs and Play Store bundles.
- [Testing](testing.html): run the tests before every export.
- [Saves, settings, i18n](saves.html): where a shipped game keeps its data.
