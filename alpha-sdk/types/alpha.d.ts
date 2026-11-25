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
 * (F1 in dev builds). Arguments are joined with spaces; non-strings as JSON.
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
  }

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

  export type Collider =
    | { circle: number; sensor?: boolean }
    | { rect: Vec2; sensor?: boolean };

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
   * With `font` (an assets.fonts name) it is rasterized at `size` pixels; without, the
   * engine's 8x8 pixel font scaled to `size`. `pos` is the anchor: `align` (default
   * "center") picks which end sits on it; vertically centered. Printable ASCII only.
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
    | { circle: number; color: Color; outline?: Color }
    | { rect: Vec2; color: Color; outline?: Color }
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

  export interface BodySpec {
    type: "static" | "kinematic" | "dynamic";
    collider: Collider;
    velocity?: Vec2;
  }

  /** Plain data describing an entity. Build it with functions, not classes. */
  export interface EntitySpec<T extends string = string, I extends string = never, S extends { [k: string]: string } = {},
    L extends string = never, F extends string = never> {
    tags: T[];
    pos: Vec2;
    shape?: Shape<I, S, F>;
    body?: BodySpec;
    emitter?: EmitterSpec<I, keyof S & string>;
    rotation?: number;
    /** Order inside the entity's layer (default 0). */
    z?: number;
    /** A config.layers name (default: the unnamed layer, z 0, parallax 1). */
    layer?: L;
  }

  export interface Body {
    get velocity(): Vec2Ref;
    set velocity(v: Vec2);
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
   * (fills keeping pixels square by showing more world; ctx.window.size grows the view).
   */
  export interface WindowConfig {
    fullscreen?: boolean;
    scaling?: "letterbox" | "integer" | "stretch" | "expand";
    /** Default true. */
    resizable?: boolean;
    /** Alt+Enter toggles fullscreen (default true). */
    altEnter?: boolean;
  }

  /** config.layers entry: drawn in `z` order (then entity z); parallax 0 = fixed to the screen, 1 = world (default). */
  export interface LayerDef {
    z: number;
    parallax?: number;
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
  }

  /** Seeded by config.seed. Math.random is disabled. */
  export interface Random {
    /** Float in [min, max). */
    range(min: number, max: number): number;
    /** Integer in [min, max]. */
    int(min: number, max: number): number;
    pick<T>(items: readonly T[]): T;
  }

  /** Persistent key/value storage; survives restarts. */
  export interface Save {
    get<T extends SaveValue>(key: string, fallback: T): T;
    set(key: string, value: SaveValue): void;
  }

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

  /** A WAV file path (relative to the game .js), or a synthesized sound. */
  export type SoundAsset = string | Synth;

  export interface PlayOptions {
    /** 0..1, default 1 */
    volume?: number;
    /** playback speed, default 1 (2 = octave up) */
    pitch?: number;
    loop?: boolean;
  }

  /** A playing sound. Stopping twice is a no-op. */
  export interface Voice {
    readonly playing: boolean;
    stop(): void;
  }

  export interface Audio<S extends string> {
    /** Plays a preloaded sound from assets.sounds. */
    play(sound: S, opts?: PlayOptions): Voice;
    /** Synthesizes and plays a sound on the fly. */
    tone(synth: Synth, opts?: PlayOptions): Voice;
    stopAll(): void;
    /** Master volume 0..1. */
    volume: number;
  }

  // ---- scene context ----------------------------------------------------------

  type ParamArgs<P> = [P] extends [undefined] ? [] : [params: P];

  export type HitHandler<T extends string> = (a: Entity<T, any>, b: Entity<T, any>) => void;

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
    readonly random: Random;
    readonly save: Save;
    readonly ui: UI<G["ui"]>;
    readonly audio: Audio<G["sounds"]>;
    readonly camera: Camera;
    readonly window: GameWindow;

    spawn(spec: EntitySpec<G["tags"], G["images"], SheetsOf<G>, LayerOf<G>, FontOf<G>>): Entity<G["tags"], AnimationOf<G>>;
    /** Removed at the end of the step. Destroying twice is a no-op. */
    destroy(entity: Entity<G["tags"], any>): void;
    /** Throws if no entity has the tag. */
    find(tag: G["tags"]): Entity<G["tags"], AnimationOf<G>>;
    findAll(tag: G["tags"]): Entity<G["tags"], AnimationOf<G>>[];

    /**
     * Draws the map's visible tile layers (one batched, culled entity per layer and
     * tileset) and makes `collide` layers solid. Scene-scoped like spawn: call it in enter.
     */
    spawnMap(name: MapOf<G>, opts?: SpawnMapOptions<G["tags"]>): TileMap;
    /** A map's data (sizes, object layers) without spawning anything. */
    map(name: MapOf<G>): TileMap;

    /** Solid contact between tags starts. `a` has tagA, `b` has tagB. */
    onHit(tagA: G["tags"], tagB: G["tags"], fn: HitHandler<G["tags"]>): void;
    /** Overlap with a sensor starts. */
    onEnter(tagA: G["tags"], tagB: G["tags"], fn: HitHandler<G["tags"]>): void;

    /** Repeats every `seconds` of game time, first call after `seconds`. */
    every(seconds: number, fn: () => void): void;
    /** Runs once after `seconds` of game time. */
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
     * Action -> device inputs, e.g. ["Space", "MouseLeft", "GamepadA"]. Sticks and
     * triggers press too: "GamepadLeftY-" (left stick up), "GamepadLeftX+", "GamepadRT".
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
    /** Named draw layers, e.g. `{ bg: { z: -100, parallax: 0.2 }, fx: { z: 100 } }`. */
    layers?: Record<LayerOf<G>, LayerDef>;
    window?: WindowConfig;
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
      sounds: Record<G["sounds"], SoundAsset>;
      images?: Record<G["images"], string>;
      /** Font name -> .ttf/.otf file; the name is the `font-family` used in .rcss. */
      fonts?: Record<FontOf<G>, string>;
      /** Document name -> .rml file. Files it links (`<link href>`, `src=`, rcss `url()`) are packed too. */
      ui?: Record<keyof G["ui"] & string, string>;
      /**
       * Sprite sheets: a grid of `frame`-sized cells in `image` (row-major, frame 0 top-left)
       * and named animations over frame indices, e.g.
       * `{ hero: { image: "img/hero.png", frame: [16, 16], animations: { run: { frames: [0, 1, 2, 3], fps: 10 } } } }`.
       */
      sheets?: { [K in keyof SheetsOf<G>]: SheetAsset<SheetsOf<G>[K] & string> };
      /** Map name -> Tiled map (.tmj JSON; tilesets embedded or .tsj; CSV tile layers). */
      maps?: Record<MapOf<G>, string>;
    };
  }

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
}

declare module "alpha/test" {
  import type { AnimationOf, AxisOf, Game, GameTypes } from "alpha";

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
  }

  export interface Run<G extends GameTypes> {
    readonly frames: number;
    readonly expect: Expect<G>;
  }

  /**
   * Headless, deterministic: same game + options => same run. Every call is a
   * fresh game instance (new world and timers, random seeded from config.seed,
   * in-memory ctx.save that starts empty), with no window and no audio output.
   * A script error stops the run; `expect.noErrors()` reports it. Tests are the
   * exported functions of test/main.test.ts, run by `alpha test`.
   */
  export function simulate<G extends GameTypes>(game: Game<G>, opts: SimulateOptions<G>): Run<G>;
}
