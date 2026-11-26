# AGENTS.md — Ghosts (alpha-star game)

A small online example for `alpha:net`: you move a square; its position goes to a
WebSocket server (`SERVER` in `src/main.ts`) ten times a second, and every `{ id, x, y }`
the server relays is drawn as a ghost. With no server the game plays alone.

The API types live in `../../alpha-sdk/types/alpha.d.ts` (module `"alpha:net"`); the
networking guide is `../../alpha-sdk/llm/net.md`.

```
npm install      # once
npm run check    # type-check
npm test         # headless tests against netFake servers (no real network)
npm run server   # local relay on ws://localhost:9000/ghosts (server/relay.mjs, plain Node)
npm start        # run it; start it twice to see the other one as a ghost
```

Rules: open sockets in `enter`, read with `receive()` every step, never trust what a
server sends (parse defensively), and test with `netFake` from `alpha/test`.
