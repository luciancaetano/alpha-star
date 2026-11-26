# Input

**What you'll build:** a circle that walks in eight directions, dashes on Space, charges a shot
while J (or the right mouse button) is held and fires it toward the mouse on release, and marks
every click. It works the same on keyboard, mouse, gamepad and touch.

Games read **actions** (buttons) and **axes** (analog values), never devices. `config.actions` and
`config.axes` map them to inputs. Replace `src/main.ts` with this and run `npm start`:

```ts
import { define, vec } from "alpha";

interface Arena {
  actions: "dash" | "fire" | "charge";
  axes: "moveX" | "moveY";
  sounds: never;
  images: never;
  tags: { hero: {}; shot: { vx: number; vy: number }; mark: {} };
  ui: {};
  scenes: { play: undefined };
}

const { scene, game } = define<Arena>();

const play = scene("play", {
  state: () => ({ dash: 0, charge: 0 }),

  enter(ctx) {
    ctx.spawn({ tags: ["hero"], pos: [160, 90], shape: { circle: 8, color: "#29adff" } });
  },

  update(ctx) {
    const hero = ctx.find("hero");
    // Axes: -1..1. Normalize so diagonals are not faster.
    const dir = vec.norm([ctx.input.axis("moveX"), ctx.input.axis("moveY")]);
    // Actions: pressed = this step only, down = while held, released = the step it goes up.
    if (ctx.input.pressed("dash")) ctx.state.dash = 0.15;
    ctx.state.dash = Math.max(0, ctx.state.dash - ctx.dt);
    const speed = ctx.state.dash > 0 ? 400 : 100;
    hero.pos.x += dir[0] * speed * ctx.dt;
    hero.pos.y += dir[1] * speed * ctx.dt;

    // Hold to charge, release to shoot toward the mouse (bigger when charged longer).
    if (ctx.input.down("charge")) ctx.state.charge = Math.min(1, ctx.state.charge + ctx.dt);
    if (ctx.input.released("charge")) {
      const target = ctx.camera.toWorld(ctx.input.mouse());
      const aim = vec.norm(vec.sub(target, [hero.pos.x, hero.pos.y]));
      ctx.spawn({ tags: ["shot"], pos: [hero.pos.x, hero.pos.y], shape: { circle: 2 + ctx.state.charge * 6, color: "#ffec27" }, data: { vx: aim[0] * 200, vy: aim[1] * 200 } });
      ctx.state.charge = 0;
      ctx.input.rumble(0.4, 0.1);
    }
    // A click marks the spot under the mouse.
    if (ctx.input.pressed("fire")) ctx.spawn({ tags: ["mark"], pos: ctx.camera.toWorld(ctx.input.mouse()), shape: { rect: [4, 4], color: "#ff004d" } });

    for (const s of ctx.findAll("shot")) {
      s.pos.x += s.data.vx * ctx.dt;
      s.pos.y += s.data.vy * ctx.dt;
      if (Math.abs(s.pos.x - 160) > 200 || Math.abs(s.pos.y - 90) > 120) ctx.destroy(s);
    }
  },
});

export default game({
  config: {
    title: "Input",
    size: [320, 180],
    background: "#1d2b53",
    seed: 1,
    actions: {
      dash: ["Space", "LeftShift", "GamepadA"],
      fire: ["MouseLeft", "Touch"],
      charge: ["J", "MouseRight", "GamepadRT"],
    },
    axes: {
      moveX: ["GamepadLeftX", "A/D", "Left/Right"],
      moveY: ["GamepadLeftY", "W/S", "Up/Down"],
    },
  },
  start: "play",
  scenes: { play },
  assets: { sounds: {} },
});
```

Expected: WASD, the arrows or the left stick move the circle at the same speed in every
direction; Space bursts it forward; holding J grows the next shot; clicks drop red marks.

## Step by step

### Actions: buttons

```ts
// types: actions: "jump" | "fire";
config: {
  actions: { jump: ["Space", "W", "Up", "GamepadA"], fire: ["MouseLeft", "GamepadRT"] },
}
```

