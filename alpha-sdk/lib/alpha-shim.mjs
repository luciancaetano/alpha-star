// Stand-in for the engine's "alpha" / "alpha/test" modules, used only by the
// exporter to evaluate a game in Node and read its definition (e.g. assets).
export function define() {
  return {
    scene: (name, def) => ({ name, def }),
    game: (def) => ({ def }),
  };
}

export function simulate() {
  throw new Error("alpha/test: simulate() is not available in the exporter");
}
