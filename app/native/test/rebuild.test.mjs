// The rebuild seam (stage 124): prepare builds an engine off the render
// thread, prepareState says how it is getting on, swap hands it over (ack with
// a new generation), discard throws it away; a build that fails leaves the
// engine that plays untouched.
import { test } from "node:test";
import assert from "node:assert/strict";
import { acks, cheapConfig, FIELDS, INSTRUMENT, nextSnapshot, P, readSnapshotBytes, sleep, throwsCode, waitFor, withHost } from "./helpers.mjs";

/** Waits for a build to leave "pending" and says where it ended. */
async function built(host, prepareId) {
  return waitFor(() => {
    const status = host.prepareState(prepareId);
    return status.state === "pending" ? null : status;
  }, 15000, `build ${prepareId} to finish`);
}

test("prepare returns an id; the build goes pending to ready", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const id = host.prepare({ preset: "i6", quality: "low" });
    assert.ok(Number.isInteger(id) && id > 0);
    const first = host.prepareState(id);
    assert.deepEqual(Object.keys(first).sort(), ["code", "message", "state"]);
    assert.ok(["pending", "ready"].includes(first.state));
    const status = await built(host, id);
    assert.deepEqual(status, { state: "ready", code: 0, message: "" });
    assert.notEqual(host.prepare({ preset: "i4", quality: "low" }), id, "ids are not reused");
  });
});

test("swap hands the engine over: an ack with a new generation and reset, new bytes after it", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const old = await nextSnapshot(host);
    assert.equal(readSnapshotBytes(old.bytes).header.cylinders, 4);
    const id = host.prepare({ preset: "i6", quality: "low" });
    assert.equal((await built(host, id)).state, "ready");
    const request = host.swap(id);
    assert.ok(Number.isInteger(request) && request > 0);
    const [ack] = await acks(host, 1);
    assert.equal(ack.requestId, request);
    assert.equal(ack.kind, "swap");
    assert.equal(ack.outcome, "ok");
    assert.equal(ack.code, 0);
    assert.equal(ack.generation, 2);
    assert.equal(ack.reset, true, "a new engine started");
    assert.equal(host.generation(), 2);
    const fresh = await waitFor(() => {
      const s = host.readSnapshot();
      return s && s.generation === 2 ? s : null;
    }, 5000, "a snapshot of the new engine");
    assert.equal(readSnapshotBytes(fresh.bytes).header.cylinders, 6, "the six is playing");
    assert.equal(fresh.bytes.byteLength, 304 + 40 * 6);
    // the build is spent, and a command now reaches the new engine as generation 2
    throwsCode(() => host.prepareState(id), "RB_HOST_ERR_ARG", "a swapped build is gone");
    const next = host.setParam(P.throttle, 0.3, 2);
    const [after] = await acks(host, 1);
    assert.equal(after.requestId, next);
    assert.equal(after.outcome, "ok");
    assert.equal(after.generation, 2);
  });
});

test("the rebuild resets the driver's controls, as every rebuild does", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    host.setParam(P.throttle, 0.7);
    await acks(host, 1);
    const id = host.prepare({ preset: "i4", quality: "low" });
    await built(host, id);
    host.swap(id);
    await acks(host, 1);
    const fresh = await waitFor(() => {
      const s = host.readSnapshot();
      return s && s.generation === 2 ? s : null;
    }, 5000, "the new engine's snapshot");
    assert.equal(readSnapshotBytes(fresh.bytes).instrument(INSTRUMENT.throttle), 0);
  });
});

test("a build with edits and parameters is the engine described", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const plain = readSnapshotBytes((await nextSnapshot(host)).bytes).header.buildId;
    const id = host.prepare({
      preset: "i4",
      quality: "low",
      edits: [{ field: FIELDS.stroke, index: 0, value: 0.07 }],
      params: [{ id: P.throttle, value: 0.4 }],
    });
    assert.equal((await built(host, id)).state, "ready");
    host.swap(id);
    await acks(host, 1);
    const fresh = await waitFor(() => {
      const s = host.readSnapshot();
      return s && s.generation === 2 ? s : null;
    }, 5000, "the new engine's snapshot");
    const read = readSnapshotBytes(fresh.bytes);
    assert.notEqual(read.header.buildId, plain);
    assert.equal(read.instrument(INSTRUMENT.throttle), 0.4);
  });
});

test("a build that fails is 'failed' with the core's code and why, and the engine plays on untouched", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const before = await nextSnapshot(host);
    const bad = host.prepare({ preset: "i6", quality: "low", edits: [{ field: 9999, index: 0, value: 1 }] });
    const status = await built(host, bad);
    assert.equal(status.state, "failed");
    assert.equal(status.code, -2, "RB_ERR_ID");
    assert.match(status.message, /edit 0/);
    // it cannot be swapped in
    throwsCode(() => host.swap(bad), "RB_HOST_ERR_STATE", "swapping a failed build");
    // the engine is as it was: same generation, still rendering, same engine, commands still work
    assert.equal(host.generation(), 1);
    const later = await waitFor(() => {
      const s = host.readSnapshot();
      return s && s.sequence > before.sequence ? s : null;
    }, 5000, "the engine to go on");
    assert.equal(later.generation, 1);
    assert.equal(readSnapshotBytes(later.bytes).header.buildId, readSnapshotBytes(before.bytes).header.buildId);
    const id = host.setParam(P.throttle, 0.2);
    const [ack] = await acks(host, 1);
    assert.deepEqual([ack.requestId, ack.outcome, ack.generation], [id, "ok", 1]);
    // the failed build is thrown away like any other
    host.discard(bad);
    throwsCode(() => host.prepareState(bad), "RB_HOST_ERR_ARG", "a discarded build is gone");
    assert.equal(host.stats().swapsOk, 0);
  });
});

