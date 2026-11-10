// The commands seam (stage 124): setParam and setEngineLive send a command and
// return its request id; the render thread's answer comes back through poll()
// as an Ack, in order, with its outcome (ok, stale, refused), the core's code
// and the generation.
import { test } from "node:test";
import assert from "node:assert/strict";
import { acks, cheapConfig, FIELDS, INSTRUMENT, nextSnapshot, P, readSnapshotBytes, sleep, throwsCode, waitFor, withHost } from "./helpers.mjs";

const ACK_KEYS = ["audioFrame", "code", "generation", "kind", "outcome", "physicsTick", "requestId", "reset"];

test("a command returns a request id, and its ack carries it, with where it took effect", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const id = host.setParam(P.throttle, 0.25);
    assert.ok(Number.isInteger(id) && id > 0);
    const [ack] = await acks(host, 1);
    assert.deepEqual(Object.keys(ack).sort(), ACK_KEYS);
    assert.equal(ack.requestId, id);
    assert.equal(ack.kind, "set-param");
    assert.equal(ack.outcome, "ok");
    assert.equal(ack.code, 0);
    assert.equal(ack.generation, 1);
    assert.equal(ack.reset, false);
    assert.ok(Number.isInteger(ack.physicsTick) && ack.physicsTick >= 0);
    assert.ok(Number.isInteger(ack.audioFrame) && ack.audioFrame > 0, "applied at the start of a block the host rendered");
    assert.deepEqual(host.poll(), [], "an ack comes once");
  });
});

test("the parameter takes effect in the engine", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    host.setParam(P.throttle, 0.6);
    await acks(host, 1);
    const throttle = await waitFor(async () => {
      const snap = host.readSnapshot();
      return snap && readSnapshotBytes(snap.bytes).instrument(INSTRUMENT.throttle) === 0.6;
    }, 5000, "the snapshot to show the throttle");
    assert.ok(throttle);
  });
});

test("request ids rise by one, and acks come back in the order the commands went", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const sent = [];
    for (let i = 0; i < 20; i++) sent.push(host.setParam(P.throttle, i / 20));
    for (let i = 1; i < sent.length; i++) assert.equal(sent[i], sent[i - 1] + 1);
    const got = await acks(host, 20);
    assert.deepEqual(got.map((a) => a.requestId), sent);
    for (let i = 1; i < got.length; i++) assert.ok(got[i].physicsTick >= got[i - 1].physicsTick, "ticks do not go back");
  });
});

test("commands sent before start() wait for the render thread and are acknowledged after it", async () => {
  await withHost(cheapConfig(), async (host) => {
    const id = host.setParam(P.ignition, 1);
    await sleep(100);
    assert.deepEqual(host.poll(), [], "nothing is applied while the host has not started");
    host.start();
    const [ack] = await acks(host, 1);
    assert.equal(ack.requestId, id);
    assert.equal(ack.outcome, "ok");
  });
});

test("a parameter the core clamps is ok with the core's clamped code", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    host.setParam(P.throttle, 99);
    const [ack] = await acks(host, 1);
    assert.equal(ack.outcome, "ok");
    assert.equal(ack.code, 1, "RB_CLAMPED");
  });
});

test("a command the core refuses is acknowledged 'refused' with its code", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    host.setParam(9999, 1); // no such parameter: RB_ERR_ID
    host.setParam(P.throttle, NaN); // RB_ERR_VALUE
    const got = await acks(host, 2);
    assert.deepEqual(got.map((a) => [a.outcome, a.code]), [["refused", -2], ["refused", -3]]);
    assert.ok(got.every((a) => a.generation === 1 && !a.reset));
    const stats = host.stats();
    assert.equal(stats.commandsRefused, 2);
    assert.equal(stats.commandsApplied, 0);
  });
});

