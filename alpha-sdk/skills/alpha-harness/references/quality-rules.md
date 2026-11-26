## Code quality rules

Keep game code small enough for an agent (and a human) to read whole. When a limit is hit, split
the function or move code to a new module in `src/`; never raise the limit.

| Rule | Limit | Source of the value |
|---|---|---|
| Parameters per function | **10** (prefer ≤ 4; pass an options object beyond that) | project rule; ESLint `max-params` default is 3, SonarQube S107 is 7 |
| Lines per function (body, non-blank) | **50** | ESLint `max-lines-per-function` default |
| Lines per file | **300** | ESLint `max-lines` default |
| Cyclomatic complexity per function | **10** | McCabe's original threshold; also SonarQube/NIST recommendation |
| Nesting depth | **4** | ESLint `max-depth` default |

- One feature per file in `src/` (player, enemies, hud, ...); `main.ts` wires them together.
- Tunable numbers live in one place (`tuning.ts`), not scattered literals.
- Every behavior change comes with an `alpha test` test; run `alpha check` and `alpha test` before you finish.
- Keep the simulation deterministic: use the engine's seeded RNG and fixed step, never `Math.random()` or wall-clock time.
