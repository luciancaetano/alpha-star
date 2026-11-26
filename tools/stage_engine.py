"""Build both engine flavors and stage them inside the SDK (like Electron's prebuilt binaries).

Usage: python tools/stage_engine.py          # this machine -> alpha-sdk/engine/<platform>-<arch>/
       python tools/stage_engine.py --linux  # same, built inside the Linux container

  release  -> engine/<target>/alpha_star[.exe]           (alpha run / alpha test)
  shipping -> engine/<target>/shipping/alpha_star[.exe]  (alpha export --standalone),
              staged only if tools/check_shipping.py passes on it

Targets use Node's names (process.platform-process.arch): win32-x64, linux-x64, ...
"""
import os
import platform
import shutil
import subprocess
import sys

import harness_config as cfg
import resource_guard
import toolchain

ARCH = {"amd64": "x64", "x86_64": "x64", "arm64": "arm64", "aarch64": "arm64"}


def target():
    system = "win32" if os.name == "nt" else sys.platform
    return f"{system}-{ARCH.get(platform.machine().lower(), platform.machine().lower())}"


# GLSL -> SPIR-V, then SPIR-V -> MSL (shadercross built without DXC).
SHADER_TOOLS = [t + (".exe" if os.name == "nt" else "") for t in ("glslang", "shadercross")]
FLAVORS = {"release": "", "shipping": "shipping"}  # preset -> subdir of engine/<target>


def owned_files(preset, exe_name):
    """The exact file set of a staged flavor dir: the engine, plus the shader compiler next to
    the release engine (alpha run / alpha export; never shipped)."""
    return {exe_name, *SHADER_TOOLS} if preset == "release" else {exe_name}


def prune(dest, keep):
    """Deletes everything in `dest` except the names in `keep` and the flavor subdirs; returns
    the deleted names (sorted)."""
    keep = set(keep) | {d for d in FLAVORS.values() if d}
    stale = sorted(p.name for p in dest.iterdir() if p.name not in keep)
    for name in stale:
        path = dest / name
        shutil.rmtree(path) if path.is_dir() else path.unlink()
    return stale


def build(preset):
    for cmd in (["cmake", "--preset", preset], ["cmake", "--build", "--preset", preset]):
        if subprocess.run(cmd, cwd=cfg.ROOT).returncode != 0:
            sys.exit(f"stage_engine: {' '.join(cmd)} failed")


def stage(preset, subdir):
    build(preset)
    exe = toolchain.built_exe(preset)
    if preset == "shipping" and subprocess.run([sys.executable, "tools/check_shipping.py", "--binary", str(exe)],
                                               cwd=cfg.ROOT).returncode != 0:
        sys.exit("stage_engine: shipping binary failed tools/check_shipping.py; not staged")
    dest = cfg.ROOT / "alpha-sdk" / "engine" / target() / subdir
    dest.mkdir(parents=True, exist_ok=True)
    shutil.copy2(exe, dest / exe.name)
    if preset == "release":  # custom shader compiler for alpha run / alpha export (never shipped)
        for tool in SHADER_TOOLS:
            if (exe.parent / tool).is_file():
                shutil.copy2(exe.parent / tool, dest / tool)
    print(f"stage_engine: {preset} {exe.name} -> {cfg.rel(dest / exe.name)}")


def main(argv):
    resource_guard.limit()
    if "--linux" in argv:
        return subprocess.run([sys.executable, "tools/linux_gate.py", "--", "python3",
                               "tools/stage_engine.py"], cwd=cfg.ROOT).returncode
    for preset, subdir in FLAVORS.items():
        stage(preset, subdir)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
