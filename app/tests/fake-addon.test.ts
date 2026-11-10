/*
 * The fake add-on: it passes the add-on contract (tests/addon-contract.ts), and the stand-in
 * behaviours the window and the control loop's tests rely on.
 */
import { describe, expect, it } from "vitest";
import { createFakeAddon } from "../src/engine/fake-addon";
import { decodeSnapshot } from "../src/shared/snapshot-v2.generated";
import { describeAddonContract } from "./addon-contract";

/** A clock the test steps by hand. */
function clock() {
  let t = 1000;
  return { now: () => t, step: (ms: number) => void (t += ms) };
}

describeAddonContract("the fake", {
  make: () => {
    // the contract's own waits step this clock, through `pass` below
    current = clock();
    return createFakeAddon({ now: current.now });
  },
  pass: (ms) => current.step(ms),
});
let current = clock();

describe("the fake's stand-ins", () => {
  function setup(options = {}) {
    const c = clock();
    const addon = createFakeAddon({ now: c.now, ...options });
    const host = addon.createHost({ backend: "null", initial: { preset: "i4" } });
    host.start();
    const id = (name: string) => addon.params().find((p) => p.name === name)!.id;
    const read = () => {
      c.step(16);
      return decodeSnapshot(host.readSnapshot()!.bytes);
    };
    return { c, addon, host, id, read };
  }

  it("returns no snapshot when no time has passed since the last", () => {
    const { host, c } = setup();
    c.step(5);
    expect(host.readSnapshot()).not.toBeNull();
    expect(host.readSnapshot()).toBeNull();
  });

  it("idles after ignition and the starter, and speeds up with the throttle", () => {
    const { host, id, read, c } = setup();
    host.setParam(id("ignition"), 1);
    host.setParam(id("starter"), 1);
    for (let i = 0; i < 40; i++) read(); // 0.64 s of cranking
    host.setParam(id("starter"), 0);
    for (let i = 0; i < 120; i++) read();
    const idle = read().instruments.rpm;
    expect(idle).toBeGreaterThan(700);
    expect(idle).toBeLessThan(1000);
    host.setParam(id("throttle"), 1);
    for (let i = 0; i < 120; i++) read();
    expect(read().instruments.rpm).toBeGreaterThan(idle + 3000);
    host.setParam(id("ignition"), 0);
    for (let i = 0; i < 300; i++) read();
    expect(read().instruments.rpm).toBeLessThan(50);
    expect(c.now()).toBeGreaterThan(1000);
  });

  it("clamps a value into the parameter's range", () => {
    const { host, id, read } = setup();
    host.setParam(id("throttle"), 7);
    expect(read().instruments.throttle).toBe(1);
  });

  it("starts a new generation with the controls at their defaults after a swap", async () => {
    const { host, id, read, c } = setup();
    host.setParam(id("ignition"), 1);
    expect(read().instruments.ignition).toBe(1);
    const prep = host.prepare({ preset: "v8-cross" });
    c.step(100);
    host.swap(prep);
    const s = read();
    expect(s.generation).toBe(2);
    expect(s.cylinder).toHaveLength(8);
    expect(s.instruments.ignition).toBe(0);
  });

  it("fails the preparation of a preset it is told to fail, with a message", () => {
    const { host, c } = setup({ failPresets: ["i6"] });
    const prep = host.prepare({ preset: "i6" });
    c.step(100);
    const state = host.prepareState(prep);
    expect(state.state).toBe("failed");
    expect(state.message).toMatch(/i6/);
  });

  it("refuses a swap whose preparation isn't ready", () => {
    const { host } = setup({ prepareMs: 1000 });
    const prep = host.prepare({ preset: "i6" });
    const request = host.swap(prep);
    expect(host.poll().find((a) => a.requestId === request)?.outcome).toBe("refused");
  });

  it("refuses commands when 256 acknowledgments are waiting, like the real queue", () => {
    const { host, id } = setup();
    for (let i = 0; i < 256; i++) host.setParam(id("throttle"), 0.1);
    expect(() => host.setParam(id("throttle"), 0.1)).toThrow(/full/);
    host.poll();
    expect(() => host.setParam(id("throttle"), 0.1)).not.toThrow();
  });
});
