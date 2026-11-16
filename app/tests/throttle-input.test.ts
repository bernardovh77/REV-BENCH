/*
 * The window's throttle (stage 066): the four styles Bernardo asked for on 2026-11-13 and the
 * latched mouse, the beat the dead-man rule listens for, and letting go when the window loses
 * focus.
 */
import { describe, expect, it } from "vitest";
import { BEAT_MS, ThrottleInput, type ThrottleStyle } from "../src/ui/lib/throttle-input";

function setup(style: ThrottleStyle) {
  let t = 0;
  const sent: { t: number; value: number; live: boolean }[] = [];
  const input = new ThrottleInput({ send: (value, live) => sent.push({ t, value, live }), riseMs: 400, fallMs: 200 });
  input.setStyle(style, t);
  sent.length = 0;
  /** time passes in 10 ms frames */
  const run = (ms: number) => {
    for (let i = 0; i < ms; i += 10) {
      t += 10;
      input.tick(t);
    }
  };
  return { input, sent, run, now: () => t, last: () => sent[sent.length - 1] };
}

describe("the on/off style", () => {
  it("snaps 0 -> 100 % -> 0 on each press, holding (not live)", () => {
    const s = setup("onoff");
    s.input.keyDown(s.now());
    expect(s.last()).toMatchObject({ value: 1, live: false });
    s.input.keyUp(s.now());
    s.run(1000);
    expect(s.input.value).toBe(1);
    s.input.keyDown(s.now());
    expect(s.last()).toMatchObject({ value: 0, live: false });
  });

  it("ignores the key repeating while held", () => {
    const s = setup("onoff");
    s.input.keyDown(s.now());
    s.input.keyDown(s.now());
    s.input.keyDown(s.now());
    expect(s.input.value).toBe(1);
  });
});

describe("the ramped key", () => {
  it("rises while held at the set rate, and falls when released", () => {
    const s = setup("ramped");
    s.input.keyDown(s.now());
    s.run(200);
    expect(s.input.value).toBeCloseTo(0.5, 6);
    s.run(400);
    expect(s.input.value).toBe(1);
    s.input.keyUp(s.now());
    s.run(100);
    expect(s.input.value).toBeCloseTo(0.5, 6);
    s.run(200);
    expect(s.input.value).toBe(0);
  });

  it("is live, and sends at the beat while above zero, even holding still", () => {
    const s = setup("ramped");
    s.input.keyDown(s.now());
    s.run(1000);
    const held = s.sent.filter((m) => m.t > 500);
    expect(held.every((m) => m.live)).toBe(true);
    for (let i = 1; i < held.length; i++) expect(held[i].t - held[i - 1].t).toBeLessThanOrEqual(BEAT_MS);
    expect(held.length).toBeGreaterThanOrEqual(500 / BEAT_MS - 1);
  });

  it("goes quiet once back at zero", () => {
    const s = setup("ramped");
    s.input.keyDown(s.now());
    s.run(100);
    s.input.keyUp(s.now());
    s.run(500);
    const n = s.sent.length;
    expect(s.last().value).toBe(0);
    s.run(2000);
    expect(s.sent.length).toBe(n);
  });
});

describe("analog travel (an analog key or a controller's trigger)", () => {
  for (const style of ["analog", "trigger"] as const) {
    it(`${style}: follows the travel, live, at the beat`, () => {
      const s = setup(style);
      s.input.analog(0.3, s.now());
      s.run(10);
      expect(s.last()).toMatchObject({ value: 0.3, live: true });
      s.input.analog(0.8, s.now());
      s.run(10);
      expect(s.input.value).toBe(0.8);
      s.run(500);
      const late = s.sent.filter((m) => m.t > 100);
      expect(late.length).toBeGreaterThanOrEqual(400 / BEAT_MS - 1);
    });
  }

  it("clamps travel outside 0..1 and ignores what isn't a number", () => {
    const s = setup("trigger");
    s.input.analog(1.7, s.now());
    expect(s.input.value).toBe(1);
    s.input.analog(-0.2, s.now());
    expect(s.input.value).toBe(0);
    s.input.analog(Number.NaN, s.now());
    expect(s.input.value).toBe(0);
  });

  it("the keyboard's key does nothing in an analog style", () => {
    const s = setup("analog");
    s.input.keyDown(s.now());
    s.run(500);
    expect(s.input.value).toBe(0);
  });
});

describe("the latched style (the mouse and wheel)", () => {
  it("holds what the slider or the wheel sets, not live", () => {
    const s = setup("latched");
    s.input.set(0.4, s.now());
    expect(s.last()).toMatchObject({ value: 0.4, live: false });
    s.input.wheel(-1, s.now()); // up a notch
    expect(s.input.value).toBeCloseTo(0.45, 9);
    s.input.wheel(+3, s.now());
    expect(s.input.value).toBeCloseTo(0.3, 9);
    s.run(2000);
    expect(s.input.value).toBeCloseTo(0.3, 9);
  });
});

describe("letting go", () => {
  it("on blur, a held ramped key is released and falls", () => {
    const s = setup("ramped");
    s.input.keyDown(s.now());
    s.run(500);
    s.input.blur(s.now());
    s.run(300);
    expect(s.input.value).toBe(0);
  });

  it("on blur, analog travel and a trigger drop to zero at once", () => {
    for (const style of ["analog", "trigger"] as const) {
      const s = setup(style);
      s.input.analog(0.9, s.now());
      s.input.blur(s.now());
      expect(s.last()).toMatchObject({ value: 0, live: true });
    }
  });

  it("on blur, the holding styles keep their value", () => {
    const s = setup("onoff");
    s.input.keyDown(s.now());
    s.input.blur(s.now());
    expect(s.input.value).toBe(1);
  });

  it("changing style starts from zero, so nothing jumps to full", () => {
    const s = setup("onoff");
    s.input.keyDown(s.now());
    s.input.setStyle("ramped", s.now());
    expect(s.input.value).toBe(0);
    expect(s.last()).toMatchObject({ value: 0 });
  });
});
