// The createHost seam (stage 124): a BuildSpec and a HostConfig become a host
// whose engine is the one asked for - quality, scene, edits, parameters,
// settling, sample rate - and a bad one is refused with a code.
import { test } from "node:test";
import assert from "node:assert/strict";
import { addon, cheapConfig, FIELDS, INSTRUMENT, nextSnapshot, P, readSnapshotBytes, throwsCode, withHost } from "./helpers.mjs";

/** Starts a host, reads its first snapshot and decodes it. */
async function firstSnapshot(config) {
  return withHost(config, async (host) => {
    host.start();
    const snap = await nextSnapshot(host);
    return { snap, ...readSnapshotBytes(snap.bytes) };
  });
}

test("a host is made and not running; it starts with the first engine at generation 1", async () => {
  await withHost(cheapConfig(), async (host) => {
    assert.equal(host.sampleRate(), 48000);
    assert.equal(host.generation(), 1);
    assert.equal(host.readSnapshot(), null, "nothing is rendered before start()");
    assert.equal(host.stats().blocks, 0);
    host.start();
    const snap = await nextSnapshot(host);
    assert.equal(snap.generation, 1);
  });
});

test("each quality name makes the engine at its physics rate", async () => {
  // rb_config_quality at 48 kHz: physics at 2x, 4x, 4x, 8x and 2x the sample rate
  const expected = { low: 96000, medium: 192000, high: 192000, ultra: 384000, balanced: 96000 };
  for (const [quality, hz] of Object.entries(expected)) {
    const { header } = await firstSnapshot(cheapConfig({ quality }));
    assert.equal(header.physicsHz, hz, `quality ${quality}`);
  }
});

test("an engine built without a quality gets its preset's real-time default", async () => {
  for (const preset of addon.presets()) {
    // the default is a name from presets(); building with that name or with none is the same engine
    const named = await firstSnapshot(cheapConfig({ preset: preset.key, quality: preset.quality }));
    const bare = await firstSnapshot({ backend: "null", initial: { preset: preset.key } });
    assert.equal(bare.header.physicsHz, named.header.physicsHz, `${preset.key} (${preset.quality})`);
    // and presets() names the quality the engine really gets (rb_config_quality at 48 kHz)
    const hz = { low: 96000, medium: 192000, high: 192000, ultra: 384000, balanced: 96000 };
    assert.equal(bare.header.physicsHz, hz[preset.quality], `${preset.key}'s listed quality ${preset.quality}`);
    assert.equal(bare.header.buildId, named.header.buildId, preset.key);
  }
});

test("each preset builds its own engine: the cylinders and the snapshot's size follow", async () => {
  // contracts/snapshot-v2.md: 304 + 40 n bytes for n cylinders
  const cylinders = { i4: 4, i6: 6, "v8-cross": 8 };
  for (const [key, n] of Object.entries(cylinders)) {
    const { header, snap } = await firstSnapshot(cheapConfig({ preset: key }));
    assert.equal(header.cylinders, n, key);
    assert.equal(snap.bytes.byteLength, 304 + 40 * n, key);
    assert.equal(header.size, snap.bytes.byteLength);
  }
});

test("every stereo scene name builds", async () => {
  for (const scene of ["asphalt", "grass", "wall", "garage", "dyno", "iso", "passby", "cabin"]) {
    const { header } = await firstSnapshot(cheapConfig({ scene }));
    assert.equal(header.cylinders, 4, scene);
  }
});

test("an engine edit and a build parameter reach the engine", async () => {
  const plain = await firstSnapshot(cheapConfig());
  // the bore is not a live field: it makes a different engine (its description's hash changes)
  const edited = await firstSnapshot(cheapConfig({ edits: [{ field: FIELDS.bore, index: 0, value: 0.09 }] }));
  assert.notEqual(edited.header.buildId, plain.header.buildId);
  // a parameter applied when the engine is built: the throttle the engine has
  const pedal = await firstSnapshot(cheapConfig({ params: [{ id: P.throttle, value: 0.5 }] }));
  assert.equal(pedal.instrument(INSTRUMENT.throttle), 0.5);
  assert.equal(plain.instrument(INSTRUMENT.throttle), 0);
});

test("an edit's index picks the cylinder it changes", async () => {
  // header_length is one value per cylinder: cylinder 0's and cylinder 1's header make different engines
  const one = (index) => ({ edits: [{ field: FIELDS.header_length, index, value: 0.9 }] });
  const first = await firstSnapshot(cheapConfig(one(0)));
  const second = await firstSnapshot(cheapConfig(one(1)));
  const plain = await firstSnapshot(cheapConfig());
  assert.notEqual(first.header.buildId, plain.header.buildId);
  assert.notEqual(second.header.buildId, plain.header.buildId);
  assert.notEqual(first.header.buildId, second.header.buildId);
});

test("settleBlocks renders blocks before the engine is handed over", async () => {
  const settled = await firstSnapshot(cheapConfig({ settleBlocks: 300 }));
  const fresh = await firstSnapshot(cheapConfig());
  // the engine has already run 300 blocks of 128 frames (and the host has not rendered a one)
  assert.ok(settled.header.physicsTick > fresh.header.physicsTick + 300 * 128 * 2 - 1, "ran ahead");
});

