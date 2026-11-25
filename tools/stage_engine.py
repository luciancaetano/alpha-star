"""Build the release engine and stage it inside the SDK (like Electron's prebuilt binaries).

Usage: python tools/stage_engine.py          # this machine -> alpha-sdk/engine/<platform>-<arch>/
       python tools/stage_engine.py --linux  # same, built inside the Linux container

Targets use Node's names (process.platform-process.arch): win32-x64, linux-x64, ...
"""
import os
import platform
import shutil
import subprocess
import sys

import harness_config as cfg
import toolchain

ARCH = {"amd64": "x64", "x86_64": "x64", "arm64": "arm64", "aarch64": "arm64"}


def target():
    system = "win32" if os.name == "nt" else sys.platform
    return f"{system}-{ARCH.get(platform.machine().lower(), platform.machine().lower())}"


def build_release():
    for cmd in (["cmake", "--preset", "release"], ["cmake", "--build", "--preset", "release"]):
        if subprocess.run(cmd, cwd=cfg.ROOT).returncode != 0:
            sys.exit(f"stage_engine: {' '.join(cmd)} failed")


def main(argv):
    if "--linux" in argv:
        return subprocess.run([sys.executable, "tools/linux_gate.py", "--", "python3",
                               "tools/stage_engine.py"], cwd=cfg.ROOT).returncode
    build_release()
    exe = toolchain.built_exe("release")
    dest = cfg.ROOT / "alpha-sdk" / "engine" / target()
    dest.mkdir(parents=True, exist_ok=True)
    shutil.copy2(exe, dest / exe.name)
    print(f"stage_engine: {exe.name} -> {cfg.rel(dest / exe.name)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
