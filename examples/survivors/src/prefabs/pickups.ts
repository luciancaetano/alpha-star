// Things on the ground: XP gems, items, braziers to break, treasure chests, scenery.
import type { Vec2 } from "alpha";
import { GEM } from "../tuning";
import type { Candle, ItemKind, Slot, Spec } from "../types";

export type { ItemKind };

type ItemAnim = Candle["sheets"]["items"];
type PropAnim = Candle["sheets"]["props"];

export const gemLook = (value: number): ItemAnim => (value >= GEM.big ? "xp3" : value >= GEM.mid ? "xp2" : "xp1");

export function gem(at: Vec2, value: number): Spec<"gem"> {
  return { tags: ["gem"], pos: at, z: 2, shape: { sheet: "items", animation: gemLook(value) }, data: { v: value, pull: 0 } };
}

export function item(at: Vec2, kind: ItemKind): Spec<"item"> {
  const chest = kind === "chest";
  return {
    tags: ["item"], pos: at, z: 2,
    shape: chest ? { sheet: "props", animation: "chest", size: [26, 26] } : { sheet: "items", animation: kind },
    ...(chest || kind === "bell" ? { light: { radius: 46, color: "#ffd36b", intensity: 1.2 } } : {}),
    data: { kind, pull: 0 },
  };
}

export function brazier(at: Vec2): Spec<"brazier"> {
  return {
    tags: ["brazier"], pos: at, z: 2,
    shape: { sheet: "props", animation: "brazier" },
    light: { radius: 120, color: "#ffa04a", intensity: 1.5 },
    emitter: { rate: 6, lifetime: [0.4, 0.9], speed: [10, 26], angle: [255, 285], size: [2, 0], color: ["#ffd36bff", "#ff5a1a00"], blend: "add", max: 12 },
    data: { hp: 1 },
  };
}

export const SCENERY: PropAnim[] = ["grave", "reeds", "shrooms", "stones", "stump", "reeds", "shrooms"];

export function prop(at: Vec2, look: PropAnim, flip: boolean, slot: Slot): Spec<"prop"> {
  const glow = look === "shrooms";
  return {
    tags: ["prop"], pos: at, z: 0, layer: "floor",
    shape: { sheet: "props", animation: look, flipX: flip },
    ...(glow ? { light: { radius: 34, color: "#4de8c2", intensity: 0.9 } } : {}),
    data: slot,
  };
}

export function groundTile(at: Vec2, slot: Slot): Spec<"ground"> {
  return { tags: ["ground"], pos: at, z: -10, layer: "floor", shape: { image: "ground" }, data: slot };
}
