// alpha-star script API. The engine provides these modules at runtime;
// scripts import them and the compiler leaves the imports external.
// Units: pixels, seconds, degrees. Origin top-left, +y down.
//
// Pattern (one way to do it):
//   1. types.ts   — `export interface MyGame { actions; sounds; tags; scenes }`
//   2. alpha.ts   — `export const { scene, game } = define<MyGame>();`
//   3. scenes/*.ts — `export default scene("play", { state, enter, update })`
//   4. main.ts    — `export default game({ config, start, scenes, assets })`

/**
 * Debug output: printed to stderr and shown in the engine's dev tools console
 * (F12 in dev and release builds). Arguments are joined with spaces; non-strings as JSON.
 */
declare var console: {
  log(...args: unknown[]): void;
  warn(...args: unknown[]): void;
  error(...args: unknown[]): void;
};

declare module "alpha" {
  // ---- game description (written once by the agent) ----------------------

  /** Every name a game uses. Misspelling any of them is a compile error. */
  export interface GameTypes {
    /** Input actions, e.g. "jump" | "fire". */
    actions: string;
    /** Analog axes (config.axes), e.g. "moveX" | "moveY". Optional: omit if the game has none. */
    axes?: string;
    /** Sound asset names. */
    sounds: string;
    /** Image asset names (use `never` if the game has none). */
    images: string;
    /** Entity tags used by find / collisions. */
    tags: string;
    /** Scene name -> params its enter() receives (`undefined` = none). */
    scenes: { [name: string]: SceneParams };
    /**
     * UI document name (assets.ui) -> ids of its elements that scripts touch,
     * e.g. `{ hud: "score"; over: "score" | "again" }`. Use `{}` if the game has no UI,
     * `never` for a document scripts only show/hide.
     */
    ui: UiTypes;
    /**
     * Sprite sheet name (assets.sheets) -> its animation names,
     * e.g. `{ hero: "idle" | "run" | "jump" }`. Optional: omit if the game has none.
     */
    sheets?: { [sheet: string]: string };
    /** Tiled map names (assets.maps), e.g. "level1". Optional. */
    maps?: string;
    /** Draw layer names (config.layers), e.g. "bg" | "fx". Optional. */
    layers?: string;
    /** Font names (assets.fonts), used by world text and .rcss. Optional (any name). */
    fonts?: string;
    /** Collision category names (config.collision.categories), e.g. "player" | "enemy". Optional. */
    categories?: string;
    /** Streamed music names (assets.music), e.g. "theme" | "boss". Optional. */
    music?: string;
    /** Locale names (assets.locales), e.g. "en" | "pt". Optional. */
    locales?: string;
    /** Extra volume buses (config.audio.buses), e.g. "ambience". Optional. */
    buses?: string;
  }

  /** The game's music names (`never` without `music`). */
  export type MusicOf<G extends GameTypes> = G extends { music: infer M extends string } ? M : never;
  /** The game's locale names (`string` without `locales`). */
  export type LocaleOf<G extends GameTypes> = G extends { locales: infer L extends string } ? L : string;
  /** Built-in buses plus config.audio.buses. */
  export type BusOf<G extends GameTypes> = "master" | "music" | "sfx" | "voice" | (G extends { buses: infer B extends string } ? B : never);

  /** The game's layer names (`never` without `layers`). */
  export type LayerOf<G extends GameTypes> = G extends { layers: infer L extends string } ? L : never;
  /** The game's font names (`string` without `fonts`). */
  export type FontOf<G extends GameTypes> = G extends { fonts: infer F extends string } ? F : string;

  /** The game's sheets (`{}` when GameTypes has no `sheets`). */
  export type SheetsOf<G extends GameTypes> = G extends { sheets: infer S extends { [k: string]: string } } ? S : {};
  /** Every animation name of every sheet. */
  export type AnimationOf<G extends GameTypes> = SheetsOf<G>[keyof SheetsOf<G>] & string;
  /** The game's map names (`never` without `maps`). */
  export type MapOf<G extends GameTypes> = G extends { maps: infer M extends string } ? M : never;

  export type UiTypes = { [doc: string]: string };

  export type SceneParams = { [key: string]: SaveValue } | undefined;

  // ---- values ---------------------------------------------------------------

  export type Vec2 = [x: number, y: number];

  /** Live vector owned by the engine. */
  export interface Vec2Ref {
    x: number;
    y: number;
  }

  export type Color = `#${string}`;

  export type SaveValue = number | string | boolean | null | SaveValue[] | { [k: string]: SaveValue };

  // ---- entities -------------------------------------------------------------

  /** Surface of a collider. Defaults: friction 0.6, restitution 0, density 1 (kg per m², 50 px = 1 m). */
  export interface ColliderMaterial {
    sensor?: boolean;
    friction?: number;
    /** Bounciness 0..1. */
    restitution?: number;
    density?: number;
    /** Solid only for bodies coming from above (smaller y); they jump up through it. */
    oneWay?: boolean;
  }

  export type Collider = ({ circle: number } | { rect: Vec2 }) & ColliderMaterial;

  /** Mirroring of an image or sheet sprite. */
  export interface Flips {
    flipX?: boolean;
    flipY?: boolean;
  }

  /** A sprite from a sheet: starts at frame 0, playing `animation` if given; `size` defaults to the frame size. */
  export type SheetShape<S extends { [k: string]: string } = {}> = {
    [K in keyof S & string]: { sheet: K; animation?: S[K]; size?: Vec2 } & Flips;
  }[keyof S & string];

  /** How a look combines with what is under it. */
  export type Blend = "alpha" | "add" | "multiply";

  /** Every look: multiplied by `tint` ("#rrggbbaa" works) and `alpha` (0..1), blended with `blend`. */
  export interface Look {
    tint?: Color;
    alpha?: number;
    blend?: Blend;
  }

  /**
   * World text: drawn in the world (moves with the camera, z ordered like any shape).
   * With `font` (an assets.fonts name) it is drawn `size` pixels high; without, the
   * built-in 8x8 font at the nearest whole multiple of 8. Glyphs are rasterized at the
   * screen's resolution (window scale x camera zoom), so text stays sharp. `pos` is the
   * anchor: `align` (default "center") picks which end sits on it; vertically centered.
   * ASCII + Latin-1 (others draw as '?').
   */
  export interface TextShape<F extends string = never> {
    text: string;
    font?: F;
    /** Pixels (1..256), default 16. */
    size?: number;
    /** Default "#ffffff". */
    color?: Color;
    align?: "left" | "center" | "right";
  }

