// ctx.audio: sounds (preloaded, panned, through a bus), streamed music (one
// track at a time, fades and crossfades), volume buses. Embedded module
// "alpha:audio"; the runtime builds ctx.audio from it.
const N = globalThis.__native;

function fail(message) {
  throw new Error(message);
}

// config.audio: { buses?: ["ambience", ...] } adds buses next to master, music, sfx, voice.
export function defineBuses(audio) {
  if (audio === undefined) return;
  if (!audio || typeof audio !== "object" || (audio.buses !== undefined && !Array.isArray(audio.buses))) {
    fail("config.audio must be { buses?: [\"ambience\", ...] }");
  }
  for (const b of audio.buses ?? []) N.addBus(b);
}

// assets.sounds entry: "file" (.wav/.ogg/.mp3), a synth spec, { file, bus? } or { synth, bus? }.
function loadSound(name, sound) {
  if (typeof sound === "string") return N.loadSound(name, sound, "sfx");
  const wrapped = sound && typeof sound === "object" && !Array.isArray(sound) && ("file" in sound || "synth" in sound);
  if (!wrapped) return N.synthSound(name, sound, "sfx");
  const bus = sound.bus ?? "sfx";
  if (typeof sound.file === "string") return N.loadSound(name, sound.file, bus);
  if (sound.synth !== undefined) return N.synthSound(name, sound.synth, bus);
  fail(`assets.sounds.${name} must be a file path, a synth spec, { file, bus? } or { synth, bus? }`);
}

// Sounds are decoded at boot; music files are only checked (streamed when played).
export function loadAudio(assets) {
  for (const [name, sound] of Object.entries(assets.sounds ?? {})) loadSound(name, sound);
  for (const [name, file] of Object.entries(assets.music ?? {})) {
    if (typeof file !== "string") fail(`assets.music.${name} must be a file path (.ogg, .mp3 or .wav), got ${JSON.stringify(file)}`);
    N.loadMusic(name, file);
  }
}

function voice(id) {
  return {
    get playing() { return N.voicePlaying(id); },
    stop: () => N.stopVoice(id),
  };
}

function playArgs(opts = {}) {
  return [opts.volume ?? 1, opts.pitch ?? 1, Boolean(opts.loop), opts.pan ?? 0];
}

function musicArgs(opts = {}) {
  return [opts.volume ?? 1, opts.loop ?? true];
}

// onVolume(bus, volume): a bus volume changed (settings persist it).
export function makeAudio(onVolume) {
  return {
    play: (sound, opts) => voice(N.playSound(sound, ...playArgs(opts))),
    tone: (synth, opts) => voice(N.playTone(synth, ...playArgs(opts))),
    stopAll: () => N.stopAll(),
    music: (name, opts = {}) => N.music(name, ...musicArgs(opts), opts.fadeIn ?? 0, false),
    crossfade: (name, seconds, opts = {}) => N.music(name, ...musicArgs(opts), seconds, true),
    stopMusic: (fadeOut = 0) => N.stopMusic(fadeOut),
    get musicPlaying() { return N.musicName(); },
    get buses() { return N.buses(); },
    bus(name) {
      N.busVolume(name);  // unknown names fail here, listing the buses
      return {
        get volume() { return N.busVolume(name); },
        set volume(v) {
          N.busVolume(name, v);
          onVolume(name, v);
        },
      };
    },
  };
}
