"""CRAP score per function: CRAP = ccn^2 * (1 - cov)^3 + ccn.

Usage: python tools/crapper.py [coverage.xml]
coverage.xml is Cobertura (OpenCppCoverage --export_type cobertura, or gcovr --xml).
Without coverage every function counts as 0% covered (worst case).
Exit 1 if any function in src/ has CRAP >= MAX_CRAP.
"""
import sys
import xml.etree.ElementTree as ET

import lizard

import harness_config as cfg


def load_hits(xml_path):
    """Map posix-relative file path -> {line: hits}."""
    hits = {}
    for cls in ET.parse(xml_path).iter("class"):
        name = cls.get("filename", "").replace("\\", "/")
        lines = hits.setdefault(_normalize(name), {})
        for ln in cls.iter("line"):
            lines[int(ln.get("number"))] = int(ln.get("hits"))
    return hits


def _normalize(name):
    marker = "src/"
    idx = name.rfind(marker)
    return name[idx:] if idx >= 0 else name


def coverage(fn, file_hits):
    span = [file_hits[n] for n in range(fn.start_line, fn.end_line + 1) if n in file_hits]
    return sum(1 for h in span if h > 0) / len(span) if span else 0.0


def crap(ccn, cov):
    return ccn * ccn * (1 - cov) ** 3 + ccn


def scores(hits):
    for path in cfg.cpp_files([cfg.SRC]):
        if path.name in cfg.UNPAIRED_CPP:
            continue  # composition root: covered by smoke run, not unit tests
        file_hits = hits.get(cfg.rel(path), {})
        for fn in lizard.analyze_file(str(path)).function_list:
            cov = coverage(fn, file_hits)
            yield crap(fn.cyclomatic_complexity, cov), cov, f"{cfg.rel(path)}:{fn.start_line} {fn.name}"


def main(argv):
    hits = load_hits(argv[0]) if argv else {}
    bad = 0
    for score, cov, where in sorted(scores(hits), reverse=True):
        flag = "FAIL" if score >= cfg.MAX_CRAP else "ok  "
        bad += flag == "FAIL"
        print(f"{flag} CRAP {score:6.2f} cov {cov:4.0%}  {where}")
    print(f"crapper: {bad} function(s) with CRAP >= {cfg.MAX_CRAP}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
