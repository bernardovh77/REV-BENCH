/*
 * The driver's seat (stage 066): keys to commands, letting go on blur and hide, the controller
 * each frame, the A/B switch and its blind mode.
 */
import { describe, expect, it } from "vitest";
import { Driver, VARIANT_MAX, type KeyLike } from "../src/ui/lib/driver.svelte";
import type { PadLike } from "../src/ui/lib/gamepad";

class Events {
  listeners = new Map<string, ((e: Event) => void)[]>();
  visibilityState = "visible";
  addEventListener(type: string, l: (e: Event) => void): void {
    this.listeners.set(type, [...(this.listeners.get(type) ?? []), l]);
  }
  removeEventListener(type: string, l: (e: Event) => void): void {
    this.listeners.set(type, (this.listeners.get(type) ?? []).filter((x) => x !== l));
  }
  fire(type: string, event: unknown = {}): void {
    for (const l of this.listeners.get(type) ?? []) l(event as Event);
  }
}

function setup(options: { pads?: PadLike[]; random?: number } = {}) {
  let t = 0;
  const throttles: { value: number; live: boolean }[] = [];
  const params: [string, number][] = [];
  let ignition = 0;
  const win = new Events();
  const doc = new Events();
  const driver = new Driver({
    store: {
      sendThrottle: (value, live) => void throttles.push({ value, live }),
      setParam: async (name, value) => {
        params.push([name, value]);
        if (name === "ignition") ignition = value;
      },
      ignition: () => ignition,
    },
    window: win,
    document: doc,
    getPads: () => options.pads ?? [],
    now: () => t,
    random: () => options.random ?? 0.9,
  });
  driver.start();
  const key = (type: "keydown" | "keyup", code: string, extra: Partial<KeyLike> = {}) => win.fire(type, { code, ...extra });
  const run = (ms: number) => {
    for (let i = 0; i < ms; i += 10) {
      t += 10;
      driver.frame();
    }
  };
  /** time passes with no frames at all (a hidden window) */
  const skip = (ms: number) => void (t += ms);
  return { driver, throttles, params, win, doc, key, run, skip };
}

describe("keys", () => {
  it("W and Up are the throttle key, ramped by default", () => {
    for (const code of ["KeyW", "ArrowUp"]) {
      const s = setup();
      s.key("keydown", code);
      s.run(500);
      expect(s.driver.throttle).toBe(1);
      expect(s.throttles.at(-1)?.live).toBe(true);
      s.key("keyup", code);
      s.run(500);
      expect(s.driver.throttle).toBe(0);
    }
  });

  it("I toggles the ignition from what the engine confirms; S holds the starter", () => {
    const s = setup();
    s.key("keydown", "KeyI");
    s.key("keydown", "KeyI", { repeat: true });
    s.key("keydown", "KeyS");
    s.key("keyup", "KeyS");
    s.key("keydown", "KeyI");
    expect(s.params).toEqual([["ignition", 1], ["starter", 1], ["starter", 0], ["ignition", 0]]);
  });

  it("L holds the 2-step while held, and lets go of it on blur (stage 133)", () => {
    const s = setup();
    s.key("keydown", "KeyL");
    s.key("keydown", "KeyL", { repeat: true });
    s.key("keyup", "KeyL");
    expect(s.params).toEqual([["launch", 1], ["launch", 0]]);
    s.key("keydown", "KeyL");
    s.win.fire("blur");
    expect(s.params.at(-1)).toEqual(["launch", 0]);
  });

  it("leaves keys alone in a form control, or with Ctrl, Alt or Meta", () => {
    const s = setup();
    s.key("keydown", "KeyI", { target: { tagName: "SELECT" } });
    s.key("keydown", "KeyI", { ctrlKey: true });
    s.key("keydown", "KeyW", { target: { tagName: "INPUT" } });
    s.run(300);
    expect(s.params).toEqual([]);
    expect(s.driver.throttle).toBe(0);
  });

  it("still drives with a button focused (after clicking Ignition)", () => {
    const s = setup();
    s.key("keydown", "KeyI", { target: { tagName: "BUTTON" } });
    expect(s.params).toEqual([["ignition", 1]]);
  });

  it("stops listening when disposed", () => {
    const s = setup();
    s.driver.dispose();
    s.key("keydown", "KeyI");
    expect(s.params).toEqual([]);
  });
});

