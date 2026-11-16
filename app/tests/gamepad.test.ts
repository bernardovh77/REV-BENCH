/*
 * Analog travel from the Gamepad API (stage 066): a controller's trigger, and an analog
 * keyboard's key learned from whichever input Razer Synapse's emulation maps it to.
 */
import { describe, expect, it } from "vitest";
import { GamepadReader, RIGHT_TRIGGER, type PadLike } from "../src/ui/lib/gamepad";

function pad(index: number, axes: number[], buttons: number[], mapping = "standard"): PadLike {
  return { index, id: `pad ${index}`, connected: true, mapping, axes, buttons: buttons.map((value) => ({ value, pressed: value > 0.5 })) };
}

describe("the trigger", () => {
  it("reads the standard mapping's right trigger", () => {
    const buttons = new Array(17).fill(0);
    buttons[RIGHT_TRIGGER] = 0.42;
    const r = new GamepadReader(() => [null, pad(1, [0, 0, 0, 0], buttons)]);
    expect(r.trigger()).toBe(0.42);
    expect(r.pads()).toEqual(["pad 1"]);
  });

  it("is null with no controller, or none with the standard mapping", () => {
    expect(new GamepadReader(() => []).trigger()).toBeNull();
    expect(new GamepadReader(() => [pad(0, [0], new Array(17).fill(0), "")]).trigger()).toBeNull();
  });
});

describe("learning an analog key", () => {
  it("keeps the button that moves past half its travel", () => {
    const buttons = new Array(17).fill(0);
    const r = new GamepadReader(() => [pad(0, [0, 0], buttons)]);
    r.learn();
    buttons[3] = 0.2;
    expect(r.analog()).toBeNull();
    expect(r.learning).toBe(true);
    buttons[3] = 0.7;
    expect(r.analog()).toBe(0.7);
    expect(r.learning).toBe(false);
    expect(r.source).toEqual({ kind: "button", pad: 0, index: 3 });
    buttons[3] = 0.25;
    expect(r.analog()).toBe(0.25);
  });

  it("keeps an axis resting at -1, read 0..1 over its whole span", () => {
    const axes = [0, -1, 0];
    const r = new GamepadReader(() => [pad(0, axes, [0, 0], "")]);
    r.learn();
    axes[1] = 0.1;
    expect(r.analog()).toBeCloseTo(0.55, 9);
    expect(r.source).toMatchObject({ kind: "axis", index: 1, rest: -1, full: 1 });
    axes[1] = -1;
    expect(r.analog()).toBe(0);
  });

  it("keeps an axis resting at 0 moving either way", () => {
    const axes = [0, 0];
    const r = new GamepadReader(() => [pad(2, axes, [], "")]);
    r.learn();
    axes[0] = -0.6;
    expect(r.analog()).toBeCloseTo(0.6, 9);
    axes[0] = -1;
    expect(r.analog()).toBe(1);
  });

  it("ignores what was already pressed when learning began", () => {
    const buttons = [1, 0];
    const r = new GamepadReader(() => [pad(0, [], buttons)]);
    r.learn();
    expect(r.analog()).toBeNull();
    buttons[1] = 1;
    expect(r.analog()).toBe(1);
    expect(r.source).toMatchObject({ index: 1 });
  });

  it("reads null once the learned controller is gone, and can be cancelled", () => {
    let pads: PadLike[] = [pad(0, [], [0])];
    const r = new GamepadReader(() => pads);
    r.learn();
    pads[0] = pad(0, [], [1]);
    expect(r.analog()).toBe(1);
    pads = [];
    expect(r.analog()).toBeNull();
    r.learn();
    r.cancelLearning();
    expect(r.learning).toBe(false);
  });
});
