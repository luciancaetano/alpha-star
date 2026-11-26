# @alpha-star/sdk

[Game showcase and example slider](https://luciancaetano.github.io/alpha-star/) | [Getting started](https://luciancaetano.github.io/alpha-star/getting-started.html) | [API reference](https://luciancaetano.github.io/alpha-star/reference.html)

Types and the `alpha` CLI for alpha-star games (TypeScript, bundled to one JS
module the engine runs). Run every command from the game project folder.

## Install

Needs Node.js 20+. npm also installs the prebuilt engine for your machine (Windows x64, Linux x64)
and the prebuilt Android engine (`alpha export --target android-arm64` needs no NDK or engine source).

```
npx @alpha-star/sdk init my-game --template platformer   # new game
cd my-game && npm install && npm test && npm start

npm install --save-dev @alpha-star/sdk                   # existing project
npm install --global @alpha-star/sdk                     # `alpha` on your PATH
npx alpha --version                                      # SDK version (= engine version)
npx alpha skills                                         # agent skills
```

Do not install with `--omit=optional`: that skips the engine. Other platforms: build the engine
from source and pass `--engine <path>` or set `ALPHA_STAR` (engine packages are described below).

## Commands

```
alpha init <dir> [--template t]     new project (+ .mcp.json): blank | platformer | topdown | puzzle | shmup | shaders
alpha --version                     SDK version (= the bundled engines' version)
alpha check                         type-check only
alpha export [--out dist]           type-check, bundle, copy assets into a pack
alpha export --standalone           ... plus the player engine (runs by itself); the game ships as encrypted game.jsbc
alpha export --no-atlas             ... without packing images into atlas pages
alpha run [-- engine args]          export, then open the pack in the engine
alpha run --watch                   ... and re-export on every change: the game hot-reloads
alpha run -- --platform android     preview the phone UI on the desktop (ctx.platform)
alpha export --target android-arm64 --out android [--variant debug|release|bundle] [--no-build]
                                    Android project; debug = APK with dev tools (shake the phone
                                    or Menu key), release = APK, bundle = AAB for Google Play
alpha run --target android-arm64 --out android [--device SERIAL]
                                    debug APK on the USB phone, then its log (docs/guide/android.md)
alpha test                          run test/main.test.ts headlessly (simulate + expect)
alpha mcp                           MCP server for AI agents (below)
alpha skills [--agents a,b] [--list] install/update the SDK's agent skills (asks which agents;
                                    Enter = .claude/skills + .agents/skills)
```

`alpha init my-game --template shmup && cd my-game && npm install && npm test && npm start`:
every template is a small complete game with tests, generated CC0 art and an `AGENTS.md` for AI
agents (`--sdk <npm spec>` writes another SDK dependency, e.g. `file:../alpha-sdk`). The docs
site (`docs/site/` in the repo, `npm run docs` here) has the guides and the API reference
generated from `types/alpha.d.ts`.

Engines come in per-platform packages, like esbuild: `@alpha-star/engine-win32-x64` and
`@alpha-star/engine-linux-x64` are `optionalDependencies` of this package with the exact same
version, and npm installs only the one matching your machine (`os`/`cpu`). Each holds the release
engine (`run`/`test`/`mcp`) and `shipping/` (players). In an alpha-star checkout the SDK uses
`engine/<platform>-<arch>/` instead (staged by `tools/stage_engine.py`). Override with
`--engine <path>` or `$ALPHA_STAR`.

## Sprites, camera and LDtk levels

```ts
assets: {
  sheets: { hero: { image: "img/hero.png", frame: [16, 16],
                    animations: { run: { frames: [1, 2, 3, 4], fps: 12 }, jump: { frames: [5], loop: false } } } },
  worlds: { main: "levels/world.ldtk" },  // LDtk project (1.0+); tileset images, .ldtkl levels
}
const hero = ctx.spawn({ tags: ["player"], pos: [40, 100], shape: { sheet: "hero", animation: "run" } });
hero.animation = "jump";          // switch (same name keeps playing); hero.play("run") restarts
hero.flipX = true;
const level = ctx.spawnLevel("main", "Level_0", {
  collide: { Walls: "walls" },                                // IntGrid -> static boxes tagged "walls"
  entities: { Coin: (e) => coin(e.pos) } });                  // LDtk entity -> spawn spec
level.setInt("Walls", 3, 7, 0);                               // edit a cell; colliders rebuild
ctx.world("main").levelAt([400, 100]);                        // data only
ctx.camera.follow(hero, { lerp: 0.15, deadzone: [48, 32] });
ctx.camera.bounds([0, 0, ...level.size]);
ctx.camera.shake(3, 0.25);
ctx.camera.toWorld(ctx.input.mouse());
```

Declare the names in `GameTypes`: `sheets: { hero: "run" | "jump" }`, `worlds: "main"`
(both optional). Animations and the camera advance on the fixed step, so `simulate`
runs stay deterministic; `expect.animation("player").equals("run")` checks one.
`alpha export` packs PNG images and sheets into `alpha-atlas-N.png` pages (fewer draw
calls; transparent to game code) and copies `.ldtk` projects with their tileset images and `.ldtkl` levels.

## Particles, looks, layers, world text, window

```ts
// One way to make particles: an `emitter` on a spawn spec (data). It rides on its entity.
ctx.spawn({ tags: ["dust"], pos: feet, layer: "fx",
  emitter: { burst: 10, oneShot: true, lifetime: [0.3, 0.5], speed: [20, 50], angle: [190, 350],
             gravity: [0, 60], size: [5, 2], color: ["#b89a6cff", "#b89a6c00"], shape: "rect" } });
bird.emit(8);                         // burst on an entity spawned with an emitter
e.tint = "#ff000080"; e.alpha = 0.5;  // any shape: tint, alpha, blend "alpha" | "add" | "multiply"
ctx.spawn({ tags: ["popup"], pos, shape: { text: "+1", font: "pixel", size: 8, color: "#ffe14d" } });
```

`config.layers: { far: { z: -100, parallax: 0.2 }, fx: { z: 100 } }` + `layer: "far"` on specs
(and `spawnLevel(world, level, { layer, layers })` for tile layers). `config.window: { fullscreen?, aspect?:
"expand" | "keep_width" | "keep_height" | "keep", scale?: "fractional" | "integer", resizable?, altEnter? }`
(Godot-style: the default `expand` fills any screen with more world, never bars); `ctx.camera.view`
([x, y, w, h] the world the screen shows: size scenery and spawns to it); `ctx.window.fullscreen`,
`ctx.window.size`; `ctx.safeArea` ([x, y, w, h] the display cutout and rounded corners leave
visible; phones always run fullscreen, immersive; UI elements with `class="safe"` are padded by it);
`simulate(game, { size: [1080, 2340] })` tests a tall phone. Particles run on the fixed step with the game's seeded random, so
`simulate(...).expect.particles().equals(n)` is exact. MCP `game_state` entities carry `layer`,
`alpha`, `text` and `particles`.

## Custom shaders

`assets.shaders: { wave: { file: "shaders/wave.frag", uniforms: { amount: 0.5 } } }`, then
`ctx.postfx.add("wave", { amount: 1 })` like a built-in effect. One Vulkan GLSL fragment shader (`#version 450`,
entry `main`, frame `sampler2D` at `set = 2, binding = 0`, float-only `std140` uniform block at `set = 3, binding = 0`
in `uniforms` order, at most 16). `alpha run` compiles it with glslang (staged with the engine) on load and on every
save (and shadercross turns the SPIR-V into MSL for Metal); `alpha export` packs `.spv` + `.msl` instead of the `.frag`. Runs on Vulkan (Windows, Linux, Android; macOS/iOS not yet);
elsewhere the effect is skipped with a warning. Details: the shaders guide and `llm/shaders.md`.

## Texture filtering (pixelMode)

`config.pixelMode` (default `true`) is the game-wide filtering of images when scaled: `true` =
crisp pixel art (nearest), `false` = smooth (linear). Images, sheets and maps override it with
`filter: "nearest" | "linear" | "sharp"` (`images: { logo: { file: "img/logo.png", filter: "linear" } }`,
`sheets.hero.filter`, `maps.level1: { file, filter }` for its tilesets); `pixelMode` on them is an
error. Fonts keep `fonts.title: { file, pixelMode }` (world-text glyph atlas).
Particles with an image or sheet shape use their source's mode; circle/rect particles get antialiased edges
with `pixelMode: false` (override per emitter with `emitter.smooth`). `alpha export` puts crisp
and smooth images on separate atlas pages (`alpha-atlas-N.png`, `alpha-atlas-smooth-N.png`).
Game UI images (RmlUi `<img>`, `decorator: image(...)`) use `config.uiFilter` instead
(`"nearest"`, `"linear"` or `"sharp"`; default follows `pixelMode`).

## Fonts and sharp text

`assets.fonts.name`: a `.ttf`/`.otf` file or an AngelCode BMFont `.fnt` (text or XML; its PNG
pages are packed), as `"file"` or `{ file, pixelMode?, license?, fallbacks? }`. The name is the `font-family`
in `.rcss` and `font` of world text. Text is rasterized at the screen's real resolution (window
scale, high-DPI, camera zoom), so it is sharp at any window size; bitmap fonts draw at whole
multiples of their size; `pixelMode: true` renders TrueType monochrome (pixel fonts stay crisp;
for an 8-px-grid font, sizes where size x window scale is a multiple of 8 look best). No fonts at
all: UI and world text use the built-in 8x8 font (public domain). Any language: TrueType/OpenType
text is shaped by HarfBuzz (ligatures, Arabic joining, Devanagari), ordered right to left where
needed (Unicode bidi; world text `dir: "auto" | "ltr" | "rtl"`, RCSS `direction`), and wraps at
world text `width` (after spaces, between CJK characters). `fallbacks: ["arabic", "cjk"]` names
other fonts tried per character the font lacks; what none has draws as a box with a log warning.
Invalid Unicode in displayed text (an unpaired surrogate) is an error naming the byte.