  /** A filled shape, an image (`size` defaults to the image's pixel size), a sheet sprite, or world text. */
  export type Shape<I extends string = never, S extends { [k: string]: string } = {}, F extends string = never> = (
    | { circle: number; color: Color; outline?: Color; smooth?: boolean }
    | { rect: Vec2; color: Color; outline?: Color; smooth?: boolean }
    | ({ image: I; size?: Vec2 } & Flips)
    | SheetShape<S>
    | TextShape<F>
  ) & Look;

  /** One number (both ends) or [min, max] (or [start, end]). */
  export type Span = number | [number, number];

  /**
   * A particle emitter on the entity. Particles are born at the entity (it may move:
   * they follow it as they are born) and then fly on their own in world space.
   * Simulated on the fixed step with the game's seeded random (deterministic), at most
   * 20000 live particles in the whole scene. A one-shot emitter emits its `burst`
   * once and its entity is destroyed when the last particle dies.
   */
  export interface EmitterSpec<I extends string = never, S extends string = never> extends Look {
    /** Particles per second (default 0: only bursts). */
    rate?: number;
    /** Particles on the first step (default 0). `entity.emit(n)` bursts later. */
    burst?: number;
    /** Seconds each particle lives (default 1). */
    lifetime?: Span;
    /** Pixels per second (default 0). */
    speed?: Span;
    /** Degrees: 0 = right, 90 = down, 270 = up (default [0, 360]: every way). */
    angle?: Span;
    /** Pixels per second squared, e.g. [0, 300]. */
    gravity?: Vec2;
    /** Pixel size at birth and at death (default 4). */
    size?: Span;
    /** One color, or [birth, death] (default ["#ffffffff", "#ffffff00"]: fades out). */
    color?: Color | [Color, Color];
    /** Particle shape (default "circle"); images and sheet frames are tinted by `color`. */
    shape?: "circle" | "rect" | { image: I } | { sheet: S; frame?: number };
    /** Most live particles of this emitter, 1..20000 (default 100). */
    max?: number;
    oneShot?: boolean;
  }

  export interface BodySpec<C extends string = string> {
    type: "static" | "kinematic" | "dynamic";
    collider: Collider;
    velocity?: Vec2;
    /** Never rotates (platformer heroes). */
    fixedRotation?: boolean;
    /** Multiplies config.gravity for this body (0 = floats). Default 1. */
    gravityScale?: number;
    /** Continuous collision against moving bodies too (fast projectiles). */
    bullet?: boolean;
    /** A config.collision.categories name (default: the implicit "no category"). */
    category?: C;
    /** Categories it collides with (default: all). Both bodies must accept each other. */
    collidesWith?: C[];
  }

  /**
   * A 2D light: full `color` x `intensity` at the entity, fading to nothing at `radius`
   * world pixels. Lights add up in a lightmap that multiplies the world (config.lighting).
   */
  export interface LightSpec {
    /** World pixels, 1..4096. */
    radius: number;
    /** Default "#ffffff". */
    color?: string;
    /** 0..8, default 1. */
    intensity?: number;
  }

  /** Built-in full-screen effects (ctx.postfx). */
  export type PostFxName = "bloom" | "crt" | "pixelate" | "tint" | "vignette";
  /** Parameters per effect; missing ones take their defaults. */
  export interface PostFxParams {
    /** Glow around bright pixels: strength 0..2 (0.6), radius 1..16 game px (4). */
    bloom: { strength?: number; radius?: number };
    /** Scanlines + vignette: strength 0..1 (0.5). */
    crt: { strength?: number };
    /** Blocks of `size` game px, 1..64 (4). */
    pixelate: { size?: number };
    /** Color grade: the screen multiplied by `color` ("#ffffff"), faded by strength 0..1 (1). */
    tint: { color?: string; strength?: number };
    /** Dark edges: strength 0..1 (0.5); radius 0..1 (0.5) = clear part from the center. */
    vignette: { strength?: number; radius?: number };
  }
  /**
   * The current scene's post effects, drawn over the (lit) world in the order first
   * added; UI and dev tools stay unaffected. Each scene starts with none. Render-only:
   * simulate() and game state never see them. Works on every renderer (software too).
   */
  export interface PostFx {
    /** Adds the effect, or replaces its params when already added (keeping its place). */
    add<N extends PostFxName>(name: N, params?: PostFxParams[N]): void;
    /** Like add, for an effect already added (error otherwise). Missing params reset to defaults. */
    set<N extends PostFxName>(name: N, params?: PostFxParams[N]): void;
    /** False when it was not added. */
    remove(name: PostFxName): boolean;
    clear(): void;
    /** Names in draw order. */
    list(): PostFxName[];
  }

  /** Plain data describing an entity. Build it with functions, not classes. */
  export interface EntitySpec<T extends string = string, I extends string = never, S extends { [k: string]: string } = {},
    L extends string = never, F extends string = never, C extends string = string> {
    tags: T[];
    pos: Vec2;
    shape?: Shape<I, S, F>;
    body?: BodySpec<C>;
    emitter?: EmitterSpec<I, keyof S & string>;
    /** A light riding on the entity; seen only with config.lighting. Render-only. */
    light?: LightSpec;
    rotation?: number;
    /** Order inside the entity's layer (default 0). */
    z?: number;
    /** A config.layers name (default: the unnamed layer, z 0, parallax 1). */
    layer?: L;
    /**
     * Makes it a child: `pos` and `rotation` are then local to the parent, it follows the
     * parent every step and is destroyed with it.
     */
    parent?: Entity<T, any>;
    /** Game data (JSON-safe plain object), copied; read and change it as `entity.data`. */
    data?: EntityData;
  }

  export type EntityData = { [k: string]: SaveValue };

  export interface Body {
    get velocity(): Vec2Ref;
    set velocity(v: Vec2);
    /** Degrees per second (positive = clockwise on screen). */
    angularVelocity: number;
    /** kg (0 for static and kinematic bodies). */
    readonly mass: number;
    readonly type: "static" | "kinematic" | "dynamic";
    /** For this step, at the center. A force of mass * a accelerates by a px/s². */
    applyForce(force: Vec2): void;
    /** Instant: an impulse of mass * dv changes the velocity by dv px/s. */
    applyImpulse(impulse: Vec2): void;
    applyTorque(torque: number): void;
  }

  /** A solid contact as an onHit handler sees it. */
  export interface Contact {
    /** Unit normal from the handler's first entity toward the second. */
    normal: Vec2;
    /** Where they touch. */
    point: Vec2;
    /** How fast they approached along the normal (px/s); 0 when they slid into contact. */
    speed: number;
  }

