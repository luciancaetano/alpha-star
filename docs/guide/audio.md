# Audio and music

**What you'll build:** a jump sound panned to where the player is, a two-note coin, a noise hit,
and a mute key, all synthesized (no sound files).

Sounds come from `.wav` / `.ogg` / `.mp3` files or from a synth spec; music streams from a file.
Everything goes through volume buses. Audio is output only: the simulation never depends on it.

## Complete example

`src/main.ts`:

```ts
import { define } from "alpha";

interface Jukebox {
  actions: "jump" | "coin" | "hurt" | "mute";
  sounds: "jump" | "coin" | "hurt";
  images: never;
  tags: "player";
  scenes: { play: undefined };
  ui: {};
}

const { scene, game } = define<Jukebox>();

const play = scene("play", {
  enter(ctx) {
    ctx.spawn({ tags: ["player"], pos: [160, 90], shape: { rect: [16, 16], color: "#29adff" } });
  },
  update(ctx) {
    const p = ctx.find("player");
    const pan = (p.pos.x - 160) / 160;                       // -1 left .. 1 right
    if (ctx.input.pressed("jump")) ctx.audio.play("jump", { pan });
    if (ctx.input.pressed("coin")) ctx.audio.play("coin", { pitch: ctx.random.range(0.95, 1.05) });
    if (ctx.input.pressed("hurt")) ctx.audio.play("hurt", { volume: 0.7 });
    if (ctx.input.pressed("mute")) {
      const master = ctx.audio.bus("master");
      master.volume = master.volume > 0 ? 0 : 1;             // persisted in settings.json
    }
  },
});

export default game({
  config: {
    title: "Jukebox", size: [320, 180], seed: 1,
    actions: { jump: ["Space"], coin: ["C"], hurt: ["H"], mute: ["M"] },
  },
  start: "play",
  scenes: { play },
  assets: {
    sounds: {
      jump: { wave: "square", freq: [220, 660], duration: 0.12, volume: 0.25 },
      coin: [
        { wave: "triangle", freq: 988, duration: 0.05 },
        { wave: "triangle", freq: 1319, duration: 0.12 },
      ],
      hurt: { wave: "noise", freq: 1, duration: 0.2, volume: 0.3, release: 0.15 },
    },
  },
});
```

`test/main.test.ts`:

```ts
import { simulate } from "alpha/test";
import game from "../src/main";

export function muteSilencesMaster() {
  const run = simulate(game, { seconds: 0.5, inputs: { jump: [0], coin: [2], hurt: [4], mute: [6] } });
  run.expect.noErrors();
  if (run.settings.volumes.master !== 0) throw new Error(`master ${run.settings.volumes.master}`);
}
```

## Step by step

### 1. Synthesized sounds

```ts
sounds: {
  jump: { wave: "square", freq: [220, 660], duration: 0.12, volume: 0.25 },     // a slide up
  coin: [                                                                         // notes in a row
    { wave: "triangle", freq: 988, duration: 0.05 },
    { wave: "triangle", freq: 1319, duration: 0.12 },
  ],
  laser: { synth: { wave: "saw", freq: [1200, 200], duration: 0.15, volume: 0.3, attack: 0, release: 0.1 } },
  explode: { synth: { wave: "noise", freq: 1, duration: 0.4, release: 0.3 }, bus: "sfx" },
},
```

| Note field | Meaning |
|---|---|
| `wave` | `"sine"`, `"square"`, `"triangle"`, `"saw"`, `"noise"` |
| `freq` | Hz, or `[from, to]` for a slide |
| `duration` | seconds |
| `volume` | 0..1, default 0.5 |
| `attack` / `release` | fade in / out seconds, defaults 0.005 / 0.05 |

The same spec is always the same sound. `{ synth, bus }` puts it on another bus.

### 2. Sound files

```ts
sounds: {
  shot: "assets/sfx/shot.wav",                                // .wav / .ogg / .mp3, decoded at boot
  line: { file: "assets/vo/line1.ogg", bus: "voice" },
},
```

A missing file stops boot with its name.

### 3. Play

```ts
ctx.audio.play("jump");
ctx.audio.play("shot", { volume: 0.5, pitch: 1.2, pan: -0.5 });
```

