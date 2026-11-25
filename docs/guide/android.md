# Android

`alpha export --target android-arm64` turns a game into an Android Studio / Gradle project; with
the Android toolchain installed it also builds the APK.

```
alpha export --target android-arm64 --out build/android
```

The project holds:

- the game pack (`game.jsbc` + `assets/...`) in `app/src/main/assets`: the engine reads it from the
  APK through SDL, the same code that reads a pack folder on desktop. `game.jsbc` is compiled by
  the SDK's release engine for this machine: the bytecode format is the same on every
  little-endian 64-bit platform, and the engine built for the APK from the same source carries the
  same build id (QuickJS source hash + `-64`). `--no-bytecode` ships `main.js` instead;
- SDL 3.2.10's Java glue (`SDLActivity`, version-checked against the engine at start);
- a CMake build of the engine from the alpha-star source, **shipping flavor** (no dev tools, MCP or
  debug features), `arm64-v8a` only. The SDK finds the source in the repo it sits in, or pass
  `--engine-src <alpha-star checkout>` / set `ALPHA_ENGINE_SRC`.

## Game config

```ts
config: {
  title: "Flappy Star",          // the app name
  size: [360, 640],              // taller than wide: portrait; otherwise landscape
  android: { package: "com.studio.flappy", icon: "art/icon.png", versionCode: 3, version: "1.2" },
}
```

All `android` fields are optional: the package id defaults to `org.alphastar.<title letters>`,
the icon to SDL's, `versionCode` to 1 and `version` to "1.0". The screen orientation follows
`size` (both landscape or both portrait directions, by the sensor).

## Install the toolchain

Without it the exporter writes the project and prints what is missing. You need:

1. A JDK 17 or newer (`JAVA_HOME`).
2. The Android SDK command-line tools, unpacked to `<sdk>/cmdline-tools/latest`
   (`%LOCALAPPDATA%\Android\Sdk` on Windows, or set `ANDROID_HOME`).
3. The packages the project pins:

```
sdkmanager "platforms;android-35" "build-tools;36.0.0" "ndk;27.2.12479018" "cmake;3.31.6"
```

Gradle 8.12 comes with the project's wrapper (it downloads itself on the first build). The first
build also fetches the engine's dependencies (SDL, Box2D, QuickJS, RmlUi, FreeType...).

## Build and install

```
cd build/android
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

## Touch, back button, lifecycle

- A finger is also the mouse: `"MouseLeft"` and `ctx.input.mouse()` work unchanged (first finger).
- `"Touch"` is held while any finger is down; `"TouchLeft"` / `"TouchRight"` while one is on that
  half of the screen (two-button games: `left: ["Left", "TouchLeft"]`). A tap shorter than one
  step still presses.
- The back button is the `"Back"` input; it does not close the app. Bind it with Escape:
  `back: ["Escape", "Back"]`, and quit or open a menu from the game.
- Switching apps pauses the game: `ctx.onPause` runs, no steps run and audio stops until
  `ctx.onResume`. Saves and settings are written when set (atomically), so nothing is lost if
  Android kills the app in the background. The engine stops stepping the moment Android
  announces the background, before its event is polled.
- Saves live in the app's private storage (`SDL_GetPrefPath`); the log goes to logcat.

## Limitations

- `arm64-v8a` only (every current phone); no x86 emulator image.
- No prebuilt Android engine in the SDK yet: the build compiles it (several minutes the first time).
- Not tested on a device in this release; report problems with `adb logcat` output.
