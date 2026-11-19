/*
 * The build saver: the saved builds' files (main/library.ts), their format (shared/builds.ts)
 * and the window's saver (ui/lib/builds.svelte.ts) against a bridge and a store of its own.
 */
import { mkdtempSync, readdirSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { BuildLibrary } from "../src/main/library";
import type { LayoutSpec } from "../src/shared/addon";
import { cleanName, idOf, isId, parseSaved, type SavedEngine } from "../src/shared/builds";
import type { EngineEdit } from "../src/shared/process-messages";
import { SavedBuilds, type BuildsBridge, type BuildsStore } from "../src/ui/lib/builds.svelte";

let dir: string;
beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), "rb-builds-"));
});
afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
});

const LS7: SavedEngine = {
  preset: "v8-cross",
  edits: [{ field: "bore", index: 0, value: 0.10478 }, { field: "header_length", index: 7, value: 0.745 }],
};
const V10: LayoutSpec = { family: "v", cylinders: 10, crank: "even", bankAngle: 72 };
let clock = 0;
const later = () => new Date(Date.UTC(2026, 10, 18, 12, 0, clock++));

describe("names and ids", () => {
  it("keeps a name tidy and makes a file id of it that is never a path", () => {
    expect(cleanName("  LS7   chop\ttune ")).toBe("LS7 chop tune");
    expect(idOf("LS7 chop tune")).toBe("ls7-chop-tune");
    expect(idOf("Ford's V10, 6.8 L")).toBe("ford-s-v10-6-8-l");
    expect(idOf("Café racer")).toBe("cafe-racer");
    expect(idOf("../../etc")).toBe("etc");
    expect(idOf("***")).toBe("");
    for (const bad of ["../x", "a/b", "a\\b", "", "A", "-a", "a-", 4, null]) expect(isId(bad)).toBe(false);
    expect(isId("ls7-chop-tune")).toBe(true);
  });
});

describe("the library on disk", () => {
  it("saves a build, lists it and reads it back whole, its settings with it", () => {
    const lib = new BuildLibrary(join(dir, "builds"), later);
    expect(lib.list()).toEqual([]);
    expect(lib.save("LS7 chop tune", LS7, { afr: 14, spark_offset: 0 }, false)).toEqual({ ok: true, id: "ls7-chop-tune" });
    expect(readdirSync(join(dir, "builds"))).toEqual(["ls7-chop-tune.json"]);
    const read = lib.read("ls7-chop-tune")!;
    expect(read).toMatchObject({ version: 1, name: "LS7 chop tune", build: LS7, settings: { afr: 14, spark_offset: 0 } });
    expect(lib.list()).toEqual([{ id: "ls7-chop-tune", name: "LS7 chop tune", savedAt: read.savedAt, preset: "v8-cross", edits: 2 }]);
  });

  it("keeps a layout, lists the newest first, and replaces one of the same name only when asked", () => {
    const lib = new BuildLibrary(dir, later);
    lib.save("First", { preset: "i4" }, {}, false);
    lib.save("V10 from the four", { preset: "i4", layout: V10 }, {}, false);
    expect(lib.list().map((b) => b.name)).toEqual(["V10 from the four", "First"]);
    expect(lib.list()[0].layout).toEqual(V10);
    expect(lib.save("first", { preset: "i6" }, {}, false)).toEqual({ ok: false, reason: "exists" });
    expect(lib.read("first")!.build.preset).toBe("i4");
    expect(lib.save("first", { preset: "i6" }, {}, true)).toEqual({ ok: true, id: "first" });
    expect(lib.read("first")).toMatchObject({ name: "first", build: { preset: "i6" } });
    expect(lib.list()[0].name).toBe("first");   // the newest now
  });

  it("refuses a bad name or build, and reads nothing by a path", () => {
    const lib = new BuildLibrary(dir, later);
    expect(lib.save("   ", LS7, {}, false)).toEqual({ ok: false, reason: "bad-name" });
    expect(lib.save("???", LS7, {}, false)).toEqual({ ok: false, reason: "bad-name" });
    expect(lib.save("x", { preset: "../i4" }, {}, false)).toEqual({ ok: false, reason: "bad-build" });
    expect(lib.save("x", { preset: "i4", layout: { family: "w" } }, {}, false)).toEqual({ ok: false, reason: "bad-build" });
    expect(lib.save("x", { preset: "i4", edits: [{ field: "bore", index: -1, value: 1 }] }, {}, false)).toEqual({ ok: false, reason: "bad-build" });
    writeFileSync(join(dir, "..", "outside.json"), "{}");
    expect(lib.read("../outside")).toBeNull();
    expect(lib.remove("../outside")).toBe(false);
  });

  it("drops unknown settings and the extra fields a hand-edited file has, and skips files that aren't builds", () => {
    const lib = new BuildLibrary(dir, later);
    lib.save("Mine", { preset: "i4", extra: 1 } as SavedEngine, { afr: 13, nonsense: 5, spark_offset: Number.NaN }, false);
    const read = lib.read("mine")!;
    expect(read.build).toEqual({ preset: "i4" });
    expect(read.settings).toEqual({ afr: 13 });
    for (const [file, text] of [["junk.json", "not json"], ["old.json", '{"version":9,"name":"x"}'], ["Upper.json", "{}"], ["notes.txt", "hi"]]) {
      writeFileSync(join(dir, file), text);
    }
    expect(lib.list().map((b) => b.id)).toEqual(["mine"]);
    expect(lib.read("junk")).toBeNull();
    expect(lib.remove("junk")).toBe(false);   // never deletes what isn't a saved build
    expect(readdirSync(dir)).toContain("junk.json");
  });

  it("deletes a build, and leaves no temporary file behind a save", () => {
    const lib = new BuildLibrary(dir, later);
    lib.save("Gone soon", LS7, {}, false);
    expect(readdirSync(dir)).toEqual(["gone-soon.json"]);
    expect(lib.remove("gone-soon")).toBe(true);
    expect(lib.remove("gone-soon")).toBe(false);
    expect(lib.list()).toEqual([]);
  });

  it("reads the LS7 saved by hand on 2026-11-18 (the format's first file)", () => {
    const text = JSON.stringify({
      version: 1, name: "LS7 chop tune", savedAt: "2026-11-18T10:05:00+00:00",
      build: { preset: "v8-cross", edits: [{ field: "bore", index: 0, value: 0.10478 }] }, settings: { afr: 14, spark_offset: 0 },
    });
    expect(parseSaved(text)).toMatchObject({ name: "LS7 chop tune", build: { preset: "v8-cross" }, settings: { afr: 14 } });
  });
});

