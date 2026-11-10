// helpers.mjs - what the add-on's tests share (stage 124).
//
// The tests load the built add-on (RB_ADDON names the file; by default
// ../build/Release/rb_native.node or ../build/rb_native.node, where cmake-js
// puts it) and use it on the null backend only: a real thread pulls audio at
// the pace of a real device and plays it nowhere. They look at the add-on
// from outside, as the engine process does: through the Addon and Host of
// app/src/shared/addon.ts, never at its C.
import { createRequire } from "node:module";
import { existsSync, readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import assert from "node:assert/strict";

const here = (relative) => fileURLToPath(new URL(relative, import.meta.url));
const require = createRequire(import.meta.url);

/** Where the built add-on is. */
export function addonPath() {
  if (process.env.RB_ADDON) return process.env.RB_ADDON;
  for (const candidate of ["../build/Release/rb_native.node", "../build/rb_native.node"]) {
    if (existsSync(here(candidate))) return here(candidate);
  }
  throw new Error("no built add-on: build it with `python tools/run_tests.py addon` or set RB_ADDON");
}

export const addon = require(addonPath());

export const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

/** Polls `read` until it gives something truthy; throws `what` after `ms`. */
export async function waitFor(read, ms, what) {
  const until = Date.now() + ms;
  for (;;) {
    const value = await read();
    if (value) return value;
    if (Date.now() > until) throw new Error(`timed out after ${ms} ms waiting for ${what}`);
    await sleep(5);
  }
}

/** The core's parameter ids by name, from the add-on's own metadata. */
export const P = Object.fromEntries(addon.params().map((p) => [p.name, p.id]));

/**
 * The engine fields' ids by name, read from the core's header (the enum
 * rb_engine_field, in order): an independent source of the numbers
 * rb_get_engine and rb_set_engine take, which the add-on only passes on.
 */
export const FIELDS = (() => {
  const text = readFileSync(here("../../../core/include/rb.h"), "utf8");
  const body = text.slice(text.indexOf("typedef enum rb_engine_field {"), text.indexOf("} rb_engine_field;"));
  const names = [...body.matchAll(/^\s*RB_ENG_([A-Z0-9_]+)/gm)].map((m) => m[1].toLowerCase());
  return Object.fromEntries(names.filter((n) => n !== "count").map((n, i) => [n, i]));
})();

/** A config for a host that is cheap to build and run: the four, at low quality. */
export function cheapConfig(initial = {}, rest = {}) {
  return { backend: "null", initial: { preset: "i4", quality: "low", ...initial }, ...rest };
}

/** Makes a host, runs `body(host)` and always destroys the host after. */
export async function withHost(config, body) {
  const host = addon.createHost(config ?? cheapConfig());
  try {
    return await body(host);
  } finally {
    try {
      host.destroy();
    } catch {
      /* already destroyed by the test */
    }
  }
}

/** Polls until `count` acknowledgments have come (or throws). */
export async function acks(host, count, ms = 5000) {
  const got = [];
  await waitFor(() => {
    got.push(...host.poll());
    return got.length >= count;
  }, ms, `${count} acknowledgments (got ${got.length})`);
  return got;
}

/** Waits for a snapshot nobody has read, and returns it. */
export async function nextSnapshot(host, ms = 5000) {
  return waitFor(() => host.readSnapshot(), ms, "a snapshot");
}

/**
 * Reads snapshot-v2 (contracts/snapshot-v2.md): the header's fields, little
 * endian, 64-bit counters as numbers. A minimal reader of the layout in the
 * contract (tools/snapshot_v2.py is its executable check).
 */
export function readSnapshotBytes(bytes) {
  const view = new DataView(bytes);
  const u64 = (at) => Number(view.getBigUint64(at, true));
  const header = {
    magic: view.getUint32(0, true),
    version: view.getUint32(4, true),
    size: view.getUint32(8, true),
    cylinders: view.getUint32(12, true),
    instruments: view.getUint32(16, true),
    cylinderFields: view.getUint32(20, true),
    buildId: view.getBigUint64(32, true),
    generation: u64(40),
    sequence: u64(48),
    physicsTick: u64(56),
    audioFrame: u64(64),
    cycleIndex: u64(72),
    physicsHz: view.getFloat64(80, true),
    audioHz: view.getFloat64(88, true),
    phaseRad: view.getFloat64(96, true),
    speedRadS: view.getFloat64(104, true),
  };
  const instrument = (i) => view.getFloat64(112 + 8 * i, true);
  return { header, instrument };
}

/** The snapshot magic "RBS2" as the contract writes it. */
export const SNAPSHOT_MAGIC = 0x32534252;

/** Asserts that `fn` throws an Error whose string `code` is `code`. */
export function throwsCode(fn, code, message) {
  assert.throws(fn, (error) => {
    assert.ok(error instanceof Error, `${message ?? "the error"} is an Error`);
    assert.equal(typeof error.code, "string", `${message ?? "the error"} has a string code`);
    assert.equal(error.code, code, message);
    return true;
  });
}

/** Instrument indices of the snapshot (the contract's table). */
export const INSTRUMENT = { rpm: 0, rpmSmooth: 1, cycleRpm: 2, map: 3, throttle: 9, ignition: 11, starter: 12 };
