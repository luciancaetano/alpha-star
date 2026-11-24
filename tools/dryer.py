"""Duplication finder: ranks function pairs by token similarity.

Usage: python tools/dryer.py
Reports pairs with similarity >= DUP_THRESHOLD (functions with at least
DUP_MIN_TOKENS tokens). Exit 1 if any are found.
"""
import sys
from difflib import SequenceMatcher
from itertools import combinations

import lizard

import harness_config as cfg


def function_tokens(path):
    lines = path.read_text(encoding="utf-8").splitlines()
    for fn in lizard.analyze_file(str(path)).function_list:
        body = "\n".join(lines[fn.start_line - 1:fn.end_line])
        yield f"{cfg.rel(path)}:{fn.start_line} {fn.name}", _tokenize(body)


def _tokenize(text):
    return [t for t in lizard.CLikeReader.generate_tokens(text) if not t.isspace()]


def candidates():
    funcs = []
    for path in cfg.cpp_files([cfg.SRC]):
        funcs.extend((w, t) for w, t in function_tokens(path) if len(t) >= cfg.DUP_MIN_TOKENS)
    return funcs


def similar_pairs(funcs):
    for (wa, ta), (wb, tb) in combinations(funcs, 2):
        rank = SequenceMatcher(None, ta, tb, autojunk=False).ratio()
        if rank >= cfg.DUP_THRESHOLD:
            yield rank, wa, wb


def main():
    pairs = sorted(similar_pairs(candidates()), reverse=True)
    for rank, wa, wb in pairs:
        print(f"{rank:.2f}  {wa}  <->  {wb}")
    print(f"dryer: {len(pairs)} pair(s) with similarity >= {cfg.DUP_THRESHOLD}")
    return 1 if pairs else 0


if __name__ == "__main__":
    sys.exit(main())