/** A bridge over a real library in the test's folder, and a store that records what it's told. */
function saver(start: { preset?: string; layout?: LayoutSpec | null; edits?: EngineEdit[]; settings?: Record<string, number> } = {}) {
  const lib = new BuildLibrary(dir, later);
  const bridge: BuildsBridge = {
    list: async () => lib.list(),
    read: async (id) => lib.read(id),
    save: async (name, build, settings, replace) => lib.save(name, build, settings, replace),
    remove: async (id) => lib.remove(id),
  };
  const loaded: SavedEngine[] = [];
  const params: [string, number][] = [];
  const store: BuildsStore & { preset: string | null; layout: LayoutSpec | null; engineEdits: EngineEdit[]; settings: Record<string, number> } = {
    preset: start.preset ?? "v8-cross",
    layout: start.layout ?? null,
    engineEdits: start.edits ?? [],
    settings: start.settings ?? { afr: 14, spark_offset: 2 },
    settingDefault: (name) => (name === "afr" ? 14.596 : 0),
    async loadBuild(build) {
      if (build.preset === "refused") throw new Error("no preset \"refused\"");
      loaded.push(build);
      store.preset = build.preset;
      store.layout = build.layout ?? null;
      store.engineEdits = build.edits ?? [];
    },
    async setParam(name, value) {
      params.push([name, value]);
      store.settings = { ...store.settings, [name]: value };
    },
  };
  return { lib, store, loaded, params, builds: new SavedBuilds(bridge, store) };
}

describe("the window's saver", () => {
  it("saves the engine playing (preset, layout, edits and the engine computer's settings) under the typed name", async () => {
    const s = saver({ layout: V10, edits: LS7.edits, settings: { afr: 13.2, spark_offset: 3 } });
    expect(s.builds.canSave).toBe(false);   // no name yet
    s.builds.name = "  My V10 ";
    expect(s.builds.canSave).toBe(true);
    await s.builds.save();
    expect(s.lib.read("my-v10")).toMatchObject({ name: "My V10", build: { preset: "v8-cross", layout: V10, edits: LS7.edits }, settings: { afr: 13.2, spark_offset: 3 } });
    expect(s.builds.list.map((b) => b.name)).toEqual(["My V10"]);
    expect(s.builds.name).toBe("");
    expect(s.builds.message).toBe('Saved "My V10"');
  });

  it("says a save replaces one of the same name, and does", async () => {
    const s = saver();
    s.builds.name = "Mine";
    await s.builds.save();
    s.store.engineEdits = [{ field: "bore", index: 0, value: 0.1 }];
    s.builds.name = "mine";
    expect(s.builds.replaces).toBe(true);
    await s.builds.save();
    expect(s.lib.list()).toHaveLength(1);
    expect(s.lib.read("mine")!.build.edits).toEqual([{ field: "bore", index: 0, value: 0.1 }]);
  });

  it("loads a build back: its engine as one rebuild, then its settings (the defaults where it has none)", async () => {
    const s = saver({ preset: "i4", settings: { afr: 12, spark_offset: 5 } });
    s.lib.save("LS7", LS7, { afr: 14 }, false);
    await s.builds.refresh();
    await s.builds.load("ls7");
    expect(s.loaded).toEqual([LS7]);
    expect(s.params).toEqual([["afr", 14], ["spark_offset", 0]]);
    expect(s.builds.message).toBe('Loaded "LS7"');
  });

  it("keeps the engine and says why when a build can't be loaded; a missing file is taken off the list", async () => {
    const s = saver();
    s.lib.save("Odd", { preset: "refused" }, {}, false);
    await s.builds.refresh();
    await s.builds.load("odd");
    expect(s.loaded).toEqual([]);
    expect(s.params).toEqual([]);
    expect(s.builds.message).toContain("couldn't be loaded");
    rmSync(join(dir, "odd.json"));
    await s.builds.load("odd");
    expect(s.builds.message).toContain("gone");
    expect(s.builds.list).toEqual([]);
  });

  it("deletes only on the second press", async () => {
    const s = saver();
    s.lib.save("Keep", LS7, {}, false);
    await s.builds.refresh();
    await s.builds.remove("keep");
    expect(s.builds.confirmDelete).toBe("keep");
    expect(s.lib.list()).toHaveLength(1);
    await s.builds.remove("keep");
    expect(s.lib.list()).toEqual([]);
    expect(s.builds.list).toEqual([]);
    expect(s.builds.message).toBe('Deleted "Keep"');
  });

  it("is off without the app's bridge (a plain browser page)", async () => {
    const s = saver();
    const off = new SavedBuilds(null, s.store);
    off.name = "x";
    expect(off.available).toBe(false);
    expect(off.canSave).toBe(false);
    await off.save();
    await off.refresh();
    expect(off.list).toEqual([]);
  });
});