  export interface RayHit<T extends string = string> {
    entity: Entity<T, any>;
    point: Vec2;
    /** Out of the hit surface. */
    normal: Vec2;
    /** 0..1 along from -> to. */
    fraction: number;
  }

  /** What a raycast or area query considers (default: every body). */
  export interface QueryFilter<T extends string = string, C extends string = string> {
    /** Only entities having any of these tags. */
    tags?: T[];
    /** Only bodies in these config.collision categories. */
    categories?: C[];
  }

  /**
   * ctx.joint options. Anchors are world points at creation: `anchor` for revolute, weld and
   * prismatic (default: b's position); `anchors` [onA, onB] for distance and rope (default:
   * the bodies' positions). limits: degrees (revolute), px along the axis (prismatic),
   * [min, max] length (distance). motor: speed in degrees/s or px/s, max torque/force in
   * kg-pixel units (keep it near what the job needs: a huge max overpowers the limits).
   */
  export interface JointSpec<T extends string = string> {
    type: "revolute" | "distance" | "rope" | "weld" | "prismatic";
    a: Entity<T, any>;
    b: Entity<T, any>;
    anchor?: Vec2;
    anchors?: [Vec2, Vec2];
    /** Prismatic slide direction. Default [1, 0]. */
    axis?: Vec2;
    /** Distance and rope (rope: the most it stretches). Default: the anchors' distance. */
    length?: number;
    limits?: [lower: number, upper: number];
    motor?: { speed: number; max: number };
    /** The two bodies still collide with each other. Default false. */
    collide?: boolean;
  }

  export interface Joint {
    /** False once destroyed, or when either body was destroyed. */
    readonly alive: boolean;
    destroy(): void;
  }

  export type Ease =
    | "linear"
    | `${"quad" | "cubic" | "quart" | "quint" | "sine" | "expo" | "circ" | "back" | "elastic" | "bounce"}${"In" | "Out" | "InOut"}`;

  /** Entity keys a tween can drive. */
  export type EntityTweenKeys = "x" | "y" | "rotation" | "alpha" | "scale";

  export interface TweenOptions<K extends string> {
    to: Partial<Record<K, number>>;
    seconds: number;
    /** Default "linear". */
    ease?: Ease;
    /** Seconds before it starts (values are read when it starts). */
    delay?: number;
    /** Goes back to the start values after reaching `to` (one cycle = there and back). */
    yoyo?: boolean;
    /** Extra cycles: an integer >= 0, or Infinity. Default 0. */
    repeat?: number;
    onDone?: () => void;
  }

  export interface Tween {
    readonly done: boolean;
    readonly active: boolean;
    cancel(): void;
  }

  export interface Entity<T extends string = string, A extends string = string> {
    readonly id: number;
    readonly tags: readonly T[];
    pos: Vec2Ref;
    rotation: number;
    z: number;
    /** Throws (pausing the game) if the entity was spawned without `body`. */
    readonly body: Body;
    /** False after destroy(); stale references are safe to check. */
    readonly alive: boolean;
    /**
     * Sheet sprites: the playing animation ("" before any). Setting a different name
     * switches to it from its first frame; setting the same name keeps it going.
     * Advances on the fixed step. Throws (listing the sheet's animations) for unknown names.
     */
    animation: A | "";
    /** Plays `name` from its first frame, even if it is already playing. */
    play(name: A): void;
    /** Mirror the shape horizontally / vertically (images and sheets). */
    flipX: boolean;
    flipY: boolean;
    /**
     * Look of the shape (or of the emitter's particles when there is no shape).
     * Reads back as "#rrggbbaa". Throws for an entity with neither.
     */
    tint: Color;
    /** 0..1. */
    alpha: number;
    blend: Blend;
    /** World text entities: the line shown. Throws for other entities. */
    text: string;
    /** The config.layers name ("" = default layer). */
    readonly layer: string;
    /** Emitters: `count` particles on the next step. Throws without `emitter`. */
    emit(count: number): void;
    /** Emitters: `rate` emission on/off (bursts still work). */
    emitting: boolean;
    /** Live particles of the entity's emitter (0 without one). */
    readonly particles: number;
    /** Size multiplier of the shape (1 = as spawned; text scales its size). Must be > 0. */
    scale: number;
    /** The entity it was spawned under (`spawn({ parent })`), or null. */
    readonly parent: Entity<T, A> | null;
    /** Living children, oldest first. */
    readonly children: Entity<T, A>[];
    /**
     * Game data: a JSON-safe plain object (spawn's `data`, or {}), changed in place
     * (`e.data.hp -= 1`) or replaced. Tools (MCP state) see it after each step.
     */
    data: EntityData;
  }

  /** ctx.window: the game window. Ignored by headless runs except for reading back. */
  export interface GameWindow {
    /** Switches at the end of the frame. Alt+Enter toggles it too (config.window.altEnter). */
    fullscreen: boolean;
    /** Window size in real pixels (the game draws in config.size game pixels). */
    readonly size: Vec2;
  }

  /**
   * config.window. scaling: "letterbox" (default; largest fit, bars), "integer" (whole
   * multiples only: crisp pixel art, bars), "stretch" (fills, aspect lost), "expand"
   * (fills keeping pixels square by showing more world; ctx.window.size grows the view),
   * "crop" (fills keeping the aspect; the edges that do not fit are cut off).
   */
  export interface WindowConfig {
    fullscreen?: boolean;
    scaling?: "letterbox" | "integer" | "stretch" | "expand" | "crop";
    /** Default true. */
    resizable?: boolean;
    /** Alt+Enter toggles fullscreen (default true). */
    altEnter?: boolean;
    /** No title bar or frame (default false). */
    borderless?: boolean;
    /** Stays above other windows (default false). */
    alwaysOnTop?: boolean;
    /** Starts maximized (default false). */
    maximized?: boolean;
    /** Wait for the display refresh (default true); false draws as fast as possible (more CPU/GPU). */
    vsync?: boolean;
  }

  /** config.layers entry: drawn in `z` order (then entity z); parallax 0 = fixed to the screen, 1 = world (default). */
  export interface LayerDef {
    z: number;
    parallax?: number;
  }

  /** ctx.positions(tag): ids[i] is at (x[i], y[i]); write() sets every position back. */
  export interface Positions {
    readonly count: number;
    readonly ids: Float64Array;
    readonly x: Float64Array;
    readonly y: Float64Array;
    write(): void;
  }

