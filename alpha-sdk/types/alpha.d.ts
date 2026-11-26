// Guide: https://luciancaetano.github.io/alpha-star/getting-started.html
// Showcase: https://luciancaetano.github.io/alpha-star/ (six real example captures).
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
    /**
     * Entity tags -> the data an entity with that tag carries in `entity.data` (`{}` = none),
     * e.g. `{ hero: {}; enemy: { kind: EnemyKind; hp: number } }`. A tag is a typed component:
     * spawning with "enemy" requires its data, and `findAll("enemy")`, queries filtered by it
     * and `onHit("shot", "enemy", ...)` give entities whose `data.hp` is a number. Data stays
     * JSON-safe (numbers, strings, booleans, null, arrays, plain objects).
     */
    tags: { [tag: string]: object };
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
    /** LDtk world names (assets.worlds), e.g. "main". Optional. */
    worlds?: string;
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
    /**
     * Custom shader names (assets.shaders) -> their uniform names,
     * e.g. `{ wave: "amount" | "time" }` (`never` for a shader without uniforms). Optional.
     */
    shaders?: { [shader: string]: string };
    /**
     * Game event name -> its payload (`undefined` = none), e.g.
     * `{ died: { enemy: Entity<MyGame, "enemy"> }; levelUp: { level: number }; dawn: undefined }`.
     * `ctx.emit` / `ctx.on` check names and payloads. Optional.
     */
    events?: { [event: string]: unknown };
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
  export type ShadersOf<G extends GameTypes> = G extends { shaders: infer S extends { [k: string]: string } } ? S : {};
  export type SheetsOf<G extends GameTypes> = G extends { sheets: infer S extends { [k: string]: string } } ? S : {};
  /** Every animation name of every sheet. */
  export type AnimationOf<G extends GameTypes> = SheetsOf<G>[keyof SheetsOf<G>] & string;
  /** The game's LDtk world names (`never` without `worlds`). */
  export type WorldOf<G extends GameTypes> = G extends { worlds: infer W extends string } ? W : never;

  /** The game's events (`{}` without `events`). */
  export type EventsOf<G extends GameTypes> = G extends { events: infer E extends { [k: string]: unknown } } ? E : {};
  /** The game's event names. */
  export type EventOf<G extends GameTypes> = keyof EventsOf<G> & string;
  /** emit's payload argument: none for `undefined` events. */
  export type PayloadArgs<P> = [P] extends [undefined] ? [] : [payload: P];

  /** The game's tag names. */
  export type TagOf<G extends GameTypes> = keyof G["tags"] & string;
  /**
   * The data of an entity having every tag in `T` (the intersection of their data shapes);
   * `{}` for `never` (an entity whose tags are not known).
   */
  export type DataOf<G extends GameTypes, T extends TagOf<G>> = [T] extends [never] ? {}
    : (T extends unknown ? (x: G["tags"][T]) => void : never) extends (x: infer I) => void ? I : never;
  /** An entity having any one of the tags in `T` (a union, one member per tag); `Entity<G>` for `never`. */
  export type EntityOf<G extends GameTypes, T extends TagOf<G>> = [T] extends [never] ? Entity<G>
    : T extends TagOf<G> ? Entity<G, T> : never;

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
   * anchor: `align` (default "center") picks which end of the lines sits on it; the block
   * of lines is vertically centered. Any Unicode text: TrueType/OpenType fonts are shaped
   * (ligatures, Arabic joining, Devanagari/Thai clusters), mixed right-to-left and
   * left-to-right runs are ordered by the Unicode bidi algorithm, characters the font lacks
   * come from its `fallbacks` (FontAsset), and what no font has draws as a box (with a
   * warning in the log). "\n" starts a new line. Invalid Unicode (an unpaired surrogate
   * from slicing an emoji) is an error naming where it is.
   */
  export interface TextShape<F extends string = never> {
    text: string;
    font?: F;
    /** Pixels (1..256), default 16. */
    size?: number;
    /** Default "#ffffff". */
    color?: Color;
    align?: "left" | "center" | "right";
    /**
     * Paragraph direction: "auto" (default) takes it from the first strong letter
     * (Arabic/Hebrew = right to left), "ltr" / "rtl" force it.
     */
    dir?: "auto" | "ltr" | "rtl";
    /**
     * Wrap lines at this many pixels (0 = default: only at "\n"). Lines break after
     * spaces and between CJK characters (not before closing punctuation such as 。、）).
     */
    width?: number;
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
    /**
     * Circle/rect particles: true = antialiased (1 px feathered) edges, false = crisp. Default:
     * `!config.pixelMode`, like filled shapes. Image/sheet particles follow their image's `filter`.
     */
    smooth?: boolean;
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
  /** A built-in effect or a custom shader (assets.shaders). */
  export type EffectOf<S> = PostFxName | (keyof S & string);
  /** A built-in's params, or a custom shader's uniforms by name (missing ones: their defaults). */
  export type EffectParams<S, N> = N extends PostFxName ? PostFxParams[N]
    : N extends keyof S ? { [U in S[N] & string]?: number } : never;
  /**
   * A custom full-screen shader: a Vulkan GLSL fragment shader (.frag; see the shaders
   * guide for its contract) and its float uniforms with defaults, in the order of its
   * std140 uniform block (`layout(set = 3, binding = 0)`, floats only), at most 16.
   * `alpha run` compiles it to SPIR-V (glslang) and MSL (shadercross) on load (and on
   * every save); `alpha export` puts the compiled .spv and .msl in the pack instead of
   * the .frag. Runs on Vulkan today (Windows, Linux, Android; the .msl is for Metal,
   * macOS/iOS not yet), which games with shaders ask for; elsewhere the effect is
   * skipped with a logged warning.
   */
  export interface ShaderAsset<U extends string> {
    file: string;
    uniforms?: Record<U, number>;
  }
  /**
   * The current scene's post effects, drawn over the (lit) world in the order first
   * added; UI and dev tools stay unaffected. Each scene starts with none. Render-only:
   * simulate() and game state never see them. Built-ins work on every renderer (software
   * too); custom shaders (assets.shaders, same calls) need the GPU renderer.
   */
  export interface PostFx<S = {}> {
    /** Adds the effect, or replaces its params when already added (keeping its place). */
    add<N extends EffectOf<S>>(name: N, params?: EffectParams<S, N>): void;
    /** Like add, for an effect already added (error otherwise). Missing params reset to defaults. */
    set<N extends EffectOf<S>>(name: N, params?: EffectParams<S, N>): void;
    /** False when it was not added. */
    remove(name: EffectOf<S>): boolean;
    clear(): void;
    /** Names in draw order. */
    list(): EffectOf<S>[];
  }

  /**
   * Plain data describing an entity with tags `T` (`tags` lists all of them or some).
   * Build it with functions, not classes: `const enemy = (...): EntitySpec<G, "enemy"> => ({...})`.
   * `data` must hold every field of every tag in `T`; it may be left out when they have none.
   */
  export type EntitySpec<G extends GameTypes, T extends TagOf<G>> = SpecBase<G, T> & DataField<DataOf<G, T>>;
  /** `data` is required when the tags' data has a required field. */
  export type DataField<D> = {} extends D ? { data?: D } : { data: D };
  /** A spec with any tags, what LDtk `entities` factories may return; `ctx.spawn` takes only EntitySpec. */
  export type AnySpec<G extends GameTypes> = SpecBase<G, TagOf<G>> & { data?: object };

  export interface SpecBase<G extends GameTypes, T extends TagOf<G>> {
    tags: readonly T[];
    pos: Vec2;
    shape?: Shape<G["images"], SheetsOf<G>, FontOf<G>>;
    body?: BodySpec<CategoryOf<G>>;
    emitter?: EmitterSpec<G["images"], keyof SheetsOf<G> & string>;
    /** A light riding on the entity; seen only with config.lighting. Render-only. */
    light?: LightSpec;
    rotation?: number;
    /** Order inside the entity's layer (default 0). */
    z?: number;
    /** A config.layers name (default: the unnamed layer, z 0, parallax 1). */
    layer?: LayerOf<G>;
    /**
     * Makes it a child: `pos` and `rotation` are then local to the parent, it follows the
     * parent every step and is destroyed with it.
     */
    parent?: Entity<G>;
  }

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

  export interface RayHit<G extends GameTypes, T extends TagOf<G> = never> {
    entity: EntityOf<G, T>;
    point: Vec2;
    /** Out of the hit surface. */
    normal: Vec2;
    /** 0..1 along from -> to. */
    fraction: number;
  }

  /** What a raycast or area query considers (default: every body). */
  export interface QueryFilter<G extends GameTypes, T extends TagOf<G> = never> {
    /** Only entities having any of these tags (the results are typed by them). */
    tags?: readonly T[];
    /** Only bodies in these config.collision categories. */
    categories?: readonly CategoryOf<G>[];
  }

  /**
   * ctx.joint options. Anchors are world points at creation: `anchor` for revolute, weld and
   * prismatic (default: b's position); `anchors` [onA, onB] for distance and rope (default:
   * the bodies' positions). limits: degrees (revolute), px along the axis (prismatic),
   * [min, max] length (distance). motor: speed in degrees/s or px/s, max torque/force in
   * kg-pixel units (keep it near what the job needs: a huge max overpowers the limits).
   */
  export interface JointSpec<G extends GameTypes> {
    type: "revolute" | "distance" | "rope" | "weld" | "prismatic";
    a: Entity<G>;
    b: Entity<G>;
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

  /** A ctx.every / ctx.after timer. */
  export interface Timer {
    /** False once it fired (after), was cancelled, its owner died or the scene changed. */
    readonly active: boolean;
    /** Stops it; cancelling twice is a no-op. */
    cancel(): void;
  }

  /** ctx.every / ctx.after options. */
  export interface TimerOptions<G extends GameTypes> {
    /** The timer goes away with this entity (never fires after it is destroyed). */
    owner?: Entity<G>;
  }

  export interface Tween {
    readonly done: boolean;
    readonly active: boolean;
    cancel(): void;
  }

  /**
   * A live entity known to have the tags in `T` (`Entity<G>`: tags not known, data `{}`).
   * Every `Entity<G, T>` is an `Entity<G>`; `has` narrows to more tags.
   */
  export interface Entity<G extends GameTypes, in T extends TagOf<G> = never> {
    readonly id: number;
    readonly tags: readonly TagOf<G>[];
    /** True when it has `tag`; narrows the entity (and its data) to that tag too. */
    has<K extends TagOf<G>>(tag: K): this is Entity<G, K>;
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
    animation: AnimationOf<G> | "";
    /** Plays `name` from its first frame, even if it is already playing. */
    play(name: AnimationOf<G>): void;
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
    readonly parent: Entity<G> | null;
    /** Living children, oldest first. */
    readonly children: Entity<G>[];
    /**
     * Game data typed by the tags (GameTypes.tags): spawn's `data`, or {}, changed in place
     * (`e.data.hp -= 1`) or replaced; JSON-safe. Tools (MCP state) see it after each step.
     */
    data: DataOf<G, T>;
  }

  /** ctx.window: the game window. Ignored by headless runs except for reading back. */
  export interface GameWindow {
    /** Switches at the end of the frame. Alt+Enter toggles it too (config.window.altEnter). */
    fullscreen: boolean;
    /** Window size in real pixels (the game draws in config.size game pixels). */
    readonly size: Vec2;
  }

  /**
   * config.window. The game is designed at config.size and scaled to the screen keeping its
   * aspect (like Godot's stretch aspect). `aspect`: "expand" (default; fills the screen by
   * showing more world on the longer axis, never bars), "keep_width" (width fixed: taller
   * screens show more height, wider ones get side bars; portrait games), "keep_height"
   * (height fixed: wider screens show more width, taller ones get bars; landscape games),
   * "keep" (exactly config.size, letterbox bars). `scale`: "fractional" (default) or
   * "integer" (whole multiples only: crisp pixel art; the revealed axes still fill the
   * screen). The revealed world is ctx.camera.view; screen-space HUD uses ctx.safeArea, and
   * UI elements with class="safe" are padded by it. (`scaling` was replaced: letterbox ->
   * aspect "keep", integer -> scale "integer".)
   */
  export interface WindowConfig {
    fullscreen?: boolean;
    aspect?: "expand" | "keep_width" | "keep_height" | "keep";
    scale?: "fractional" | "integer";
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
    follow(entity: Entity<any> | null, opts?: { lerp?: number; deadzone?: Vec2 }): void;
    /** Shakes by up to `intensity` pixels, fading out over `seconds` (deterministic). */
    shake(intensity: number, seconds: number): void;
    /** Keeps the visible area inside [x, y, width, height] (e.g. the map's size); null removes them. */
    bounds(rect: [x: number, y: number, width: number, height: number] | null): void;
    /** Screen point (game pixels, e.g. ctx.input.mouse()) -> world point. */
    toWorld(screen: Vec2): Vec2;
    /**
     * [x, y, width, height]: the world rect the screen shows now, read-only. config.window.aspect
     * reveals more than config.size on phones and resized windows (split evenly around the
     * design center), so size backgrounds/ground to it and spawn at its edges; read it each
     * time (it follows window resizes, zoom and camera moves).
     */
    readonly view: readonly [x: number, y: number, width: number, height: number];
  }

  /**
   * An LDtk field value as plain data: numbers, strings (also colors "#rrggbb" and
   * enum values), booleans, null, Point as a cell [cx, cy], EntityRef as iids
   * (`level.entity(ref.entity)` gives the spawned entity), Tile as a tileset rect, arrays.
   */
  export type LevelField =
    | null
    | number
    | string
    | boolean
    | [cx: number, cy: number]
    | { entity: string; layer: string; level: string }
    | { tileset: string | null; rect: [x: number, y: number, w: number, h: number] }
    | LevelField[];

  /** One LDtk entity instance, as plain data (a copy). */
  export interface LevelEntity {
    /** The LDtk entity identifier, e.g. "Player". */
    id: string;
    iid: string;
    /** World pixels of its pivot point (level position + layer offset + `local`). */
    pos: Vec2;
    /** Pixels of its pivot inside the layer (LDtk `px`). */
    local: Vec2;
    /** Cell [cx, cy] in its layer. */
    grid: Vec2;
    size: Vec2;
    /** 0..1 each: [0, 0] = top-left, [0.5, 1] = bottom middle. */
    pivot: Vec2;
    /** Its Entities layer. */
    layer: string;
    tags: string[];
    fields: { [name: string]: LevelField };
    /** Editor tile of the entity, if it has one. */
    tile: { tileset: string | null; rect: [x: number, y: number, w: number, h: number] } | null;
  }

  /** The top tile of a cell (level.tile). */
  export interface LevelTile {
    /** Tile id in the layer's tileset (row-major). */
    id: number;
    flipX: boolean;
    flipY: boolean;
    /** Enum values tagged on this tile in the tileset (LDtk "Enum tags"). */
    tags: string[];
    /** The tile's custom data string, or null. */
    data: string | null;
  }

  /** A level as data: ctx.world(w).level(id), and the base of a spawned Level. */
  export interface LevelData {
    /** LDtk identifier, e.g. "Level_0". */
    readonly id: string;
    readonly iid: string;
    /** World pixels of its top-left (its LDtk world position, or spawnLevel's `pos`). */
    readonly pos: Vec2;
    readonly size: Vec2;
    readonly depth: number;
    /** Background color "#rrggbb", or null. */
    readonly bg: string | null;
    readonly fields: { [name: string]: LevelField };
    /** Layer identifiers in draw order (bottom first). */
    readonly layers: string[];
    /** Adjacent levels; `dir` is LDtk's: "n" | "s" | "e" | "w" | "ne" | "nw" | "se" | "sw" | "<" | ">" | "o". */
    readonly neighbours: { level: string | null; dir: string }[];
    /** Entity instances (copies, world pixels), all or one identifier; an identifier not in the project throws. */
    entities(id?: string): LevelEntity[];
    /** IntGrid value at a cell (0 = empty or outside the layer). */
    int(layer: string, cx: number, cy: number): number;
    /** The top tile at a cell of a tile / auto / IntGrid layer, or null. */
    tile(layer: string, cx: number, cy: number): LevelTile | null;
    /** The cell under a world point (grid of `layer`, default the first layer). */
    toCell(pos: Vec2, layer?: string): Vec2;
    /** World pixels of a cell's center (grid of `layer`, default the first layer). */
    toWorld(cell: Vec2, layer?: string): Vec2;
  }

  /** A spawned level (ctx.spawnLevel): data plus live edits. */
  export interface Level<G extends GameTypes> extends LevelData {
    /**
     * Sets an IntGrid cell to a value (number or its LDtk identifier; 0 clears).
     * Collision rules of that layer rebuild their merged colliders. Auto-layer
     * tiles are not re-run: change visuals with setTile.
     */
    setInt(layer: string, cx: number, cy: number, value: number | string): void;
    /** Replaces the tiles of a cell with tile id `tile` (null clears). The layer redraws in place. */
    setTile(layer: string, cx: number, cy: number, tile: number | null, opts?: { flipX?: boolean; flipY?: boolean; alpha?: number }): void;
    /** The entity a factory spawned for LDtk entity `iid` (resolves EntityRef fields), or null. */
    entity(iid: string): Entity<G> | null;
    /** Removes everything spawnLevel made: tile layers, colliders, background and factory entities still alive. */
    destroy(): void;
  }

  /** A loaded LDtk project (ctx.world): data only. */
  export interface World {
    readonly name: string;
    readonly layout: "Free" | "GridVania" | "LinearHorizontal" | "LinearVertical";
    /** Level identifiers in project order. */
    readonly levels: string[];
    /** A level's data; throws listing the levels. */
    level(id: string): LevelData;
    /** The level whose world rect holds a point, or null. */
    levelAt(pos: Vec2): LevelData | null;
    /** An entity instance anywhere in the world by iid (with its level id), or null. */
    entity(iid: string): (LevelEntity & { level: string }) | null;
  }

  /** A collision rule for one IntGrid value. */
  export type IntGridCollider<T extends string> =
    | T
    | { tag: T; oneWay?: boolean; sensor?: boolean; friction?: number; restitution?: number };

  export interface SpawnLevelOptions<G extends GameTypes> {
    /**
     * IntGrid layers that become merged static colliders: a tag for every
     * non-zero value (`{ Collisions: "wall" }`), or a rule per value / identifier
     * (`{ Collisions: { wall: "wall", platform: { tag: "ledge", oneWay: true } } }`).
     */
    collide?: { [intGridLayer: string]: TagOf<G> | { [value: string]: IntGridCollider<TagOf<G>> } };
    /** LDtk entity identifier -> factory; it returns spawn spec(s) or null. */
    entities?: { [entity: string]: (e: LevelEntity) => AnySpec<G> | AnySpec<G>[] | null | undefined };
    /** z of every tile layer and the background (default 0; later layers draw over earlier ones). */
    z?: number;
    /** World pixels of the level's top-left (default its LDtk world position). */
    pos?: Vec2;
    /** config.layers layer of every tile layer (default: the unnamed layer). */
    layer?: string;
    /** Per LDtk layer identifier: its config.layers layer (overrides `layer`), e.g. parallax. */
    layers?: { [ldtkLayer: string]: string };
    /** Fill the level's LDtk background color under it (default false: config.background shows). */
    background?: boolean;
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

  /** One finger on the screen (ctx.input.touches()). */
  export interface Touch {
    id: number;
    pos: Vec2;
    pressed: boolean;
    released: boolean;
  }

  /**
   * A virtual stick on touch screens (config.touchSticks), read like a gamepad stick.
   * Mobile only (ctx.platform.mobile; desktop `--platform android` previews it): elsewhere
   * it neither draws nor captures fingers and its sources read 0. A stick "move" makes the
   * sources TouchMoveX / TouchMoveY (axes, -1..1, +y down), TouchMoveX- / X+ / Y- / Y+
   * (past 0.5) and TouchMoveActive (a finger on it) for config.axes / config.actions.
   */
  export interface TouchStick {
    /** Where a new finger grabs it: a half of ctx.safeArea, or [x, y, w, h] fractions of it (0..1). */
    zone: "left" | "right" | [number, number, number, number];
    /** floating (default): the base appears where the finger lands; fixed: it stays at `anchor`. */
    mode?: "floating" | "fixed";
    /** fixed: center as fractions of ctx.safeArea; default the zone's center. */
    anchor?: Vec2;
    /** Knob reach in game pixels (default 64). */
    radius?: number;
    /** 0..0.9 (default 0.15): below it reads 0, above it is remapped to 0..1. */
    deadzone?: number;
    /** "both" (default), "x" or "y" only. */
    axes?: "both" | "x" | "y";
    /** Quantize the direction to 4 or 8 ways; 0 = analog (default). */
    snap?: 0 | 4 | 8;
    /** Default "touching" for floating, "always" for fixed. */
    visible?: "always" | "touching";
    /** 0..1 at rest (default 0.6); 1 while touched. */
    opacity?: number;
    /** Images instead of the default grey circles. */
    skin?: StickSkin;
    /** Whose input it is (default 1). */
    player?: Player;
    /** Scenes where it exists (draws and captures), e.g. ["play"]; default every scene. */
    scenes?: string[];
  }

  /** Image names: assets.images names or "sheet:frame" (frame index of an assets.sheets sheet). */
  export interface StickSkin {
    base: string;
    knob: string;
    /** Diameter in game pixels (default radius * 2). */
    baseSize?: number;
    /** Default radius. */
    knobSize?: number;
    /** Images shown while touched. */
    pressed?: { knob?: string; base?: string };
    /** Multiplies the images, e.g. "#ffd27fcc". */
    tint?: Color;
  }

  /**
   * A virtual button on touch screens (config.touchButtons): button "jump" makes the input
   * source TouchJump, held while its finger is down (dragging off does not release it).
   * Mobile only, like TouchStick; a button wins over a stick zone it sits in. Buttons whose
   * touch areas (radius + hitSlop) overlap are a config error.
   */
  export interface TouchButton {
    /** Center as fractions of ctx.safeArea (0..1). */
    pos: Vec2;
    /** Game pixels (default 40). */
    radius?: number;
    /** Extra touch margin in game pixels (default 8). */
    hitSlop?: number;
    /** Text on the default look, e.g. "A". */
    label?: string;
    /** "never" = an invisible touch area (default "always"). */
    visible?: "always" | "never";
    /** 0..1 at rest (default 0.6); 1 while pressed. */
    opacity?: number;
    skin?: ButtonSkin;
    player?: Player;
    /** Scenes where it exists, e.g. ["play"]; default every scene. */
    scenes?: string[];
  }

  export interface ButtonSkin {
    /** Image at rest (assets.images name or "sheet:frame"). */
    up: string;
    /** Image while pressed; default `up` darkened. */
    down?: string;
    /** Diameter in game pixels (default radius * 2). */
    size?: number;
    tint?: Color;
  }

  /** ctx.input: actions and axes of any player; `player(n)` for one. */
  export interface Input<A extends string, X extends string = never> extends PlayerInput<A, X> {
    /** Gamepads get players in connection order; a reconnected pad gets its slot back. */
    player(n: Player): PlayerInput<A, X>;
    /** Mouse pointer in game pixels (screen space; ctx.camera.toWorld for the world). While a finger no touch control took is down, that finger. */
    mouse(): Vec2;
    /**
     * This step's fingers that no game UI element and no touch stick/button captured, in
     * game pixels (screen space). `pressed`: touched down this step; `released`: lifted
     * this step (pos = where). At most 10 fingers.
     */
    touches(): Touch[];
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

  export type HitHandler<G extends GameTypes, A extends TagOf<G>, B extends TagOf<G>> = (a: Entity<G, A>, b: Entity<G, B>, contact: Contact) => void;
  export type PairHandler<G extends GameTypes, A extends TagOf<G>, B extends TagOf<G>> = (a: Entity<G, A>, b: Entity<G, B>) => void;
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
    /** Full-screen effects of the current scene (bloom, crt, pixelate, tint, vignette, assets.shaders). */
    readonly postfx: PostFx<ShadersOf<G>>;
    readonly window: GameWindow;
    /**
     * [x, y, width, height] in screen (game) pixels: the part of the screen no display cutout,
     * rounded corner or system bar hides. Screen pixels span the whole revealed screen
     * (config.size grown by config.window.aspect), so [0, 0] is the real top-left corner. The
     * whole screen on desktop and in headless runs; on phones (always fullscreen, drawing into
     * the cutout) keep HUD and buttons inside it.
     */
    readonly safeArea: readonly [x: number, y: number, width: number, height: number];
    /**
     * Freezes the scene's world while true: physics (bodies keep their velocity), contact
     * events, ctx.every/after timers, tweens, animations, particles, animated tiles and
     * ctx.time stop; `update`, UI handlers, input, the camera and audio keep running (level-up
     * and pause menus). False on every scene enter. Not ctx.app.paused (the OS paused the app).
     */
    frozen: boolean;

    /** `data` is checked against every tag's data (GameTypes.tags). */
    spawn<T extends TagOf<G>>(spec: EntitySpec<G, T>): Entity<G, T>;
    /** Gone from queries at once (alive = false), freed at the end of the step. Destroying twice is a no-op. */
    destroy(entity: Entity<G>): void;
    /** The oldest living entity with the tag; throws if none has it. */
    find<T extends TagOf<G>>(tag: T): Entity<G, T>;
    /** Living entities with the tag, oldest first. */
    findAll<T extends TagOf<G>>(tag: T): Entity<G, T>[];
    /**
     * Every living entity with the tag, positions in bulk (oldest first). For many
     * entities per frame this is several times cheaper than `entity.pos` per entity:
     * edit `x[i]` / `y[i]`, then `write()` once. Entities destroyed since are skipped.
     */
    positions(tag: TagOf<G>): Positions;

    /**
     * Spawns an LDtk level: draws its visible tile layers (one batched, culled entity
     * per layer, animated tiles in a second one), makes `collide` IntGrid values solid,
     * runs `entities` factories. Scene-scoped like spawn: call it in enter (or later for
     * room transitions; `level.destroy()` removes it).
     */
    spawnLevel(
      world: WorldOf<G>,
      level: string,
      opts?: SpawnLevelOptions<G>,
    ): Level<G>;
    /** An LDtk world's data (levels, entities, cells) without spawning anything. */
    world(name: WorldOf<G>): World;

    /** Solid contact between tags starts. `a` has tagA, `b` has tagB; `contact` normal points a -> b. */
    onHit<A extends TagOf<G>, B extends TagOf<G>>(tagA: A, tagB: B, fn: HitHandler<G, A, B>): void;
    /** Solid contact between tags ends (both still alive). */
    onHitEnd<A extends TagOf<G>, B extends TagOf<G>>(tagA: A, tagB: B, fn: PairHandler<G, A, B>): void;
    /** Overlap with a sensor starts. */
    onEnter<A extends TagOf<G>, B extends TagOf<G>>(tagA: A, tagB: B, fn: PairHandler<G, A, B>): void;
    /** Overlap with a sensor ends (both still alive). */
    onExit<A extends TagOf<G>, B extends TagOf<G>>(tagA: A, tagB: B, fn: PairHandler<G, A, B>): void;

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

    /**
     * First solid collider on the segment (sensors ignored), or null. Queries never return
     * entities destroyed earlier in the step; `opts.tags` types the results.
     */
    raycast<T extends TagOf<G> = never>(from: Vec2, to: Vec2, opts?: QueryFilter<G, T>): RayHit<G, T> | null;
    /** Every solid collider on the segment, nearest first. */
    raycastAll<T extends TagOf<G> = never>(from: Vec2, to: Vec2, opts?: QueryFilter<G, T>): RayHit<G, T>[];
    /** Living entities whose colliders (sensors too) overlap the circle, oldest first. */
    queryCircle<T extends TagOf<G> = never>(center: Vec2, radius: number, opts?: QueryFilter<G, T>): EntityOf<G, T>[];
    /** Living entities whose colliders overlap [x, y, width, height] (top-left), oldest first. */
    queryRect<T extends TagOf<G> = never>(rect: [x: number, y: number, width: number, height: number], opts?: QueryFilter<G, T>): EntityOf<G, T>[];
    /** Connects two bodies. Scene-scoped; goes away with either body. */
    joint(spec: JointSpec<G>): Joint;
    /**
     * Eases numbers on the fixed step (deterministic, scene-scoped). An entity tweens
     * x, y, rotation, alpha, scale (a kinematic body's x/y moves by velocity, so riders are
     * carried); a plain object tweens its number keys.
     */
    tween(target: Entity<G>, opts: TweenOptions<EntityTweenKeys>): Tween;
    tween<O extends object>(target: O, opts: TweenOptions<{ [K in keyof O]: O[K] extends number ? K : never }[keyof O] & string>): Tween;

    /** Repeats every `seconds` (>= 0.001) of game time, first call after `seconds`. Stops while frozen. */
    every(seconds: number, fn: () => void, opts?: TimerOptions<G>): Timer;
    /** Runs once after `seconds` (>= 0.001) of game time. Stops while frozen. */
    after(seconds: number, fn: () => void, opts?: TimerOptions<G>): Timer;

    /**
     * Sends a game event (GameTypes.events). Delivered to every `ctx.on` handler at the end of
     * the step, after contact handlers, in the order sent (events sent by handlers too, at
     * most 1000 per step); also while frozen. Scene-scoped like every handler.
     */
    emit<K extends EventOf<G>>(name: K, ...payload: PayloadArgs<EventsOf<G>[K]>): void;
    /** Handles a game event; handlers run in the order added. */
    on<K extends EventOf<G>>(name: K, fn: (payload: EventsOf<G>[K]) => void): void;

    /** Switches scene at the end of this step. */
    go<K extends keyof G["scenes"] & string>(scene: K, ...params: ParamArgs<G["scenes"][K]>): void;
  }

  // ---- scenes and game --------------------------------------------------------

  /**
   * When a system runs: "running" (default: not while ctx.frozen), "frozen" (only while
   * frozen: menus) or "always".
   */
  export type When = "running" | "frozen" | "always";

  /**
   * One piece of a scene's logic, run every step in the order of `systems`, after `update`:
   * - `(ctx) => void`, or `{ name?, when?, run(ctx) }`;
   * - `{ name?, when?, tag: "enemy", each(ctx, e) }`: once per living entity with the tag
   *   (oldest first; one destroyed earlier in the same pass is skipped), `e` typed by it.
   * A script error inside names the system (`name`, else its tag or position).
   */
  export type System<G extends GameTypes, S> =
    | ((ctx: Context<G, S>) => void)
    | { name?: string; when?: When; run(ctx: Context<G, S>): void }
    | { [T in TagOf<G>]: { name?: string; when?: When; tag: T; each(ctx: Context<G, S>, e: Entity<G, T>): void } }[TagOf<G>];

  export interface SceneDef<G extends GameTypes, P, S> {
    /** Builds fresh scene state on every enter. Omit for stateless scenes. */
    state?: (params: P) => S;
    enter?(ctx: Context<G, S>, params: P): void;
    /** Runs every step, also while frozen; then `systems`. */
    update?(ctx: Context<G, S>): void;
    /** The scene's systems, run in this order after `update` (see System). */
    systems?: System<G, S>[];
    exit?(ctx: Context<G, S>): void;
  }

  /** Opaque scene value returned by `scene()`. */
  export interface Scene<G extends GameTypes, K extends keyof G["scenes"]> {
    readonly name: K;
    readonly __game?: G;
  }

  export interface GameConfig<G extends GameTypes> {
    title: string;
    /**
     * The design canvas in game pixels (world and UI units), not the window: a bigger size makes
     * everything smaller on screen. The first window is 2x it (integer scale) or 1.5x, fitted to the screen.
     */
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
     * Fingers a touch stick or button captured are none of these. Touch controls add
     * their own inputs: "TouchJump" (config.touchButtons.jump), "TouchMoveX+" /
     * "TouchMoveActive" (config.touchSticks.move). The name "axes" is reserved.
     */
    actions: Record<G["actions"], string[]>;
    /**
     * Axis -> sources; the axis reads the strongest one. A source is an analog axis
     * ("GamepadLeftX", "GamepadLeftY", "GamepadRightX", "GamepadRightY", "GamepadLT",
     * "GamepadRT"), a "Negative/Positive" input pair: "A/D", "Left/Right", or a touch
     * stick axis: "TouchMoveX" (config.touchSticks.move).
     * e.g. `{ moveX: ["GamepadLeftX", "A/D", "Left/Right", "TouchMoveX"] }`.
     */
    axes?: Record<AxisOf<G>, string[]>;
    /**
     * On-screen sticks for touch screens, name -> stick (at most 4), e.g.
     * `{ move: { zone: "left" } }` -> axes sources "TouchMoveX" / "TouchMoveY". Drawn over
     * the world and its effects, under the game UI; a finger on a UI element never reaches
     * them. Zones may overlap: the first stick in the object wins the finger.
     */
    touchSticks?: Record<string, TouchStick>;
    /** On-screen buttons for touch screens, name -> button (at most 12), e.g. `{ jump: { pos: [0.88, 0.8], label: "A" } }` -> input "TouchJump". */
    touchButtons?: Record<string, TouchButton>;
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
     * and maps override it with `filter`; fonts with `{ file, pixelMode }`; filled shapes and emitters with `smooth`.
     */
    pixelMode?: boolean;
    /**
     * How game UI images (`<img>`, `image(...)` decorators) are sampled when the UI is scaled:
     * "nearest" for pixel-art UI, "sharp" for crisp pixels with soft edges at any scale.
     * Default: follows `pixelMode` ("nearest" when true, "linear" when false). UI text is always made at the output resolution.
     */
    uiFilter?: ImageFilter;
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
       * Texts are UTF-8; world text and RmlUi draw what their fonts (and `fallbacks`) have.
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
      fonts?: Record<FontOf<G>, FontAsset<FontOf<G>>>;
      /** Document name -> .rml file. Files it links (`<link href>`, `src=`, rcss `url()`) are packed too. */
      ui?: Record<keyof G["ui"] & string, string>;
      /**
       * Sprite sheets: a grid of `frame`-sized cells in `image` (row-major, frame 0 top-left)
       * and named animations over frame indices, e.g.
       * `{ hero: { image: "img/hero.png", frame: [16, 16], animations: { run: { frames: [0, 1, 2, 3], fps: 10 } } } }`.
       */
      sheets?: { [K in keyof SheetsOf<G>]: SheetAsset<SheetsOf<G>[K] & string> };
      /**
       * World name -> LDtk project (.ldtk; levels embedded or saved as .ldtkl). `filter` applies to
       * its tilesets. Animate a tile with tileset custom data `{"frames": [ids], "fps": n}`.
       */
      worlds?: Record<WorldOf<G>, ImageAsset>;
      /**
       * Custom full-screen shader name -> `{ file: "shaders/wave.frag", uniforms: { amount: 0.5 } }`,
       * then `ctx.postfx.add("wave", { amount: 1 })`. See ShaderAsset.
       */
      shaders?: { [K in keyof ShadersOf<G>]: ShaderAsset<ShadersOf<G>[K] & string> };
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
   * A font: "path" or `{ file, pixelMode, license, fallbacks }`; `license` is its license
   * file, credited in the pack. Text is rasterized at the screen's real resolution, so it
   * stays sharp at any window scale; bitmap fonts draw at whole multiples of their size.
   * pixelMode true: TrueType glyphs are rendered monochrome (pixel fonts stay crisp).
   * `fallbacks` (TrueType/OpenType only): other assets.fonts names, tried in order for
   * characters this font lacks, e.g. `main: { file: "NotoSans.ttf", fallbacks: ["arabic", "cjk"] }`.
   */
  export type FontAsset<F extends string = string> =
    | string
    | { file: string; pixelMode?: boolean; license?: string; fallbacks?: F[] };

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
  import type { AnimationOf, AxisOf, DataOf, Game, GameTypes, LocaleOf, MusicOf, Os, TagOf, Vec2 } from "alpha";

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
    /**
     * Fingers on the screen, in game pixels: `path` entries `[frame, [x, y]]` move (or put
     * down) the finger, `[frame, null]` lifts it. With `platform: "android"` they drive
     * config.touchSticks / touchButtons end to end; otherwise they are plain touches
     * (ctx.input.touches(), "Touch", "MouseLeft").
     * e.g. `[{ id: 1, path: [[0, [80, 600]], [10, [140, 600]], [40, null]] }]`.
     */
    touches?: { id: number; path: Array<[frame: number, pos: Vec2 | null]> }[];
    /** Screen size in pixels to run on, e.g. a tall phone `[1080, 2340]`: config.window.aspect grows ctx.camera.view and ctx.safeArea as the engine would (default: config.size). */
    size?: [number, number];
  }

  export interface Expect<G extends GameTypes> {
    scene(name: keyof G["scenes"] & string): void;
    noErrors(): void;
    count(tag: TagOf<G>): { equals(n: number): void; atLeast(n: number): void };
    /** Plain text of a UI element at the end of the run. */
    text<D extends keyof G["ui"] & string>(doc: D, id: G["ui"][D]): { equals(text: string): void };
    /** Animation of the oldest living entity with the tag at the end of the run ("" = none). */
    animation(tag: TagOf<G>): { equals(name: AnimationOf<G> | ""): void };
    /** The UI document is shown at the end of the run. */
    shown(doc: keyof G["ui"] & string): void;
    /** Live particles of every emitter at the end of the run. */
    particles(): { equals(n: number): void; atLeast(n: number): void };
    /** The music playing at the end of the run (null = none). */
    music(name: MusicOf<G> | null): void;
  }

  export interface EntitySnapshot<D = { [k: string]: unknown }> {
    id: number;
    pos: [number, number];
    rotation: number;
    scale: number;
    alpha: number;
    data: D;
    /** null without a body. */
    velocity: [number, number] | null;
  }

  export interface Run<G extends GameTypes> {
    readonly frames: number;
    readonly expect: Expect<G>;
    /** Scene state at the end of the run (JSON copy; undefined when not JSON-safe). */
    readonly state: unknown;
    /** Living entities with the tag at the end of the run, oldest first (plain data). */
    entities<T extends TagOf<G>>(tag: T): EntitySnapshot<DataOf<G, T>>[];
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
