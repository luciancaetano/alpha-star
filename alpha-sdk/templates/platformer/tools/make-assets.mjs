// Generates the platformer's placeholder art (PNG sprite sheets, tileset) and
// its LDtk world, deterministically: `node tools/make-assets.mjs`. The files are
// committed; rerun only to change them. Original art made here, CC0.
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { encodePng } from "@alpha-star/sdk/png";

const root = path.join(path.dirname(fileURLToPath(import.meta.url)), "..");

function canvas(width, height) {
  return { width, height, rgba: Buffer.alloc(width * height * 4) };
}

function hex(color) {
  return [1, 3, 5].map((i) => parseInt(color.slice(i, i + 2), 16)).concat(255);
}

function rect(c, x, y, w, h, color) {
  const px = hex(color);
  for (let j = Math.max(0, y); j < Math.min(c.height, y + h); j++) {
    for (let i = Math.max(0, x); i < Math.min(c.width, x + w); i++) c.rgba.set(px, (j * c.width + i) * 4);
  }
}

function save(file, c) {
  fs.mkdirSync(path.dirname(path.join(root, file)), { recursive: true });
  fs.writeFileSync(path.join(root, file), encodePng(c));
}

// Hero: 16x16 frames. 0 idle, 1-4 run, 5 jump, 6 fall.
function hero() {
  const legs = [[5, 9], [3, 11], [5, 9], [7, 7], [5, 9], [4, 10], [6, 8]];
  const c = canvas(16 * legs.length, 16);
  legs.forEach(([left, right], f) => {
    const x = f * 16;
    const lift = f === 5 ? 2 : 0;
    rect(c, x + 4, 1, 8, 6, "#f2c79b");          // head
    rect(c, x + 8, 3, 2, 2, "#1d1d1d");          // eye (facing right)
    rect(c, x + 3, 0, 10, 2, "#c0392b");         // cap
    rect(c, x + 4, 7, 8, 5, "#2e86de");          // body
    rect(c, x + left, 12 - lift, 3, 4, "#34495e");
    rect(c, x + right, 12 - lift, 3, 4, "#34495e");
  });
  save("assets/img/hero.png", c);
}

// Coin: 8x8, 4 frames of a spin.
function coin() {
  const c = canvas(32, 8);
  [8, 6, 2, 6].forEach((w, f) => {
    rect(c, f * 8 + (8 - w) / 2, 0, w, 8, "#f1c40f");
    rect(c, f * 8 + (8 - w) / 2, 1, Math.max(1, w / 2), 6, "#f9e79f");
  });
  save("assets/img/coin.png", c);
}

// Tileset: 16x16 tiles (LDtk ids). 0 grass, 1 dirt, 2 brick, 3 bush, 4-5 flower (two sway frames).
function tiles() {
  const c = canvas(96, 16);
  rect(c, 0, 0, 16, 16, "#8e5b34");
  rect(c, 0, 0, 16, 5, "#4caf50");
  rect(c, 16, 0, 16, 16, "#8e5b34");
  rect(c, 20, 6, 2, 2, "#6d4425");
  rect(c, 32, 0, 16, 16, "#b5523b");
  for (const [x, y, w] of [[32, 7, 16], [32, 15, 16], [39, 0, 1], [35, 8, 1], [43, 8, 1]]) rect(c, x, y, w, 1, "#7b3426");
  rect(c, 50, 6, 12, 10, "#2e7d32");
  rect(c, 53, 3, 6, 4, "#2e7d32");
  rect(c, 71, 8, 2, 8, "#2e7d32");
  rect(c, 69, 4, 6, 4, "#e84393");
  rect(c, 87, 8, 2, 8, "#2e7d32");
  rect(c, 86, 5, 6, 4, "#e84393");
  save("assets/tiles/tiles.png", c);
}

const W = 60;
const H = 12;
const G = 16;

// Cells to fill: set(x, y, value).
function grid(fill) {
  const data = new Array(W * H).fill(0);
  fill((x, y, v) => { data[y * W + x] = v; });
  return data;
}

// LDtk tiles for the non-zero cells; tileOf(cell index) -> tile id.
const tilesOf = (data, tileOf) => data.flatMap((v, i) => {
  if (!v) return [];
  const t = tileOf(i);
  return [{ px: [(i % W) * G, Math.floor(i / W) * G], src: [t * G, 0], f: 0, t, d: [i], a: 1 }];
});

