import { define } from "alpha";
import type { Game } from "./types";

// Builders bound to the game's names. Import `scene` / `game` from here.
export const { scene, game } = define<Game>();