test("a build with a bad parameter fails the same way", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const bad = host.prepare({ preset: "i6", quality: "low", params: [{ id: 9999, value: 1 }] });
    const status = await built(host, bad);
    assert.equal(status.state, "failed");
    assert.match(status.message, /parameter 1/, "the source is parameter 0, ours the next");
    assert.equal(host.generation(), 1);
  });
});

test("a swap for another generation is 'stale': the old engine stays", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const id = host.prepare({ preset: "i6", quality: "low" });
    await built(host, id);
    const request = host.swap(id, 9);
    const [ack] = await acks(host, 1);
    assert.equal(ack.requestId, request);
    assert.equal(ack.kind, "swap");
    assert.equal(ack.outcome, "stale");
    assert.equal(ack.reset, false);
    assert.equal(ack.generation, 1);
    assert.equal(host.generation(), 1);
    assert.equal(host.stats().swapsOk, 0);
  });
});

test("a swap whose build began before another swap landed is stale by default", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const first = host.prepare({ preset: "i6", quality: "low" });
    const second = host.prepare({ preset: "i4", quality: "low", params: [{ id: P.throttle, value: 0.1 }] });
    await built(host, first);
    await built(host, second);
    host.swap(first);
    host.swap(second); // began at generation 1; generation 2 is active by the time it arrives
    const got = await acks(host, 2);
    assert.deepEqual(got.map((a) => a.outcome), ["ok", "stale"]);
    assert.equal(host.generation(), 2);
  });
});

test("discard throws a ready build away; an unknown one is refused", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const id = host.prepare({ preset: "i6", quality: "low" });
    await built(host, id);
    host.discard(id);
    throwsCode(() => host.prepareState(id), "RB_HOST_ERR_ARG", "gone");
    throwsCode(() => host.swap(id), "RB_HOST_ERR_ARG", "can't swap what was thrown away");
    throwsCode(() => host.discard(id), "RB_HOST_ERR_ARG", "twice");
    throwsCode(() => host.discard(424242), "RB_HOST_ERR_ARG", "never made");
    assert.equal(host.generation(), 1);
  });
});

test("the host holds eight builds at once; the ninth is RB_HOST_ERR_FULL", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const ids = [];
    for (let i = 0; i < 8; i++) ids.push(host.prepare({ preset: "i4", quality: "low" }));
    throwsCode(() => host.prepare({ preset: "i4", quality: "low" }), "RB_HOST_ERR_FULL", "the ninth");
    for (const id of ids) assert.equal((await built(host, id)).state, "ready");
    for (const id of ids) host.discard(id);
    // room again
    const again = host.prepare({ preset: "i4", quality: "low" });
    assert.equal((await built(host, again)).state, "ready");
    host.discard(again);
  });
});

test("a build is made while the engine plays, and the render thread keeps its pace", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    await nextSnapshot(host);
    const underruns = host.stats().underruns;
    const id = host.prepare({ preset: "v8-cross", quality: "low" });
    let reads = 0;
    // while it builds, snapshots keep coming
    await waitFor(() => {
      if (host.readSnapshot()) reads++;
      return host.prepareState(id).state !== "pending" && reads >= 1;
    }, 15000, "the build, with snapshots meanwhile");
    await sleep(50);
    assert.ok(host.stats().underruns - underruns <= 1, "the device was not starved by the build");
    host.discard(id);
  });
});

test("bad arguments to the rebuild calls are RB_HOST_ERR_ARG, an unknown preset RB_ERR_ID", async () => {
  await withHost(cheapConfig(), async (host) => {
    const bad = (fn, why) => throwsCode(fn, "RB_HOST_ERR_ARG", why);
    bad(() => host.prepare(), "nothing");
    bad(() => host.prepare(5), "a number");
    bad(() => host.prepare({}), "no preset");
    bad(() => host.prepare({ preset: 3 }), "a preset index");
    bad(() => host.prepare({ preset: "i4", settleBlocks: "many" }), "settleBlocks");
    throwsCode(() => host.prepare({ preset: "nope" }), "RB_ERR_ID", "an unknown preset");
    throwsCode(() => host.prepare({ preset: "i4", quality: "nope" }), "RB_ERR_ID", "an unknown quality");
    throwsCode(() => host.prepare({ preset: "i4", scene: "nope" }), "RB_ERR_ID", "an unknown scene");
    bad(() => host.prepareState(), "no id");
    bad(() => host.prepareState("1"), "a string id");
    bad(() => host.prepareState(-1), "a negative id");
    bad(() => host.prepareState(999), "an unknown id");
    bad(() => host.swap(), "no id");
    bad(() => host.swap(999), "an unknown id");
    bad(() => host.swap(1, "now"), "a string generation");
    bad(() => host.discard(), "no id");
    // a refused prepare started nothing
    assert.equal(host.stats().blocks, 0);
  });
});