// LDtk layer instance (the fields LDtk itself writes).
function layer(def, extra) {
  return {
    __identifier: def.identifier, __type: def.type, __cWid: W, __cHei: H, __gridSize: G, __opacity: 1,
    __pxTotalOffsetX: 0, __pxTotalOffsetY: 0, __tilesetDefUid: def.tilesetDefUid,
    __tilesetRelPath: def.tilesetDefUid ? "../tiles/tiles.png" : null, iid: `layer-${def.uid}`, levelId: 0,
    layerDefUid: def.uid, pxOffsetX: 0, pxOffsetY: 0, visible: true, optionalRules: [], intGridCsv: [],
    autoLayerTiles: [], seed: 1, overrideTilesetUid: null, gridTiles: [], entityInstances: [], ...extra,
  };
}

function layerDef(uid, identifier, type, extra = {}) {
  return {
    __type: type, identifier, type, uid, doc: null, uiColor: null, gridSize: G, guideGridWid: 0, guideGridHei: 0,
    displayOpacity: 1, inactiveOpacity: 1, hideInList: false, hideFieldsWhenInactive: true, canSelectWhenInactive: true,
    renderInWorldView: true, pxOffsetX: 0, pxOffsetY: 0, parallaxFactorX: 0, parallaxFactorY: 0, parallaxScaling: true,
    requiredTags: [], excludedTags: [], autoTilesKilledByOtherLayerUid: null, uiFilterTags: [], useAsyncRender: false,
    intGridValues: [], intGridValuesGroups: [], autoRuleGroups: [], autoSourceLayerDefUid: null, tilesetDefUid: null,
    tilePivotX: 0, tilePivotY: 0, biomeFieldUid: null, ...extra,
  };
}

function entityDef(uid, identifier, color, size) {
  return {
    identifier, uid, tags: [], exportToToc: false, allowOutOfBounds: false, doc: null, width: size, height: size,
    resizableX: false, resizableY: false, minWidth: null, maxWidth: null, minHeight: null, maxHeight: null,
    keepAspectRatio: false, tileOpacity: 1, fillOpacity: 0.08, lineOpacity: 0, hollow: false, color,
    renderMode: "Rectangle", showName: true, tilesetId: null, tileRenderMode: "FitInside", tileRect: null,
    uiTileRect: null, nineSliceBorders: [], maxCount: 0, limitScope: "PerLevel", limitBehavior: "MoveLastOne",
    pivotX: 0.5, pivotY: 0.5, fieldDefs: [],
  };
}

function entity(def, i, [x, y]) {
  return {
    __identifier: def.identifier, __grid: [Math.floor(x / G), Math.floor(y / G)], __pivot: [def.pivotX, def.pivotY],
    __tags: [], __tile: null, __smartColor: def.color, iid: `${def.identifier.toLowerCase()}-${i}`,
    width: def.width, height: def.height, defUid: def.uid, px: [x, y], fieldInstances: [], __worldX: x, __worldY: y,
  };
}

// Walls (IntGrid): 1 ground, 2 brick; every non-zero cell is solid.
function walls() {
  return grid((set) => {
    for (let x = 0; x < W; x++) {
      if (x >= 24 && x < 27) continue;           // a pit
      set(x, 10, 1);
      set(x, 11, 1);
    }
    for (let x = 12; x < 16; x++) set(x, 7, 2);  // brick platforms
    for (let x = 30; x < 34; x++) set(x, 6, 2);
    for (let x = 40; x < 43; x++) set(x, 8, 2);
    for (let y = 0; y < 10; y++) { set(0, y, 2); set(W - 1, y, 2); }  // side walls
  });
}

