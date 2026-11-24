"""Gate: function/file size, complexity, params, and .cpp/.hpp pairing.

Usage: python tools/check_limits.py [files...]   (default: src/ and tests/)
Exit 1 on any violation.
"""
import sys
from pathlib import Path

import lizard

import harness_config as cfg

HEADER_BODY_ALLOWED_LINES = 1  # trivial one-liners only in headers


def file_violations(path):
    lines = path.read_text(encoding="utf-8").splitlines()
    if len(lines) > cfg.MAX_FILE_LINES:
        yield f"{cfg.rel(path)}: {len(lines)} lines > {cfg.MAX_FILE_LINES}"
    yield from pairing_violations(path)


def pairing_violations(path):
    in_src = cfg.SRC in path.resolve().parents
    if not in_src or path.suffix != ".cpp" or path.name in cfg.UNPAIRED_CPP:
        return
    if not path.with_suffix(".hpp").exists():
        yield f"{cfg.rel(path)}: missing matching .hpp"


def function_violations(path):
    info = lizard.analyze_file(str(path))
    for fn in info.function_list:
        where = f"{cfg.rel(path)}:{fn.start_line} {fn.name}"
        yield from limit_checks(where, fn, path.suffix == ".hpp")


def limit_checks(where, fn, in_header):
    body = fn.end_line - fn.start_line + 1
    if body > cfg.MAX_FUNCTION_LINES:
        yield f"{where}: {body} lines > {cfg.MAX_FUNCTION_LINES}"
    if fn.cyclomatic_complexity > cfg.MAX_CCN:
        yield f"{where}: CCN {fn.cyclomatic_complexity} > {cfg.MAX_CCN}"
    if len(fn.parameters) > cfg.MAX_PARAMS:
        yield f"{where}: {len(fn.parameters)} params > {cfg.MAX_PARAMS}"
    if in_header and body > HEADER_BODY_ALLOWED_LINES and "template" not in fn.long_name:
        yield f"{where}: implementation in header; move to .cpp"


def violations(paths):
    for path in paths:
        yield from file_violations(path)
        yield from function_violations(path)


def main(argv):
    paths = [Path(a) for a in argv] or list(cfg.cpp_files())
    found = list(violations(p for p in paths if p.suffix in cfg.CPP_EXT))
    for v in found:
        print(v)
    print(f"check_limits: {len(found)} violation(s) in {len(paths)} file(s)")
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
