# Building for Android

How to set up the Android toolchain on Windows so that `alpha export --target android-arm64` builds
an APK. The game config side (package id, icon, touch input, signing) is in
[docs/guide/android.md](docs/guide/android.md).

The exporter writes a Gradle project and compiles the engine from source (**shipping** flavor,
`arm64-v8a`), so it needs a JDK, the Android SDK and exactly these packages, pinned by the project
(`alpha-sdk/lib/android.mjs`, `TOOLCHAIN`):

| Package | Version | sdkmanager name |
|---|---|---|
| SDK Platform | Android 15 (API 35) | `platforms;android-35` |
| Build-Tools | 36.0.0 | `build-tools;36.0.0` |
| NDK (Side by side) | 27.2.12479018 | `ndk;27.2.12479018` |
| CMake | 3.31.6 | `cmake;3.31.6` |

Gradle 8.12 is not installed by hand: the project's wrapper (`gradlew.bat`) downloads it.

## 1. Install a JDK (17 or newer)

```powershell
winget install EclipseAdoptium.Temurin.17.JDK
```

Or download it from <https://adoptium.net/temurin/releases/?version=17> (Windows x64, `.msi`; in the
installer enable **Set JAVA_HOME variable**). Android Studio's bundled JBR (JDK 21) also works:
point `JAVA_HOME` at `C:\Program Files\Android\Android Studio\jbr`.

## 2. Install the Android SDK

Pick **one** of the two ways.

### Option A: Android Studio (graphical)

1. Download from <https://developer.android.com/studio> (or `winget install Google.AndroidStudio`)
   and install with the defaults.
2. First launch: choose **Standard** setup. It installs the SDK in `%LOCALAPPDATA%\Android\Sdk`.
3. **More Actions → SDK Manager** (or *Settings → Languages & Frameworks → Android SDK*):
   - **SDK Platforms** tab: check **Android 15.0 ("VanillaIceCream") API 35**.
   - **SDK Tools** tab: check **Show Package Details** (bottom right), then select:
     - **Android SDK Build-Tools** → `36.0.0`
     - **NDK (Side by side)** → `27.2.12479018`
     - **CMake** → `3.31.6`
     - **Android SDK Command-line Tools (latest)**
     - **Android SDK Platform-Tools** (for `adb`)
   - **Apply**, accept the licenses.

### Option B: command-line tools only (no IDE)

1. From <https://developer.android.com/studio#command-tools> download
   **Command line tools only → Windows** (`commandlinetools-win-..._latest.zip`).
2. Unpack it so the path is `<sdk>\cmdline-tools\latest\bin\sdkmanager.bat` (the zip contains a
   `cmdline-tools` folder: rename it to `latest` inside a new `cmdline-tools` folder):

```powershell
$sdk = "$env:LOCALAPPDATA\Android\Sdk"
New-Item -ItemType Directory -Force "$sdk\cmdline-tools" | Out-Null
Expand-Archive "$HOME\Downloads\commandlinetools-win-*_latest.zip" "$sdk\cmdline-tools"
Rename-Item "$sdk\cmdline-tools\cmdline-tools" latest
```

3. Install the pinned packages and accept the licenses:

```powershell
& "$sdk\cmdline-tools\latest\bin\sdkmanager.bat" --licenses
& "$sdk\cmdline-tools\latest\bin\sdkmanager.bat" "platform-tools" "platforms;android-35" "build-tools;36.0.0" "ndk;27.2.12479018" "cmake;3.31.6"
```

## 3. Environment variables

The exporter looks for the SDK in `ANDROID_HOME`, `ANDROID_SDK_ROOT`, then
`%LOCALAPPDATA%\Android\Sdk`, and for the JDK in `JAVA_HOME`. Set them once (new terminals pick
them up):

```powershell
setx ANDROID_HOME "$env:LOCALAPPDATA\Android\Sdk"
setx JAVA_HOME "C:\Program Files\Eclipse Adoptium\jdk-17.0.x-hotspot"   # your JDK folder
setx PATH "$env:PATH;$env:LOCALAPPDATA\Android\Sdk\platform-tools"      # adb
```

Check, in a new terminal:

```powershell
java -version
& "$env:ANDROID_HOME\cmdline-tools\latest\bin\sdkmanager.bat" --list_installed
adb version
```

## 4. Build the APK

From a game folder (e.g. `examples/flappy`), inside an engine checkout (or pass
`--engine-src <alpha-star checkout>` / set `ALPHA_ENGINE_SRC`):

```powershell
npm install
npx alpha export --target android-arm64 --out build/android
```

With the toolchain complete it runs Gradle and prints the APK path; otherwise it writes the
project and lists what is missing. The first build compiles the engine and fetches its
dependencies (several minutes). The APK's `game.jsbc` is encrypted with the key of the SDK's
release engine; the engine Gradle builds reads the same key from the checkout's `build/jsbc.secret`
(or `$ALPHA_JSBC_SECRET`), so build from the checkout the SDK engines were staged from. To rebuild by hand:

```powershell
cd build/android
.\gradlew.bat assembleRelease
```

## 5. Install on a phone

1. On the phone: **Settings → About phone →** tap **Build number** 7 times, then
   **Developer options → USB debugging** on. Connect by USB and accept the prompt.
2. Install and watch the log:

```powershell
adb devices
adb install -r build/android/app/build/outputs/apk/release/app-release.apk
adb logcat -s SDL SDL/APP
```

Without your own key the APK is signed with the debug key; release signing is in
[docs/guide/android.md](docs/guide/android.md#build-and-install).

## Troubleshooting

| Message / symptom | Fix |
|---|---|
| `Android toolchain missing: ...` | Install the listed package with the exact version above. |
| `JDK 17+ (JAVA_HOME)` missing | Set `JAVA_HOME` to the JDK folder (the one with `bin\java.exe`), open a new terminal. |
| `NDK ... did not have a source.properties file` | Wrong/partial NDK: reinstall `ndk;27.2.12479018`. |
| Android shows "app not 16 KB compatible / LOAD segment not aligned" | The project predates the 16 KB flag: export again (adds `-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON`). |
| `Package ... not found` from sdkmanager | Newer command-line tools use `/` in names: `ndk/27.2.12479018`. |
| `SDL_CreateWindowAndRenderer failed: Could not fetch native window` | The phone was locked when the app started: unlock it and launch again. |
| `License for package ... not accepted` | `sdkmanager --licenses`. |
| `adb devices` lists nothing / `unauthorized` | USB debugging on, accept the RSA prompt, try another cable/port. |
| `INSTALL_FAILED_NO_MATCHING_ABIS` | The device/emulator is not arm64: only `arm64-v8a` is built (no x86 emulator). |