function defsOf() {
  const tileset = {
    __cWid: 6, __cHei: 1, identifier: "Tiles", uid: 1, relPath: "../tiles/tiles.png", embedAtlas: null,
    pxWid: 96, pxHei: 16, tileGridSize: G, spacing: 0, padding: 0, tags: [], tagsSourceEnumUid: null, enumTags: [],
    customData: [{ tileId: 4, data: JSON.stringify({ frames: [4, 5], fps: 2 }) }], savedSelections: [], cachedPixelData: null,
  };
  return {
    entities: [entityDef(20, "Player", "#2E86DE", 16), entityDef(21, "Coin", "#F1C40F", 8)],
    layers: [
      layerDef(10, "Entities", "Entities"),
      layerDef(11, "Walls", "IntGrid", { intGridValues: [
        { value: 1, identifier: "ground", color: "#8E5B34", tile: null, groupUid: 0 },
        { value: 2, identifier: "brick", color: "#B5523B", tile: null, groupUid: 0 }] }),
      layerDef(12, "Ground", "Tiles", { tilesetDefUid: 1 }),
      layerDef(13, "Deco", "Tiles", { tilesetDefUid: 1 }),
    ],
    tilesets: [tileset], enums: [], externalEnums: [], levelFields: [],
  };
}

function levelOf(defs) {
  const solid = walls();
  const deco = grid((set) => {
    for (const x of [5, 18, 37, 50]) set(x, 9, 3);  // bushes
    for (const x of [8, 21, 45, 53]) set(x, 9, 4);  // flowers (animated)
  });
  // brick, dirt under ground, grass on top
  const ground = tilesOf(solid, (i) => (solid[i] === 2 ? 2 : i >= W && solid[i - W] === 1 ? 1 : 0));
  const [player, coinDef] = defs.entities;
  const coins = [[100, 150], [130, 150], [216, 100], [500, 84], [530, 84], [660, 116], [800, 150], [880, 150]];
  const [entities, intgrid, tiles, decoDef] = defs.layers;
  return {
    identifier: "Level_0", iid: "level-0", uid: 0, worldX: 0, worldY: 0, worldDepth: 0, pxWid: W * G, pxHei: H * G,
    __bgColor: "#6EC1E4", bgColor: "#6EC1E4", useAutoIdentifier: false, bgRelPath: null, bgPos: null, bgPivotX: 0.5,
    bgPivotY: 0.5, __smartColor: "#ADADB5", __bgPos: null, externalRelPath: null, fieldInstances: [], __neighbours: [],
    layerInstances: [  // top layer first, as LDtk saves them
      layer(entities, { entityInstances: [entity(player, 0, [40, 140]), ...coins.map((c, i) => entity(coinDef, i, c))] }),
      layer(intgrid, { intGridCsv: solid }),
      layer(tiles, { gridTiles: ground }),
      layer(decoDef, { gridTiles: tilesOf(deco, (i) => deco[i]) }),
    ],
  };
}

function world() {
  const defs = defsOf();
  const project = {
    __header__: { fileType: "LDtk Project JSON", app: "LDtk", doc: "https://ldtk.io/json", schema: "https://ldtk.io/files/JSON_SCHEMA.json",
      appAuthor: "Sebastien Benard", appVersion: "1.5.3", url: "https://ldtk.io" },
    iid: "platformer-world", jsonVersion: "1.5.3", appBuildId: 473703, nextUid: 30, identifierStyle: "Capitalize", toc: [],
    worldLayout: "Free", worldGridWidth: 256, worldGridHeight: 256, defaultLevelWidth: W * G, defaultLevelHeight: H * G,
    defaultPivotX: 0, defaultPivotY: 0, defaultGridSize: G, defaultEntityWidth: G, defaultEntityHeight: G,
    bgColor: "#40465B", defaultLevelBgColor: "#6EC1E4", minifyJson: false, externalLevels: false, exportTiled: false,
    simplifiedExport: false, imageExportMode: "None", exportLevelBg: true, pngFilePattern: null, backupOnSave: false,
    backupLimit: 10, backupRelPath: null, levelNamePattern: "Level_%idx", tutorialDesc: null, customCommands: [], flags: [],
    defs, levels: [levelOf(defs)], worlds: [], dummyWorldIid: "platformer-dummy",
  };
  fs.rmSync(path.join(root, "assets/maps"), { recursive: true, force: true });
  fs.mkdirSync(path.join(root, "assets/levels"), { recursive: true });
  fs.writeFileSync(path.join(root, "assets/levels/world.ldtk"), JSON.stringify(project, null, 1));
}

hero();
coin();
tiles();
world();
console.log("wrote assets/img, assets/tiles, assets/levels");
