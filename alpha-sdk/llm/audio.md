# Audio

> summary: Sound assets (files and synthesized notes), playback options, streamed music with fades/crossfades, and volume buses; read when adding sound effects or music.

## API

```ts
// types.ts: sounds: "jump" | "coin"; music?: "theme" | "boss"; buses?: "ambience";
assets: {
  sounds: {
    jump: "assets/sfx/jump.ogg",                                   // .wav/.ogg/.mp3, decoded at boot
    line: { file: "assets/vo/line1.mp3", bus: "voice" },
    blip: { wave: "square", freq: [440, 880], duration: 0.08, volume: 0.3 },   // synth
    coin: [{ wave: "triangle", freq: 988, duration: 0.05 }, { wave: "triangle", freq: 1319, duration: 0.1 }],
    amb:  { synth: { wave: "noise", freq: 1, duration: 1 }, bus: "ambience" },
  },
  music: { theme: "assets/music/theme.ogg" },                      // streamed, never loaded whole
},
config: { audio: { buses: ["ambience"] } },

interface Note { wave: "sine" | "square" | "triangle" | "saw" | "noise";
  freq: number | [from, to]; duration: number; volume? /*0.5*/; attack? /*0.005*/; release? /*0.05*/ }

ctx.audio.play(sound, { volume?, pitch?, loop?, pan? /* -1..1 */ }): Voice  // voice.playing, voice.stop()
ctx.audio.tone(synth, opts?): Voice                  // on the fly, sfx bus
ctx.audio.stopAll()
ctx.audio.music.play(name, { volume?, loop? /*true*/, fadeIn? })
ctx.audio.music.crossfade(name, seconds, opts?)
ctx.audio.music.stop(fadeOut?)
ctx.audio.music.playing                               // name | null
ctx.audio.buses                                      // "master","music","sfx","voice", ...config.audio.buses
ctx.audio.bus(name).volume = 0.5;                    // 0..1, persisted in settings
```

## Example (examples/platformer)

```ts
sounds: {
  jump: { wave: "square", freq: [250, 500], duration: 0.1, volume: 0.2 },
  land: { wave: "noise", freq: 1, duration: 0.08, volume: 0.25 },
},
// play.ts
if (ctx.input.pressed("jump") && grounded) ctx.audio.play("jump");
```
See also examples/flappy (OGG music, volume settings) and examples/shmup (WAV loop, fades, panning).

## Rules and gotchas

- Audio is output only: it never affects the simulation; tests stay deterministic.
- Sounds default to the "sfx" bus; music uses "music"; everything goes through "master".
- One music track at a time (a crossfade overlaps two); starting one stops the other at once.
- The same synth spec is always the same sound.
- Music files are checked at boot but never preloaded.
- Audio stops while the app is paused (focus lost / background).
- Tests: no audio output, but music and buses are tracked: `run.expect.music("theme")`, `run.expect.music(null)`, `run.music`, `run.settings.volumes`.
