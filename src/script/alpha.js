// Module "alpha": the only import game scripts need. Types: alpha-sdk/types/alpha.d.ts.
export function define() {
  return {
    scene(name, def) {
      return { name, def };
    },
    game(def) {
      return { def };
    },
  };
}
