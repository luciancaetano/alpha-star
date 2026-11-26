// Physics and tween glue for the runtime: entity.body, ctx queries and joints,
// tween targets. Embedded in the engine as "alpha:physics".
const N = globalThis.__native;

function fail(message) {
  throw new Error(message);
}

function pair(v, what) {
  if (!Array.isArray(v) || v.length !== 2) fail(`${what} must be [x, y], got ${JSON.stringify(v)}`);
  return v;
}

// entity.body for entity `id`.
export function bodyOf(id) {
  // Live, like entity.pos: velocity.x = 0 stops it horizontally.
  const velocity = {
    get x() { return N.velocity(id)[0]; },
    set x(v) { N.setVelocity(id, v, N.velocity(id)[1]); },
    get y() { return N.velocity(id)[1]; },
    set y(v) { N.setVelocity(id, N.velocity(id)[0], v); },
  };
  return {
    get velocity() {
      N.velocity(id);  // no body: the error names the entity
      return velocity;
    },
    set velocity(v) {
      if (!Array.isArray(v)) fail(`entity ${id}: body.velocity must be set to [x, y]`);
      N.setVelocity(id, v[0], v[1]);
    },
    get angularVelocity() { return N.angularVelocity(id); },
    set angularVelocity(v) { N.angularVelocity(id, v); },
    get mass() { return N.mass(id); },
    get type() { return N.bodyType(id) ?? fail(`entity ${id} has no body; spawn it with \`body\``); },
    applyForce: (f) => N.applyForce(id, ...pair(f, "body.applyForce(force)")),
    applyImpulse: (i) => N.applyImpulse(id, ...pair(i, "body.applyImpulse(impulse)")),
    applyTorque: (t) => N.applyTorque(id, t),
  };
}

// The contact of a hit row [a, b, nx, ny, px, py, speed] seen from a (normal a -> b);
// `swapped` when the handler's first entity is b.
export function contactOf(row, swapped) {
  const s = swapped ? -1 : 1;
  return { normal: [row[2] * s, row[3] * s], point: [row[4], row[5]], speed: row[6] };
}

function queryArgs(opts, what) {
  if (opts === undefined) return [null, null];
  if (typeof opts !== "object" || opts === null) fail(`${what}: options must be { tags?, categories? }`);
  return [opts.tags ?? null, opts.categories ?? null];
}

const JOINTS = ["revolute", "distance", "rope", "weld", "prismatic"];

function isEntity(v) {
  return v !== null && typeof v === "object" && typeof v.id === "number" && "alive" in v;
}

// ctx.raycast / raycastAll / queryCircle / queryRect / joint. `entity(id)` makes entity objects.
export function physicsCtx(entity) {
  const hit = (r) => ({ entity: entity(r[0]), point: [r[1], r[2]], normal: [r[3], r[4]], fraction: r[5] });
  const cast = (from, to, opts, what) => N.raycast(...pair(from, `${what} from`), ...pair(to, `${what} to`), ...queryArgs(opts, what));
  return {
    raycast(from, to, opts) {
      const rows = cast(from, to, opts, "raycast");
      return rows.length ? hit(rows[0]) : null;
    },
    raycastAll: (from, to, opts) => cast(from, to, opts, "raycastAll").map(hit),
    queryCircle: (center, radius, opts) =>
      N.query([...pair(center, "queryCircle center"), radius], ...queryArgs(opts, "queryCircle")).map(entity),
    queryRect(rect, opts) {
      if (!Array.isArray(rect) || rect.length !== 4) fail(`queryRect must be given [x, y, width, height], got ${JSON.stringify(rect)}`);
      return N.query(rect, ...queryArgs(opts, "queryRect")).map(entity);
    },
    joint(opts) {
      if (!opts || !JOINTS.includes(opts.type)) fail(`joint({ type, a, b, ... }): type must be one of ${JOINTS.join(", ")}, got ${JSON.stringify(opts?.type)}`);
      if (!isEntity(opts.a) || !isEntity(opts.b)) fail("joint: a and b must be entities (from ctx.spawn / find), both with a body");
      const h = N.joint(opts.a.id, opts.b.id, opts);
      return { get alive() { return N.jointAlive(h); }, destroy: () => N.jointDestroy(h) };
    },
  };
}

const ENTITY_KEYS = ["x", "y", "rotation", "alpha", "scale"];

// Kinematic bodies move by velocity so what stands on them is carried along.
function entityAccess(e) {
  const kinematic = () => N.bodyType(e.id) === "kinematic";
  const axis = { x: 0, y: 1 };
  const setVelocity = (i, v) => {
    const vel = N.velocity(e.id);
    vel[i] = v;
    N.setVelocity(e.id, vel[0], vel[1]);
  };
  return {
    check(keys) {
      for (const k of keys) if (!ENTITY_KEYS.includes(k)) fail(`tween(entity): cannot tween "${k}"; entity keys: ${ENTITY_KEYS.join(", ")}`);
    },
    get: (k) => (k in axis ? e.pos[k] : e[k]),
    set(k, v, dt) {
      if (k in axis && kinematic()) setVelocity(axis[k], (v - e.pos[k]) / dt);
      else if (k in axis) e.pos[k] = v;
      else e[k] = v;
    },
    settle(k) {
      if (k in axis && kinematic()) setVelocity(axis[k], 0);
    },
  };
}

function objectAccess(o) {
  return {
    check(keys) {
      const numeric = Object.keys(o).filter((k) => typeof o[k] === "number");
      for (const k of keys) if (typeof o[k] !== "number") fail(`tween(object): to.${k}: the object has no number "${k}"; number keys: ${numeric.join(", ") || "(none)"}`);
    },
    get: (k) => o[k],
    set: (k, v) => {
      o[k] = v;
    },
    settle() {},
  };
}

export function tweenAccess(target) {
  if (isEntity(target)) return entityAccess(target);
  if (target !== null && typeof target === "object") return objectAccess(target);
  return fail(`tween(target, opts): target must be an entity or an object, got ${JSON.stringify(target)}`);
}

export function aliveOf(target) {
  return isEntity(target) ? () => target.alive : () => true;
}