describe("letting go", () => {
  it("on blur the held throttle key and starter are released", () => {
    const s = setup();
    s.key("keydown", "KeyW");
    s.key("keydown", "KeyS");
    s.run(300);
    s.win.fire("blur");
    expect(s.params.at(-1)).toEqual(["starter", 0]);
    s.run(500);
    expect(s.driver.throttle).toBe(0);
  });

  it("when hidden, analog travel drops to zero at once", () => {
    const buttons = new Array(17).fill(0).map((value) => ({ value, pressed: false }));
    const s = setup({ pads: [{ index: 0, id: "pad", connected: true, mapping: "standard", axes: [], buttons }] });
    s.driver.setStyle("trigger");
    buttons[7] = { value: 0.6, pressed: true };
    s.run(10);
    expect(s.throttles.at(-1)).toEqual({ value: 0.6, live: true });
    s.doc.visibilityState = "hidden";
    s.doc.fire("visibilitychange");
    expect(s.throttles.at(-1)).toEqual({ value: 0, live: true });
  });
});

describe("when the window's events don't come (2026-11-16)", () => {
  it("a frame after a long gap lets go, and the key's auto-repeat doesn't open it again", () => {
    const s = setup();
    s.key("keydown", "KeyW");
    s.run(500);
    expect(s.driver.throttle).toBe(1);
    // hidden: no frames, no blur, no visibility event; the key still physically down
    const t = s.driver;
    s.skip(1500); // the next frame comes 1.5 s later
    s.run(10);
    s.key("keydown", "KeyW", { repeat: true });
    s.run(500);
    expect(t.throttle).toBe(0);
    // a fresh press opens it
    s.key("keyup", "KeyW");
    s.key("keydown", "KeyW");
    s.run(500);
    expect(t.throttle).toBe(1);
  });

  it("an auto-repeat after a blur is not a press either", () => {
    const s = setup();
    s.key("keydown", "KeyW");
    s.run(200);
    s.win.fire("blur");
    s.key("keydown", "KeyW", { repeat: true });
    s.run(800);
    expect(s.driver.throttle).toBe(0);
  });
});

describe("the controller", () => {
  it("the trigger style follows the right trigger every frame", () => {
    const buttons = new Array(17).fill(0).map((value) => ({ value, pressed: false }));
    const s = setup({ pads: [{ index: 0, id: "Xbox", connected: true, mapping: "standard", axes: [], buttons }] });
    s.driver.setStyle("trigger");
    buttons[7] = { value: 0.6, pressed: true };
    s.run(10);
    expect(s.driver.throttle).toBe(0.6);
    expect(s.driver.pads).toEqual(["Xbox"]);
  });

  it("the analog style learns its input, then follows it", () => {
    const axes = [0, -1];
    const s = setup({ pads: [{ index: 0, id: "Razer", connected: true, mapping: "", axes, buttons: [] }] });
    s.driver.setStyle("analog");
    s.driver.learnAnalog();
    s.run(10);
    expect(s.driver.learning).toBe(true);
    axes[1] = 0.5;
    s.run(10);
    expect(s.driver.learning).toBe(false);
    expect(s.driver.throttle).toBeCloseTo(0.75, 9);
  });

  it("the wheel and the slider switch to the latched style", () => {
    const s = setup();
    s.driver.setLatched(0.5);
    expect(s.driver.style).toBe("latched");
    s.driver.wheel(-2);
    expect(s.driver.throttle).toBeCloseTo(0.6, 9);
    expect(s.throttles.at(-1)?.live).toBe(false);
  });
});

describe("the A/B switch", () => {
  it("B flips the variant between 0 and the candidate", () => {
    const s = setup();
    s.driver.setCandidate(3);
    expect(s.params.at(-1)).toEqual(["variant", 0]);
    s.key("keydown", "KeyB");
    expect(s.driver.side).toBe("B");
    expect(s.params.at(-1)).toEqual(["variant", 3]);
    s.key("keydown", "KeyB");
    expect(s.params.at(-1)).toEqual(["variant", 0]);
  });

  it("with no candidate both sides are the accepted sound", () => {
    const s = setup();
    s.key("keydown", "KeyB");
    expect(s.driver.variant).toBe(0);
  });

  it("blind mode lets a coin decide which side is the candidate, and reveals it after", () => {
    for (const [coin, candidateSide] of [[0.9, "A"], [0.1, "B"]] as const) {
      const s = setup({ random: coin });
      s.driver.setCandidate(2);
      s.driver.setBlind(true);
      expect(s.driver.reveal()).toBe(candidateSide);
      expect(s.driver.variant).toBe(candidateSide === "A" ? 2 : 0);
      s.driver.flip();
      expect(s.driver.variant).toBe(candidateSide === "A" ? 0 : 2);
    }
  });

  it("refuses a candidate the core can't name", () => {
    const s = setup();
    s.driver.setCandidate(VARIANT_MAX + 1);
    s.driver.setCandidate(1.5);
    expect(s.driver.candidate).toBe(0);
  });
});
