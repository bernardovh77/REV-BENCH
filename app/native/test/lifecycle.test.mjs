// The Host's lifecycle seam (stage 124): start, stop, the stats, devices and
// power, destroy and what comes after it, and the host's end when nobody
// destroyed it (garbage collection, a process that ends, a worker).
import { test } from "node:test";
import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import { Worker } from "node:worker_threads";
import { acks, addon, addonPath, cheapConfig, nextSnapshot, P, sleep, throwsCode, waitFor, withHost } from "./helpers.mjs";

const fixture = (name) => fileURLToPath(new URL(`./fixtures/${name}`, import.meta.url));
const childEnv = () => ({ ...process.env, RB_ADDON: addonPath() });

test("start() twice is RB_HOST_ERR_STATE; stop() then start() runs again", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    throwsCode(() => host.start(), "RB_HOST_ERR_STATE", "a second start");
    const a = await nextSnapshot(host);
    host.stop();
    host.stop(); // stopping a stopped host is fine
    host.start();
    const b = await waitFor(() => {
      const s = host.readSnapshot();
      return s && s.sequence > a.sequence ? s : null;
    }, 5000, "the restarted engine's snapshot");
    assert.equal(b.generation, 1, "the engine stays as it was");
  });
});

test("stats() before and while the engine runs", async () => {
  await withHost(cheapConfig({}, { targetFillMs: 20 }), async (host) => {
    const idle = host.stats();
    assert.equal(idle.blocks, 0);
    assert.equal(idle.underruns, 0);
    assert.equal(idle.swapsOk, 0);
    host.start();
    await waitFor(() => host.stats().blocks >= 100, 10000, "100 blocks");
    await sleep(300);
    const s = host.stats();
    assert.deepEqual(Object.keys(s).sort(), [
      "blocks", "boosted", "commandsApplied", "commandsRefused", "deadline", "eventsLost", "forbiddenCalls",
      "priority", "processMb", "renderMsMax", "renderMsMean", "ringFillMs", "ringTargetMs", "swapsOk", "underruns",
    ]);
    assert.ok(s.blocks >= 100);
    assert.ok(s.renderMsMean > 0 && s.renderMsMean <= s.renderMsMax, `render ${s.renderMsMean}/${s.renderMsMax} ms`);
    assert.ok(s.renderMsMax < 128 / 48000 * 1000 * 5, "no block took five block-times");
    assert.equal(s.deadline.length, 12, "ten tenths, one to two, two or more");
    assert.ok(Math.abs(s.deadline.reduce((a, b) => a + b, 0) - 1) < 1e-9, "shares of the blocks");
    assert.ok(s.deadline.every((x) => x >= 0 && x <= 1));
    assert.ok(s.deadline[10] + s.deadline[11] < 0.05, "a block of 128 frames at low quality meets its deadline");
    assert.equal(Math.round(s.ringTargetMs), 20);
    assert.ok(s.ringFillMs > 5 && s.ringFillMs < 80, `ring fill ${s.ringFillMs} ms`);
    assert.equal(typeof s.priority, "string");
    assert.ok(s.priority.length > 0);
    assert.equal(typeof s.boosted, "boolean");
    assert.equal(s.forbiddenCalls, 0, "the render thread allocates, locks and logs nothing");
    assert.ok(s.processMb > 1);
    assert.equal(s.eventsLost, 0);
  });
});

test("a swap is counted in the stats", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const id = host.prepare({ preset: "i6", quality: "low" });
    await waitFor(() => host.prepareState(id).state === "ready", 15000, "the build");
    host.swap(id);
    await acks(host, 1);
    assert.equal(host.stats().swapsOk, 1);
  });
});

test("selectDevice moves the sound to another device and the engine plays on", async () => {
  await withHost(cheapConfig(), async (host) => {
    throwsCode(() => host.selectDevice(0), "RB_HOST_ERR_STATE", "before start()");
    host.start();
    const a = await nextSnapshot(host);
    host.selectDevice(0);
    host.selectDevice(-1);
    const b = await waitFor(() => {
      const s = host.readSnapshot();
      return s && s.sequence > a.sequence ? s : null;
    }, 5000, "snapshots after the device change");
    assert.equal(b.generation, 1);
    throwsCode(() => host.selectDevice(40), "RB_HOST_ERR_DEVICE", "a device that isn't there");
    await nextSnapshot(host); // and the failed change left it playing
    throwsCode(() => host.selectDevice("0"), "RB_HOST_ERR_ARG", "a string");
    throwsCode(() => host.selectDevice(0.5), "RB_HOST_ERR_ARG", "a fraction");
    throwsCode(() => host.selectDevice(), "RB_HOST_ERR_ARG", "nothing");
    // the device change was acknowledged to no one: it is not a command
    assert.deepEqual(host.poll().filter((a) => a.kind === "set-param"), []);
  });
});

