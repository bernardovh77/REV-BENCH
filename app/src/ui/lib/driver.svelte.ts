/*
 * driver.svelte.ts - the driver's seat (stage 066): the keys, the wheel, a controller and the A/B
 * switch, turned into commands for the engine store.
 *
 *   W or Up      the throttle key (its style: throttle-input.ts)
 *   I            ignition, on and off
 *   S            the starter, while held
 *   L            the 2-step (launch control), while held (stage 133): the engine computer holds
 *                its launch limit instead of the rev limit
 *   B            the A/B switch: flips RB_PARAM_VARIANT between 0 (the accepted sound) and the
 *                candidate chosen in the developer menu (stage 127's switch)
 *
 * Keys are ignored while a text field, a drop-down or a slider has the focus (they have their
 * own keys; a focused button doesn't count), or with Ctrl, Alt or Meta held (the menus' shortcuts). Losing the focus or being
 * hidden lets go of every held key and of analog travel (the dead-man rule's other half is in the
 * engine process). The window calls frame() every animation frame.
 */
import { GamepadReader, type PadLike } from "./gamepad";
import { ThrottleInput, type ThrottleStyle } from "./throttle-input";

/** What the driver needs of the engine store. */
export interface DriverStore {
  sendThrottle(value: number, live: boolean): void;
  setParam(name: string, value: number): Promise<void>;
  /** the engine's confirmed ignition, 0 or 1, if known */
  ignition(): number | null;
}

export interface KeyLike {
  code: string;
  repeat?: boolean;
  ctrlKey?: boolean;
  altKey?: boolean;
  metaKey?: boolean;
  target?: unknown;
  preventDefault?(): void;
}

/** The parts of window and document the driver listens to (so tests can give their own). */
export interface DriverEvents {
  addEventListener(type: string, listener: (event: Event) => void): void;
  removeEventListener(type: string, listener: (event: Event) => void): void;
}

export interface DriverDeps {
  store: DriverStore;
  window: DriverEvents;
  document: DriverEvents & { visibilityState: string };
  getPads(): readonly (PadLike | null)[];
  now(): number;
  /** 0 or 1, for the blind A/B (default Math.random) */
  random?: () => number;
}

export const THROTTLE_KEYS: readonly string[] = ["KeyW", "ArrowUp"];
/** A gap between frames this long means the window was away (hidden, minimised, stalled). */
export const FRAME_GAP_MS = 250;
export const IGNITION_KEY = "KeyI";
export const STARTER_KEY = "KeyS";
export const LAUNCH_KEY = "KeyL";
export const AB_KEY = "KeyB";
/** The core's RB_VARIANT_MAX: the candidates the switch can name. */
export const VARIANT_MAX = 7;

/** Where a key must not drive the engine: it belongs to a form control. (A focused button keeps
 *  only Space and Enter, which aren't the driver's, so it doesn't count.) */
function inFormControl(target: unknown): boolean {
  const tag = (target as { tagName?: unknown } | null)?.tagName;
  return tag === "INPUT" || tag === "SELECT" || tag === "TEXTAREA";
}

export class Driver {
  style = $state<ThrottleStyle>("ramped");
  /** the throttle the window last sent, 0..1 */
  throttle = $state(0);
  /** the controllers the Gamepad API sees */
  pads = $state<string[]>([]);
  learning = $state(false);
  /** the A/B switch: the candidate (1..VARIANT_MAX, 0 none), which side plays, blind or not */
  candidate = $state(0);
  side = $state<"A" | "B">("A");
  blind = $state(false);

  readonly input: ThrottleInput;
  readonly gamepad: GamepadReader;
  /** in blind mode, which variant each side plays: decided when blind mode starts */
  private blindB = 0;
  private starterHeld = false;
  private launchHeld = false;
  /** the throttle key's own press is current: after a let-go, only a fresh press counts */
  private throttlePressed = false;
  /** the last animation frame's time, to notice a window that stopped getting frames */
  private lastFrame: number | null = null;
  private started = false;
  private readonly onKeyDown = (event: Event) => this.keyDown(event as unknown as KeyLike);
  private readonly onKeyUp = (event: Event) => this.keyUp(event as unknown as KeyLike);
  private readonly onBlur = () => this.letGo();
  private readonly onVisibility = () => {
    if (this.deps.document.visibilityState !== "visible") this.letGo();
  };

  constructor(private readonly deps: DriverDeps) {
    this.input = new ThrottleInput({
      send: (value, live) => {
        this.throttle = value;
        deps.store.sendThrottle(value, live);
      },
    });
    this.gamepad = new GamepadReader(deps.getPads);
  }

  start(): void {
    if (this.started) return;
    this.started = true;
    const { window, document } = this.deps;
    window.addEventListener("keydown", this.onKeyDown);
    window.addEventListener("keyup", this.onKeyUp);
    window.addEventListener("blur", this.onBlur);
    document.addEventListener("visibilitychange", this.onVisibility);
    // nothing is sent yet: the throttle starts at zero, where the engine's does
  }

