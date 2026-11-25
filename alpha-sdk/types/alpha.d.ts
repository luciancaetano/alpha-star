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
  }

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

  /** A filled shape, or an image (`size` defaults to the image's pixel size). */
  export type Shape<I extends string = never> =
    | { circle: number; color: Color; outline?: Color }
    | { rect: Vec2; color: Color; outline?: Color }
    | { image: I; size?: Vec2 };

  export interface BodySpec {
    type: "static" | "kinematic" | "dynamic";
    collider: Collider;
    velocity?: Vec2;
  }

  /** Plain data describing an entity. Build it with functions, not classes. */
  export interface EntitySpec<T extends string = string, I extends string = never> {
    tags: T[];
    pos: Vec2;
    shape?: Shape<I>;
    body?: BodySpec;
    rotation?: number;
    z?: number;
  }

  export interface Body {
    get velocity(): Vec2Ref;
    set velocity(v: Vec2);
  }

  export interface Entity<T extends string = string> {
    readonly id: number;
    readonly tags: readonly T[];
    pos: Vec2Ref;
    rotation: number;
    z: number;
    /** Throws (pausing the game) if the entity was spawned without `body`. */
    readonly body: Body;
    /** False after destroy(); stale references are safe to check. */
    readonly alive: boolean;
  }

  // ---- services -------------------------------------------------------------

  export interface Input<A extends string> {
    /** True only on the step the action went down. */
    pressed(action: A): boolean;
    /** True while held. */
    down(action: A): boolean;
    /** True only on the step the action went up. */
    released(action: A): boolean;
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

  export type HitHandler<T extends string> = (a: Entity<T>, b: Entity<T>) => void;

  /** Everything a scene can touch. Scene-scoped: cleared on scene change. */
  export interface Context<G extends GameTypes, S> {
    readonly config: Readonly<GameConfig<G>>;
    /** Fixed step in seconds (constant). */
    readonly dt: number;
    /** Seconds since scene enter. */
    readonly time: number;
    /** Scene state built by the scene's `state()`; reset on every enter. */
    state: S;
    readonly input: Input<G["actions"]>;
    readonly random: Random;
    readonly save: Save;
    readonly ui: UI<G["ui"]>;
    readonly audio: Audio<G["sounds"]>;

    spawn(spec: EntitySpec<G["tags"], G["images"]>): Entity<G["tags"]>;
    /** Removed at the end of the step. Destroying twice is a no-op. */
    destroy(entity: Entity<G["tags"]>): void;
    /** Throws if no entity has the tag. */
    find(tag: G["tags"]): Entity<G["tags"]>;
    findAll(tag: G["tags"]): Entity<G["tags"]>[];

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
    /** Action -> device inputs, e.g. ["Space", "MouseLeft", "GamepadA"]. */
    actions: Record<G["actions"], string[]>;
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
      fonts?: Record<string, string>;
      /** Document name -> .rml file. Files it links (`<link href>`, `src=`, rcss `url()`) are packed too. */
      ui?: Record<keyof G["ui"] & string, string>;
    };
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
  import type { Game, GameTypes } from "alpha";

  export interface SimulateOptions<G extends GameTypes> {
    /** Scene to start in (default: the game's `start`). */
    scene?: keyof G["scenes"] & string;
    /** Simulated time; the run is round(seconds * 60) fixed steps of 1/60 s. */
    seconds: number;
    /** Action -> frame numbers (0-based) on which it is pressed for one step. */
    inputs?: Partial<Record<G["actions"], number[]>>;
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
    /** The UI document is shown at the end of the run. */
    shown(doc: keyof G["ui"] & string): void;
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