```ts
ctx.input.pressed("jump");   // true only on the step it went down: jumps, menu choices
ctx.input.down("fire");      // true while held: auto-fire, charging, running
ctx.input.released("jump");  // true only on the step it went up: variable jump height, release to shoot
```

Expected: one key press gives exactly one `pressed` step, however long it is held. Any input in
the list triggers the action; every player's devices count on `ctx.input` itself.

### Axes: analog values

```ts
// types: axes: "moveX" | "moveY" | "throttle";
config: {
  axes: {
    moveX: ["GamepadLeftX", "A/D", "Left/Right"],   // stick, or a "Negative/Positive" key pair
    moveY: ["GamepadLeftY", "W/S", "Up/Down"],
    throttle: ["GamepadRT"],                          // triggers: 0..1
  },
  deadzone: 0.2,                                      // stick and trigger deadzone (default 0.2)
}
```

```ts
const x = ctx.input.axis("moveX");   // -1..1: +x right
const y = ctx.input.axis("moveY");   // -1..1: +y down
```

Expected: keys give exactly -1, 0 or 1; a stick gives anything in between. The axis reads the
strongest source. Diagonal keys give `[1, 1]`: normalize with `vec.norm` for equal speed.

### Mouse and touch

```ts
const screen = ctx.input.mouse();           // game pixels on screen
const world = ctx.camera.toWorld(screen);   // where that is in the world (camera moved/zoomed)
ctx.find("cursor").pos.x = world[0];
ctx.find("cursor").pos.y = world[1];
```

Buttons are actions: `"MouseLeft"`, `"MouseRight"`, `"MouseMiddle"`. On touch screens `"Touch"`
is any finger, `"TouchLeft"` / `"TouchRight"` a finger on that half of the screen, and a finger is
also `"MouseLeft"` at `ctx.input.mouse()`. `"Back"` is the Android back button. Every finger:
`ctx.input.touches()` (`{ id, pos, pressed, released }`, game pixels). Fingers a touch stick or
button took (below) are none of these.

### Touch sticks and buttons (phones)

On-screen controls are config, not code: each one becomes an input source, so keyboard, gamepad
and touch drive the same actions and axes.

```ts
export const config = {
  touchSticks: {
    move: { zone: "left" },                                   // default look
    aim: { zone: "right", mode: "fixed",
           skin: { base: "ui/stick_base", knob: "ui/stick_knob", tint: "#ffd27fcc" } },
  },
  touchButtons: {
    jump: { pos: [0.88, 0.8], label: "A" },                     // fractions of ctx.safeArea
    dash: { pos: [0.75, 0.88], skin: { up: "ui/btn_dash", down: "ui/btn_dash_down" } },
  },
  axes: {
    moveX: ["A/D", "GamepadLeftX", "TouchMoveX"],
    moveY: ["W/S", "GamepadLeftY", "TouchMoveY"],
  },
  actions: { jump: ["Space", "GamepadA", "TouchJump"], fire: ["MouseLeft", "TouchAimActive"] },
};
```

Expected: on a phone a finger on the left half grows a stick where it lands and `moveX`/`moveY`
follow it; tapping the "A" circle jumps. `update` reads `ctx.input.axis("moveX")` as before.

- Sources of a stick `move`: `TouchMoveX` / `TouchMoveY` (axes, -1..1, +y down), `TouchMoveX-`,
  `X+`, `Y-`, `Y+` (past 0.5) and `TouchMoveActive` (inputs). A button `jump`: `TouchJump`.
- Stick fields: `zone` (`"left"`, `"right"` or `[x, y, w, h]` fractions of the safe area), `mode`
  (`"floating"` default / `"fixed"` at `anchor`), `radius` (64), `deadzone` (0.15), `axes`
  (`"both"`, `"x"`, `"y"`), `snap` (0, 4, 8), `visible` (`"touching"` / `"always"`), `opacity`
  (0.6, 1 while touched), `skin` (`base`, `knob`, `baseSize`, `knobSize`, `pressed: { base, knob }`,
  `tint`), `player`, `scenes` (e.g. `["play"]`: only there; default every scene).
