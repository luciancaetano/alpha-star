import type { Context, Entity } from "alpha";
import { scene } from "../alpha";
import { coin } from "../prefabs/coin";
import { dust, hill, popup } from "../prefabs/effects";
import { ledge, lift } from "../prefabs/platforms";
import { player } from "../prefabs/player";
import { CAMERA, COIN_POP, DUST, GROUND_RAY, LEDGES, LIFT, PLAYER, POPUP } from "../tuning";
import type { Platformer } from "../types";

type State = { coins: number; total: number; grounded: boolean; fallSpeed: number };
type Play = Context<Platformer, State>;
type Hero = Entity<Platformer, "player">;

const FLOOR = ["walls", "ledge", "lift"] as const;

// What the hero stands on: a short ray down from the feet (center, then both edges) hits a floor.
function groundUnder(ctx: Play, p: Hero): Entity<Platformer> | null {
  if (p.body.velocity.y < -1) return null;  // rising (e.g. jumping up through a ledge)
  const feet = p.pos.y + PLAYER.size[1] / 2;
  for (const dx of [0, -GROUND_RAY.edge, GROUND_RAY.edge]) {
    const hit = ctx.raycast([p.pos.x + dx, feet - 1], [p.pos.x + dx, feet + GROUND_RAY.reach], { tags: FLOOR });
    if (hit) return hit.entity;
  }
  return null;
}

// Idle / run on the ground, jump / fall in the air; faces where it moves.
function animate(p: Hero, ctx: Play, move: number) {
  if (move !== 0) p.flipX = move < 0;
  if (!ctx.state.grounded) p.animation = p.body.velocity.y < 0 ? "jump" : "fall";
  else p.animation = move !== 0 ? "run" : "idle";
}

// Just touched ground: dust at the feet; a hard landing also shakes the camera.
function land(ctx: Play, p: Hero) {
  if (ctx.state.fallSpeed > DUST.landing) ctx.spawn(dust([p.pos.x, p.pos.y + 8]));
  if (ctx.state.fallSpeed > PLAYER.hardLanding) {
    ctx.camera.shake(CAMERA.shake, CAMERA.shakeSeconds);
    ctx.audio.play("land");
  }
}

// A coin popup rises and fades, then goes away.
function floatPopup(ctx: Play, t: Entity<Platformer, "popup">) {
  t.pos.y -= POPUP.rise * ctx.dt;
  const alpha = t.alpha - POPUP.fade * ctx.dt;
  if (alpha <= 0) ctx.destroy(t);
  else t.alpha = alpha;
}

// A picked coin counts once, grows and fades out (tween), then goes away.
function pick(ctx: Play, c: Entity<Platformer, "coin">) {
  if (c.data.taken) return;
  c.data.taken = true;
  ctx.spawn(popup([c.pos.x, c.pos.y]));
  ctx.tween(c, { to: { scale: COIN_POP.scale, alpha: 0 }, seconds: COIN_POP.seconds, ease: "quadOut", onDone: () => ctx.destroy(c) });
  ctx.state.coins += 1;
  ctx.audio.play("coin");
  ctx.emit("coinTaken");
}

// The HUD counts coins; the last one wins the level.
function countCoins(ctx: Play) {
  ctx.ui.set("hud", "coins", `coins ${ctx.state.coins}/${ctx.state.total}`);
  if (ctx.state.coins === ctx.state.total) ctx.go("won");
}

// Runs and jumps; on the lift, moves with it (its velocity is what the tween drives).
function move(ctx: Play, p: Hero) {
  const dir = Math.sign(Math.round(ctx.input.axis("moveX") * 10));
  const ground = groundUnder(ctx, p);
  const grounded = ground !== null;
  const carried = ground?.body.type === "kinematic" ? ground.body.velocity.x : 0;
  if (grounded && !ctx.state.grounded) land(ctx, p);
  ctx.state.grounded = grounded;
  if (ctx.input.pressed("jump") && grounded) {
    p.body.velocity.y = PLAYER.jump;
    ctx.state.grounded = false;
    ctx.audio.play("jump");
  }
  ctx.state.fallSpeed = Math.max(0, p.body.velocity.y);
  p.body.velocity.x = carried + dir * PLAYER.speed;
  animate(p, ctx, dir);
}

export default scene("play", {
  state: () => ({ coins: 0, total: 0, grounded: false, fallSpeed: 0 }),

  enter(ctx) {
    // LDtk level: IntGrid "Walls" -> merged colliders tagged "walls"; entities through factories.
    const level = ctx.spawnLevel("main", "Level_0", { collide: { Walls: "walls" }, entities: { Player: (e) => player(e.pos), Coin: coin } });
    const hero = ctx.find("player");
    for (let x = 0; x < level.size[0] * 0.2 + ctx.camera.view[2]; x += 70) ctx.spawn(hill(x, 45 + (x % 3) * 15));
    ctx.state.total = ctx.findAll("coin").length;
    for (const l of LEDGES) ctx.spawn(ledge(l.pos, l.width));
    const lifter = ctx.spawn(lift());
    ctx.tween(lifter, { to: { x: LIFT.toX }, seconds: LIFT.seconds, ease: "sineInOut", yoyo: true, repeat: Infinity });

    ctx.camera.bounds([0, 0, level.size[0], level.size[1]]);
    ctx.camera.follow(hero, { lerp: CAMERA.lerp, deadzone: CAMERA.deadzone });
    ctx.ui.show("hud");
    ctx.ui.set("hud", "coins", `coins 0/${ctx.state.total}`);

    ctx.onEnter("player", "coin", (_p, c) => pick(ctx, c));
    ctx.on("coinTaken", () => countCoins(ctx));
  },

  update(ctx) {
    // Scene changes stay in update: fell into the pit, or restart.
    if (ctx.find("player").pos.y > 400 || ctx.input.pressed("restart")) ctx.go("play");
  },

  // Run in this order after update; a tag system runs once per living entity.
  systems: [
    { tag: "player", each: move },
    { tag: "popup", each: floatPopup },
  ],
});
