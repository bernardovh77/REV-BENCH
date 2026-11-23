/*
 * keep-up.ts - what the "Can't keep up" warning offers (stage 069).
 *
 * The warning (the store's fallingBehind) has two causes that want different cures, told apart by
 * the render's mean share of real time:
 * - **Too slow:** the render takes most of each block's time (TOO_SLOW_SHARE or more), so the ring
 *   drains whatever its size: a cheaper quality is the cure.
 * - **Late now and then:** the render keeps up on average, but the system holds the render thread
 *   back for longer than the ring holds (Bernardo's laptop on battery, 2026-11-22: 66 % of real
 *   time, 10 ms of latency, underruns): a longer latency is the cure, the next one up.
 * On battery a laptop slows itself down: the warning says so either way. Nothing is switched
 * without the user's click (app-design section 11).
 */
import type { Quality } from "../../shared/addon";
import { cheaperQuality } from "./qualities";

/** the latencies offered, ms (the Latency selector's) */
export const LATENCIES = [10, 20, 30, 40, 60, 80] as const;
/** a mean render share of real time at or over this is too slow for any latency */
export const TOO_SLOW_SHARE = 0.85;

export type KeepUpAction = { kind: "quality"; quality: Quality } | { kind: "latency"; ms: number } | null;

export interface KeepUpAdvice {
  cause: "too-slow" | "late-now-and-then";
  /** the warning's tooltip, in words */
  why: string;
  /** the one thing it offers, or null when there's nothing left to offer */
  action: KeepUpAction;
}

/** The next longer latency than `ms`, or null at the longest. */
export function longerLatency(ms: number): number | null {
  return LATENCIES.find((l) => l > ms) ?? null;
}

/**
 * What to say and offer, from the render's mean share of real time (0..1, null if unknown), the
 * latency (ms), the quality playing and whether the computer is on battery (null if unknown).
 */
export function keepUpAdvice(share: number | null, latencyMs: number, playing: Quality | null, onBattery: boolean | null): KeepUpAdvice {
  const battery = onBattery ? " It's on battery, which slows it down: plugging it in helps." : "";
  if (share !== null && share >= TOO_SLOW_SHARE) {
    const cheaper = cheaperQuality(playing);
    return {
      cause: "too-slow",
      why: `This computer takes ${Math.round(100 * share)} % of real time to make the sound at this quality: the sound may break up.${battery}`,
      action: cheaper ? { kind: "quality", quality: cheaper } : null,
    };
  }
  const longer = longerLatency(latencyMs);
  return {
    cause: "late-now-and-then",
    why: `The sound is made fast enough, but this computer holds it back now and then for longer than the ${latencyMs} ms latency covers: the sound may break up. A longer latency gives it room.${battery}`,
    action: longer === null ? null : { kind: "latency", ms: longer },
  };
}
