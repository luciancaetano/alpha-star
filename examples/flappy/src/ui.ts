import type { UI } from "alpha";
import { WORLD } from "./tuning";

/** Horizontally centered text. */
export function centered(ui: UI, content: string | (() => string), y: number, size: number): void {
  ui.text(content, { pos: [WORLD.width / 2, y], size, align: "center" });
}
