# Hot reload

```
npx alpha run --watch
```

Every save re-exports the pack (200 ms after the last change; `dist/`, `build/`, `node_modules/`
and `.git/` are not watched) and the running engine reloads the game. A type error is printed and
the pack is not rewritten, so the game keeps running the last good code.

What survives a reload: the save; the current scene and its params (if plain JSON); `ctx.state`
if it is plain JSON and the scene still declares `state`. The world is rebuilt: `enter` spawns the
entities again. Keep long-lived values in `ctx.state`, not in entity data.

A reload that fails while booting the new code keeps the old game running and shows the error.
Agents reload on demand with the MCP tool `game_reload` instead (an agent-driven engine does not
watch files). Shipping engines have no hot reload.
