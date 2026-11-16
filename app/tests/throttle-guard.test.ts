/*
 * The dead-man rule (stage 066, docs/review-2026-11-15.md problem 1): while a live throttle style
 * is active the window sends the throttle at a steady beat; if none arrives for 250 ms the engine
 * process ramps it to zero over 100 ms. The on/off and latched styles hold by design.
 */
import { describe, expect, it } from "vitest";
import { DEADMAN_RAMP_MS, DEADMAN_TIMEOUT_MS, ThrottleGuard } from "../src/engine/throttle-guard";

/** Runs the guard from `from` to `to` in 5 ms steps, collecting what it says to send. */
function run(guard: ThrottleGuard, from: number, to: number): { t: number; value: number }[] {
  const sent: { t: number; value: number }[] = [];
  for (let t = from; t <= to; t += 5) {
    const v = guard.tick(t);
    if (v !== null) sent.push({ t, value: v });
  }
  return sent;
}

describe("the throttle guard", () => {
  it("has the card's numbers", () => {
    expect(DEADMAN_TIMEOUT_MS).toBe(250);
    expect(DEADMAN_RAMP_MS).toBe(100);
  });

  it("sends nothing while the beat keeps coming", () => {
    const g = new ThrottleGuard();
    const sent: number[] = [];
    for (let t = 0; t < 2000; t += 50) {
      g.beat(0.8, true, t);
      for (let u = t; u < t + 50; u += 5) {
        const v = g.tick(u);
        if (v !== null) sent.push(v);
      }
    }
    expect(sent).toEqual([]);
  });

  it("ramps a live throttle to zero over 100 ms once the beat stops for 250 ms", () => {
    const g = new ThrottleGuard();
    g.beat(0.8, true, 0);
    const sent = run(g, 0, 1000);
    expect(sent.length).toBeGreaterThan(3);
    // nothing before the timeout
    expect(sent[0].t).toBeGreaterThanOrEqual(DEADMAN_TIMEOUT_MS);
    // falling all the way, never rising
    for (let i = 1; i < sent.length; i++) expect(sent[i].value).toBeLessThanOrEqual(sent[i - 1].value);
    const last = sent[sent.length - 1];
    expect(last.value).toBe(0);
    expect(last.t).toBeLessThanOrEqual(DEADMAN_TIMEOUT_MS + DEADMAN_RAMP_MS + 5);
    // a ramp, not a cut: at its middle it is near half the value
    const mid = sent.find((s) => s.t >= DEADMAN_TIMEOUT_MS + DEADMAN_RAMP_MS / 2)!;
    expect(mid.value).toBeGreaterThan(0.3);
    expect(mid.value).toBeLessThan(0.5);
    // and then it stays quiet
    expect(run(g, 1000, 3000)).toEqual([]);
  });

  it("holds an on/off or latched throttle however long the window is silent", () => {
    const g = new ThrottleGuard();
    g.beat(1, false, 0);
    expect(run(g, 0, 5000)).toEqual([]);
  });

  it("does nothing for a live throttle already at zero", () => {
    const g = new ThrottleGuard();
    g.beat(0, true, 0);
    expect(run(g, 0, 2000)).toEqual([]);
  });

  it("gives the throttle back to the window when a beat arrives mid-ramp", () => {
    const g = new ThrottleGuard();
    g.beat(1, true, 0);
    const ramp = run(g, 0, 300);
    expect(ramp.length).toBeGreaterThan(0);
    g.beat(0.6, true, 300);
    expect(run(g, 305, 500)).toEqual([]);
  });

  it("forgets the live style when the window switches to a holding one", () => {
    const g = new ThrottleGuard();
    g.beat(0.5, true, 0);
    g.beat(0.5, false, 100);
    expect(run(g, 100, 2000)).toEqual([]);
  });

  it("can be told the window went away: ramps at once", () => {
    const g = new ThrottleGuard();
    g.beat(0.9, true, 0);
    g.release(10);
    const sent = run(g, 10, 200);
    expect(sent[0].t).toBeLessThan(20);
    expect(sent[sent.length - 1].value).toBe(0);
  });

  it("ignores a value that is not a finite number in 0..1", () => {
    const g = new ThrottleGuard();
    for (const bad of [Number.NaN, Infinity, -0.1, 1.1]) expect(() => g.beat(bad, true, 0)).toThrow();
  });
});