test("a command for another generation is 'stale' and is not applied", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const stale = host.setParam(P.throttle, 0.9, 7); // the engine is generation 1
    const good = host.setParam(P.starter, 1, 1);
    const any = host.setParam(P.ignition, 1, 0); // 0 means any
    const got = await acks(host, 3);
    assert.deepEqual(got.map((a) => [a.requestId, a.outcome]), [[stale, "stale"], [good, "ok"], [any, "ok"]]);
    assert.equal(got[0].generation, 1, "the ack says the generation that IS active");
    await sleep(100);
    const header = readSnapshotBytes((await nextSnapshot(host)).bytes);
    assert.equal(header.instrument(INSTRUMENT.throttle), 0, "a stale command changed nothing");
    assert.equal(header.instrument(INSTRUMENT.starter), 1);
  });
});

test("setEngineLive changes a live field and the core refuses one that is not live", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const live = host.setEngineLive(FIELDS.idle_rpm, 0, 900);
    const notLive = host.setEngineLive(FIELDS.bore, 0, 0.09);
    const stale = host.setEngineLive(FIELDS.idle_rpm, 0, 800, 5);
    const got = await acks(host, 3);
    assert.deepEqual(got.map((a) => a.requestId), [live, notLive, stale]);
    assert.ok(got.every((a) => a.kind === "set-engine-live"));
    assert.deepEqual(got.map((a) => a.outcome), ["ok", "refused", "stale"]);
    assert.equal(got[1].code, -3, "RB_ERR_VALUE: a field that would rebuild is a swap's job");
  });
});

test("a full command queue refuses with RB_HOST_ERR_FULL, sends nothing, and the rest are acknowledged", async () => {
  await withHost(cheapConfig(), async (host) => {
    // the host has not started, so nothing drains the queue (capacity 256)
    const ids = [];
    let failure = null;
    for (let i = 0; i < 400 && !failure; i++) {
      try {
        ids.push(host.setParam(P.throttle, 0.1));
      } catch (error) {
        failure = error;
      }
    }
    assert.ok(failure, "the queue filled");
    assert.equal(failure.code, "RB_HOST_ERR_FULL");
    assert.ok(ids.length >= 200 && ids.length <= 256, `${ids.length} fit`);
    host.start();
    const got = await acks(host, ids.length);
    assert.deepEqual(got.map((a) => a.requestId), ids, "every command that was accepted is answered");
    // the refused one took no id: the next command continues the numbering
    const next = host.setParam(P.throttle, 0.2);
    assert.equal(next, ids.at(-1) + 1);
  });
});

test("bad arguments to commands are RB_HOST_ERR_ARG", async () => {
  await withHost(cheapConfig(), async (host) => {
    const bad = (fn, why) => throwsCode(fn, "RB_HOST_ERR_ARG", why);
    bad(() => host.setParam(), "nothing");
    bad(() => host.setParam(P.throttle), "no value");
    bad(() => host.setParam("throttle", 1), "a name for an id");
    bad(() => host.setParam(1.5, 1), "a fractional id");
    bad(() => host.setParam(-1, 1), "a negative id");
    bad(() => host.setParam(P.throttle, "1"), "a string value");
    bad(() => host.setParam(P.throttle, 1, -1), "a negative generation");
    bad(() => host.setParam(P.throttle, 1, 1.5), "a fractional generation");
    bad(() => host.setParam(P.throttle, 1, "1"), "a string generation");
    bad(() => host.setEngineLive(FIELDS.idle_rpm, 0), "no value");
    bad(() => host.setEngineLive(FIELDS.idle_rpm, "0", 900), "a string index");
    bad(() => host.setEngineLive(-1, 0, 900), "a negative field");
    // nothing was queued by any of them
    host.start();
    await sleep(150);
    assert.deepEqual(host.poll(), []);
  });
});

test("the stats count what the render thread did with the commands", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    host.setParam(P.ignition, 1);
    host.setParam(P.starter, 1);
    host.setParam(9999, 0);
    await acks(host, 3);
    const stats = host.stats();
    assert.equal(stats.commandsApplied, 2);
    assert.equal(stats.commandsRefused, 1);
  });
});
