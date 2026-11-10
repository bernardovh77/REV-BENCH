// The add-on's module-level seam (stage 124): loading it, its versions, the
// presets, parameter and telemetry metadata, and the device list. Everything
// the engine process asks for before it has a host.
import { test } from "node:test";
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { addon, throwsCode } from "./helpers.mjs";

const QUALITIES = ["low", "medium", "high", "ultra", "balanced"];

test("the module exports exactly the Addon of addon.ts", () => {
  assert.deepEqual(Object.keys(addon).sort(), ["createHost", "listDevices", "params", "presets", "telemetry", "version"]);
  for (const name of Object.keys(addon)) assert.equal(typeof addon[name], "function", name);
});

test("version() names the core's version, snapshot version 2 and a Node-API version", () => {
  const v = addon.version();
  assert.deepEqual(Object.keys(v).sort(), ["addon", "nodeApi", "snapshot"]);
  // the core's version is in its header, an independent source
  const header = readFileSync(fileURLToPath(new URL("../../../core/include/rb.h", import.meta.url)), "utf8");
  const core = /#define RB_VERSION_STRING "([^"]+)"/.exec(header)[1];
  assert.equal(v.addon, core);
  // contracts/snapshot-v2.md: version 2
  assert.equal(v.snapshot, 2);
  assert.ok(Number.isInteger(v.nodeApi) && v.nodeApi >= 8, `Node-API version ${v.nodeApi}`);
});

test("presets() lists the core's presets with a real-time quality each", () => {
  const presets = addon.presets();
  assert.ok(presets.length >= 5, `${presets.length} presets`);
  presets.forEach((p, i) => {
    assert.deepEqual(Object.keys(p).sort(), ["index", "key", "name", "quality"]);
    assert.equal(p.index, i);
    assert.match(p.key, /^[a-z0-9][a-z0-9-]*$/);
    assert.ok(p.name.length > 3, `name of ${p.key}`);
    assert.ok(QUALITIES.includes(p.quality), `${p.key}'s quality ${p.quality}`);
  });
  assert.equal(new Set(presets.map((p) => p.key)).size, presets.length, "keys are unique");
  const keys = presets.map((p) => p.key);
  for (const key of ["i4", "i6", "v8-cross"]) assert.ok(keys.includes(key), `preset ${key}`);
  assert.equal(presets.find((p) => p.key === "i4").name, "2.0 L inline-4");
});

test("params() carries the core's parameter table, with its units, ranges and flags", () => {
  const params = addon.params();
  params.forEach((p, i) => {
    assert.deepEqual(Object.keys(p).sort(), ["default", "flags", "id", "max", "min", "name", "unit"]);
    assert.equal(p.id, i, "ids run from 0 without gaps");
    assert.ok(p.min <= p.default && p.default <= p.max, `${p.name}'s default is in its range`);
    assert.ok(Array.isArray(p.flags));
    for (const flag of p.flags) assert.ok(["control", "rebuild", "integer"].includes(flag), flag);
  });
  assert.equal(new Set(params.map((p) => p.name)).size, params.length, "names are unique");
  const byName = Object.fromEntries(params.map((p) => [p.name, p]));
  // rows of core/src/rb_params.c, the independent source of these literals
  assert.deepEqual(byName.master_gain, { id: 0, name: "master_gain", unit: "", min: 0, max: 4, default: 1, flags: [] });
  assert.deepEqual(byName.ignition.flags, ["control", "integer"]);
  assert.deepEqual(byName.throttle.flags, ["control"]);
  assert.deepEqual(byName.seed.flags, ["rebuild", "integer"]);
  assert.deepEqual(byName.source.flags, ["integer"]);
  assert.equal(byName.load_torque.unit, "N m");
  assert.equal(byName.load_torque.min, -500);
  assert.equal(byName.load_torque.max, 5000);
  assert.equal(byName.hold.default, 1);
  assert.equal(byName.overrun_transition.default, 0.15);
});

test("telemetry() lists the core's ids; the instruments from rpm on are the snapshot's", () => {
  const telemetry = addon.telemetry();
  telemetry.forEach((t, i) => {
    assert.deepEqual(Object.keys(t).sort(), ["id", "name", "unit"]);
    assert.equal(t.id, i);
    assert.ok(t.name.length > 0);
    assert.equal(typeof t.unit, "string");
  });
  const rpm = telemetry.findIndex((t) => t.name === "rpm");
  assert.ok(rpm > 0, "rpm is among them");
  assert.deepEqual(telemetry[rpm], { id: rpm, name: "rpm", unit: "rpm" });
  // contracts/snapshot-v2.md: 24 instruments from rpm on, in this order
  const instruments = telemetry.slice(rpm).map((t) => t.name);
  assert.equal(instruments.length, 24);
  assert.deepEqual(instruments.slice(0, 4), ["rpm", "rpm_smooth", "cycle_rpm", "map"]);
  assert.equal(instruments[9], "throttle");
  assert.equal(instruments[23], "tailpipe_temperature");
});

test("listDevices('null') finds the null backend's one device", () => {
  const devices = addon.listDevices("null");
  assert.equal(devices.length, 1);
  const [d] = devices;
  assert.deepEqual(Object.keys(d).sort(), ["index", "isDefault", "name", "nativeRate", "rate"]);
  assert.equal(d.index, 0);
  assert.equal(d.isDefault, true);
  assert.ok(d.name.length > 0);
  assert.ok(d.rate === 44100 || d.rate === 48000, `rate ${d.rate}`);
  assert.ok(d.nativeRate === null || Number.isInteger(d.nativeRate));
});

test("listDevices('default') lists without playing, or says why it can't", () => {
  // a machine without a sound card (CI) may have none or fail: both are answers
  let devices;
  try {
    devices = addon.listDevices("default");
  } catch (error) {
    assert.equal(typeof error.code, "string");
    assert.match(error.code, /^RB_HOST_ERR_/);
    return;
  }
  assert.ok(Array.isArray(devices));
  for (const d of devices) {
    assert.ok(d.rate === 44100 || d.rate === 48000);
    assert.equal(typeof d.isDefault, "boolean");
  }
});

test("listDevices refuses a missing, mistyped or unknown backend with RB_HOST_ERR_ARG", () => {
  throwsCode(() => addon.listDevices(), "RB_HOST_ERR_ARG", "no argument");
  throwsCode(() => addon.listDevices(0), "RB_HOST_ERR_ARG", "a number");
  throwsCode(() => addon.listDevices("speakers"), "RB_HOST_ERR_ARG", "an unknown name");
  throwsCode(() => addon.listDevices("manual"), "RB_HOST_ERR_ARG", "a backend the add-on doesn't offer");
});