`alpha export` writes `THIRD_PARTY_NOTICES.txt` into every pack (engine dependencies incl. the
FreeType License credit) plus the text of each font's `license` file. The font's own license is
what matters: SIL OFL is fine; a bitmap font made from a font follows that font's license.

## Physics

```ts
config: { gravity: [0, 900], collision: { categories: ["hero", "enemy", "world"] } }
ctx.spawn({ tags: ["hero"], pos, body: { type: "dynamic", fixedRotation: true, gravityScale: 1,
  bullet: false, category: "hero", collidesWith: ["world", "enemy"],
  collider: { rect: [12, 16], friction: 0.6, restitution: 0, density: 1 } } });
ctx.spawn({ tags: ["plank"], pos, body: { type: "static", collider: { rect: [48, 6], oneWay: true } } });
ctx.onHit("hero", "enemy", (h, e, contact) => contact.speed > 300 && hurt(h));  // normal h -> e, point
ctx.onHitEnd("hero", "world", (h, w) => {}); ctx.onExit("hero", "zone", (h, z) => {});
const hit = ctx.raycast([x, y], [x, y + 12], { tags: ["walls"] });  // { entity, point, normal, fraction } | null
ctx.raycastAll(from, to, { categories: ["enemy"] }); ctx.queryCircle(c, r); ctx.queryRect([x, y, w, h]);
const rope = ctx.joint({ type: "rope", a: hook, b: crate, length: 80 });  // revolute | distance | weld | prismatic
rope.destroy();
hero.body.applyImpulse([0, -hero.body.mass * 300]); hero.body.angularVelocity = 90;
```

