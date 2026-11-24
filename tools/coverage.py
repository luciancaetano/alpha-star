"""Collect line coverage of the test binary as Cobertura XML.

Usage: python tools/coverage.py [--preset harness]  -> build/<preset>/coverage.xml
Uses Microsoft.CodeCoverage.Console (ships with Visual Studio, incl. Community).
"""
import argparse
import subprocess
import sys

import harness_config as cfg
import toolchain

COVERAGE_TOOL = ("Common7/IDE/Extensions/Microsoft/CodeCoverage.Console/"
                 "Microsoft.CodeCoverage.Console.exe")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preset", default="harness")
    preset = ap.parse_args().preset
    tool = toolchain.vs_tool(COVERAGE_TOOL)
    if tool is None:
        sys.exit("coverage: Microsoft.CodeCoverage.Console not found (needs Visual Studio)")
    tests = next((cfg.ROOT / "build" / preset / "tests").rglob("alpha_tests.exe"))
    out = cfg.ROOT / "build" / preset / "coverage.xml"
    cmd = [str(tool), "collect", "--output", str(out), "--output-format", "cobertura", str(tests)]
    rc = subprocess.run(cmd, cwd=cfg.ROOT).returncode
    print(f"coverage: {out}" if rc == 0 else "coverage: FAILED")
    return rc


if __name__ == "__main__":
    sys.exit(main())
