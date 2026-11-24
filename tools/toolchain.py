"""Locate machine-specific tools (Visual Studio, debuggers, built exes) portably."""
import os
import shutil
import subprocess
from pathlib import Path

import harness_config as cfg

VSWHERE = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) \
    / "Microsoft Visual Studio/Installer/vswhere.exe"


def vs_tool(relative):
    """Path to a file inside the latest Visual Studio install, or None."""
    if not VSWHERE.exists():
        return None
    out = subprocess.run([str(VSWHERE), "-latest", "-products", "*", "-property",
                          "installationPath"], capture_output=True, text=True)
    path = Path(out.stdout.strip()) / relative
    return path if out.stdout.strip() and path.exists() else None


def debugger_command(exe):
    """Command that launches exe under the platform's debugger."""
    devenv = vs_tool("Common7/IDE/devenv.exe")
    if devenv:
        return [str(devenv), "/debugexe", str(exe)]
    for name, args in (("lldb", ["--"]), ("gdb", ["--args"])):
        if shutil.which(name):
            return [name, *args, str(exe)]
    raise SystemExit("toolchain: no debugger found (Visual Studio, lldb or gdb)")


def built_exe(preset, name="alpha_star"):
    """Find the built executable regardless of generator (single/multi-config)."""
    suffix = ".exe" if os.name == "nt" else ""
    hits = sorted((cfg.ROOT / "build" / preset / "src").rglob(name + suffix))
    if not hits:
        raise SystemExit(f"toolchain: {name} not built for preset '{preset}'")
    return hits[0]
