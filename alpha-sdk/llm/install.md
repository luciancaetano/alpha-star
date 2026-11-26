# Install

> summary: Installing @alpha-star/sdk from npm (new game, existing project, global), the per-platform engine packages, checking the version and fixing a missing engine; read when setting up a machine or project for alpha-star.

Needs Node.js 20+. `@alpha-star/sdk` = `alpha` CLI + `types/alpha.d.ts` + templates. Its exact-version `optionalDependencies` `@alpha-star/engine-win32-x64` / `@alpha-star/engine-linux-x64` hold the desktop engine (release for run/test/mcp + glslang + `shipping/` for players; npm installs only the one for the machine); `@alpha-star/engine-android-arm64` holds the prebuilt Android engine (installed on every OS; `alpha export --target android-arm64` needs no NDK). All are built by GitHub Actions and share one version and game.jsbc key.

```
npx @alpha-star/sdk init my-game --template platformer   # new game (blank|platformer|topdown|puzzle|shmup|shaders)
cd my-game && npm install
npm test                                                 # headless tests
npm start                                                # play
npm install --save-dev @alpha-star/sdk                   # add to an existing project
npm install --global @alpha-star/sdk                     # alpha on PATH
npx alpha --version                                      # SDK version (engine has the same)
npx alpha skills                                         # install agent skills (.claude/skills, .agents/skills, ...)
npm install --save-dev @alpha-star/sdk@latest            # upgrade (engine follows)
```

- Never `--omit=optional` / `--no-optional`: no engine is installed.
- macOS, Linux arm64, others: no engine package. `alpha check` / `alpha export` (packs) still work; for run/test build the engine from source (repo BUILDING.md) and pass `--engine <path>` or set `ALPHA_STAR`.
- Engine lookup order: `--engine` / `$ALPHA_STAR`, then `@alpha-star/engine-<platform>-<arch>`, then `alpha-sdk/engine/<platform>-<arch>/` in a repo checkout.
- Errors about missing options or bytecode format: engine and SDK versions differ (troubleshooting.md).