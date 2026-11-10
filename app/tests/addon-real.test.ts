/*
 * The add-on contract (tests/addon-contract.ts) against the real rb_native.node, on the null
 * backend, from Node. It runs when RB_NATIVE_PATH points at a built add-on (tools/run_tests.py
 * addon builds one into <out>/addon-build, and the app-e2e suite sets the variable); without it
 * the tests are skipped, unless RB_REQUIRE_NATIVE=1 (the app-e2e suite), when a missing add-on
 * is a failure. Loading it inside Electron's utility process is the e2e test's.
 */
import { existsSync } from "node:fs";
import { createRequire } from "node:module";
import { describe, expect, it } from "vitest";
import type { Addon } from "../src/shared/addon";
import { describeAddonContract } from "./addon-contract";
import { createFakeAddon } from "../src/engine/fake-addon";
import { decodeSnapshot } from "../src/shared/snapshot-v2.generated";

const path = process.env.RB_NATIVE_PATH;
const required = process.env.RB_REQUIRE_NATIVE === "1";

if (required) {
  it("has the built add-on it was told to test", () => {
    expect(path, "RB_NATIVE_PATH").toBeTruthy();
    expect(existsSync(path!), String(path)).toBe(true);
  });
}

const load = (): Addon => createRequire(import.meta.url)(path!) as Addon;

describe.runIf(Boolean(path) && existsSync(path ?? ""))("the real add-on", () => {
  describeAddonContract("the real rb_native.node", {
    make: load,
    pass: (ms) => new Promise((resolve) => setTimeout(resolve, ms)),
  });

  it("lists the same presets as the fake (the fake's list is the core's)", () => {
    expect(load().presets().map((p) => [p.key, p.name, p.quality])).toEqual(
      createFakeAddon().presets().map((p) => [p.key, p.name, p.quality]),
    );
  });

  it("knows every parameter the app sends by name", () => {
    const names = load().params().map((p) => p.name);
    for (const name of ["ignition", "starter", "throttle", "load_torque", "hold", "clutch"]) {
      expect(names).toContain(name);
    }
  });

  it("describes the telemetry the snapshot carries, in the snapshot's order", () => {
    const telemetry = load().telemetry().map((t) => t.name);
    const snapshot = Object.keys(decodeSnapshotOfNewHost().instruments);
    // the snapshot's instruments are the telemetry from rpm on, in order
    expect(telemetry.slice(telemetry.indexOf("rpm"), telemetry.indexOf("rpm") + snapshot.length)).toEqual(snapshot);
  });
});

function decodeSnapshotOfNewHost() {
  const addon = load();
  const host = addon.createHost({ backend: "null", sampleRate: 48000, initial: { preset: "i4" } });
  try {
    host.start();
    const deadline = Date.now() + 5000;
    for (;;) {
      const read = host.readSnapshot();
      if (read) return decodeSnapshot(read.bytes);
      if (Date.now() > deadline) throw new Error("no snapshot in 5 s");
      Atomics.wait(new Int32Array(new SharedArrayBuffer(4)), 0, 0, 10);
    }
  } finally {
    host.destroy();
  }
}
