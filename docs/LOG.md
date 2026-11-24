# Action log

## 2026-11-24 — harness bootstrap

| # | Action | Reason |
|---|---|---|
| 1 | Inspected empty repo + toolchain (CMake 4.3, VS 18 MSVC 19.51, Python 3.13) | Know the ground before choosing tools |
| 2 | `CLAUDE.md` with hard limits + workflow rules | One place the agent reads the rules |
| 3 | `tools/harness_config.py` holds every limit/path | One decision lives in one place |
| 4 | CMake: root, `cmake/Warnings.cmake` (/W4 /WX), `cmake/Dependencies.cmake` (FetchContent, pinned tags) | Reproducible deps; never silence compiler |
| 5 | Presets `harness` (core+tests, no runtime deps) and `dev` (SDL3, Box2D, QuickJS-ng, EnTT) | Fast gate for the inner loop |
| 6 | `alpha_core` + `version` as first unit, version passed from CMake via `ALPHA_VERSION` | Smallest real unit; version defined once |
| 7 | `tools/check_limits.py` (fn ≤40 lines, file ≤300, CCN ≤10, params ≤5, .cpp↔.hpp pairing, no bodies in headers) | Enforce size rules mechanically |
| 8 | `tools/crapper.py`, `tools/dryer.py`, `tools/mutator.py`, `tools/coverage.py`, `tools/gate.py` | Build the levers the rules require |
| 9 | PostToolUse hook in `.claude/settings.json` runs `check_limits` after edits | Agent sees violations immediately |
| 10 | Gate run 1 failed: doctest 2.4.11 vs CMake 4 → bumped to 2.4.12 | Root cause: removed CMake <3.5 compat |
| 11 | ctest passed with 0 tests → `noTestsAction: error` | A passing gate with no tests is dishonest |
| 12 | doctest fwd-declares `std::ostream`, breaks new MSVC STL → `DOCTEST_CONFIG_USE_STD_HEADERS` | Root-cause fix, not a warning disable |
| 13 | `main` CRAP 12 → split `fail()` helper; excluded composition root from crapper | Entry point verified by smoke run, not unit test |
| 14 | Added real coverage (Microsoft.CodeCoverage.Console → Cobertura) | CRAP without coverage is worst-case guessing |
| 15 | Negative test: planted 48-line fn, 7-param fn, unpaired .cpp, 0.99 duplicate → all caught | Prove the gate fails when it should |
| 16 | Mutator on `version.cpp`: 0/1 survived | Test kills the mutant |
| 17 | `dev` preset: fetched + built SDL3 3.2.10, Box2D 3.1.0, QuickJS-ng 0.10.1, EnTT 3.15.0; smoke run opened window, printed `alpha-star 0.1.0`, exit 0 | Prove runtime deps link before engine work |
| 18 | `release` preset + `Makefile` (build-debug, build-prod, run-debug, debug) | User request |
| 19 | Removed hardcoded VS paths: `tools/toolchain.py` resolves VS via vswhere, falls back to lldb/gdb, finds exe per generator; `coverage.py` uses it | Hardcoded `D:/...` only worked on one PC |
