"""Collect line coverage of the test binary as Cobertura XML.

Usage: python tools/coverage.py [--preset dev]  -> build/<preset>/coverage.xml
Windows: Microsoft.CodeCoverage.Console (ships with Visual Studio, incl. Community).
Linux:   gcovr over the gcov data of a build configured with ALPHA_COVERAGE=ON.
"""
import argparse
import os
import shutil
import subprocess
import sys

import harness_config as cfg
import toolchain

COVERAGE_TOOL = ("Common7/IDE/Extensions/Microsoft/CodeCoverage.Console/"
                 "Microsoft.CodeCoverage.Console.exe")


def windows(tests, out):
    tool = toolchain.vs_tool(COVERAGE_TOOL)
    if tool is None:
        sys.exit("coverage: Microsoft.CodeCoverage.Console not found (needs Visual Studio)")
    cmd = [str(tool), "collect", "--output", str(out), "--output-format", "cobertura", str(tests)]
    return subprocess.run(cmd, cwd=cfg.ROOT).returncode


def gcov(tests, out, build):
    if shutil.which("gcovr") is None:
        sys.exit("coverage: gcovr not found (pip install gcovr)")
    if subprocess.run([str(tests)], cwd=cfg.ROOT).returncode != 0:
        return 1
    cmd = ["gcovr", "--root", str(cfg.ROOT), "--filter", str(cfg.SRC), "--object-directory",
           str(build), "--cobertura-pretty", "--output", str(out)]
    return subprocess.run(cmd, cwd=cfg.ROOT).returncode


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preset", default="dev")
    preset = ap.parse_args().preset
    build = cfg.ROOT / "build" / preset
    tests = toolchain.built_exe(preset, "alpha_tests", "tests")
    out = build / "coverage.xml"
    rc = windows(tests, out) if os.name == "nt" else gcov(tests, out, build)
    print(f"coverage: {out}" if rc == 0 else "coverage: FAILED")
    return rc


if __name__ == "__main__":
    sys.exit(main())
