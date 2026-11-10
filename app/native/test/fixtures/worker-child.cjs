// Run by lifecycle.test.mjs as a worker thread: loads the add-on in its own
// environment, makes a host, runs it and destroys it, and says what it saw.
"use strict";
const { parentPort, workerData } = require("node:worker_threads");
const addon = require(workerData.addon);

const host = addon.createHost({ backend: "null", initial: { preset: "i6", quality: "low" } });
host.start();
const waitForSnapshot = () => {
  const snap = host.readSnapshot();
  if (!snap) return setTimeout(waitForSnapshot, 5);
  const cylinders = new DataView(snap.bytes).getUint32(12, true);
  host.destroy();
  parentPort.postMessage({ ok: true, cylinders, presets: addon.presets().length });
};
waitForSnapshot();
