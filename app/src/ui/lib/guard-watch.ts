/*
 * guard-watch.ts - the model's own warning (stage 068). The core counts its numerical guards
 * stepping in (rb.h: RB_TLM_GUARD_RATE, per second over each cycle); averaged over a second
 * above RB_GUARD_RATE_WARN the build is outside what the model handles well. Each snapshot gives
 * a rate; the watch averages the last WINDOW_S of them, timed by the audio (the host's frames),
 * and says so while the average is over, and for QUIET_S after it falls back (so the warning
 * doesn't blink on every lift-off).
 */

/** rb.h's RB_GUARD_RATE_WARN (the app's test reads the header and checks they agree). */
export const GUARD_RATE_WARN = 1000;
export const WINDOW_S = 1;
export const QUIET_S = 3;

export class GuardWatch {
  private samples: { at: number; rate: number }[] = [];
  private overAt = -Infinity;

  /** A snapshot's guard rate at `seconds` of audio; whether to warn now. */
  update(rate: number, seconds: number): boolean {
    this.samples.push({ at: seconds, rate: Number.isFinite(rate) && rate > 0 ? rate : 0 });
    while (this.samples.length && this.samples[0].at < seconds - WINDOW_S) this.samples.shift();
    if (this.average() > GUARD_RATE_WARN) this.overAt = seconds;
    return seconds - this.overAt <= QUIET_S;
  }

  /** The mean rate over the window. */
  average(): number {
    if (!this.samples.length) return 0;
    return this.samples.reduce((sum, s) => sum + s.rate, 0) / this.samples.length;
  }

  reset(): void {
    this.samples = [];
    this.overAt = -Infinity;
  }
}
