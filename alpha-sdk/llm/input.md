# Input

> summary: Actions and axes (config.actions/axes), device input names, mouse/touch, on-screen touch sticks and buttons (config.touchSticks/touchButtons), ctx.platform hints, multiple players, rumble and rebinding; read when wiring controls or reading player input.

## API

```ts
// types.ts
actions: "jump" | "fire" | "back";
axes: "moveX" | "moveY";                     // optional
// config
actions: { jump: ["Space", "W", "GamepadA"], fire: ["MouseLeft", "GamepadRT"], back: ["Escape", "Back"] },
axes: { moveX: ["GamepadLeftX", "A/D", "Left/Right"], moveY: ["GamepadLeftY", "W/S", "Up/Down"] },
deadzone: 0.2,                               // 0 <= d < 1, sticks (radial) and triggers

ctx.input.pressed(action)    // only on the step it went down
ctx.input.down(action)       // while held
ctx.input.released(action)   // only on the step it went up
ctx.input.axis(name)         // [-1, 1] after deadzone; +x right, +y down; triggers 0..1
ctx.input.mouse(): Vec2      // game pixels, screen space; ctx.camera.toWorld(...) for world
ctx.input.touches()          // [{ id, pos, pressed, released }]: fingers no UI/touch control took
ctx.input.player(n: 1|2|3|4) // PlayerInput: pressed/down/released/axis for one player
ctx.input.players(): number  // 1 (keyboard) up to highest connected gamepad's player
ctx.input.rumble(strength 0..1, seconds 0..10, player?)  // all players if omitted; no-op without a pad
ctx.input.rebind(action, inputs: string[])               // persisted in settings
ctx.input.bindings(action): string[]
```

## Input names

- Keys: SDL names (`Space`, `Left`, `A`, `W`, `R`, `Escape`, `Backspace`...). Mouse: `MouseLeft`.
- Gamepad: `GamepadA B X Y`, `GamepadLB RB LT RT LS RS`, `GamepadStart`, `GamepadBack`, `GamepadUp Down Left Right`; stick directions as buttons: `GamepadLeftY-` (left stick up), `GamepadLeftX+`.
- Axis sources: `GamepadLeftX LeftY RightX RightY LT RT`, or a `"Negative/Positive"` pair: `"A/D"`, `"Left/Right"`. The axis reads the strongest source.
- Touch: `Touch` (any finger), `TouchLeft` / `TouchRight` (finger on that half); a finger is also `MouseLeft` at `ctx.input.mouse()`. `Back` = Android back button.
- The action name `axes` is reserved.

## Touch sticks and buttons (phones)

```ts
touchSticks: { move: { zone: "left" } },                 // floating stick, default look
touchButtons: { jump: { pos: [0.88, 0.8], label: "A" } }, // pos = fractions of ctx.safeArea
axes: { moveX: ["A/D", "GamepadLeftX", "TouchMoveX"], moveY: ["W/S", "GamepadLeftY", "TouchMoveY"] },
actions: { jump: ["Space", "GamepadA", "TouchJump"] },
```

- Stick `move` sources: axes `TouchMoveX`/`TouchMoveY`; inputs `TouchMoveX-`/`X+`/`Y-`/`Y+` (past 0.5), `TouchMoveActive`. Button `jump`: input `TouchJump`. Names share the `Touch<Name>` space.
- Stick: `zone` ("left"|"right"|[x,y,w,h] in 0..1), `mode` ("floating" default|"fixed"), `anchor`, `radius` 64, `deadzone` 0.15 (0..0.9), `axes` "both"|"x"|"y", `snap` 0|4|8, `visible` "always"|"touching", `opacity` 0.6, `skin` { base, knob, baseSize?, knobSize?, pressed?: { base?, knob? }, tint? }, `player`, `scenes` (["play"] = only in that scene; default all). Entering a scene releases every control.
- Button: `pos` (required), `radius` 40, `hitSlop` 8, `label`, `visible` "always"|"never", `opacity`, `skin` { up, down?, size?, tint? }, `player`, `scenes`.
- Skin images: `assets.images` name or `"sheet:frame"`; a missing one is a boot error with "did you mean".
- Only on mobile (`ctx.platform.mobile`); desktop: inert, sources read 0. Preview: `alpha run -- --platform android`.
- A finger belongs to the first control it lands on (UI > buttons > sticks in order) until lifted; dragging off a button keeps it down. App pause releases all. Max 4 sticks, 12 buttons; overlapping buttons = config error.
- Tests: `simulate(game, { platform: "android", touches: [{ id: 1, path: [[0, [80, 300]], [10, [140, 300]], [40, null]] }] })` (frame, game-pixel pos, null = lift).
- MCP `game_state.input.touchSticks`: `{ name: { value: [x, y], finger } }`.

## Example (examples/platformer)

```ts
const move = Math.sign(Math.round(ctx.input.axis("moveX") * 10));
if (ctx.input.pressed("jump") && grounded) { vy = PLAYER.jump; ctx.audio.play("jump"); }
p.body.velocity = [move * PLAYER.speed, vy];
if (ctx.input.pressed("restart")) ctx.go("play");
```

Two players (see examples/topdown): `ctx.input.player(2).axis("moveX")`.

## Rules and gotchas

- Read actions and axes, never devices. On `ctx.input` an action is any player's; an axis is the player with the largest magnitude.
- Player 1 has the keyboard, mouse and first gamepad; later pads are the next players; a reconnected pad gets its slot back.
- Unknown action at runtime: `...("x"): not in config.actions; actions: ...`.
- Input consumed by game UI (clicked button, focus navigation, typing) never also triggers game actions.
- `pressed` is true for exactly one fixed step; poll it in `update`, not in timers.
- Tests: `simulate(game, { seconds: 2, inputs: { jump: [30, 60], axes: { moveX: [[0, 1], [90, 0]] } } })`; per player: `inputs: [{ fire: [0] }, { axes: { moveX: [[0, -1]] } }]`. Action frames press for one step each; axis entries hold from that frame on.
- Hints per platform: `ctx.platform` is `{ os: "windows"|"macos"|"linux"|"android"|"ios"|"web"|"other", mobile }` (mobile = android/ios). Use it for UI text/layout only ("tap" vs "press Space"), never rules. Preview `alpha run -- --platform android`; test `simulate(game, { platform: "android" })`.
- Rebinds and `ctx.settings.reset()` persist in settings.json (saves-and-i18n.md).