- Button fields: `pos`, `radius` (40), `hitSlop` (8), `label`, `visible` (`"always"` / `"never"`),
  `opacity`, `skin` (`up`, `down` (default `up` darkened), `size`, `tint`), `player`, `scenes`.
- Skin images are `assets.images` names or `"sheet:frame"`; a missing one fails at boot with a
  "did you mean" hint.
- Mobile only: on desktop they neither draw nor capture; `npx alpha run -- --platform android`
  previews them (with a touch screen).
- Each finger belongs to the first control it lands on (game UI first, then buttons, then sticks
  in declaration order) until it lifts: dragging off a button keeps it pressed. Pausing the app
  lets every control go. At most 4 sticks and 12 buttons; overlapping buttons are a config error.

### Several players

```ts
for (let n = 1; n <= ctx.input.players(); n++) {
  const p = ctx.input.player(n as Player);         // import type { Player } from "alpha"
  if (p.pressed("jump")) console.log(`player ${n} jumped`);
  p.axis("moveX");
}
```

Player 1 has the keyboard, the mouse and the first gamepad; each further gamepad is the next
player, and a reconnected pad gets its slot back. On `ctx.input` itself an action is any player's
and an axis is the strongest one. See `examples/topdown` for drop-in co-op.

### Rumble

```ts
ctx.input.rumble(0.8, 0.3, 2);   // strength 0..1, seconds 0..10, player 2
ctx.input.rumble(0.5, 0.2);      // every player
ctx.input.rumble(0, 0);          // stop
```

No-op without a gamepad, so call it freely.

### Rebinding (a controls menu)

```ts
ctx.input.rebind("jump", ["W", "GamepadA"]);   // replaces config.actions' list; saved in settings
ctx.input.bindings("jump");                    // ["W", "GamepadA"]
ctx.settings.reset();                          // everything back to defaults (bindings too)
```

Bindings persist across restarts and are applied at boot. Build the menu with [UI](ui.md).

### Platform hints

```ts
const hint = ctx.platform.mobile ? "tap to jump" : "press Space to jump";
ctx.spawn({ tags: ["label"], pos: [160, 20], shape: { text: hint } });
```

`ctx.platform` is `{ os, mobile }`, for hints and layout only, never game rules. Preview with
`npx alpha run -- --platform android`.

### Input in tests

```ts
simulate(game, { seconds: 2, inputs: { jump: [30, 60], axes: { moveX: [[0, 1], [90, 0]] } } });
simulate(game, { seconds: 2, inputs: [{ fire: [0] }, { axes: { moveX: [[0, -1]] } }] });  // players 1, 2
```

Action frames press for one step each (frame 30 = 0.5 s); axis entries hold a value from that
frame on. Fingers go through the touch sticks and buttons like on a phone:

```ts
simulate(game, { seconds: 1, platform: "android",
  touches: [{ id: 1, path: [[0, [80, 300]], [10, [140, 300]], [40, null]] }] });  // [frame, pos | null = lift]
```

See [Testing](testing.md).

## Recipes

### Coyote time and jump buffering

Accept a jump pressed a little before landing, or a little after walking off a ledge:

```ts
// state: () => ({ coyote: 0, buffer: 0 }); `grounded` from a ground check (see Physics)
const s = ctx.state;
s.coyote = grounded ? 0.1 : Math.max(0, s.coyote - ctx.dt);
s.buffer = ctx.input.pressed("jump") ? 0.1 : Math.max(0, s.buffer - ctx.dt);
if (s.buffer > 0 && s.coyote > 0) {
  s.buffer = 0;
  s.coyote = 0;
  hero.body.velocity = [hero.body.velocity.x, -300];
}
```

### Variable jump height

```ts
const hero = ctx.find("hero");
if (ctx.input.released("jump") && hero.body.velocity.y < 0) {
  hero.body.velocity = [hero.body.velocity.x, hero.body.velocity.y * 0.5];   // cut the rise short
}
```

### Grid movement (one cell per step, held keys repeat)

