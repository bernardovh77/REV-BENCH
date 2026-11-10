// The snapshot seam (stage 124): readSnapshot() gives the newest snapshot
// nobody has read as a fresh ArrayBuffer of the core's snapshot-v2 bytes, with
// the host's own numbers beside it, or null when there is none.
import { test } from "node:test";
import assert from "node:assert/strict";
import { cheapConfig, INSTRUMENT, nextSnapshot, P, readSnapshotBytes, SNAPSHOT_MAGIC, sleep, waitFor, withHost } from "./helpers.mjs";

test("there is no snapshot before the host runs, and none twice in a row", async () => {
  await withHost(cheapConfig(), async (host) => {
    assert.equal(host.readSnapshot(), null);
    host.start();
    const first = await nextSnapshot(host);
    assert.ok(first);
    // nobody has published a newer one in the next microsecond (60 a second)
    assert.equal(host.readSnapshot(), null, "the same snapshot is not given twice");
  });
});

test("the bytes are snapshot-v2: magic, version, size and the engine's shape", async () => {
  await withHost(cheapConfig({ preset: "v8-cross" }), async (host) => {
    host.start();
    const snap = await nextSnapshot(host);
    assert.ok(snap.bytes instanceof ArrayBuffer);
    const { header } = readSnapshotBytes(snap.bytes);
    assert.equal(header.magic, SNAPSHOT_MAGIC, "RBS2");
    assert.equal(header.version, 2);
    assert.equal(header.size, snap.bytes.byteLength);
    assert.equal(snap.bytes.byteLength, 304 + 40 * 8, "the V8's 624 bytes");
    assert.equal(header.cylinders, 8);
    assert.equal(header.instruments, 24);
    assert.equal(header.cylinderFields, 5);
    assert.equal(header.audioHz, 48000);
    assert.ok(header.physicsHz > 0);
    assert.ok(header.phaseRad >= 0 && header.phaseRad < 4 * Math.PI);
  });
});

test("the info beside the bytes says which snapshot it is", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const a = await nextSnapshot(host);
    assert.deepEqual(Object.keys(a).sort(), ["audioFrame", "bytes", "generation", "physicsTick", "sequence"]);
    assert.equal(a.generation, 1, "the host's generation, not the core's builds");
    assert.ok(Number.isInteger(a.sequence) && a.sequence >= 1);
    assert.ok(Number.isInteger(a.audioFrame) && a.audioFrame > 0);
    // the engine's tick is in the bytes too
    assert.equal(a.physicsTick, readSnapshotBytes(a.bytes).header.physicsTick);
    const b = await nextSnapshot(host);
    assert.ok(b.sequence > a.sequence, "the sequence rises");
    assert.ok(b.audioFrame > a.audioFrame, "audio frames rise");
    assert.ok(b.physicsTick > a.physicsTick, "physics ticks rise");
  });
});

test("each read is a copy of its own: changing one changes neither the next nor an earlier one", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const a = await nextSnapshot(host);
    const keep = new Uint8Array(a.bytes.slice(0));
    const b = await nextSnapshot(host);
    assert.notStrictEqual(a.bytes, b.bytes);
    new Uint8Array(b.bytes).fill(0xff); // scribble over the second
    const c = await nextSnapshot(host);
    assert.equal(readSnapshotBytes(c.bytes).header.magic, SNAPSHOT_MAGIC, "the third is not the scribbled one");
    assert.deepEqual(new Uint8Array(a.bytes), keep, "the first is as it was");
    // and a copy keeps its bytes however long the host runs on and however much it frees
    await sleep(200);
    assert.deepEqual(new Uint8Array(a.bytes), keep);
    assert.equal(readSnapshotBytes(a.bytes).header.magic, SNAPSHOT_MAGIC);
  });
});

test("the engine's speed rises after the starter turns it", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    const before = readSnapshotBytes((await nextSnapshot(host)).bytes);
    assert.equal(before.instrument(INSTRUMENT.rpm), 0, "at rest until it is driven");
    // a free crank (hold 0) with the ignition on, turned by the starter
    host.setParam(P.hold, 0);
    host.setParam(P.ignition, 1);
    host.setParam(P.starter, 1);
    const turning = await waitFor(() => {
      const s = host.readSnapshot();
      if (!s) return null;
      const read = readSnapshotBytes(s.bytes);
      return read.instrument(INSTRUMENT.rpm) > 150 ? read : null;
    }, 15000, "the starter to spin the engine over 150 rpm");
    assert.equal(turning.instrument(INSTRUMENT.starter), 1);
    assert.equal(turning.instrument(INSTRUMENT.ignition), 1);
    // the header's speed in rad/s is the same speed
    const rpm = turning.instrument(INSTRUMENT.rpm);
    assert.ok(Math.abs(turning.header.speedRadS * 60 / (2 * Math.PI) - rpm) < 0.02 * rpm + 5, "speed_rad_s agrees with rpm");
    assert.ok(turning.header.cycleIndex >= 0);
  });
});

test("a snapshot is whole: the crank's phase and tick go forward together", async () => {
  await withHost(cheapConfig(), async (host) => {
    host.start();
    let last = null;
    for (let i = 0; i < 10; i++) {
      const snap = await nextSnapshot(host);
      const { header } = readSnapshotBytes(snap.bytes);
      assert.equal(header.size, snap.bytes.byteLength, "never torn: the size in the header is the buffer's");
      if (last) assert.ok(header.physicsTick > last.physicsTick);
      last = header;
    }
  });
});
