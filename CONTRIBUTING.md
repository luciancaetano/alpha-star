# Contributing to alpha-star

Thanks for helping. `AGENTS.md` is the full rule book (for people and AI agents alike); this is
the short version.

## Never freeze the machine

Every build, test and benchmark runs through a guarded entry point: `python tools/gate.py`,
`make gate`, `make test`, `python tools/stage_engine.py`, `python tools/bench.py`, or
`python tools/resource_guard.py -- <command>` (memory cap, CPU cap, low priority, the whole
process tree dies with it). Use `CMAKE_BUILD_PARALLEL_LEVEL=8` and one build at a time.
Never run long engine or test binaries directly.

## The gate

A change is done only when it passes:

```
python tools/gate.py                     # build + tests + coverage + every check below: GATE PASS
cd alpha-sdk && npm test                 # SDK, templates, docs, examples' engine runs
python tools/check_shipping.py           # after a shipping build: no dev/debug/agent markers
cd examples/<game> && npx alpha test     # every example you touched
```

The Linux gate (`make linux-gate`) runs on a dedicated Linux machine.

## Hard limits (never raise them; split the code instead)

| Rule | Limit |
|---|---|
| Lines per function (non-blank body) | 40 |
| Lines per file (.cpp / .hpp) | 300 |
| Cyclomatic complexity | 10 |
| Parameters | 10 |
| CRAP score (`tools/crapper.py`) | < 10 |
| Duplication similarity (`tools/dryer.py`) | < 0.8 |

Every `.cpp` in `src/` has a matching `.hpp`. Warnings are errors; no blanket suppressions.
All file access in `src/` goes through `platform/vfs` (`tools/check_file_access.py`).

## Test first

Write the failing test, see it fail for the expected reason, make the smallest change that
passes it. Test behavior the way a user calls the code; a test that would pass if every function
returned nothing gets deleted. Fix root causes: a guard that hides a crash is not a fix.

## Build flavors

Every dev, debug or agent-only feature lands behind its `ALPHA_*` flag (`ALPHA_DEVTOOLS`,
`ALPHA_MCP`, `ALPHA_DEBUG_TOOLS`), tested with `#if ALPHA_X` (never `#ifdef`), and is absent from
the shipping flavor. Strings in embedded JS end up in the binary too.

## SDK, templates, docs

- Templates for `alpha init` mirror `examples/{platformer,topdown,puzzle,shmup}`; edit the example,
  then `cd alpha-sdk && npm run templates`. `blank` lives only in `alpha-sdk/templates/blank`.
- The docs site is generated: edit `docs/guide/*.md` or `alpha-sdk/types/alpha.d.ts`, then
  `cd alpha-sdk && npm run docs`. The SDK tests fail while either is stale.

## Versions

The engine (`project(... VERSION)` in `CMakeLists.txt`) and `@alpha-star/sdk` always carry the
same version: the SDK bundles engines built from the same commit. Semantic Versioning: while in
0.x, a minor bump (0.2 -> 0.3) may change the script API, a patch bump only fixes. Every change
gets a line under `[Unreleased]` in `CHANGELOG.md`.

## Log and commits

Append each action and its reason to `docs/LOG.md`, keep `docs/GLOSSARY.md` current, and use
Conventional Commits (`feat(sdk): ...`, `fix(render): ...`).
