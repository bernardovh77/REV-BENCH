// Run by lifecycle.test.mjs with --expose-gc: makes hosts, starts some,
// destroys none, lets go of all of them and collects; prints a JSON line with
// how many it made and the process's memory at the baseline, the peak and
// after the collection (read through a host that is kept).
"use strict";
const addon = require(process.env.RB_ADDON);
const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

async function main() {
  const keeper = addon.createHost({ backend: "null", initial: { preset: "i4", quality: "low" } });
  const baseMb = keeper.stats().processMb;
  let made = 0;
  (function makeThem() {
    for (let i = 0; i < 30; i++) {
      const host = addon.createHost({ backend: "null", initial: { preset: "i4", quality: "low" } });
      if (i % 3 === 0) host.start(); // a render thread and a device that nobody will stop
      made++;
    }
  })();
  const peakMb = keeper.stats().processMb;
  // the finalizers run after the collection, on a later turn of the loop
  for (let i = 0; i < 40; i++) {
    global.gc();
    await sleep(50);
  }
  const afterGcMb = keeper.stats().processMb;
  keeper.destroy();
  console.log(JSON.stringify({ made, baseMb, peakMb, afterGcMb }));
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
