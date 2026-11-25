// Boundary fuzzer game (tests/script/fuzz_test.cpp fills the __PLACEHOLDERS__).
// Each update makes __CALLS__ seeded random calls to natives and ctx functions.
import { define } from 'alpha';
const { scene, game } = define();
const N = globalThis.__native;
const ONLY = new RegExp(String.raw`__ONLY__`);
const TRACE = __TRACE__;  // ALPHA_FUZZ_TRACE=1: log each call
let r = __SEED__ >>> 0 || 1;
const rnd = () => { r ^= r << 13; r >>>= 0; r ^= r >>> 17; r ^= r << 5; r >>>= 0; return r / 4294967296; };
const pick = (a) => a[Math.floor(rnd() * a.length)];
const KEYS = ['pos', 'shape', 'rect', 'circle', 'outline', 'parallax', 'shake', 'follow', 'bounds', 'type', 'size',
  'radius', 'color', 'text', 'font', 'sprite', 'sheet', 'animation', 'body', 'sensor', 'friction', 'restitution',
  'density', 'emitter', 'rate', 'burst', 'life', 'speed', 'angle', 'spread', 'max', 'gravity', 'tags', 'layer', 'z',
  'data', 'parent', 'collider', 'category', 'collidesWith', 'oneWay', 'bullet', 'fixedRotation', 'kind', 'a', 'b',
  'anchor', 'length', 'motor', 'limits', 'wave', 'freq', 'duration', 'volume', 'pitch', 'loop', 'pan', 'attack',
  'decay', 'sustain', 'release', 'slide', 'smooth', 'alpha', 'blend', 'tint', 'scale', 'rotation', 'velocity',
  'width', 'height', 'points', 'offset', 'fadeIn', 'colors', 'sizes', 'frame', 'frames', 'fps', 'image', 'file',
  'pixelMode', 'filter', 'bus', 'from', 'to', 'ease'];
const WORDS = ['rect', 'circle', 'text', 'dynamic', 'static', 'kinematic', 'add', 'multiply', 'alpha', 'square',
  'sine', 'noise', 'triangle', 'saw', 'master', 'sfx', 'music', 'hud', 'x', 'a', '', '#fff', '#00000000', 'red',
  'weld', 'revolute', 'distance', 'prismatic', 'wheel', 'mouse', 'Space', 'KeyA', 'jump', 'main', '..', '/', '\u0000'];
const hostile = () => pick([
  () => new Proxy({}, { get() { throw new Error('proxy get'); }, ownKeys() { throw new Error('keys'); } }),
  () => ({ get pos() { throw new Error('getter'); } }),
  () => ({ toString() { throw new Error('toString'); }, valueOf() { throw new Error('valueOf'); } }),
  () => { const a = []; a.length = 2 ** 21; return a; },  // over kMaxArrayItems; String() of it stays fast
  () => { const a = [1]; a.push(a); return a; },
  () => { const o = {}; o.self = o; return o; },
  () => Symbol('s'),
  () => 10n,
  () => new Float64Array(4),
  () => new ArrayBuffer(8),
  () => () => 1,
  () => 'x'.repeat(100000),
])();
let ids = [];
function value(depth) {
  const k = rnd();
  if (k < 0.08) return hostile();
  if (k < 0.35) return pick([undefined, null, true, false, 0, -0, 1, -1, 0.5, 2, 7, 255, 1e9, -1e9, 2 ** 31, 2 ** 32,
    2 ** 53, -(2 ** 53), NaN, Infinity, -Infinity, Number.MIN_VALUE, Number.MAX_VALUE]);
  if (k < 0.5) return pick(WORDS);
  if (k < 0.6 && ids.length) return pick(ids);
  if (depth > 2) return Math.floor(rnd() * 100);
  if (k < 0.75) return Array.from({ length: Math.floor(rnd() * 5) }, () => value(depth + 1));
  const o = {};
  for (let i = Math.floor(rnd() * 6); i > 0; i--) o[pick(KEYS)] = value(depth + 1);
  return o;
}
const argsFor = (n) => Array.from({ length: n + Math.floor(rnd() * 3) }, () => value(0));
function functionsOf(root, prefix, out, depth) {
  for (const key of Object.keys(root)) {
    let v;
    try { v = root[key]; } catch { continue; }
    if (typeof v === 'function') out.push([prefix + key, v, root]);
    else if (v && typeof v === 'object' && depth < 2) functionsOf(v, prefix + key + '.', out, depth + 1);
  }
  return out;
}
const SKIP = new Set(['log', 'devWarn']);  // console output: harmless, only noise
const natives = Object.keys(N).filter((k) => !SKIP.has(k)).map((k) => ['N.' + k, N[k], N]);
const shown = (a) => { try { return Array.isArray(a) ? 'array(' + a.length + ')' : String(a).slice(0, 40); } catch { return typeof a; } };
let made = 0;
let bad = 0;
function attempt(name, fn, self, args) {
  made++;
  if (TRACE) N.log(0, 'FUZZ ' + name + ' ' + args.map(shown).join(' | '));
  try { fn.apply(self, args); } catch (e) {
    if (!(e instanceof Error) || typeof e.message !== 'string' || e.message.length === 0) {
      bad++;
      console.error('FUZZ-BAD ' + name + ' threw a non-Error: ' + shown(e));
    }
  }
}
const play = scene('play', {
  enter(ctx) {
    for (let i = 0; i < 5; i++) ids.push(ctx.spawn({ pos: [i * 10, 0], tags: ['x'], shape: { rect: [4, 4], color: '#fff' } }).id);
  },
  update(ctx) {
    const targets = natives.concat(functionsOf(ctx, 'ctx.', [], 0)).filter(([name]) => ONLY.test(name));
    const home = N.instanceUse(0);
    N.instanceUse(home);
    for (let i = 0; i < __CALLS__; i++) {
      const [name, fn, self] = pick(targets);
      if (name === 'ctx.goto') continue;  // leaving the scene ends the fuzz; tested on its own
      attempt(name, fn, self, argsFor(fn.length));
      try { N.instanceUse(home); } catch { /* the instance itself may be gone: that run ends */ }
    }
    try { ids = ids.concat(N.findAll('x')); } catch { /* fuzzed tags */ }
    console.log('FUZZ-DONE ' + made + ' calls, ' + bad + ' bad');
  },
});
export default game({ config: { title: 't', size: [200, 100], seed: 1, actions: { jump: ['Space'] } },
  start: 'play', scenes: { play } });
