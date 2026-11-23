/*
 * Stage 069: what the "Can't keep up" warning offers, told apart by the render's share of real
 * time: a cheaper quality when the render is too slow, a longer latency when it keeps up but the
 * system holds it back now and then (Bernardo's laptop on battery, 2026-11-22).
 */
import { describe, expect, it } from "vitest";
import { LATENCIES, TOO_SLOW_SHARE, keepUpAdvice, longerLatency } from "../src/ui/lib/keep-up";
import { LATENCY_MAX_MS, LATENCY_MIN_MS } from "../src/engine/control";

describe("the keep-up advice", () => {
  it("offers the latencies the engine process takes, the next one up each time", () => {
    expect(LATENCIES[0]).toBe(LATENCY_MIN_MS);
    expect(LATENCIES.at(-1)).toBe(LATENCY_MAX_MS);
    expect(longerLatency(10)).toBe(20);
    expect(longerLatency(25)).toBe(30);
    expect(longerLatency(60)).toBe(80);
    expect(longerLatency(80)).toBeNull();
  });

  it("offers a cheaper quality when the render is too slow", () => {
    const a = keepUpAdvice(0.94, 20, "low", false);
    expect(a.cause).toBe("too-slow");
    expect(a.action).toEqual({ kind: "quality", quality: "lean" });
    expect(a.why).toContain("94 %");
    expect(a.why).not.toContain("battery");
    expect(keepUpAdvice(TOO_SLOW_SHARE, 20, "low", null).cause).toBe("too-slow");
    expect(keepUpAdvice(0.94, 20, "lean", null).action).toBeNull();   // nothing cheaper
  });

  it("offers a longer latency when the render keeps up but arrives late", () => {
    const a = keepUpAdvice(0.66, 10, "lean", true);
    expect(a.cause).toBe("late-now-and-then");
    expect(a.action).toEqual({ kind: "latency", ms: 20 });
    expect(a.why).toContain("10 ms");
    expect(a.why).toContain("It's on battery");
    expect(keepUpAdvice(TOO_SLOW_SHARE - 0.01, 40, "low", null).action).toEqual({ kind: "latency", ms: 60 });
    expect(keepUpAdvice(0.5, 80, "low", null).action).toBeNull();      // the longest already
    expect(keepUpAdvice(null, 20, "low", null).cause).toBe("late-now-and-then");   // no statistics yet
  });
});