test("the sample rate, the ring's fill target and the device are taken from the config", async () => {
  await withHost(cheapConfig({}, { sampleRate: 44100, targetFillMs: 30, device: 0 }), async (host) => {
    assert.equal(host.sampleRate(), 44100);
    host.start();
    const snap = await nextSnapshot(host);
    assert.equal(readSnapshotBytes(snap.bytes).header.audioHz, 44100);
    assert.equal(Math.round(host.stats().ringTargetMs), 30);
  });
  await withHost(cheapConfig({}, { device: -1 }), async (host) => assert.equal(host.sampleRate(), 48000));
});

test("an unknown preset, quality or scene name is RB_ERR_ID", () => {
  throwsCode(() => addon.createHost(cheapConfig({ preset: "model-t" })), "RB_ERR_ID", "preset");
  throwsCode(() => addon.createHost(cheapConfig({ quality: "extreme" })), "RB_ERR_ID", "quality");
  throwsCode(() => addon.createHost(cheapConfig({ scene: "moon" })), "RB_ERR_ID", "scene");
});

test("an engine the core cannot build is RB_HOST_ERR_BUILD", () => {
  throwsCode(() => addon.createHost(cheapConfig({ edits: [{ field: 9999, index: 0, value: 1 }] })), "RB_HOST_ERR_BUILD", "field");
  throwsCode(() => addon.createHost(cheapConfig({ params: [{ id: 9999, value: 1 }] })), "RB_HOST_ERR_BUILD", "param");
});

test("more edits or parameters than a build holds is RB_HOST_ERR_FULL", () => {
  const edits = Array.from({ length: 65 }, () => ({ field: FIELDS.bore, index: 0, value: 0.09 }));
  throwsCode(() => addon.createHost(cheapConfig({ edits })), "RB_HOST_ERR_FULL", "65 edits");
  const params = Array.from({ length: 32 }, () => ({ id: P.throttle, value: 0.1 }));
  throwsCode(() => addon.createHost(cheapConfig({ params })), "RB_HOST_ERR_FULL", "32 params and the source");
});

test("a bad config is RB_HOST_ERR_ARG, and says which part", () => {
  const bad = (config, why) => throwsCode(() => addon.createHost(config), "RB_HOST_ERR_ARG", why);
  bad(undefined, "no config");
  bad(5, "a number");
  bad(null, "null");
  bad({}, "no backend");
  bad({ backend: "null" }, "no initial build");
  bad({ backend: "jack", initial: { preset: "i4" } }, "an unknown backend");
  bad({ backend: "manual", initial: { preset: "i4" } }, "a backend that is the host's tests'");
  bad(cheapConfig({}, { sampleRate: 22050 }), "a sample rate");
  bad(cheapConfig({}, { sampleRate: "48000" }), "a string rate");
  bad(cheapConfig({}, { targetFillMs: 5 }), "a ring fill under 10 ms");
  bad(cheapConfig({}, { targetFillMs: 81 }), "a ring fill over 80 ms");
  bad(cheapConfig({}, { device: "first" }), "a device that is not a number");
  bad(cheapConfig({}, { device: 0.5 }), "a device that is not whole");
  bad({ backend: "null", initial: 7 }, "initial is not an object");
  bad({ backend: "null", initial: {} }, "no preset");
  bad({ backend: "null", initial: { preset: 4 } }, "a preset that is not a name");
  bad(cheapConfig({ quality: 2 }), "a quality that is not a name");
  bad(cheapConfig({ settleBlocks: -1 }), "negative settling");
  bad(cheapConfig({ settleBlocks: 2.5 }), "fractional settling");
  bad(cheapConfig({ edits: {} }), "edits that are not a list");
  bad(cheapConfig({ edits: [{ field: 1, index: 0 }] }), "an edit without a value");
  bad(cheapConfig({ edits: [{ field: "bore", index: 0, value: 1 }] }), "an edit's field by name");
  bad(cheapConfig({ params: [3] }), "a param that is not an object");
  bad(cheapConfig({ params: [{ id: 1 }] }), "a param without a value");
  bad(cheapConfig({ params: [{ value: 1 }] }), "a param without an id");
});

test("a refused config leaves nothing running: hosts can still be made", async () => {
  for (let i = 0; i < 5; i++) throwsCode(() => addon.createHost({ backend: "nope" }), "RB_HOST_ERR_ARG");
  await withHost(cheapConfig(), (host) => assert.equal(host.generation(), 1));
});

test("hosts are independent: two run side by side", async () => {
  await withHost(cheapConfig(), async (one) => {
    await withHost(cheapConfig({ preset: "i6" }), async (two) => {
      one.start();
      two.start();
      const a = readSnapshotBytes((await nextSnapshot(one)).bytes).header;
      const b = readSnapshotBytes((await nextSnapshot(two)).bytes).header;
      assert.equal(a.cylinders, 4);
      assert.equal(b.cylinders, 6);
    });
  });
});