```ts
// actions: "left" | "right" | "up" | "down"; state: () => ({ cell: [0, 0] as [number, number], moveCooldown: 0 })
const s = ctx.state;
s.moveCooldown = Math.max(0, s.moveCooldown - ctx.dt);
const dx = ctx.input.down("right") ? 1 : ctx.input.down("left") ? -1 : 0;
const dy = dx !== 0 ? 0 : ctx.input.down("down") ? 1 : ctx.input.down("up") ? -1 : 0;
if ((dx || dy) && s.moveCooldown === 0) {
  s.cell = [s.cell[0] + dx, s.cell[1] + dy];
  s.moveCooldown = 0.15;
  const hero = ctx.find("hero");
  ctx.tween(hero, { to: { x: s.cell[0] * 16 + 8, y: s.cell[1] * 16 + 8 }, seconds: 0.12, ease: "quadOut" });
}
```

### Twin-stick aim

```ts
// axes: aimX: ["GamepadRightX"], aimY: ["GamepadRightY"]
const aim: Vec2 = [ctx.input.axis("aimX"), ctx.input.axis("aimY")];
if (vec.len(aim) > 0.3) ctx.find("hero").rotation = vec.angle(aim);
```

### Click to select

```ts
if (ctx.input.pressed("fire")) {
  const [hit] = ctx.queryCircle(ctx.camera.toWorld(ctx.input.mouse()), 1, { tags: ["hero"] });
  if (hit) hit.tint = "#ffec27";
}
```

Area queries see colliders, so the clickable entity needs a `body` (a `sensor` is enough; see
[Physics](physics.md)).

### Back / pause button on every platform

```ts
config: { actions: { back: ["Escape", "Back", "GamepadStart"] } }
// in update:
if (ctx.input.pressed("back")) ctx.go("title");
```

## Every input name

| Kind | Names |
|---|---|
| Keys | `A`..`Z`, `0`..`9`, `Space`, `Enter`, `Escape`, `Tab`, `Backspace`, `Left`, `Right`, `Up`, `Down`, `LeftShift`, `RightShift`, `LeftCtrl`, `RightCtrl` |
| Mouse | `MouseLeft`, `MouseRight`, `MouseMiddle` |
| Gamepad buttons | `GamepadA`, `GamepadB`, `GamepadX`, `GamepadY`, `GamepadStart`, `GamepadBack`, `GamepadUp`, `GamepadDown`, `GamepadLeft`, `GamepadRight`, `GamepadLB`, `GamepadRB`, `GamepadLS`, `GamepadRS`, `GamepadLT`, `GamepadRT` |
| Sticks as buttons | `GamepadLeftX-`, `GamepadLeftX+`, `GamepadLeftY-` (up), `GamepadLeftY+`, `GamepadRightX-`, `GamepadRightX+`, `GamepadRightY-`, `GamepadRightY+` |
| Touch / phone | `Touch`, `TouchLeft`, `TouchRight`, `Back`; touch controls: `Touch<Button>`, `Touch<Stick>X-`..`Y+`, `Touch<Stick>Active` |
| Axis sources | `GamepadLeftX`, `GamepadLeftY`, `GamepadRightX`, `GamepadRightY`, `GamepadLT`, `GamepadRT`, a key pair `"Neg/Pos"` such as `"A/D"`, or a touch stick's `Touch<Stick>X` / `Touch<Stick>Y` |

A misspelled name is an error at boot that lists every valid one.

## Gotchas

- Read `pressed` / `released` in `update`: inside `ctx.every` or `ctx.after` you will miss them.
- `config.actions` must map every declared action; the action name `axes` is reserved.
- What the game UI used (a clicked button, menu navigation) never also triggers game actions.
- `ctx.input.mouse()` is screen space: use `ctx.camera.toWorld` once the camera moves or zooms.
- Use `ctx.platform` for hints only; the simulation must be the same on every OS.

## See also

- [Physics](physics.md): ground checks and jumping with bodies.
- [UI](ui.md): menus, focus and gamepad navigation; [Android](android.md): touch and the back button.
- [Testing](testing.md): scripted inputs; `examples/topdown`: two players and rumble.