  /**
   * ctx.camera: which part of the world the screen shows. Each scene starts with the
   * identity camera (pos = center of config.size, zoom 1): world pixels = screen pixels.
   * Moves on the fixed step (deterministic); the UI and overlays never move with it.
   */
  export interface Camera {
    /** World point at the screen center. */
    pos: Vec2Ref;
    /** > 0; 2 = everything twice as big. */
    zoom: number;
    /** Degrees; the world turns the other way on screen. */
    rotation: number;
    /**
     * Keeps `entity` in view, or stops following (null). `lerp`: fraction of the remaining
     * distance covered per fixed step, (0, 1], default 1 (locked). `deadzone`: [w, h] box
     * around the center inside which the entity moves without moving the camera.
     */
    follow(entity: Entity<any, any> | null, opts?: { lerp?: number; deadzone?: Vec2 }): void;
    /** Shakes by up to `intensity` pixels, fading out over `seconds` (deterministic). */
    shake(intensity: number, seconds: number): void;
    /** Keeps the visible area inside [x, y, width, height] (e.g. the map's size); null removes them. */
    bounds(rect: [x: number, y: number, width: number, height: number] | null): void;
    /** Screen point (game pixels, e.g. ctx.input.mouse()) -> world point. */
    toWorld(screen: Vec2): Vec2;
  }

  /** One object of a Tiled object layer, as plain data. */
  export interface MapObject {
    id: number;
    name: string;
    /** Tiled's class / type field. */
    type: string;
    /** Top-left in map pixels (point objects: the point). */
    x: number;
    y: number;
    width: number;
    height: number;
    rotation: number;
    point: boolean;
    properties: { [name: string]: string | number | boolean };
  }

  /** A loaded Tiled map (ctx.map / ctx.spawnMap). */
  export interface TileMap {
    readonly name: string;
    /** In tiles. */
    readonly width: number;
    readonly height: number;
    readonly tileSize: Vec2;
    /** In pixels. */
    readonly size: Vec2;
    readonly properties: { [name: string]: string | number | boolean };
    /** Layer names in draw order. */
    readonly layers: string[];
    /** A copy of an object layer's objects; throws listing the object layers. */
    objects(layer: string): MapObject[];
  }

  export interface SpawnMapOptions<T extends string> {
    /** Tile layers that become static colliders (merged rects) tagged with the layer name. */
    collide?: T[];
    /** z of every tile layer (default 0; later layers draw over earlier ones). */
    z?: number;
    /** Where the map's top-left goes (default [0, 0]). */
    pos?: Vec2;
    /** config.layers layer of every tile layer (default: the unnamed layer). */
    layer?: string;
    /** Per Tiled tile layer name: its config.layers layer (overrides `layer`). */
    layers?: { [tiledLayer: string]: string };
  }

  // ---- services -------------------------------------------------------------

  /** The game's axis names (`never` when GameTypes has no `axes`). */
  export type AxisOf<G extends GameTypes> = G extends { axes: infer X extends string } ? X : never;

  /** Player number: player 1 has the keyboard, the mouse and the first gamepad. */
  export type Player = 1 | 2 | 3 | 4;

  /** One player's input (or any player's, on ctx.input itself). */
  export interface PlayerInput<A extends string, X extends string> {
    /** True only on the step the action went down. */
    pressed(action: A): boolean;
    /** True while held. */
    down(action: A): boolean;
    /** True only on the step the action went up. */
    released(action: A): boolean;
    /**
     * Axis value in [-1, 1] after config.deadzone (sticks +x right, +y down;
     * triggers 0..1). On ctx.input: the player with the largest magnitude.
     */
    axis(name: X): number;
  }

  /** ctx.input: actions and axes of any player; `player(n)` for one. */
  export interface Input<A extends string, X extends string = never> extends PlayerInput<A, X> {
    /** Gamepads get players in connection order; a reconnected pad gets its slot back. */
    player(n: Player): PlayerInput<A, X>;
    /** Mouse pointer in game pixels (screen space; ctx.camera.toWorld for the world). */
    mouse(): Vec2;
    /** Players with a device: 1 (keyboard) up to the highest connected gamepad's player. */
    players(): number;
    /** Vibrates gamepads: strength 0..1 for 0..10 seconds (0 stops); every player if omitted. No-op without a pad. */
    rumble(strength: number, seconds: number, player?: Player): void;
    /**
     * The player's own inputs for an action (a controls menu), replacing config.actions'
     * list; persisted in settings and applied at every boot. `ctx.settings.reset()` undoes it.
     */
    rebind(action: A, inputs: string[]): void;
    /** The inputs an action has now (rebound or from config.actions). */
    bindings(action: A): string[];
  }

  /** Seeded by config.seed. Math.random is disabled. */
  export interface Random {
    /** Float in [min, max). */
    range(min: number, max: number): number;
    /** Integer in [min, max]. */
    int(min: number, max: number): number;
    pick<T>(items: readonly T[]): T;
  }

  /**
   * Persistent key/value storage; survives restarts. Each slot is one file in the
   * player's data folder, written atomically (temp file + rename, the previous one
   * kept as .bak; a damaged file falls back to its .bak with a logged warning) and
   * stamped with config.save.version (older files go through config.save.migrate).
   */
  export interface SaveSlot {
    get<T extends SaveValue>(key: string, fallback: T): T;
    /** JSON-safe values only (a copy is stored); writes the slot file. */
    set(key: string, value: SaveValue): void;
    has(key: string): boolean;
    remove(key: string): void;
    clear(): void;
    keys(): string[];
  }

  /** ctx.save: the "main" slot itself, plus named slots (e.g. one per save game). */
  export interface Save extends SaveSlot {
    /** Slot names: 1..32 letters, digits, '-' or '_'. slot("main") is ctx.save. */
    slot(name: string): SaveSlot;
  }

  /** config.save: version the save data; `migrate` upgrades older slots once when read. */
  export interface SaveConfig {
    /** Integer >= 1, default 1. Bump it when the data's shape changes. */
    version?: number;
    /** Returns the data in the current shape; called once per older slot (files before versioning are version 1). */
    migrate?(data: { [k: string]: SaveValue }, fromVersion: number): { [k: string]: SaveValue };
  }

