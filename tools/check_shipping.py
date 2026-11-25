"""Fail if a shipping engine binary contains dev/debug/agent-only features.

Usage: python tools/check_shipping.py [--preset shipping | --binary PATH] [--expect-found]

Default binary: the shipping preset's alpha_star (cmake --preset shipping);
--preset NAME checks another preset's build (e.g. release for the negative test).
Scans the raw bytes for markers of each ALPHA_* flag (cmake/Flavors.cmake) and
checks the binary form: Windows GUI subsystem (no console window) and no PDB
reference; on ELF, no symbol table (stripped).

--expect-found inverts the verdict, for the negative test: run on a release
binary (ALPHA_DEBUG_TOOLS ON) it must find markers, proving the check bites.
"""
import argparse
import re
import struct
import sys
from pathlib import Path

import toolchain

# flag -> byte patterns that only exist when that feature is compiled in.
MARKERS = {
    "ALPHA_DEVTOOLS": [rb"(?i)imgui"],
    "ALPHA_MCP": [rb"(?i)modelcontextprotocol", rb"jsonrpc"],
    "ALPHA_DEBUG_TOOLS": [rb"--screenshot", rb"--frames", rb"--tap-every", rb"--devtools", rb"--assets",
                          rb"alpha/test", rb"simulate", rb"holdActions", rb"holdAxis", rb"testNames", rb"instanceCreate",
                          rb"stepBudget", rb"uiView", rb"tagCounts"],
}


def marker_hits(data):
    """[(flag, pattern, count)] for every marker found."""
    hits = []
    for flag, patterns in MARKERS.items():
        for pattern in patterns:
            count = len(re.findall(pattern, data))
            if count:
                hits.append((flag, pattern.decode().removeprefix("(?i)"), count))
    return hits


def pe_problems(data):
    """Windows: GUI subsystem (2) and no PDB path."""
    offset = struct.unpack_from("<I", data, 0x3C)[0]
    subsystem = struct.unpack_from("<H", data, offset + 24 + 68)[0]
    problems = [] if subsystem == 2 else [f"PE subsystem {subsystem}, expected 2 (GUI: no console window)"]
    if re.search(rb"(?i)\.pdb\x00", data):
        problems.append("references a .pdb (debug info)")
    return problems


def elf_problems(data):
    return ["ELF has a .symtab (not stripped)"] if b".symtab\x00" in data else []


def form_problems(data):
    if data[:2] == b"MZ":
        return pe_problems(data)
    if data[:4] == b"\x7fELF":
        return elf_problems(data)
    return []


def report(binary, hits, problems):
    print(f"check_shipping: {binary} ({binary.stat().st_size // 1024} KiB)")
    for flag, pattern, count in hits:
        print(f"  FOUND {flag}: '{pattern}' x{count}")
    for problem in problems:
        print(f"  FORM  {problem}")


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--binary", type=Path)
    ap.add_argument("--preset", default="shipping")
    ap.add_argument("--expect-found", action="store_true")
    a = ap.parse_args(argv)
    binary = a.binary or toolchain.built_exe(a.preset)
    data = binary.read_bytes()
    hits = marker_hits(data)
    problems = [] if a.expect_found else form_problems(data)
    report(binary, hits, problems)
    if a.expect_found:
        ok = bool(hits)
        print("check_shipping: negative test " + ("OK (markers found)" if ok else "FAILED: no markers found"))
    else:
        ok = not hits and not problems
        print("check_shipping: " + ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
