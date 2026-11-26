// Scene flow for the runtime: systems (scene.systems), game events (ctx.emit / ctx.on) and
// timers (ctx.every / ctx.after with owners). Embedded in the engine as "alpha:flow".
const N = globalThis.__native;

function fail(message) {
  throw new Error(message);
}

const WHEN = ["running", "frozen", "always"];
// Most events delivered in one step (handlers that keep re-sending would never end).
export const MAX_EVENTS_PER_STEP = 1000;

// A system as { label, when, tag?, fn }, from its scene.systems entry `i`.
function systemOf(scene, sys, i) {
  const where = `scene "${scene}" systems[${i}]`;
  if (typeof sys === "function") return { label: `${where}`, when: "running", fn: sys };
  if (!sys || typeof sys !== "object") fail(`${where} must be a function (ctx) => void, { run(ctx) } or { tag, each(ctx, e) }, got ${typeof sys}`);
  const when = sys.when ?? "running";
  if (!WHEN.includes(when)) fail(`${where}.when must be "running", "frozen" or "always", got ${JSON.stringify(sys.when)}`);
  if (sys.name !== undefined && typeof sys.name !== "string") fail(`${where}.name must be a string`);
  if (sys.tag !== undefined) {
    if (typeof sys.tag !== "string" || typeof sys.each !== "function") fail(`${where}: a tag system is { tag: "enemy", each(ctx, e) }`);
    return { label: `system "${sys.name ?? sys.tag}"`, when, tag: sys.tag, fn: sys.each };
  }
  if (typeof sys.run !== "function") fail(`${where} needs run(ctx) (or tag + each(ctx, e))`);
  return { label: sys.name ? `system "${sys.name}"` : where, when, fn: sys.run };
}

// Checks scene.systems once, at enter (errors name the entry).
export function systemsOf(scene, systems) {
  if (systems === undefined) return [];
  if (!Array.isArray(systems)) fail(`scene "${scene}" systems must be an array`);
  return systems.map((sys, i) => systemOf(scene, sys, i));
}

function runs(sys, frozen) {
  return sys.when === "always" || (sys.when === "frozen") === frozen;
}

// An error thrown inside a system says which one (the stack still points into the game).
function named(label, fn) {
  try {
    fn();
  } catch (e) {
    if (e instanceof Error) e.message = `${label}: ${e.message}`;
    throw e;
  }
}

// One pass over the systems; tag systems skip entities destroyed earlier in the pass.
export function runSystems(systems, ctx, frozen, entity) {
  for (const sys of systems) {
    if (!runs(sys, frozen)) continue;
    if (sys.tag === undefined) named(sys.label, () => sys.fn(ctx));
    else named(sys.label, () => { for (const id of N.findAll(sys.tag)) if (N.alive(id)) sys.fn(ctx, entity(id)); });
  }
}

// ctx.emit / ctx.on: queued, delivered by flush() at the end of the step.
export function makeEvents() {
  const handlers = new Map();
  let queue = [];
  return {
    on(name, fn) {
      if (typeof name !== "string") fail(`on(name, fn): name must be an event name (GameTypes.events), got ${JSON.stringify(name)}`);
      if (typeof fn !== "function") fail(`on("${name}", fn): fn must be a function`);
      if (!handlers.has(name)) handlers.set(name, []);
      handlers.get(name).push(fn);
    },
    emit(name, payload) {
      if (typeof name !== "string") fail(`emit(name, payload?): name must be an event name (GameTypes.events), got ${JSON.stringify(name)}`);
      queue.push([name, payload]);
    },
    flush() {
      let sent = 0;
      while (queue.length) {
        const [name, payload] = queue.shift();
        if (++sent > MAX_EVENTS_PER_STEP) {
          queue = [];
          fail(`more than ${MAX_EVENTS_PER_STEP} events in one step (last: "${name}"): an on() handler keeps emitting; send at most one event per change`);
        }
        for (const fn of handlers.get(name) ?? []) fn(payload);
      }
    },
    clear() {
      handlers.clear();
      queue = [];
    },
  };
}

// opts.owner of ctx.every / ctx.after: an entity, or undefined.
function ownerOf(opts, what) {
  if (opts === undefined) return undefined;
  if (!opts || typeof opts !== "object") fail(`${what}: options must be { owner? }`);
  const o = opts.owner;
  if (o !== undefined && (o === null || typeof o.id !== "number" || !("alive" in o))) fail(`${what}: owner must be an entity (from ctx.spawn / find)`);
  return o;
}

// A ctx.every / ctx.after timer: `timers` maps native id -> callback (cleared on scene change).
export function addTimer(timers, id, fn, opts, once, what) {
  if (typeof fn !== "function") fail(`${what}: callback must be a function`);
  const owner = ownerOf(opts, what);
  const stop = () => {
    if (timers.get(id) !== call) return;  // fired, cancelled or another scene's timer
    timers.delete(id);
    N.timerCancel(id);
  };
  const call = () => {
    if (owner && !owner.alive) return stop();
    if (once) timers.delete(id);
    fn();
  };
  timers.set(id, call);
  return {
    get active() { return timers.get(id) === call && (!owner || owner.alive); },
    cancel: stop,
  };
}
