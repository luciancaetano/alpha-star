---
name: alpha-harness
description: Set up or refresh the AI-agent harness of a game made with the alpha-star engine. It copies the SDK's LLM API docs into the project's docs/llm/ and creates or updates the project's AGENTS.md with an index of those docs (which file to read for which feature) and code quality limits (params, function and file length, complexity). Use it whenever the user works in a project that depends on @alpha-star/sdk and asks to set up agents, create or update AGENTS.md, "bring the llm docs", prepare the project for Claude/Codex/AI, or after updating the SDK version. Also use it proactively when an alpha-star game has no AGENTS.md or no docs/llm/ folder.
---

# alpha-harness

An alpha-star game is written by AI agents. They write correct code only when they read the real
API instead of guessing, and keep it maintainable only with hard size limits. This skill installs
both into the game project:

- `docs/llm/*.md`: short per-feature API docs, copied from `@alpha-star/sdk/llm/` (the SDK is the
  source of truth, so the copies match the installed engine version).
- A managed block in `AGENTS.md`: a table that summarizes each doc and says when to read it, plus
  the code quality rules from `references/quality-rules.md`.

## Steps

1. **Find the project root**: the folder with a `package.json` that depends on `@alpha-star/sdk`
   (usually the current directory). If none, stop and tell the user this is not an alpha-star game
   (`alpha init` creates one).
2. **Make sure the SDK is installed**: if `node_modules/@alpha-star/sdk` is missing, run `npm install`.
3. **Run the sync script** (idempotent, safe to rerun):
   ```
   node <this skill dir>/scripts/harness.mjs <project root>
   ```
   Add `--sdk <path>` when the SDK lives elsewhere (e.g. a local checkout `.../alpha-sdk`).
   It overwrites `docs/llm/*.md` and replaces only the text between
   `<!-- alpha-harness:start ... -->` and `<!-- alpha-harness:end -->` in `AGENTS.md`. Everything
   the user wrote outside that block is kept; a missing `AGENTS.md` is created.
4. **Review AGENTS.md outside the block**: if the file is new or has no game-specific section,
   add a short one above the block (what the game is, how to run it: `npm start`, `npm test`,
   where the game's modules live). Read `src/main.ts` to write it; keep it under ~20 lines.
   Do not duplicate what the docs already say.
5. **Check existing code against the rules** only if the user asks; otherwise just report that the
   rules are in place.
6. **Report**: number of docs copied, whether AGENTS.md was created or updated.

## Notes

- Never edit `docs/llm/` by hand in the game: changes are lost on the next sync. API doc fixes go
  to the engine repository (`alpha-sdk/llm/`). This skill itself is versioned there too, in
  `skills/alpha-harness/`.
- If the script says no docs were found, the installed SDK predates the LLM docs: update
  `@alpha-star/sdk` or pass `--sdk`.
- `CLAUDE.md`: if the project has one and no `AGENTS.md` reference, add the line `@AGENTS.md`
  to it so Claude Code loads the same instructions.