  /**
   * ctx.settings: the player's preferences, kept apart from saves (settings.json) and
   * applied at boot. Set accessibility flags here; volumes, locale, fullscreen and
   * bindings are set where they live (audio.bus(b).volume, ctx.locale,
   * window.fullscreen, input.rebind), which persists them too.
   */
  export interface Settings<G extends GameTypes> {
    /** UI scale 0.5..3 (default 1): the RmlUi dp ratio, so sizes in `dp` grow with it. */
    uiScale: number;
    /** Adds class "high-contrast" to every UI document's body: style `.high-contrast ...` in .rcss. */
    highContrast: boolean;
    /** Camera shake intensity and particle counts (rate, burst, max, emit) scaled to 1/4. */
    reducedMotion: boolean;
    readonly volumes: Record<BusOf<G>, number>;
    readonly locale: LocaleOf<G> | null;
    readonly fullscreen: boolean;
    readonly bindings: Partial<Record<G["actions"], string[]>>;
    /** Back to defaults (volumes 1, config.locale, config.actions, flags off), applied and saved. */
    reset(): void;
  }

  export type Os = "windows" | "macos" | "linux" | "android" | "ios" | "web" | "other";
  export interface Platform {
    readonly os: Os;
    /** Phone or tablet (android, ios): touch first, no keyboard. */
    readonly mobile: boolean;
  }

  /** Placeholders `{name}` in a text are filled from params; `count` also picks plural forms. */
  export type TextParams = { [name: string]: string | number };

  /**
   * Game UI: RmlUi documents (HTML/CSS-like `.rml` + `.rcss`) declared in
   * assets.ui, loaded hidden before the first scene. Scene-scoped: on every
   * scene change all documents are reloaded hidden (texts back to the file's)
   * and all handlers dropped. Drawn over the game, in game pixels.
   * Focusable elements (buttons) need `tab-index: auto` in the RCSS; style
   * `:hover` / `:focus` / `:active`. Gamepad: D-pad / left stick move focus,
   * A clicks the focused element, B is "back"; mouse and touch click.
   * Input the UI uses (a click on an element with a handler, A/Enter/Space on a
   * focused element, navigation, typing in a text field) never also triggers
   * game actions.
   */
  export interface UI<U extends UiTypes> {
    show(doc: keyof U & string): void;
    hide(doc: keyof U & string): void;
    /** Sets the element's content to plain text (never parsed as markup). */
    set<D extends keyof U & string>(doc: D, id: U[D], text: string | number): void;
    /**
     * "click": the element (or anything inside it) is clicked by mouse, touch,
     * or Enter/Space/gamepad A while focused. "back": Escape or gamepad B while
     * the document is shown. Handlers run at the start of the next step.
     */
    on<D extends keyof U & string>(doc: D, id: U[D], event: "click" | "back", fn: () => void): void;
  }

  /** One procedural note. `freq` is Hz, or [from, to] for a slide. Seconds for times. */
  export interface Note {
    wave: "sine" | "square" | "triangle" | "saw" | "noise";
    freq: number | [from: number, to: number];
    duration: number;
    /** 0..1, default 0.5 */
    volume?: number;
    /** fade-in seconds, default 0.005 */
    attack?: number;
    /** fade-out seconds, default 0.05 */
    release?: number;
  }
  /** A note, or notes played one after another. Same spec = same sound, always. */
  export type Synth = Note | Note[];

  /**
   * A sound: a .wav/.ogg/.mp3 file path (decoded whole at boot) or a synthesized sound,
   * on the "sfx" bus; or `{ file, bus }` / `{ synth, bus }` to put it on another bus.
   */
  export type SoundAsset<B extends string = string> = string | Synth | { file: string; bus?: B } | { synth: Synth; bus?: B };

  export interface PlayOptions {
    /** 0..1, default 1 */
    volume?: number;
    /** playback speed, default 1 (2 = octave up) */
    pitch?: number;
    loop?: boolean;
    /** -1 (left) .. 1 (right), default 0 (both sides, full volume) */
    pan?: number;
  }

  export interface MusicOptions {
    /** 0..1, default 1 */
    volume?: number;
    /** default true */
    loop?: boolean;
  }

  /** A volume bus: every sound on it (and every bus, through "master") scales by `volume`. */
  export interface Bus {
    /** 0..1, default 1; persisted in settings. */
    volume: number;
  }

  /** A playing sound. Stopping twice is a no-op. */
  export interface Voice {
    readonly playing: boolean;
    stop(): void;
  }

  /**
   * Audio is output only: it never affects the simulation. Sounds go through
   * their bus, music through "music", everything through "master".
   */
  export interface Audio<S extends string, M extends string = never, B extends string = BusOf<GameTypes>> {
    /** Plays a preloaded sound from assets.sounds. */
    play(sound: S, opts?: PlayOptions): Voice;
    /** Synthesizes and plays a sound on the fly (sfx bus). */
    tone(synth: Synth, opts?: PlayOptions): Voice;
    /** Stops every sound and the music. */
    stopAll(): void;
    /**
     * Streams music from assets.music (decoded a little at a time, never loaded whole).
     * One track at a time: whatever played stops at once. `fadeIn` in seconds.
     */
    music(name: M, opts?: MusicOptions & { fadeIn?: number }): void;
    /** Fades the current music out and `name` in over `seconds`. */
    crossfade(name: M, seconds: number, opts?: MusicOptions): void;
    /** Fades the music out over `fadeOut` seconds (default 0 = at once). */
    stopMusic(fadeOut?: number): void;
    /** The music playing (not fading out), or null. */
    readonly musicPlaying: M | null;
    /** "master", "music", "sfx", "voice", then config.audio.buses. */
    readonly buses: readonly B[];
    bus(name: B): Bus;
  }

  // ---- scene context ----------------------------------------------------------

  type ParamArgs<P> = [P] extends [undefined] ? [] : [params: P];

  export type HitHandler<T extends string> = (a: Entity<T, any>, b: Entity<T, any>, contact: Contact) => void;
  export type PairHandler<T extends string> = (a: Entity<T, any>, b: Entity<T, any>) => void;
  export type CategoryOf<G extends GameTypes> = G extends { categories: infer C extends string } ? C : string;