`volume` 0..1, `pitch` is playback speed (2 = an octave up), `pan` -1 (left) .. 1 (right).

### 4. Loops and stopping

```ts
const wind = ctx.audio.play("wind", { loop: true, volume: 0.4 });
// later
if (wind.playing) wind.stop();      // stopping twice is a no-op
ctx.audio.stopAll();                // every sound and the music
```

### 5. One-off tones

```ts
ctx.audio.tone({ wave: "sine", freq: 660, duration: 0.1 });
ctx.audio.tone([{ wave: "square", freq: 523, duration: 0.08 }, { wave: "square", freq: 784, duration: 0.16 }], { volume: 0.5 });
```

Synthesized on the fly on the sfx bus: good for values that change (a pitch from the combo count).

### 6. Music

```ts
// GameTypes: music: "theme" | "boss"
assets: { music: { theme: "assets/music/theme.ogg", boss: "assets/music/boss.mp3" } },
// a scene
ctx.audio.music("theme", { volume: 0.6, fadeIn: 1 });   // loops by default
ctx.audio.crossfade("boss", 2);                          // theme fades out, boss in, over 2 s
ctx.audio.stopMusic(1.5);                                // fade out over 1.5 s
```

Music is streamed (decoded a little at a time, never loaded whole). One track plays at a time:
`music()` stops whatever played at once. `ctx.audio.musicPlaying` is the current track or null.

### 7. Buses and volumes

```ts
// GameTypes: buses: "ambience";  config: { audio: { buses: ["ambience"] } }
wind: { file: "assets/sfx/wind.ogg", bus: "ambience" },

ctx.audio.bus("music").volume = 0.5;
ctx.audio.bus("ambience").volume = 0.3;
for (const b of ctx.audio.buses) console.log(b, ctx.audio.bus(b).volume);
```

Buses: `master`, `music`, `sfx`, `voice`, plus `config.audio.buses`. Sounds go to `sfx` unless they
name a bus; music goes to `music`; everything goes through `master`. Volumes are persisted in
settings and applied at boot; `ctx.settings.volumes` reads them all.

## Recipes

### Keep the title music playing across scenes

```ts
enter(ctx) {
  if (ctx.audio.musicPlaying !== "theme") ctx.audio.music("theme", { volume: 0.6, fadeIn: 1 });
},
```

(`examples/flappy/src/scenes/title.ts`.)

### Boss fight music

```ts
ctx.onHit("player", "bossDoor", () => ctx.audio.crossfade("boss", 2));
```

### Pitch that climbs with a combo

```ts
ctx.audio.play("coin", { pitch: 1 + Math.min(ctx.state.combo, 12) / 12 });
```

### Varied footsteps

```ts
ctx.audio.play("step", { pitch: ctx.random.range(0.9, 1.1), volume: ctx.random.range(0.6, 0.8) });
```

The seeded random keeps it deterministic.

### Volume settings screen

`-` / `+` buttons that nudge `ctx.audio.bus("music").volume` in steps of 0.1: see the
[UI settings recipe](ui.html#settings-screen-with-volume-sliders) and
`examples/flappy/src/scenes/settings.ts`.

### Mute toggle

The `mute` key in the complete example: `ctx.audio.bus("master").volume = 0` or `1`.

## Gotchas

- `play` takes only names from `assets.sounds` (a typo is a compile error); use `tone` for
  sounds built at runtime.
- `music()` cuts the current track at once; use `crossfade` for a smooth change.
- When the app pauses (focus lost, minimized, backgrounded) audio stops with the game and
  resumes with it.
- Tests have no audio output, but music and buses are tracked: `run.expect.music("theme")`,
  `run.expect.music(null)`, `run.music`, `run.settings.volumes`; seed volumes with
  `simulate(game, { settings: { volumes: { music: 0.5 } } })`.

## See also

- [Game UI](ui.html) for the settings screen.
- [Saves, settings, i18n](saves.html): volumes are settings.
- `examples/flappy` (OGG music, volume settings), `examples/shmup` (a generated WAV loop, fades,
  panning), `examples/puzzle` (all-synth sounds).
- In `alpha.d.ts`: `Audio`, `Note`, `Synth`, `SoundAsset`, `PlayOptions`, `Bus`, `Voice`.
