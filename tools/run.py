"""Run the engine, optionally under a debugger.

Usage: python tools/run.py [--preset dev] [--debug]
"""
import argparse
import subprocess
import sys

import toolchain


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preset", default="dev")
    ap.add_argument("--debug", action="store_true")
    a = ap.parse_args()
    exe = toolchain.built_exe(a.preset)
    cmd = toolchain.debugger_command(exe) if a.debug else [str(exe)]
    return subprocess.run(cmd).returncode


if __name__ == "__main__":
    sys.exit(main())
