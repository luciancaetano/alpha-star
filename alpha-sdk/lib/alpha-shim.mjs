// Stand-in for the engine's "alpha" / "alpha/test" modules, used only by the
// exporter to evaluate a game in Node and read its definition (e.g. assets).
export function define() {
  return {
    scene: (name, def) => ({ name, def }),
    game: (def) => ({ def }),
  };
}

// Same as the engine's src/script/alpha.js (test/pack.test.mjs checks every export is here).
// Vector math on plain [x, y] arrays (pure, new arrays; angles in degrees, 0 = right, 90 = down).
const DEG = Math.PI / 180;
export const vec = {
  add: (a, b) => [a[0] + b[0], a[1] + b[1]],
  sub: (a, b) => [a[0] - b[0], a[1] - b[1]],
  scale: (a, s) => [a[0] * s, a[1] * s],
  len: (a) => Math.hypot(a[0], a[1]),
  norm(a) {
    const l = Math.hypot(a[0], a[1]);
    return l === 0 ? [0, 0] : [a[0] / l, a[1] / l];
  },
  dot: (a, b) => a[0] * b[0] + a[1] * b[1],
  lerp: (a, b, t) => [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t],
  angle: (a) => Math.atan2(a[1], a[0]) / DEG,
  fromAngle: (degrees, length = 1) => [Math.cos(degrees * DEG) * length, Math.sin(degrees * DEG) * length],
  dist: (a, b) => Math.hypot(b[0] - a[0], b[1] - a[1]),
};

export function simulate() {
  throw new Error("alpha/test: simulate() is not available in the exporter");
}
