/*
 * throttle-guard.ts - the dead-man rule, in the engine process (stage 066; the review of
 * 2026-11-15, problem 1). While a live throttle style is active (a ramped key, an analog key, a
 * controller's trigger) the window sends the throttle at a steady beat. If no beat arrives for
 * DEADMAN_TIMEOUT_MS the guard ramps the throttle to zero over DEADMAN_RAMP_MS. Chromium slows a
 * hidden window's timers and stops sending a page its key-ups and controller state once it loses
 * focus, so without this an engine could stay at full throttle with nobody able to lift.
 *
 * The on/off and latched styles hold their value by design (a switch, a slider) and are exempt.
 * The guard only decides values; control.ts sends them to the host. Pure and clocked from
 * outside, so it is tested without timers.
 */

/** No beat for this long and a live throttle starts falling. */
export const DEADMAN_TIMEOUT_MS = 250;
/** How long the fall to zero takes. */
export const DEADMAN_RAMP_MS = 100;
/** The guard sends a ramp's values no more often than this. */
const RAMP_STEP_MS = 10;

export class ThrottleGuard {
  /** the throttle the window last sent */
  private value = 0;
  private live = false;
  private lastBeat = 0;
  /** when the ramp began, and from what value; null when not ramping */
  private rampFrom: { t: number; value: number } | null = null;
  private lastSent = -Infinity;

  /** The window's throttle: `live` for the styles the rule covers. Throws for a bad value. */
  beat(value: number, live: boolean, now: number): void {
    if (typeof value !== "number" || !Number.isFinite(value) || value < 0 || value > 1) {
      throw new RangeError("a throttle is a number from 0 to 1");
    }
    this.value = value;
    this.live = live;
    this.lastBeat = now;
    this.rampFrom = null;
  }

  /** The window is gone (closed, reloaded, hidden): a live throttle falls now, not in 250 ms. */
  release(now: number): void {
    if (this.live && this.value > 0 && !this.rampFrom) this.startRamp(now);
  }

  /** Called every few milliseconds: a throttle to send, or null for nothing. */
  tick(now: number): number | null {
    if (!this.live || this.value <= 0) return null;
    if (!this.rampFrom) {
      if (now - this.lastBeat < DEADMAN_TIMEOUT_MS) return null;
      this.startRamp(now);
    }
    const ramp = this.rampFrom!;
    const left = 1 - (now - ramp.t) / DEADMAN_RAMP_MS;
    if (left <= 0) {
      this.value = 0;
      this.live = false;
      this.rampFrom = null;
      return 0;
    }
    if (now - this.lastSent < RAMP_STEP_MS) return null;
    this.lastSent = now;
    return ramp.value * left;
  }

  private startRamp(now: number): void {
    this.rampFrom = { t: now, value: this.value };
    this.lastSent = -Infinity;
  }
}