  dispose(): void {
    if (!this.started) return;
    this.started = false;
    const { window, document } = this.deps;
    window.removeEventListener("keydown", this.onKeyDown);
    window.removeEventListener("keyup", this.onKeyUp);
    window.removeEventListener("blur", this.onBlur);
    document.removeEventListener("visibilitychange", this.onVisibility);
    this.letGo();
  }

  setStyle(style: ThrottleStyle): void {
    this.style = style;
    this.input.setStyle(style, this.deps.now());
  }

  /** The slider and the wheel: they set the throttle in the latched style, and switch to it. */
  setLatched(value: number): void {
    if (this.style !== "latched") this.setStyle("latched");
    this.input.set(value, this.deps.now());
  }

  wheel(notches: number): void {
    if (this.style !== "latched") this.setStyle("latched");
    this.input.wheel(notches, this.deps.now());
  }

  /** Learns which controller input is the analog key: the next one pressed past half-way. */
  learnAnalog(): void {
    this.gamepad.learn();
    this.learning = true;
  }

  /** Every animation frame: controllers, the ramped key, the beat. */
  frame(): void {
    const now = this.deps.now();
    // Chromium gives a hidden or minimised window no frames, and its blur and visibility
    // events don't always arrive: a frame after a long gap lets go of everything first
    if (this.lastFrame !== null && now - this.lastFrame > FRAME_GAP_MS) this.letGo();
    this.lastFrame = now;
    if (this.style === "trigger") {
      const v = this.gamepad.trigger();
      if (v !== null) this.input.analog(v, now);
    } else if (this.style === "analog" || this.gamepad.learning) {
      const v = this.gamepad.analog();
      if (v !== null && this.style === "analog") this.input.analog(v, now);
    }
    this.learning = this.gamepad.learning;
    const pads = this.gamepad.pads();
    if (pads.join("\n") !== this.pads.join("\n")) this.pads = pads;
    this.input.tick(now);
  }

  /** The A/B switch's candidate (developer menu): 0 means none, and the two sides are the same. */
  setCandidate(candidate: number): void {
    if (!Number.isInteger(candidate) || candidate < 0 || candidate > VARIANT_MAX) return;
    this.candidate = candidate;
    this.chooseBlind();
    this.side = "A";
    void this.applyVariant();
  }

  /** Blind mode: a coin decides which side is the candidate; the menu doesn't say which. */
  setBlind(blind: boolean): void {
    this.blind = blind;
    this.chooseBlind();
    this.side = "A";
    void this.applyVariant();
  }

  /** The B key. */
  flip(): void {
    this.side = this.side === "A" ? "B" : "A";
    void this.applyVariant();
  }

  /** The variant playing now (what is sent as RB_PARAM_VARIANT). */
  get variant(): number {
    const candidateSide = this.blind ? (this.blindB ? "B" : "A") : "B";
    return this.side === candidateSide ? this.candidate : 0;
  }

  /** After a blind session: which side was the candidate. */
  reveal(): "A" | "B" {
    return this.blind ? (this.blindB ? "B" : "A") : "B";
  }

  private chooseBlind(): void {
    this.blindB = (this.deps.random ?? Math.random)() < 0.5 ? 1 : 0;
  }

  private applyVariant(): Promise<void> {
    return this.deps.store.setParam("variant", this.variant);
  }

  private keyDown(event: KeyLike): void {
    if (event.ctrlKey || event.altKey || event.metaKey || inFormControl(event.target)) return;
    const now = this.deps.now();
    if (THROTTLE_KEYS.includes(event.code)) {
      event.preventDefault?.();
      // an auto-repeat of a press the app has let go of (blur, hide, a stalled window) is not a
      // press: the key must be pressed again to open the throttle
      if (event.repeat && !this.throttlePressed) return;
      this.throttlePressed = true;
      this.input.keyDown(now);
    } else if (event.code === IGNITION_KEY && !event.repeat) {
      void this.deps.store.setParam("ignition", this.deps.store.ignition() === 1 ? 0 : 1);
    } else if (event.code === STARTER_KEY && !event.repeat) {
      this.starterHeld = true;
      void this.deps.store.setParam("starter", 1);
    } else if (event.code === LAUNCH_KEY && !event.repeat) {
      this.launchHeld = true;
      void this.deps.store.setParam("launch", 1);
    } else if (event.code === AB_KEY && !event.repeat) {
      this.flip();
    }
  }

  private keyUp(event: KeyLike): void {
    const now = this.deps.now();
    if (THROTTLE_KEYS.includes(event.code)) {
      this.throttlePressed = false;
      this.input.keyUp(now);
    }
    else if (event.code === STARTER_KEY && this.starterHeld) this.releaseStarter();
    else if (event.code === LAUNCH_KEY && this.launchHeld) this.releaseLaunch();
  }

  private releaseLaunch(): void {
    this.launchHeld = false;
    void this.deps.store.setParam("launch", 0);
  }

  private releaseStarter(): void {
    this.starterHeld = false;
    void this.deps.store.setParam("starter", 0);
  }

  /** The window lost the focus or was hidden: let go of everything held. */
  private letGo(): void {
    this.throttlePressed = false;
    this.input.blur(this.deps.now());
    if (this.starterHeld) this.releaseStarter();
    if (this.launchHeld) this.releaseLaunch();
  }
}
