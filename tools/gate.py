"""Full gate: configure, build, test, limits, CRAP, duplication.

Usage: python tools/gate.py [--preset harness|dev] [--coverage cov.xml] [--mutate]
"""
import argparse
import subprocess
import sys

import harness_config as cfg


def step(name, cmd):
    print(f"== {name}", flush=True)
    ok = subprocess.run(cmd, cwd=cfg.ROOT).returncode == 0
    print(f"== {name}: {'PASS' if ok else 'FAIL'}", flush=True)
    return ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preset", default="harness")
    ap.add_argument("--coverage")
    ap.add_argument("--mutate", action="store_true")
    a = ap.parse_args()
    py = sys.executable
    steps = [
        ("configure", ["cmake", "--preset", a.preset]),
        ("build", ["cmake", "--build", "--preset", a.preset]),
        ("test", ["ctest", "--preset", a.preset]),
        ("coverage", [py, "tools/coverage.py", "--preset", a.preset]),
        ("limits", [py, "tools/check_limits.py"]),
        ("crap", [py, "tools/crapper.py", a.coverage or f"build/{a.preset}/coverage.xml"]),
        ("dry", [py, "tools/dryer.py"]),
    ]
    if a.mutate:
        steps.append(("mutate", [py, "tools/mutator.py", "--preset", a.preset]))
    failed = [name for name, cmd in steps if not step(name, cmd)]
    print("GATE PASS" if not failed else f"GATE FAIL: {', '.join(failed)}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
