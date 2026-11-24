"""Mutation testing over src/: mutate one operator, rebuild, run tests.

Usage: python tools/mutator.py [--preset harness] [files...]
A mutant that still passes the tests SURVIVED: add a test that kills it,
or delete the code if it is dead. Exit 1 if any mutant survives.
Each mutant restores the original file in a finally block (idempotent).
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

import harness_config as cfg

MUTATIONS = [
    (r"==", "!="), (r"!=", "=="), (r"<=", "<"), (r">=", ">"),
    (r"(?<![<>=!])<(?![<=])", "<="), (r"(?<![<>=\-])>(?![>=])", ">="),
    (r"&&", "||"), (r"\|\|", "&&"), (r"\btrue\b", "false"), (r"\bfalse\b", "true"),
    (r"(?<![+])\+(?![+=])", "-"), (r"\breturn ([^;]+);", r"return {};"),
]
SKIP_LINE = re.compile(r"^\s*(#|//)")


def mutants(path):
    lines = path.read_text(encoding="utf-8").splitlines(keepends=True)
    for idx, line in enumerate(lines):
        if SKIP_LINE.match(line):
            continue
        for pattern, repl in MUTATIONS:
            for m in re.finditer(pattern, line):
                mutated = line[:m.start()] + m.expand(repl) + line[m.end():]
                yield idx + 1, line.strip(), lines[:idx] + [mutated] + lines[idx + 1:]


def passes(preset):
    build = subprocess.run(["cmake", "--build", "--preset", preset], capture_output=True)
    if build.returncode != 0:
        return False  # did not compile: killed
    test = subprocess.run(["ctest", "--preset", preset], capture_output=True)
    return test.returncode == 0


def run_file(path, preset):
    original = path.read_text(encoding="utf-8")
    survivors, total = [], 0
    try:
        for line_no, text, mutated in mutants(path):
            total += 1
            path.write_text("".join(mutated), encoding="utf-8")
            if passes(preset):
                survivors.append(f"{cfg.rel(path)}:{line_no}  {text}")
    finally:
        path.write_text(original, encoding="utf-8")
    return survivors, total


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preset", default="harness")
    ap.add_argument("files", nargs="*")
    args = ap.parse_args()
    files = [Path(f) for f in args.files] or [p for p in cfg.cpp_files([cfg.SRC]) if p.suffix == ".cpp"]
    survivors, total = [], 0
    for path in files:
        s, t = run_file(path, args.preset)
        survivors += s
        total += t
    for s in survivors:
        print(f"SURVIVED  {s}")
    print(f"mutator: {len(survivors)}/{total} mutant(s) survived")
    return 1 if survivors else 0


if __name__ == "__main__":
    sys.exit(main())
