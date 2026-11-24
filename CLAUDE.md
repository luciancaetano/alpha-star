# alpha-star — 2D engine for AI agents

C++20 core (SDL3 window/render, Box2D v3 physics, internal ECS) driven by
QuickJS-ng scripts that AI agents write for vibecoder users.

## Hard limits (enforced by `tools/check_limits.py`, run before every commit)

| Rule | Limit |
|---|---|
| Lines per function (body, non-blank) | **40** |
| Lines per file (.cpp / .hpp) | **300** |
| Cyclomatic complexity per function | **10** |
| Parameters per function | **5** |
| CRAP score per function (`tools/crapper.py`) | **< 10** |
| Duplication similarity (`tools/dryer.py`) | **< 0.8** |

- Every `.cpp` in `src/` has a matching `.hpp` (except `main.cpp`).
- Headers declare; implementations live in `.cpp`. Only trivial one-line
  accessors, `constexpr` and templates may live in a header.
- Exceeding a limit means split the function/file. Never raise the limit.

## Layout

```
src/<module>/   engine code; one module per folder (core, ecs, render, physics, script, mcp)
tests/<module>/ doctest tests mirroring src/
tools/          harness levers (limits, crapper, dryer, mutator)
docs/           GLOSSARY.md, LOG.md
cmake/          dependency + warnings config
```

## Commands

```
make build-debug | build-prod | run-debug | debug   # debug = run under VS/lldb/gdb
```

```
cmake --preset dev                 # configure (MSVC, Ninja-free VS generator)
cmake --build --preset dev         # build
ctest --preset dev                 # tests
python tools/check_limits.py       # size/complexity/pairing gate
python tools/crapper.py [cov.xml]  # CRAP per function (Cobertura coverage)
python tools/dryer.py              # duplication report
python tools/mutator.py            # mutation testing over src/
```

`python tools/gate.py` runs build + tests + all checks. A change is done only when it passes.

## Workflow

### First make it work
- **Laziness:** one decision lives in one place; ship the smallest edit that solves it.
- **Foundations:** data shape before logic; do first what helps every later phase;
  if a concurrent writer would matter, do not share state.
- **TDD:** write the failing test first, see it fail for the expected reason,
  make the smallest change that passes. If an honest test is impractical, write none.
- **Redesign from first principles:** don't bolt requirements onto old design;
  design as if it had been there from the start, deliver in pieces.
- **Attack the premise:** when two fixes sharing one premise fail the same gate,
  write the premise in one sentence, find which actors hold the imbalance,
  remove that assignment before any further fix.
- **Subtract before you add.**
- **Outcome-oriented:** aim at a checkable end state; mid-way breaks only if planned and undoable.
- **Build the lever:** if work isn't obvious at a glance, build a tool; do one unit by hand first.

### Architecture
- Model the domain so illegal states are impossible; keep `docs/GLOSSARY.md` current.
- Boundary discipline: validate where data enters (script API, files, MCP); trust inside;
  business logic in pure functions.
- Types: make illegal values unwritable; parse outside data once; never silence the
  compiler (warnings are errors; no blanket `#pragma warning(disable)`/NOLINT).
- Operations are idempotent (rerun or crash-midway ends in the same state).
- Separate before serializing shared state; if shared, one writer enforced by structure.

### Then make it right
- Fix root causes: reproduce, fix the cause; a guard that hides a crash is not a fix.
- Sequence work into small verifiable units; finish and check one before the next.
- Test behavior, not implementation: call code as its user does. A test that would
  pass if every function returned nothing gets deleted.
- **Crapper:** every function CRAP < 10 — split it, test it, or both.
- **Dryer:** eliminate duplication ranked ≥ 0.8.
- **Mutator:** cover the uncovered, kill survivors, delete dead code.
- Refactoring: pin behavior, change structure, prove behavior unchanged.

### Practice
- **Keep a log:** append every action and its reason to `docs/LOG.md`; report it on completion.
- **Guard the context window:** keep the main thread small; send bulk work to subagents and keep summaries.

## Agent-facing API principles (the product)
- Small, predictable script API; one way to do each thing.
- Errors are written for the agent that will fix them (what failed, what was available).
- A script error pauses the game; it never crashes the engine.
- Deterministic simulation (fixed step, seeded RNG) so agent tests are not flaky.