Raycasts ignore sensors; area queries include them. Joints and queries are scene-scoped.
Units are pixels, degrees and kg (an impulse of `mass * dv` changes the velocity by `dv` px/s).

## Tweens, vectors, parent/child, entity data, prefabs

```ts
import { vec } from "alpha";  // add sub scale len norm dot lerp angle fromAngle dist on [x, y]
ctx.tween(lift, { to: { x: 470 }, seconds: 2.5, ease: "sineInOut", yoyo: true, repeat: Infinity });
ctx.tween(coin, { to: { scale: 2.5, alpha: 0 }, seconds: 0.25, onDone: () => ctx.destroy(coin) });
const sword = ctx.spawn({ tags: ["sword"], pos: [8, 0], parent: hero });  // pos local; follows; dies with hero
const e = ctx.spawn({ ...enemy(at), data: { hp: 3 } }); e.data.hp -= 1;       // JSON-safe, seen by MCP
```

Tweens run on the fixed step (deterministic), cleared on scene change; entities tween
`x y rotation alpha scale` (a kinematic body's x/y moves by velocity so riders are carried),
plain objects tween their number keys. Eases: `linear` and `quad cubic quart quint sine expo circ
back elastic bounce` + `In | Out | InOut`. Prefabs stay plain functions returning a spawn spec
(see `examples/platformer/src/prefabs`). Entity data is re-created by the scene's `enter` on hot
reload (like every entity); keep long-lived values in `ctx.state`. Tests read
`simulate(...).entities(tag)` (pos, rotation, scale, alpha, data, velocity) and `.state`.
MCP `game_state` adds `data` per entity and `joints`.

## Audio, saves, settings, localization, accessibility

```ts
assets: {
  sounds: { jump: "sfx/jump.ogg", line: { file: "vo/line1.mp3", bus: "voice" }, beep: { wave: "sine", freq: 880, duration: 0.1 } },
  music: { theme: "music/theme.ogg" },                       // streamed, never loaded whole
  locales: { en: "locales/en.json", pt: "locales/pt.json" },
}
config: { audio: { buses: ["ambience"] }, save: { version: 2, migrate: (d, from) => ({ ...d, gold: d.coins }) }, locale: "en" }

ctx.audio.play("jump", { pan: -0.5 });                       // -1 left .. 1 right
ctx.audio.music("theme", { loop: true, volume: 0.6, fadeIn: 1 });
ctx.audio.crossfade("boss", 2); ctx.audio.stopMusic(1.5); ctx.audio.musicPlaying;
ctx.audio.bus("music").volume = 0.5;                         // master / music / sfx / voice / config.audio.buses
ctx.save.set("best", 9); ctx.save.slot("slot2").get("level", 1);
ctx.locale = "pt"; ctx.t("coins", { count: 3 });            // "{count} moedas" via { one, other }
ctx.input.rebind("jump", ["W", "GamepadA"]);
ctx.settings.uiScale = 1.5; ctx.settings.highContrast = true; ctx.settings.reducedMotion = true;
```

Sounds (.wav/.ogg/.mp3) are decoded at boot; music is decoded a chunk at a time on the main
thread while it plays (~0.1 ms per 60 Hz frame in a debug build), one track at a time (crossfades
overlap two). Audio is output only: simulations never depend on it. Saves are versioned slot files
(`save.json`, `save-<slot>.json`) written atomically (temp file + rename, previous file kept as
`.bak`; a damaged file falls back to its `.bak` with a logged warning). Settings live in
`settings.json` (bus volumes, locale, fullscreen, rebinds, uiScale, highContrast, reducedMotion)
and are applied at boot; a value the game can no longer use is skipped with a warning.
RmlUi elements with `data-t="key"` show `ctx.t(key)` and re-translate when the locale changes;
`.high-contrast` is set on every document body; `uiScale` is RmlUi's dp ratio (size in `dp`).
Reduced motion scales camera shake and particle counts to 1/4. Locales may use any script (fonts
with `fallbacks`). Tests: `simulate(game, { settings: { locale: "pt" }, saves: { main: {...} } })`,
then `run.music`, `run.locale`, `run.settings`, `run.saves`, `expect.music("theme")`. MCP
`game_state` adds `locale` and `music`.

## Using alpha-star with an AI agent (MCP)

`alpha mcp` is a [Model Context Protocol](https://modelcontextprotocol.io) server
on stdio. It lets an agent build and test the game, then start it and play it:
send input, step frame by frame, read the state, take screenshots, read logs.

Claude Code: `alpha init` writes this `.mcp.json` in the game folder; for an older project add it (or `claude mcp add alpha -- npx alpha mcp`):

```json
{
  "mcpServers": {
    "alpha": { "command": "npx", "args": ["alpha", "mcp"] }
  }
}
```

Any other MCP client: run `npx alpha mcp` with the game folder as the working
directory (`{"command": "npx", "args": ["alpha", "mcp"], "cwd": "<game folder>"}`).

| Tool | What it does |
|---|---|
| `alpha_check` | Type-check; returns `no problems` or `file:line:col message` lines |
| `alpha_export` | Build the pack (`out`, `standalone`) |
| `alpha_test` | Run the headless tests; PASS/FAIL per test |
| `game_start` | Export + launch the game with the control channel (`hidden` for no window); returns the state |
| `game_state` | Scene, paused, script error (source-mapped), frame, fps, entities (id, tags, pos, rotation, z, velocity), UI documents and their texts |
| `game_step` | Pause and advance exactly N fixed steps (1/60 s), return the state |
| `game_pause` / `game_resume` | Stop / restart real-time play |
| `game_input` | `press` (one step), `hold`, `release` an action, or set an `axis` (-1..1), per player 1..4 |
| `game_click` | Click a UI element: `"document#elementId"` |
| `game_screenshot` | PNG of the current frame |
| `game_logs` | console lines and script errors since a cursor |
| `game_reload` | Re-export and hot-reload the running game (keeps scene, params, JSON `ctx.state`, save); type/load errors change nothing and come back as the error |
| `game_stop` | Close the game (also done when the server exits) |

A typical loop: edit code â†’ `alpha_check` â†’ `alpha_test` â†’ `game_start` â†’
`game_pause` â†’ `game_input` + `game_step` â†’ `game_state` / `game_screenshot` â†’
fix â†’ repeat. Input goes through the game's own actions, exactly like a device,
and `game_step` is deterministic (fixed step, seeded random).

## Hot reload

Change the code or an asset while the game runs and see it without restarting:

- `alpha run --watch`: exports, opens the game, then watches the project (everything but
  `dist/`, `build/`, `node_modules/`, `.git/`) and re-exports 200 ms after the last change.
  The engine (release/dev builds) watches its pack (~4 polls/s, reloads once the pack has
  been quiet for 0.3 s, so one export = one reload). Type errors are printed and the pack is
  not rewritten, so the running game keeps working. `--watch` is opt-in: plain `alpha run`
  stays "export, run, exit with the game's code" for scripts.
- MCP `game_reload`: the same, on demand, for an agent. Under an agent (`--control`) the
  engine does not watch files, so an export never restarts a scene in the middle of a
  `game_step` sequence; the agent decides when to reload.

What survives a reload: the save; the current scene and its `enter` params (if they are
plain JSON, otherwise the game restarts at `start`); `ctx.state` if it is plain JSON (no
functions, class instances, `undefined`, NaN/Infinity) and the new scene still declares
`state` - otherwise `state(params)` runs again. The world is rebuilt: the scene's `enter`
spawns its entities again. Window size and title are fixed at launch. A reload that fails
(syntax, type or runtime error while booting the new code) keeps the old game running and
reports the error in the terminal, as an overlay on the game and in the MCP reply.
Shipping engines have no hot reload.

### How it works (and why it is safe)

`game_start` launches the release engine as `alpha_star <pack> --control <port>`.
The engine serves a small line-delimited JSON protocol (`alpha-control`, see
`src/mcp/dispatch.hpp` in the engine repo) on **127.0.0.1 only**: connections
from any other address are closed, one client at a time, and every connection
must first present a random token that `alpha mcp` passes to the engine in
`ALPHA_CONTROL_TOKEN` (so other local programs cannot drive the game). Requests
are served on the engine's main thread between frames. The control channel
exists only in dev and release engines; the shipping engine players get
(`alpha export --standalone`) contains none of it.