test("suspend() stops the device and resume() restarts it in a new epoch", async () => {
  await withHost(cheapConfig(), async (host) => {
    throwsCode(() => host.suspend(), "RB_HOST_ERR_STATE", "before start()");
    throwsCode(() => host.resume(), "RB_HOST_ERR_STATE", "before start()");
    host.start();
    throwsCode(() => host.resume(), "RB_HOST_ERR_STATE", "when not suspended");
    const a = await nextSnapshot(host);
    host.suspend();
    host.suspend(); // already suspended: fine
    host.resume();
    const b = await waitFor(() => {
      const s = host.readSnapshot();
      return s && s.sequence > a.sequence ? s : null;
    }, 5000, "snapshots after waking");
    assert.equal(b.generation, 1);
    // a command still reaches the engine after the sleep
    const id = host.setParam(P.throttle, 0.1);
    const got = await acks(host, 1);
    assert.equal(got.find((x) => x.requestId === id).outcome, "ok");
  });
});

test("after destroy() every call throws DESTROYED, destroy() itself included", async () => {
  const host = addon.createHost(cheapConfig());
  host.start();
  await nextSnapshot(host);
  host.destroy();
  const calls = {
    start: [], stop: [], destroy: [], sampleRate: [], generation: [], setParam: [P.throttle, 0.5],
    setEngineLive: [0, 0, 1], prepare: [{ preset: "i4" }], prepareState: [1], swap: [1], discard: [1],
    poll: [], readSnapshot: [], stats: [], selectDevice: [0], suspend: [], resume: [],
  };
  for (const [name, args] of Object.entries(calls)) {
    throwsCode(() => host[name](...args), "DESTROYED", `${name}() after destroy`);
  }
  // and bad arguments after destroy are still DESTROYED: the handle is checked first
  throwsCode(() => host.setParam(), "DESTROYED", "no arguments");
});

test("every Host method on the prototype is covered by the destroyed check", () => {
  const host = addon.createHost(cheapConfig());
  const methods = Object.getOwnPropertyNames(Object.getPrototypeOf(host)).filter((n) => n !== "constructor");
  host.destroy();
  assert.deepEqual(methods.sort(), [
    "destroy", "discard", "generation", "poll", "prepare", "prepareState", "readSnapshot", "sampleRate",
    "selectDevice", "setEngineLive", "setParam", "start", "stats", "stop", "suspend", "swap", "resume",
  ].sort());
  for (const name of methods) throwsCode(() => host[name](), "DESTROYED", name);
});

test("destroy() of a host that never started, and of one that runs, both work", async () => {
  addon.createHost(cheapConfig()).destroy();
  const running = addon.createHost(cheapConfig());
  running.start();
  running.setParam(P.ignition, 1);
  const id = running.prepare({ preset: "i6", quality: "low" }); // a build in flight
  assert.ok(id > 0);
  running.destroy();
  // a destroyed host is gone for good, but the add-on goes on
  const another = addon.createHost(cheapConfig());
  assert.equal(another.generation(), 1);
  another.destroy();
});

test("a method called on something that is not a Host is refused", () => {
  const host = addon.createHost(cheapConfig());
  try {
    const proto = Object.getPrototypeOf(host);
    // V8 itself refuses a receiver that is not an instance of the class ("Illegal invocation")
    assert.throws(() => proto.sampleRate.call({}), TypeError, "a plain object");
    assert.throws(() => proto.sampleRate.call(undefined), TypeError, "undefined");
    assert.throws(() => proto.sampleRate.call(proto), TypeError, "the prototype");
    // an instance the class made without createHost carries no host: the add-on refuses it
    throwsCode(() => new host.constructor().stats(), "RB_HOST_ERR_ARG", "an object made by the class itself");
    assert.equal(host.sampleRate(), 48000, "the real one is untouched");
  } finally {
    host.destroy();
  }
});

test("hosts nobody destroyed are freed by the garbage collector without a crash", () => {
  const run = spawnSync(process.execPath, ["--expose-gc", fixture("gc-child.cjs")], {
    env: childEnv(), encoding: "utf8", timeout: 120000,
  });
  assert.equal(run.status, 0, `exit ${run.status}\n${run.stdout}\n${run.stderr}`);
  const report = JSON.parse(run.stdout.trim().split("\n").at(-1));
  assert.equal(report.made, 30);
  assert.ok(report.afterGcMb < report.peakMb - 0.4 * (report.peakMb - report.baseMb), `memory: ${JSON.stringify(report)}`);
});

test("a process that ends with a running host exits cleanly", () => {
  for (const mode of ["fall-off-the-end", "process-exit"]) {
    const run = spawnSync(process.execPath, [fixture("exit-child.cjs"), mode], {
      env: childEnv(), encoding: "utf8", timeout: 60000,
    });
    assert.equal(run.status, 0, `${mode}: exit ${run.status} ${run.signal}\n${run.stdout}\n${run.stderr}`);
    assert.match(run.stdout, /running/);
  }
});

test("the add-on loads in a worker thread, and a host there lives and dies on its own", async () => {
  const worker = new Worker(fixture("worker-child.cjs"), { workerData: { addon: addonPath() } });
  const message = await new Promise((resolve, reject) => {
    worker.once("message", resolve);
    worker.once("error", reject);
    worker.once("exit", (code) => reject(new Error(`the worker exited (${code}) without answering`)));
  });
  assert.deepEqual(message, { ok: true, cylinders: 6, presets: addon.presets().length });
  await worker.terminate();
  // and the main thread's add-on is fine
  await withHost(cheapConfig(), (host) => assert.equal(host.generation(), 1));
});
