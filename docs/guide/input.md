# Input

Games read **actions** and **axes**, never devices. `config.actions` maps each action to inputs;
`config.axes` maps each axis to sources.

```ts
config: {
  actions: { jump: ["Space", "W", "GamepadA"], fire: ["MouseLeft", "GamepadRT"] },
  axes: { moveX: ["GamepadLeftX", "A/D", "Left/Right"], moveY: ["GamepadLeftY", "W/S", "Up/Down"] },
  deadzone: 0.2,
}
```

```ts
ctx.input.pressed("jump");   // only on the step it went down
ctx.input.down("fire");      // while held
ctx.input.released("jump");
ctx.input.axis("moveX");     // -1..1 after the deadzone (+x right, +y down)
ctx.input.mouse();           // game pixels; ctx.camera.toWorld(...) for the world
```

Gamepad names: `GamepadA B X Y`, `GamepadLB RB LT RT LS RS`, `GamepadStart Back`, `GamepadUp
Down Left Right`, axes `GamepadLeftX LeftY RightX RightY`, and stick directions as buttons
(`GamepadLeftY-` = left stick up). Keys use SDL names (`Space`, `Left`, `A`, `Backspace`...).

## Players

Player 1 has the keyboard, the mouse and the first gamepad; each further gamepad is the next
player (a reconnected pad gets its slot back). `ctx.input.player(2).axis("moveX")` reads one
player; on `ctx.input` itself an action is any player's and an axis is the strongest one.
`ctx.input.players()` is how many players have a device. See `examples/topdown`.

## Rumble and rebinding

```ts
ctx.input.rumble(0.6, 0.2, 1);                 // strength, seconds, player (all if omitted)
ctx.input.rebind("jump", ["W", "GamepadA"]);   // persisted in settings
ctx.input.bindings("jump");
```

## In tests

```ts
simulate(game, { seconds: 2, inputs: { jump: [30, 60], axes: { moveX: [[0, 1], [90, 0]] } } });
simulate(game, { seconds: 2, inputs: [{ fire: [0] }, { axes: { moveX: [[0, -1]] } }] });  // players 1, 2
```

Action frames press for one step each; axis entries hold a value from that frame on.
Input the game UI consumed (a clicked button, navigation) never also triggers game actions.
