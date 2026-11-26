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
  game.jsbc               your game, compiled to QuickJS bytecode and encrypted
  alpha-atlas-*.png ...   assets as plain files
  assets/...
  THIRD_PARTY_NOTICES.txt
```

Nothing is appended to or embedded in the executable; the layout is the same on every platform.
The SDK's release engine compiles the bundle (`alpha_star --compile main.js game.jsbc`) without
source text or debug info, then encrypts it (XChaCha20-Poly1305, Monocypher) with a key of that
engine build, under a fresh random nonce per export; the pack has no `main.js` and no source map.
The shipping engine decrypts it in memory at start (the plain bytecode is wiped right after it
loads); it never touches the disk. `alpha run`, `alpha test` and hot reload keep plain JavaScript.

**Version lock.** `game.jsbc` carries a stamp: engine version, QuickJS version and a build id.
The engine runs only bytecode stamped with exactly its own build; anything else (or a truncated or
damaged file) stops at start with a message naming both versions, shown on screen and in the
log, telling the player to update the game. Ship the engine and `game.jsbc` from the same export.
The export refuses to write a pack if the SDK's release and shipping engines are different builds
(restage both).

**What this protects, and what it doesn't.** Players get no readable source, comments or source
map, and `game.jsbc` is encrypted: no strings, names or bytecode show up in it, so `strings` or a
bytecode decompiler run on the file gets nothing. This **stops casual extraction, not a determined
attacker**: the key ships inside the engine (only masked, never as one plain value), so someone
who reverse-engineers the executable, or dumps the game's memory while it runs, can still get the
bytecode and decompile it. Do not put secrets (API keys) in a game.

**Threat model: the pack folder is trusted.** The header carries the build stamp, the engine key's
public id and the nonce, all authenticated with the body: a truncated download, a damaged disk, a
file from another version or any edited byte is refused at start (`game.jsbc failed
authentication; re-export with this engine`). Only someone holding the key (extracted from the
engine) could craft a file that QuickJS reads, and QuickJS does not validate bytecode, so such a
file could crash the engine; but anyone who can replace `game.jsbc` can replace the engine
executable next to it too. So the rule is the same as for any
native game: whoever can write to the game's folder can run code as the player, and the engine
only ever loads bytecode from its own pack (never from saves, downloads or the network). Protect
the folder (installer permissions, store packages) and sign the executable (below) so players can
tell your build from a modified one.

## Signing

```
set ALPHA_SIGN_CMD=signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /a {file}
npx alpha export --standalone --sign --out build/game
python tools/check_shipping.py --pack build/game --signed       # in the alpha-star repo
```

`--sign` runs **your** signing command on the engine copy in the pack (`{file}` becomes its
quoted path; appended when missing). The SDK's own engine stays unmodified, and the SDK never
creates, downloads or stores certificates or keys. If the command fails, the pack is deleted, so an
unsigned build never looks signed.

`check_shipping --pack` requires the engine to be byte-identical to the staged shipping engine;
with `--signed` it requires it to be identical once the Authenticode signature is removed
(checksum, security directory and certificate table), so a signed engine that was otherwise
changed still fails.

- **Windows (Authenticode):** `signtool` from the Windows SDK with your code-signing certificate
  (a cloud HSM / Azure Trusted Signing command works the same way). Timestamp it (`/tr`) so the
  signature outlives the certificate. SmartScreen reputation builds up per certificate.
- **macOS (not verified in this release):** `codesign --force --options runtime --timestamp
  --sign "Developer ID Application: ..." {file}`, then notarize the zipped folder or `.app`
  with `xcrun notarytool submit --wait` and `xcrun stapler staple`. Needs an Apple Developer ID.
- **Android:** the APK is signed by Gradle with your release keystore from the environment; see
  [Android](android.html#build-and-install).

**Debugging a standalone build:** `--no-bytecode` ships `main.js` instead (readable stack traces
with line numbers). Error messages from `game.jsbc` have no line numbers. `alpha run`, `alpha test`,
hot reload and the MCP tools always use JS + source maps.

Check a pack before shipping it: `python tools/check_shipping.py --pack build/game` (in the
alpha-star repo) fails on any JS source or source map, and when the engine differs from the staged
shipping one.

The pack includes `THIRD_PARTY_NOTICES.txt` (the engine's dependencies' licenses plus the license
files your fonts name with `{ file, license }`). The font's own license is what matters for a font
(OFL is fine).

Engines come from the platform package npm installed next to the SDK
(`@alpha-star/engine-win32-x64`, `@alpha-star/engine-linux-x64`: release engine for run/test/mcp,
`shipping/` for players); in an alpha-star checkout from `alpha-sdk/engine/<platform>-<arch>/`.
Override with `--engine <path>` or `$ALPHA_STAR`.

Android: `alpha export --target android-arm64` writes a Gradle project (and builds the APK when the
toolchain is installed); see [Android](android.html).
