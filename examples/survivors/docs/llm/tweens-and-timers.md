# Tweens and Timers

> summary: ctx.tween on entities and plain objects (eases, yoyo, repeat, onDone), ctx.every/ctx.after timers and the vec helpers; read when animating values over time or scheduling work.

## API

```ts
ctx.tween(entity, { to: { x?, y?, rotation?, alpha?, scale? }, seconds, ease?, delay?, yoyo?, repeat?, onDone? }): Tween
ctx.tween(obj, { to: { someNumberKey: 1 }, seconds, ... }): Tween   // number keys of a plain object
interface Tween { readonly done: boolean; readonly active: boolean; cancel(): void }

type Ease = "linear" | `${"quad"|"cubic"|"quart"|"quint"|"sine"|"expo"|"circ"|"back"|"elastic"|"bounce"}${"In"|"Out"|"InOut"}`;
// defaults: ease "linear", repeat 0 (integer >= 0 or Infinity); delay: values read when it starts

ctx.every(seconds, fn)   // repeats every `seconds` (>= 0.001) of game time; first call after `seconds`
ctx.after(seconds, fn)   // once, after `seconds` (>= 0.001)

import { vec } from "alpha";
vec.add(a, b) vec.sub(a, b) vec.scale(a, s) vec.len(a) vec.norm(a) vec.dot(a, b)
vec.lerp(a, b, t) vec.angle(a) vec.fromAngle(deg, len?) vec.dist(a, b)   // pure, new arrays
```

## Example (examples/platformer)

```ts
// a lift moving back and forth forever (kinematic body: carries riders)
const lifter = ctx.spawn(lift());
ctx.tween(lifter, { to: { x: LIFT.toX }, seconds: LIFT.seconds, ease: "sineInOut", yoyo: true, repeat: Infinity });

// a picked coin grows, fades, then goes away
ctx.tween(c, { to: { scale: COIN_POP.scale, alpha: 0 }, seconds: COIN_POP.seconds, ease: "quadOut",
               onDone: () => ctx.destroy(c) });
```

Timers:
```ts
enter(ctx) {
  ctx.every(1.5, () => ctx.spawn(enemy([ctx.random.int(0, 300), -10])));
  ctx.after(3, () => ctx.ui.hide("hud"));
}
```

## Rules and gotchas

- Tweens and timers run on the fixed step in game time (deterministic) and are scene-scoped: cleared on scene change.
- One yoyo cycle = there and back; `repeat` counts extra cycles.
- A tween on a kinematic body's x/y drives its velocity, so bodies standing on it ride along.
- For dynamic bodies prefer velocity/impulses (physics.md) over tweening position.
- Seconds below 0.001 for `every`/`after` are rejected.
- Timers do not run while the app is paused; resume skips the paused time.
- `ctx.settings.reducedMotion` is the player's flag: games may snap tweens when it is on.
