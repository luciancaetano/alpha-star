# Audio and music

```ts
assets: {
  sounds: {
    jump: "assets/sfx/jump.ogg",                                     // .wav / .ogg / .mp3, decoded at boot
    line: { file: "assets/vo/line1.mp3", bus: "voice" },
    blip: { wave: "square", freq: [440, 880], duration: 0.08, volume: 0.3 },   // synthesized
    coin: [{ wave: "triangle", freq: 988, duration: 0.05 }, { wave: "triangle", freq: 1319, duration: 0.1 }],
  },
  music: { theme: "assets/music/theme.ogg" },                         // streamed, never loaded whole
}
config: { audio: { buses: ["ambience"] } }
```

```ts
ctx.audio.play("jump", { volume: 0.5, pitch: 1.2, pan: -0.5 });   // pan -1 left .. 1 right
ctx.audio.tone({ wave: "sine", freq: 660, duration: 0.1 });        // on the fly
ctx.audio.music("theme", { fadeIn: 1, volume: 0.6 });
ctx.audio.crossfade("boss", 2);
ctx.audio.stopMusic(1.5);
ctx.audio.bus("music").volume = 0.5;   // master, music, sfx, voice + config.audio.buses; persisted
```

Synth waves: `sine`, `square`, `triangle`, `saw`, `noise`; `freq` in Hz or `[from, to]` for a
slide. The same spec is always the same sound. One music track plays at a time (a crossfade
overlaps two). Audio is output only: the simulation never depends on it.

Examples: `flappy` (OGG music, volume settings), `shmup` (a generated WAV loop, fades, panning).

## In tests

`simulate` has no audio output but tracks music and buses: `run.expect.music("theme")`,
`run.expect.music(null)`, `run.music`, `run.settings.volumes`.