  /** Everything a scene can touch. Scene-scoped: cleared on scene change. */
  export interface Context<G extends GameTypes, S> {
    readonly config: Readonly<GameConfig<G>>;
    /** Fixed step in seconds (constant). */
    readonly dt: number;
    /** Seconds since scene enter. */
    readonly time: number;
    /** Scene state built by the scene's `state()`; reset on every enter. */
    state: S;
    readonly input: Input<G["actions"], AxisOf<G>>;
    /**
     * Where the game runs, for hints and layout ("tap" vs "press Space"). Never use it for
     * game rules: the simulation stays the same everywhere. `alpha run --platform android`
     * previews another OS; tests pass `simulate(game, { platform })`.
     */
    readonly platform: Platform;
    readonly random: Random;
    readonly save: Save;
    readonly settings: Settings<G>;
    /**
     * The current locale (assets.locales; null without any). Setting it re-translates
     * UI elements with `data-t="key"` and is persisted in settings.
     */
    locale: LocaleOf<G> | null;
    /**
     * Text of `key` in the current locale (nested keys with dots: "menu.play"), with
     * `{name}` placeholders filled from params; a plural entry `{ zero?, one, other }`
     * is picked by params.count. Falls back to config.locale's text; a missing key
     * shows "[key]" (and is logged once in dev builds).
     */
    t(key: string, params?: TextParams): string;
    readonly ui: UI<G["ui"]>;
    readonly audio: Audio<G["sounds"], MusicOf<G>, BusOf<G>>;
    readonly camera: Camera;
    /** Full-screen effects of the current scene (bloom, crt, pixelate, tint, vignette). */
    readonly postfx: PostFx;
    readonly window: GameWindow;

    spawn(spec: EntitySpec<G["tags"], G["images"], SheetsOf<G>, LayerOf<G>, FontOf<G>, CategoryOf<G>>): Entity<G["tags"], AnimationOf<G>>;
    /** Removed at the end of the step. Destroying twice is a no-op. */
    destroy(entity: Entity<G["tags"], any>): void;
    /** Throws if no entity has the tag. */
    find(tag: G["tags"]): Entity<G["tags"], AnimationOf<G>>;
    findAll(tag: G["tags"]): Entity<G["tags"], AnimationOf<G>>[];
    /**
     * Every living entity with the tag, positions in bulk (oldest first). For many
     * entities per frame this is several times cheaper than `entity.pos` per entity:
     * edit `x[i]` / `y[i]`, then `write()` once. Entities destroyed since are skipped.
     */
    positions(tag: G["tags"]): Positions;

    /**
     * Draws the map's visible tile layers (one batched, culled entity per layer and
     * tileset) and makes `collide` layers solid. Scene-scoped like spawn: call it in enter.
     */
    spawnMap(name: MapOf<G>, opts?: SpawnMapOptions<G["tags"]>): TileMap;
    /** A map's data (sizes, object layers) without spawning anything. */
    map(name: MapOf<G>): TileMap;

    /** Solid contact between tags starts. `a` has tagA, `b` has tagB; `contact` normal points a -> b. */
    onHit(tagA: G["tags"], tagB: G["tags"], fn: HitHandler<G["tags"]>): void;
    /** Solid contact between tags ends (both still alive). */
    onHitEnd(tagA: G["tags"], tagB: G["tags"], fn: PairHandler<G["tags"]>): void;
    /** Overlap with a sensor starts. */
    onEnter(tagA: G["tags"], tagB: G["tags"], fn: PairHandler<G["tags"]>): void;
    /** Overlap with a sensor ends (both still alive). */
    onExit(tagA: G["tags"], tagB: G["tags"], fn: PairHandler<G["tags"]>): void;

    /**
     * The app paused: window lost focus or was minimized (unless `config.pauseOnFocusLoss` is
     * false) or the OS sent it to the background. No steps run and audio stops until it resumes.
     * Save here if progress must survive the OS killing a backgrounded game. Scene-scoped.
     */
    onPause(fn: (ctx: Context<G, S>) => void): void;
    /** The app runs again (no catch-up: the paused time is skipped). Scene-scoped. */
    onResume(fn: (ctx: Context<G, S>) => void): void;
    /** The OS is low on memory (mobile): free what you can. Scene-scoped. */
    onLowMemory(fn: (ctx: Context<G, S>) => void): void;
    /** App state: `paused` is true between onPause and onResume. */
    readonly app: { readonly paused: boolean };

    /** First solid collider on the segment (sensors ignored), or null. */
    raycast(from: Vec2, to: Vec2, opts?: QueryFilter<G["tags"], CategoryOf<G>>): RayHit<G["tags"]> | null;
    /** Every solid collider on the segment, nearest first. */
    raycastAll(from: Vec2, to: Vec2, opts?: QueryFilter<G["tags"], CategoryOf<G>>): RayHit<G["tags"]>[];
    /** Entities whose colliders (sensors too) overlap the circle, oldest first. */
    queryCircle(center: Vec2, radius: number, opts?: QueryFilter<G["tags"], CategoryOf<G>>): Entity<G["tags"], AnimationOf<G>>[];
    /** Entities whose colliders overlap [x, y, width, height] (top-left), oldest first. */
    queryRect(rect: [x: number, y: number, width: number, height: number], opts?: QueryFilter<G["tags"], CategoryOf<G>>): Entity<G["tags"], AnimationOf<G>>[];
    /** Connects two bodies. Scene-scoped; goes away with either body. */
    joint(spec: JointSpec<G["tags"]>): Joint;
    /**
     * Eases numbers on the fixed step (deterministic, scene-scoped). An entity tweens
     * x, y, rotation, alpha, scale (a kinematic body's x/y moves by velocity, so riders are
     * carried); a plain object tweens its number keys.
     */
    tween(target: Entity<G["tags"], any>, opts: TweenOptions<EntityTweenKeys>): Tween;
    tween<O extends object>(target: O, opts: TweenOptions<{ [K in keyof O]: O[K] extends number ? K : never }[keyof O] & string>): Tween;

    /** Repeats every `seconds` (>= 0.001) of game time, first call after `seconds`. */
    every(seconds: number, fn: () => void): void;
    /** Runs once after `seconds` (>= 0.001) of game time. */
    after(seconds: number, fn: () => void): void;

    /** Switches scene at the end of this step. */
    go<K extends keyof G["scenes"] & string>(scene: K, ...params: ParamArgs<G["scenes"][K]>): void;
  }

  // ---- scenes and game --------------------------------------------------------

  export interface SceneDef<G extends GameTypes, P, S> {
    /** Builds fresh scene state on every enter. Omit for stateless scenes. */
    state?: (params: P) => S;
    enter?(ctx: Context<G, S>, params: P): void;
    update?(ctx: Context<G, S>): void;
    exit?(ctx: Context<G, S>): void;
  }

  /** Opaque scene value returned by `scene()`. */
  export interface Scene<G extends GameTypes, K extends keyof G["scenes"]> {
    readonly name: K;
    readonly __game?: G;
  }

