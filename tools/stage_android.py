"""Build the prebuilt Android engine (arm64-v8a) in both APK flavors and stage it inside the SDK,
so `alpha export --target android-arm64` needs no engine source, NDK or CMake.

Usage: python tools/stage_android.py     # -> alpha-sdk/engine/android-arm64/{debug,shipping}/

  debug    -> engine/android-arm64/debug/{libmain.so,libSDL3.so}     (debug APK: dev tools, no MCP)
  shipping -> engine/android-arm64/shipping/{libmain.so,libSDL3.so}  (release APK / AAB), staged
              only if tools/check_shipping.py passes on libmain.so

Needs the NDK of alpha-sdk/android/app/build.gradle (ndkVersion) and its CMake: $ANDROID_NDK_HOME
(or $ANDROID_HOME/ndk/<version>) and cmake on PATH. Same flags as the Gradle template; the game.jsbc
key comes from $ALPHA_JSBC_SECRET or build/jsbc.secret, as for the desktop engines, so one staging's
desktop release engine compiles bytecode these libraries run.
"""
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

import harness_config as cfg
import resource_guard

TARGET = "android-arm64"
LIBS = ["libmain.so", "libSDL3.so"]
COMMON = ["-DANDROID_ABI=arm64-v8a", "-DANDROID_PLATFORM=android-21", "-DANDROID_STL=c++_static",
          "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON", "-DALPHA_BUILD_TESTS=OFF", "-DCMAKE_BUILD_TYPE=Release",
          # No C++20 modules here; without this CMake may scan with the host's clang-scan-deps
          # (another clang than the NDK's: "PCH file built from a different branch").
          "-DCMAKE_CXX_SCAN_FOR_MODULES=OFF"]
# alpha-sdk/android/app/build.gradle buildTypes, one decision each: debug = release flavor without MCP.
FLAVORS = {"debug": ["-DALPHA_SHIPPING=OFF", "-DALPHA_DEVTOOLS=ON", "-DALPHA_MCP=OFF", "-DALPHA_DEBUG_TOOLS=ON"],
           "shipping": ["-DALPHA_SHIPPING=ON"]}


def ndk_version():
    gradle = (cfg.ROOT / "alpha-sdk" / "android" / "app" / "build.gradle").read_text(encoding="utf-8")
    return re.search(r'ndkVersion "([^"]+)"', gradle).group(1)


def find_ndk(env=os.environ):
    """The NDK folder: $ANDROID_NDK_HOME, else <$ANDROID_HOME or $ANDROID_SDK_ROOT>/ndk/<version>."""
    candidates = [env.get("ANDROID_NDK_HOME")]
    candidates += [str(Path(env[k]) / "ndk" / ndk_version()) for k in ("ANDROID_HOME", "ANDROID_SDK_ROOT") if env.get(k)]
    return next((Path(c) for c in candidates if c and (Path(c) / "build" / "cmake" / "android.toolchain.cmake").is_file()), None)


def strip_tool(ndk):
    exe = ".exe" if os.name == "nt" else ""
    return next((ndk / "toolchains" / "llvm" / "prebuilt").glob(f"*/bin/llvm-strip{exe}"))


def build(ndk, flavor):
    out = cfg.ROOT / "build" / f"android-{flavor}"
    configure = ["cmake", "-S", str(cfg.ROOT), "-B", str(out), "-G", "Ninja",
                 f"-DCMAKE_TOOLCHAIN_FILE={ndk / 'build' / 'cmake' / 'android.toolchain.cmake'}", *COMMON, *FLAVORS[flavor]]
    for cmd in (configure, ["cmake", "--build", str(out), "--target", "alpha_star"]):
        if subprocess.run(cmd, cwd=cfg.ROOT).returncode != 0:
            sys.exit(f"stage_android: {' '.join(cmd)} failed")
    return {name: next(out.rglob(name)) for name in LIBS}


def stage(ndk, flavor):
    built = build(ndk, flavor)
    dest = cfg.ROOT / "alpha-sdk" / "engine" / TARGET / flavor
    shutil.rmtree(dest, ignore_errors=True)
    dest.mkdir(parents=True)
    for name, path in built.items():
        subprocess.run([str(strip_tool(ndk)), "--strip-unneeded", "-o", str(dest / name), str(path)], check=True)
    # Checked as shipped: stripped (the symbol table would carry dev-only function names).
    if flavor == "shipping" and subprocess.run([sys.executable, "tools/check_shipping.py", "--binary", str(dest / "libmain.so")],
                                               cwd=cfg.ROOT).returncode != 0:
        shutil.rmtree(dest)
        sys.exit("stage_android: shipping libmain.so failed tools/check_shipping.py; not staged")
    for name in built:
        print(f"stage_android: {flavor} {name} -> {cfg.rel(dest / name)}")


def main(argv):
    resource_guard.limit()
    ndk = find_ndk()
    if not ndk:
        sys.exit(f"stage_android: no NDK {ndk_version()}: set ANDROID_NDK_HOME or install it with "
                 f"sdkmanager \"ndk;{ndk_version()}\" under $ANDROID_HOME")
    if not shutil.which("ninja"):
        sys.exit("stage_android: ninja not on PATH (the Android SDK's cmake/<version>/bin has one)")
    for flavor in FLAVORS:
        stage(ndk, flavor)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
