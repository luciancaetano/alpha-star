# Android

The same game runs on Android phones. `alpha export --target android-arm64` writes an Android
Studio / Gradle project and, with the Android toolchain installed, builds the APK. The engine
comes prebuilt with the SDK: no NDK, CMake or C++ needed.

## From game to phone

**1.** Name the app in `src/main.ts` (every `android` field is optional):

```ts
config: {
  title: "Flappy Star",          // the app name on the phone
  size: [360, 640],              // taller than wide: portrait; otherwise landscape
  android: { package: "com.studio.flappy", icon: "art/icon.png", versionCode: 3, version: "1.2" },
  // ...actions, axes as before
},
```

Defaults: package `org.alphastar.<title letters>`, SDL's icon, `versionCode` 1, `version` "1.0".
The screen orientation follows `size`.

**2.** Plug in the phone by USB with USB debugging on (Developer options), accept the prompt on
the phone, and run:

```
npx alpha run --target android-arm64 --out android
```

**3. Expected:** the project is written to `android/`, the debug APK is built, installed with
   `adb install -r` and started, and the terminal follows the phone's log (the engine's log and
   your `console.log`) until Ctrl+C. The game runs fullscreen.

Without the toolchain, step 2 writes the project and prints what is missing: see
[Install the toolchain](#install-the-toolchain).

## Step by step

### Make the controls work with touch

A finger is also the mouse, and the screen halves are buttons. Bind touch next to the keys:

```ts
actions: {
  flap: ["Space", "MouseLeft", "GamepadA"],          // a tap is MouseLeft (first finger)
  left: ["Left", "TouchLeft"],                       // a finger on the left half
  right: ["Right", "TouchRight"],                    // ...the right half
  back: ["Escape", "Back"],                          // the Android back button
},
```

- `"Touch"` is held while any finger is down; `"TouchLeft"` / `"TouchRight"` while one is on that
  half. A tap shorter than one step still presses. `ctx.input.mouse()` reads the first finger.
- `"Back"` does **not** close the app: quit or open a menu from the game.
- On-screen sticks and buttons: `config.touchSticks` / `config.touchButtons` add sources such as
  `"TouchMoveX"` and `"TouchJump"` next to the keys (see [Input](input.html#touch-sticks-and-buttons-phones)).

### Show the right hints

`ctx.platform` is `{ os, mobile }` (`os`: `"windows"`, `"macos"`, `"linux"`, `"android"`,
`"ios"`, `"web"`, `"other"`; `mobile` is true on Android and iOS). Use it for hints and layout,
never for rules, so the simulation is the same everywhere. From `examples/flappy`:

```ts
if (ctx.platform.mobile) ctx.ui.set("title", "hint", ctx.t("title.hintTouch"));   // "tap to flap"
```

Preview the phone build on the desktop, and lock it in with a test:

```
npx alpha run -- --platform android
```

```ts
simulate(game, { seconds: 0.1, platform: "android" }).expect.text("title", "hint").equals("tap to flap");
```

### Fit any screen: aspect and the safe area

The game draws edge to edge, into the display cutout, with the system bars hidden (sticky
immersive). `config.window.aspect` decides what a phone's different shape shows: the default
`expand` (and `keep_width` for portrait / `keep_height` for landscape games) reveals more world
instead of bars, so size backgrounds and spawns to `ctx.camera.view`; only `keep` letterboxes.

`ctx.safeArea` is `[x, y, width, height]` in game pixels: the part no cutout or rounded corner
hides. Keep HUD text and touch buttons inside it. In UI documents, put `class="safe"` on the HUD
root and the engine pads it.

```ts
const [x, y, w, h] = ctx.safeArea;   // the whole view on desktop
```

Test a tall phone headlessly (from `examples/shmup`):

```ts
const run = simulate(game, { seconds: 0.1, size: [1080, 2340], platform: "android" });
```

### Survive the app lifecycle

Switching apps pauses the game: `ctx.onPause` runs, no steps run and audio stops until
`ctx.onResume`. Saves and settings are written when set (atomically), so nothing is lost if
Android kills the app in the background. Saves live in the app's private storage. Test it with
`appPauses` ([Testing](testing.html)).

## Variants: debug, release, store

```
npx alpha export --target android-arm64 --out android --variant debug     # APK with dev tools, for your phone
npx alpha export --target android-arm64 --out android --variant release   # APK, shipping engine (side-load, other stores)
npx alpha export --target android-arm64 --out android --variant bundle    # AAB for Google Play, shipping engine
npx alpha export --target android-arm64 --out android --no-build          # the project only
npx alpha run --target android-arm64 --out android --device SERIAL        # pick a phone from `adb devices`
```

`alpha run` always uses the debug variant (an AAB cannot be installed). The native libraries are
16 KB-aligned, as Google Play requires for apps targeting Android 15+.

`examples/flappy/package.json` wires these as npm scripts you can copy:

```json
"android:project": "alpha export --target android-arm64 --out android --no-build",
"android:run": "alpha run --target android-arm64 --out android",
"android:debug": "alpha export --target android-arm64 --out android --variant debug",
"android:release": "alpha export --target android-arm64 --out android --variant release",
"android:bundle": "alpha export --target android-arm64 --out android --variant bundle",
"start:mobile": "alpha run -- --platform android"
```

## Install the toolchain

1. A JDK 17 or newer (`JAVA_HOME`).
2. The Android SDK command-line tools, unpacked to `<sdk>/cmdline-tools/latest`
   (`%LOCALAPPDATA%\Android\Sdk` on Windows, or set `ANDROID_HOME`).
3. The packages the project pins:

```
sdkmanager "platforms;android-35" "build-tools;36.0.0"
sdkmanager "ndk;27.2.12479018" "cmake;3.31.6"     # only to build the engine from source (--engine-src)
```

Newer command-line tools (the `android sdk` CLI) name packages with `/` instead of `;`
(`"ndk/27.2.12479018"`); with `;` they answer `Package ... not found`. Gradle 8.12 comes with the
project's wrapper and downloads itself on the first build.

## Build and install

To build by hand from the exported project:

```
cd android
gradlew.bat assembleRelease          # ./gradlew on Linux/macOS
adb install -r app/build/outputs/apk/release/app-release.apk
adb logcat -s SDL SDL/APP            # the engine's log and console.log
```

Without a release key the APK is signed with the debug key so it installs at once. To sign with
your own key, point the build at your keystore through the environment (nothing is written into
the project, and the SDK never creates or stores keys):

```
keytool -genkeypair -v -keystore my-release.jks -keyalg RSA -keysize 4096 -validity 10000 -alias game
set ALPHA_ANDROID_KEYSTORE=C:\keys\my-release.jks      # export ... on Linux/macOS
set ALPHA_ANDROID_KEYSTORE_PASSWORD=...
set ALPHA_ANDROID_KEY_ALIAS=game
set ALPHA_ANDROID_KEY_PASSWORD=...
gradlew.bat assembleRelease
```

Keep the keystore and its passwords out of the repository; losing it means you cannot update the
app on Google Play (use Play App Signing).

## Keep the project in your repository

The exported project can be committed, like React Native's `android/` folder: export into the
game (`--out android`) and ignore what each build regenerates. `examples/flappy/android` is one:

```
# android/.gitignore
.gradle/
build/
local.properties
app/.cxx/
app/src/main/assets/
app/src/debug/jniLibs/
app/src/release/jniLibs/
```

Exporting again writes the SDK template over the project (your edits to template files are
replaced; Gradle's caches stay) and replaces the game pack in `app/src/main/assets` whole.

## Dev tools on the phone

The debug variant carries the engine dev tools. Open and close them the way React Native opens its
dev menu: **shake the phone** firmly side to side, or send the Menu key with
`adb shell input keyevent 82` (Ctrl+M in the emulator). A gamepad's Back+Start works too. Release
APKs and AABs have no dev tools at all, and the debug APK has no MCP channel (`alpha mcp` drives
desktop engines).

## What is inside the project

- The game pack (`game.jsbc` + assets) in `app/src/main/assets`; the engine reads it from the APK.
  `game.jsbc` is compiled by the SDK's release engine on your machine (the bytecode is the same on
  every little-endian 64-bit platform) and encrypted with the key every engine of one SDK version
  shares. `--no-bytecode` ships `main.js` instead.
- SDL's Java glue (`SDLActivity`, version-checked against the engine at start).
- The engine for `arm64-v8a`, prebuilt from `@alpha-star/engine-android-arm64` (an optional
  dependency of the SDK, installed on every OS): the dev-tools flavor in `app/src/debug/jniLibs`,
  the shipping flavor in `app/src/release/jniLibs`. To build it from source instead (engine
  work), pass `--engine-src <alpha-star checkout>` or set `ALPHA_ENGINE_SRC`; Gradle then needs
  the NDK and CMake above.

## Gotchas

- **Keep the phone unlocked** while `alpha run` starts the game: a locked phone gives no window
  (`Could not fetch native window`).
- **Several devices:** pass `--device` with a serial from `adb devices`.
- **`arm64-v8a` only** (every current phone); there is no x86 emulator image.
- **Rules must not depend on the platform**; only hints and layout should.
- If `npm install` ran with `--omit=optional`, the prebuilt Android engine is missing: reinstall
  with optional dependencies.

## See also

- [Input](input.html): actions, touch and gamepads.
- [Game UI](ui.html): the `safe` class and scaling.
- [Exporting](exporting.html): desktop builds and what the bytecode protects.
- [Testing](testing.html): `platform`, `size` and `appPauses`.
