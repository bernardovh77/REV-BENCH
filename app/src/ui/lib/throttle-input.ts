/*
 * throttle-input.ts - the driver's throttle in the window (stage 066). Turns keys, the mouse
 * wheel, a slider and analog travel (an analog keyboard's key through its controller emulation,
 * or a controller's trigger, both read with the Gamepad API by gamepad.ts) into the throttle the
 * engine process is sent, as the `throttle` command (src/shared/protocol.ts). The styles, as
 * Bernardo asked on 2026-11-13:
 *
 *   onoff    a key snaps the throttle 0 <-> 100 % on each press; it holds
 *   ramped   held, the throttle rises at a set rate; released, it falls
 *   analog   an analog key's travel
 *   trigger  a controller's analog trigger
 *   latched  the mouse: the slider and the wheel set it; it holds
 *
 * ramped, analog and trigger are live: the dead-man rule (src/engine/throttle-guard.ts) covers
 * them, so while their throttle is above zero it is sent every BEAT_MS even when it doesn't
 * change, and on blur they let go (Chromium stops sending a page key-ups and controller state
 * once it loses focus). Pure and clocked from outside: the window calls tick() every frame.
 */

export type ThrottleStyle = "onoff" | "ramped" | "analog" | "trigger" | "latched";

export const THROTTLE_STYLES: readonly ThrottleStyle[] = ["onoff", "ramped", "analog", "trigger", "latched"];

/** A live throttle above zero is sent at least this often (the guard waits 250 ms). */
export const BEAT_MS = 50;
/** One notch of the wheel moves a latched throttle by this much. */
export const WHEEL_STEP = 0.05;

export interface ThrottleInputDeps {
  /** sends the throttle to the engine: `live` when the dead-man rule covers it */
  send(value: number, live: boolean): void;
  /** the ramped key's time from 0 to 100 % held, ms (default 400) */
  riseMs?: number;
  /** and from 100 % to 0 released, ms (default 250) */
  fallMs?: number;
}

const LIVE: ReadonlySet<ThrottleStyle> = new Set(["ramped", "analog", "trigger"]);

function clamp01(v: number): number {
  return v < 0 ? 0 : v > 1 ? 1 : v;
}

export class ThrottleInput {
  style: ThrottleStyle = "ramped";
  /** the throttle now, 0..1 */
  value = 0;
  private held = false;
  private lastTick: number | null = null;
  private lastSent = -Infinity;
  private sentValue = -1;

  constructor(private readonly deps: ThrottleInputDeps) {}

  get live(): boolean {
    return LIVE.has(this.style);
  }

  /** Another style: the throttle starts from zero, so changing never jumps to full. */
  setStyle(style: ThrottleStyle, now: number): void {
    this.style = style;
    this.held = false;
    this.value = 0;
    this.sentValue = -1;
    this.lastTick = now;
    this.emit(now);
  }

  /** The throttle key went down (a repeat while held is ignored). */
  keyDown(now: number): void {
    if (this.held) return;
    this.held = true;
    if (this.style === "onoff") {
      this.value = this.value > 0 ? 0 : 1;
      this.emit(now);
    }
  }

  keyUp(_now: number): void {
    this.held = false;
  }

  /** Analog travel, 0..1, read every frame (analog and trigger styles only). */
  analog(travel: number, now: number): void {
    if (this.style !== "analog" && this.style !== "trigger") return;
    if (typeof travel !== "number" || Number.isNaN(travel)) return;
    const v = clamp01(travel);
    if (v === this.value) return;
    this.value = v;
    this.emit(now);
  }

  /** The slider (latched style). */
  set(value: number, now: number): void {
    if (this.style !== "latched" || !Number.isFinite(value)) return;
    this.value = clamp01(value);
    this.emit(now);
  }

  /** The wheel, in notches: negative is up, more throttle (latched style). */
  wheel(notches: number, now: number): void {
    if (this.style !== "latched" || !Number.isFinite(notches) || notches === 0) return;
    this.value = clamp01(Math.round((this.value - notches * WHEEL_STEP) * 1e9) / 1e9);
    this.emit(now);
  }

  /** The window lost focus or was hidden: held keys and analog travel let go. */
  blur(now: number): void {
    this.held = false;
    if (this.style === "analog" || this.style === "trigger") {
      this.value = 0;
      this.emit(now);
    }
  }

  /** Every frame: moves the ramped key and keeps the beat. */
  tick(now: number): void {
    const dt = this.lastTick === null ? 0 : Math.max(0, now - this.lastTick);
    this.lastTick = now;
    if (this.style === "ramped" && dt > 0) {
      const rate = this.held ? 1 / (this.deps.riseMs ?? 400) : -1 / (this.deps.fallMs ?? 250);
      const v = clamp01(this.value + rate * dt);
      if (v !== this.value) {
        this.value = v;
        this.emit(now);
        return;
      }
    }
    if (this.live && this.value > 0 && now - this.lastSent >= BEAT_MS) this.emit(now);
  }

  private emit(now: number): void {
    // a holding style sends only changes; a live one also every beat
    if (!this.live && this.value === this.sentValue) return;
    this.lastSent = now;
    this.sentValue = this.value;
    this.deps.send(this.value, this.live);
  }
}