  export interface GameConfig<G extends GameTypes> {
    title: string;
    size: Vec2;
    background?: Color;
    seed: number;
    gravity?: Vec2;
    /**
     * Android export (`alpha export --target android-arm64`): the package id (default
     * "org.alphastar.<title letters>"), the launcher icon (a PNG in the project), versionCode
     * (whole number, default 1) and version (default "1.0"). The screen orientation follows
     * `size`: portrait only when taller than wide.
     */
    android?: { package?: string; icon?: string; versionCode?: number; version?: string };
    /**
     * Action -> device inputs, e.g. ["Space", "MouseLeft", "GamepadA"]. Sticks and
     * triggers press too: "GamepadLeftY-" (left stick up), "GamepadLeftX+", "GamepadRT".
     * Touch screens: "Touch" (any finger), "TouchLeft" / "TouchRight" (a finger on that
     * half of the screen; a touch is also "MouseLeft" at ctx.input.mouse()); "Back" is
     * the Android back button (bind it with "Escape": `back: ["Escape", "Back"]`).
     * The name "axes" is reserved.
     */
    actions: Record<G["actions"], string[]>;
    /**
     * Axis -> sources; the axis reads the strongest one. A source is an analog axis
     * ("GamepadLeftX", "GamepadLeftY", "GamepadRightX", "GamepadRightY", "GamepadLT",
     * "GamepadRT") or a "Negative/Positive" input pair: "A/D", "Left/Right".
     * e.g. `{ moveX: ["GamepadLeftX", "A/D", "Left/Right"] }`.
     */
    axes?: Record<AxisOf<G>, string[]>;
    /** Stick (radial) and trigger deadzone, 0 <= d < 1. Default 0.2. */
    deadzone?: number;
    /**
     * Collision categories (at most 63) for body `category` / `collidesWith` and query
     * filters, e.g. `{ categories: ["player", "enemy", "world"] }`.
     */
    collision?: { categories: CategoryOf<G>[] };
    /**
     * Look default: true (default) = crisp pixel art (nearest image filtering, hard shape
     * edges), false = smooth (linear filtering, antialiased circle/rect edges). Images, sheets
     * and maps override it with `filter`; fonts with `{ file, pixelMode }`; filled shapes with `smooth`.
     */
    pixelMode?: boolean;
    /** Named draw layers, e.g. `{ bg: { z: -100, parallax: 0.2 }, fx: { z: 100 } }`. */
    layers?: Record<LayerOf<G>, LayerDef>;
    /**
     * 2D lighting: the world is multiplied by a lightmap that is `ambient` where no light
     * reaches, plus every entity `light`. Off when absent. Render-only (UI unaffected).
     */
    lighting?: { ambient: string };
    window?: WindowConfig;
    /** Pause (ctx.onPause) when the window loses focus or is minimized. Default true. */
    pauseOnFocusLoss?: boolean;
    /** Extra volume buses next to master, music, sfx and voice, e.g. `{ buses: ["ambience"] }`. */
    audio?: { buses?: Exclude<BusOf<G>, "master" | "music" | "sfx" | "voice">[] };
    /** Save versioning and migration. */
    save?: SaveConfig;
    /** Default and fallback locale (default: the first of assets.locales). */
    locale?: LocaleOf<G>;
  }

  /** Scenes enterable without params. */
  type StartScene<G extends GameTypes> = {
    [K in keyof G["scenes"]]: [G["scenes"][K]] extends [undefined] ? K : never;
  }[keyof G["scenes"]];

  export interface GameDef<G extends GameTypes> {
    config: GameConfig<G>;
    start: StartScene<G>;
    /** Every declared scene, exactly once. */
    scenes: { [K in keyof G["scenes"]]: Scene<G, K> };
    /** Every asset is loaded before the start scene; a missing file stops boot with its name. */
    assets: {
      sounds: Record<G["sounds"], SoundAsset<BusOf<G>>>;
      /** Music name -> .ogg/.mp3/.wav file, streamed when played (checked at boot, never preloaded). */
      music?: Record<MusicOf<G>, string>;
      /**
       * Locale name -> JSON file of texts, e.g. `{ en: "locales/en.json", pt: "locales/pt.json" }`.
       * Texts are UTF-8; world text draws ASCII + Latin-1, RmlUi whatever its fonts have.
       */
      locales?: Record<LocaleOf<G>, string>;
      /** Image name -> file, or `{ file, filter }` to override config.pixelMode for it. */
      images?: Record<G["images"], ImageAsset>;
      /**
       * Font name -> .ttf/.otf file or AngelCode BMFont .fnt (text or XML; its PNG pages are
       * packed too); the name is the `font-family` used in .rcss and `font` of world text.
       * `{ file, license }` names the font's license file: `alpha export` copies it and puts
       * its text in the pack's THIRD_PARTY_NOTICES.txt. The FONT's license is what matters
       * (OFL is fine; a bitmap font made from a font follows that font's license).
       * No fonts at all: UI and world text use the engine's built-in 8x8 font (public domain).
       */
      fonts?: Record<FontOf<G>, FontAsset>;
      /** Document name -> .rml file. Files it links (`<link href>`, `src=`, rcss `url()`) are packed too. */
      ui?: Record<keyof G["ui"] & string, string>;
      /**
       * Sprite sheets: a grid of `frame`-sized cells in `image` (row-major, frame 0 top-left)
       * and named animations over frame indices, e.g.
       * `{ hero: { image: "img/hero.png", frame: [16, 16], animations: { run: { frames: [0, 1, 2, 3], fps: 10 } } } }`.
       */
      sheets?: { [K in keyof SheetsOf<G>]: SheetAsset<SheetsOf<G>[K] & string> };
      /** Map name -> Tiled map (.tmj JSON; tilesets embedded or .tsj; CSV tile layers). */
      maps?: Record<MapOf<G>, ImageAsset>;
    };
  }

  /**
   * How an image is sampled when drawn scaled or rotated:
   * - "nearest": crisp pixel art; at non-whole scales or rotated, pixels come out uneven/jagged.
   * - "linear": smooth (photos, painted art); pixel art gets blurry.
   * - "sharp": pixel art at any size or angle: square pixels with soft ~1 screen-pixel edges
   *   (the texture is grown 4x with nearest, then drawn linear; 16x the texture memory).
   * Default: config.pixelMode (true = "nearest", false = "linear").
   */
  export type ImageFilter = "nearest" | "linear" | "sharp";
  /** An image (or a map's tilesets): "path", or `{ file, filter }`. */
  export type ImageAsset = string | { file: string; filter?: ImageFilter };
  /**
   * A font: "path" or `{ file, pixelMode, license }`; `license` is its license file, credited in the pack.
   * Text is rasterized at the screen's real resolution, so it stays sharp at any window
   * scale; bitmap fonts draw at whole multiples of their size. pixelMode true: TrueType
   * glyphs are rendered monochrome (pixel fonts stay crisp).
   */
  export type FontAsset = string | { file: string; pixelMode?: boolean; license?: string };

