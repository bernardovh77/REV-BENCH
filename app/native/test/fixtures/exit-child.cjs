// Run by lifecycle.test.mjs: starts a host and never destroys it; the process
// then ends by falling off the end of its event loop or by process.exit().
// Its exit code must be 0 (no crash from threads still running).
"use strict";
const addon = require(process.env.RB_ADDON);
const mode = process.argv[2];

const host = addon.createHost({ backend: "null", initial: { preset: "i4", quality: "low" } });
host.start();
host.setParam(addon.params().find((p) => p.name === "ignition").id, 1);
setTimeout(() => {
  console.log(`running ${host.stats().blocks > 0}`);
  if (mode === "process-exit") process.exit(0);
  // else: nothing left to do, and the render thread must not keep the process alive or crash it
}, 300);
