"""Fail if a shipping engine binary contains dev/debug/agent-only features.

Usage: python tools/check_shipping.py [--preset shipping | --binary PATH] [--expect-found]
       python tools/check_shipping.py --pack DIR   # a standalone pack: encrypted game.jsbc, no
                                                   # readable JS / source maps / bytecode atoms,
                                                   # engine == staged shipping one
       python tools/check_shipping.py --pack DIR --absent TEXT   # also: TEXT not readable in game.jsbc
       python tools/check_shipping.py --pack DIR --signed   # after alpha export --sign: engine ==
                                                   # staged one once its Authenticode signature is removed

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
    "ALPHA_MCP": [rb"(?i)modelcontextprotocol", rb"jsonrpc", rb"alpha-control", rb"--control", rb"--hidden",
                  rb"ALPHA_CONTROL_TOKEN"],
    "ALPHA_DEBUG_TOOLS": [rb"--screenshot", rb"--frames", rb"--tap-every", rb"--devtools", rb"--assets", rb"--compile",
                          rb"alpha/test", rb"simulate", rb"holdActions", rb"holdAxis", rb"testNames", rb"instanceCreate",
                          rb"stepBudget", rb"uiView", rb"tagCounts", rb"previous game keeps running",
                          rb"glslang", rb"ALPHA_GLSLANG", rb"shadercross", rb"ALPHA_SHADERCROSS"],
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


SOURCE_SUFFIXES = (".js", ".mjs", ".cjs", ".ts", ".map", ".frag", ".glsl", ".hlsl")  # shaders ship as .spv
SOURCE_TEXT = [rb"sourceMappingURL", rb"export default", rb"import \{", rb"\bfunction\s*\w*\s*\([^)]*\)\s*\{"]


JSBC_FORMAT = 2  # src/core/bytecode.hpp: encrypted
# Atoms every game's plain bytecode holds (its `import ... from "alpha"`, `export default`):
# found in game.jsbc, the body is not encrypted.
PLAIN_BYTECODE = [b"alpha", b"default"]


def jsbc_problems(data, extra=()):
    """game.jsbc: an encrypted (format 2) file with no readable source, atoms or `extra` strings."""
    if not data.startswith(b"ALPHAJBC") or len(data) < 112:
        return ["no game.jsbc (ALPHAJBC header)"]
    problems = [f"game.jsbc contains source text /{p.decode()}/" for p in SOURCE_TEXT if re.search(p, data)]
    if (fmt := struct.unpack_from("<I", data, 8)[0]) != JSBC_FORMAT:
        problems.append(f"game.jsbc is format {fmt}, not the encrypted format {JSBC_FORMAT}")
    body = data[112:]
    problems += [f"game.jsbc holds plain text {s!r} (bytecode not encrypted)" for s in [*PLAIN_BYTECODE, *extra] if s in body]
    return problems


def pack_problems(pack, staged, signed=False, strings=()):
    """Standalone pack: encrypted game.jsbc and no readable JS; the engine is the staged shipping one, unmodified."""
    problems = [f"ships source file {p.relative_to(pack)}" for p in pack.rglob("*") if p.suffix in SOURCE_SUFFIXES]
    jsbc = pack / "game.jsbc"
    problems += jsbc_problems(jsbc.read_bytes() if jsbc.is_file() else b"", [s.encode() for s in strings])
    exe = pack / staged.name
    if not exe.is_file() or not same_engine(exe.read_bytes(), staged.read_bytes(), signed):
        what = "the staged shipping engine plus a signature" if signed else "unmodified"
        problems.append(f"{exe.name} differs from the staged shipping engine {staged} (must be {what})")
    return problems


def unsigned_pe(data):
    """A PE image without its Authenticode signature: checksum and security directory zeroed,
    certificate table (and signtool's 8-byte alignment padding) cut off. Anything else: as is."""
    if data[:2] != b"MZ":
        return data
    opt = struct.unpack_from("<I", data, 0x3C)[0] + 24
    dirs = opt + (112 if struct.unpack_from("<H", data, opt)[0] == 0x20B else 96)
    cert = struct.unpack_from("<I", data, dirs + 4 * 8)[0]
    out = bytearray(data[:cert] if cert else data)
    struct.pack_into("<I", out, opt + 64, 0)
    struct.pack_into("<II", out, dirs + 4 * 8, 0, 0)
    return bytes(out)


def same_engine(exe, staged, signed):
    """Unsigned packs: byte-identical. Signed packs (alpha export --sign): identical once the
    Authenticode signature is removed (PE); other formats can't be unsigned here, so they must
    at least pass the shipping marker/form checks."""
    if not signed:
        return exe == staged
    if exe[:2] == b"MZ":
        bare, base = unsigned_pe(exe), unsigned_pe(staged)
        return bare[:len(base)] == base and not bare[len(base):].strip(b"\0")
    return not marker_hits(exe) and not form_problems(exe)


def check_pack(pack, signed=False, strings=()):
    staged = sorted((Path(__file__).resolve().parent.parent / "alpha-sdk" / "engine").glob("*/shipping/alpha_star*"))
    host = [s for s in staged if (pack / s.name).is_file()
            and same_engine((pack / s.name).read_bytes(), s.read_bytes(), signed)]
    problems = pack_problems(pack, (host or staged or [Path("alpha_star")])[0], signed, strings)
    print(f"check_shipping: pack {pack}")
    for problem in problems:
        print(f"  PACK  {problem}")
    print("check_shipping: " + ("PASS" if not problems else "FAIL"))
    return 0 if not problems else 1


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--binary", type=Path)
    ap.add_argument("--preset", default="shipping")
    ap.add_argument("--expect-found", action="store_true")
    ap.add_argument("--pack", type=Path, help="check a standalone pack (alpha export --standalone) instead")
    ap.add_argument("--signed", action="store_true",
                    help="with --pack: the engine was signed by alpha export --sign (compare without the signature)")
    ap.add_argument("--absent", action="append", default=[], metavar="TEXT",
                    help="with --pack: a string of the game (e.g. a name in its code) that must not be readable in game.jsbc")
    a = ap.parse_args(argv)
    if a.pack:
        return check_pack(a.pack, a.signed, a.absent)
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