  export interface AnimationDef {
    /** Frame indices into the sheet. */
    frames: number[];
    /** Frames per second, default 10. */
    fps?: number;
    /** Default true; a one-shot holds its last frame. */
    loop?: boolean;
  }

  export interface SheetAsset<A extends string> {
    image: string;
    frame: Vec2;
    animations?: Record<A, AnimationDef>;
    /** Texture filtering of the sheet (see ImageFilter; default: config.pixelMode). */
    filter?: ImageFilter;
  }

  export interface Game<G extends GameTypes> {
    readonly __game?: G;
  }

  export interface Builders<G extends GameTypes> {
    scene<K extends keyof G["scenes"] & string, S = undefined>(
      name: K,
      def: SceneDef<G, G["scenes"][K], S>,
    ): Scene<G, K>;
    game(def: GameDef<G>): Game<G>;
  }

  /** Binds the builders to the game's types. Call once, re-export. */
  export function define<G extends GameTypes>(): Builders<G>;

  /** Vector math on plain [x, y] arrays: pure, returns new arrays. Angles in degrees (0 = right, 90 = down). */
  export const vec: {
    add(a: Vec2, b: Vec2): Vec2;
    sub(a: Vec2, b: Vec2): Vec2;
    scale(a: Vec2, s: number): Vec2;
    len(a: Vec2): number;
    /** Unit length, or [0, 0] for the zero vector. */
    norm(a: Vec2): Vec2;
    dot(a: Vec2, b: Vec2): number;
    lerp(a: Vec2, b: Vec2, t: number): Vec2;
    angle(a: Vec2): number;
    fromAngle(degrees: number, length?: number): Vec2;
    dist(a: Vec2, b: Vec2): number;
  };
}

declare module "alpha/test" {
  import type { AnimationOf, AxisOf, Game, GameTypes, LocaleOf, MusicOf, Os } from "alpha";

  /**
   * One player's simulated input: action -> frames (pressed for one step each),
   * and `axes`: axis -> [frame, value] entries (value held until the next entry).
   */
  export type PlayerInputs<G extends GameTypes> = Partial<Record<G["actions"], number[]>> & {
    axes?: Partial<Record<AxisOf<G>, Array<[frame: number, value: number]>>>;
  };

  export interface SimulateOptions<G extends GameTypes> {
    /** Scene to start in (default: the game's `start`). */
    scene?: keyof G["scenes"] & string;
    /** Simulated time; the run is round(seconds * 60) fixed steps of 1/60 s. */
    seconds: number;
    /**
     * Player 1's input, or an array with one entry per player (1..4); frames are 0-based.
     * e.g. `{ flap: [0, 30], axes: { moveX: [[0, 1], [60, 0]] } }`.
     */
    inputs?: PlayerInputs<G> | PlayerInputs<G>[];
    /**
     * "document#elementId" -> frame numbers on which that element is clicked
     * (before the step; its handler runs in that step). Clicking a hidden
     * document or an unknown id fails the test.
     */
    clicks?: { [target: string]: number[] };
    /** The player's settings at boot, as settings.json holds them, e.g. `{ locale: "pt", reducedMotion: true, volumes: { music: 0.5 } }`. */
    settings?: Partial<{
      volumes: Record<string, number>; locale: string; fullscreen: boolean; bindings: Record<string, string[]>;
      uiScale: number; highContrast: boolean; reducedMotion: boolean;
    }>;
    /** Save slots at boot: `{ main: data, slotName: data }` (main = ctx.save). */
    saves?: { [slot: string]: { [k: string]: unknown } };
    /** `[[from, to], ...]` frames during which the app is paused (focus lost, background): no steps, onPause at `from`, onResume at `to`. */
    appPauses?: [number, number][];
    /** What ctx.platform reports (default: the machine running the test). */
    platform?: Os;
  }

  export interface Expect<G extends GameTypes> {
    scene(name: keyof G["scenes"] & string): void;
    noErrors(): void;
    count(tag: G["tags"]): { equals(n: number): void; atLeast(n: number): void };
    /** Plain text of a UI element at the end of the run. */
    text<D extends keyof G["ui"] & string>(doc: D, id: G["ui"][D]): { equals(text: string): void };
    /** Animation of the oldest living entity with the tag at the end of the run ("" = none). */
    animation(tag: G["tags"]): { equals(name: AnimationOf<G> | ""): void };
    /** The UI document is shown at the end of the run. */
    shown(doc: keyof G["ui"] & string): void;
    /** Live particles of every emitter at the end of the run. */
    particles(): { equals(n: number): void; atLeast(n: number): void };
    /** The music playing at the end of the run (null = none). */
    music(name: MusicOf<G> | null): void;
  }

  export interface EntitySnapshot {
    id: number;
    pos: [number, number];
    rotation: number;
    scale: number;
    alpha: number;
    data: { [k: string]: unknown };
    /** null without a body. */
    velocity: [number, number] | null;
  }

  export interface Run<G extends GameTypes> {
    readonly frames: number;
    readonly expect: Expect<G>;
    /** Scene state at the end of the run (JSON copy; undefined when not JSON-safe). */
    readonly state: unknown;
    /** Living entities with the tag at the end of the run, oldest first (plain data). */
    entities(tag: G["tags"]): EntitySnapshot[];
    /** At the end of the run: the music playing, the locale, the settings, the save slots. */
    readonly music: MusicOf<G> | null;
    readonly locale: LocaleOf<G> | null;
    readonly settings: { volumes: Record<string, number>; locale: string | null; uiScale: number; highContrast: boolean; reducedMotion: boolean; bindings: Record<string, string[]> };
    readonly saves: { [slot: string]: { [k: string]: unknown } };
  }

  /**
   * Headless, deterministic: same game + options => same run. Every call is a
   * fresh game instance (new world and timers, random seeded from config.seed,
   * in-memory ctx.save and settings, empty unless opts.saves / opts.settings seed them),
   * with no window and no audio output (music and buses are still tracked).
   * A script error stops the run; `expect.noErrors()` reports it. Tests are the
   * exported functions of test/main.test.ts, run by `alpha test`.
   */
  export function simulate<G extends GameTypes>(game: Game<G>, opts: SimulateOptions<G>): Run<G>;
}
