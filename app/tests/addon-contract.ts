/*
 * The add-on contract (src/shared/addon.ts) as a test, for any implementation: the fake runs it
 * now (fake-addon.test.ts) and the real rb_native.node runs it in stage 124's second phase.
 * It uses only what the contract promises, from outside.
 */
import { describe, expect, it } from "vitest";
import type { Addon, Host } from "../src/shared/addon";
import { decodeSnapshot } from "../src/shared/snapshot-v2.generated";

export interface ContractEnv {
  /** an add-on, new each time */
  make(): Addon;
  /** lets `ms` of the host's time pass: the fake steps its clock, the real one sleeps */
  pass(ms: number): Promise<void> | void;
}

export function describeAddonContract(label: string, env: ContractEnv): void {
  const hostOn = (addon: Addon, preset = "i4"): Host =>
    addon.createHost({ backend: "null", sampleRate: 48000, initial: { preset } });

  /** polls until the host says the preparation is no longer pending, up to 5 s of its time */
  async function prepared(host: Host, id: number) {
    for (let waited = 0; waited <= 5000; waited += 10) {
      const state = host.prepareState(id);
      if (state.state !== "pending") return state;
      await env.pass(10);
    }
    throw new Error("a preparation stayed pending");
  }

  describe(`the add-on contract: ${label}`, () => {
    it("says its versions", () => {
      const v = env.make().version();
      expect(v.addon).toMatch(/\S/);
      expect(v.snapshot).toBe(2);
      expect(v.nodeApi).toBeGreaterThan(0);
    });

    it("lists the presets, with a key, a name and a quality each", () => {
      const presets = env.make().presets();
      expect(presets.length).toBeGreaterThan(0);
      expect(presets.map((p) => p.key)).toContain("i4");
      expect(new Set(presets.map((p) => p.key)).size).toBe(presets.length);
      for (const p of presets) {
        expect(p.name).toMatch(/\S/);
        expect(["low", "medium", "high", "ultra", "balanced"]).toContain(p.quality);
      }
    });

    it("describes the parameters: unique names and ids, a default inside its range", () => {
      const params = env.make().params();
      expect(new Set(params.map((p) => p.name)).size).toBe(params.length);
      expect(new Set(params.map((p) => p.id)).size).toBe(params.length);
      for (const p of params) {
        expect(p.min).toBeLessThanOrEqual(p.default);
        expect(p.default).toBeLessThanOrEqual(p.max);
      }
      const names = params.map((p) => p.name);
      for (const needed of ["ignition", "starter", "throttle"]) expect(names).toContain(needed);
      expect(params.find((p) => p.name === "throttle")?.flags).toContain("control");
    });

    it("describes the telemetry, the rpm among it", () => {
      const names = env.make().telemetry().map((t) => t.name);
      expect(names).toContain("rpm");
    });

    it("lists the null backend's device as the default", () => {
      const devices = env.make().listDevices("null");
      expect(devices.length).toBeGreaterThan(0);
      expect(devices.some((d) => d.isDefault)).toBe(true);
      for (const d of devices) expect([44100, 48000]).toContain(d.rate);
    });

    it("makes a host on the null backend at the rate asked for", () => {
      const host = hostOn(env.make());
      expect(host.sampleRate()).toBe(48000);
      expect(host.generation()).toBeGreaterThanOrEqual(1);
      host.destroy();
    });

    it("refuses a host for a preset that doesn't exist, with an error that has a code", () => {
      const addon = env.make();
      let caught: unknown;
      try {
        hostOn(addon, "no-such-engine");
      } catch (e) {
        caught = e;
      }
      expect(caught).toBeInstanceOf(Error);
      expect((caught as { code: string }).code).toMatch(/\S/);
    });

    it("acknowledges a command by its request id, through poll()", async () => {
      const addon = env.make();
      const host = hostOn(addon);
      host.start();
      const throttle = addon.params().find((p) => p.name === "throttle")!;
      const id = host.setParam(throttle.id, 0.5);
      let ack;
      for (let i = 0; i < 500 && !ack; i++) {
        ack = host.poll().find((a) => a.requestId === id);
        if (!ack) await env.pass(10);
      }
      expect(ack).toBeDefined();
      expect(ack!.kind).toBe("set-param");
      expect(ack!.outcome).toBe("ok");
      expect(ack!.generation).toBe(host.generation());
      host.destroy();
    });

    it("refuses an id that is not a whole number by throwing, and a command the core can't take by acknowledging it refused", async () => {
      const host = hostOn(env.make());
      host.start();
      expect(() => host.setParam(-1, 1)).toThrow();
      expect(() => host.setParam(1.5, 1)).toThrow();
      const unknown = host.setParam(9999, 1);
      const notANumber = host.setParam(0, Number.NaN);
      const acks = new Map<number, string>();
      for (let i = 0; i < 500 && acks.size < 2; i++) {
        for (const a of host.poll()) acks.set(a.requestId, a.outcome);
        if (acks.size < 2) await env.pass(10);
      }
      expect(acks.get(unknown)).toBe("refused");
      expect(acks.get(notANumber)).toBe("refused");
      host.destroy();
    });

    it("acknowledges a command for another generation as stale", async () => {
      const addon = env.make();
      const host = hostOn(addon);
      host.start();
      const throttle = addon.params().find((p) => p.name === "throttle")!;
      const id = host.setParam(throttle.id, 0.1, host.generation() + 7);
      let ack;
      for (let i = 0; i < 500 && !ack; i++) {
        ack = host.poll().find((a) => a.requestId === id);
        if (!ack) await env.pass(10);
      }
      expect(ack?.outcome).toBe("stale");
      host.destroy();
    });

    it("rebuilds: prepare, wait, swap; the ack says a new engine started", async () => {
      const addon = env.make();
      const host = hostOn(addon);
      host.start();
      const before = host.generation();
      const prep = host.prepare({ preset: "i6" });
      const state = await prepared(host, prep);
      expect(state.state).toBe("ready");
      const id = host.swap(prep);
      let ack;
      for (let i = 0; i < 500 && !ack; i++) {
        ack = host.poll().find((a) => a.requestId === id);
        if (!ack) await env.pass(10);
      }
      expect(ack?.kind).toBe("swap");
      expect(ack?.outcome).toBe("ok");
      expect(ack?.reset).toBe(true);
      expect(ack!.generation).toBe(before + 1);
      expect(host.generation()).toBe(before + 1);
      host.destroy();
    });

    it("refuses to prepare a preset that doesn't exist", () => {
      const host = hostOn(env.make());
      expect(() => host.prepare({ preset: "no-such-engine" })).toThrow();
      host.destroy();
    });

    it("gives snapshots in the contract's layout, each newer than the last", async () => {
      const host = hostOn(env.make());
      host.start();
      let first = null;
      for (let i = 0; i < 500 && !first; i++) {
        first = host.readSnapshot();
        if (!first) await env.pass(10);
      }
      expect(first).not.toBeNull();
      const decoded = decodeSnapshot(first!.bytes);
      // the info beside the bytes is the host's, with its own counts: its generation starts at 1 for
      // the first engine, the core's (in the bytes) counts every build since its sim was made, and
      // its sequence counts what the host published (the core's counts every snapshot it took)
      expect(first!.generation).toBeGreaterThanOrEqual(1);
      expect(decoded.generation).toBeGreaterThanOrEqual(first!.generation);
      expect(first!.bytes.byteLength).toBe(decoded.size);
      let next = null;
      for (let i = 0; i < 500 && !next; i++) {
        next = host.readSnapshot();
        if (!next) await env.pass(10);
      }
      expect(next!.sequence).toBeGreaterThan(first!.sequence);
      expect(decodeSnapshot(next!.bytes).sequence).toBeGreaterThan(decoded.sequence);
      host.destroy();
    });

    it("selects only devices that exist", () => {
      const addon = env.make();
      const host = hostOn(addon);
      host.start(); // a device can only be selected on a running host
      host.selectDevice(addon.listDevices("null")[0].index);
      expect(() => host.selectDevice(9999)).toThrow();
      host.destroy();
    });

    it("keeps statistics, with no forbidden calls on the render thread", () => {
      const host = hostOn(env.make());
      host.start();
      const stats = host.stats();
      expect(stats.forbiddenCalls).toBe(0);
      expect(stats.underruns).toBeGreaterThanOrEqual(0);
      host.destroy();
    });

    it("throws DESTROYED for any call after destroy", () => {
      const host = hostOn(env.make());
      host.destroy();
      for (const call of [() => host.start(), () => host.poll(), () => host.readSnapshot(), () => host.stats()]) {
        expect(call).toThrow();
        try {
          call();
        } catch (e) {
          expect((e as { code: string }).code).toBe("DESTROYED");
        }
      }
    });
  });
}
