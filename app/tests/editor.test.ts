/*
 * The engine editor's state (stage 067): the draft applied as one transaction or kept with the
 * reason, live fields at once, the preview, undo and redo, sections from the core's fields.
 */
import { describe, expect, it } from "vitest";
import type { EngineFieldInfo } from "../src/shared/addon";
import type { EngineEdit } from "../src/shared/process-messages";
import { CHOICES, Editor, labelOf, mergeEdits, type EditorStore } from "../src/ui/lib/editor.svelte";

const FIELDS: EngineFieldInfo[] = [
  { id: 0, name: "cylinders", unit: "", min: null, max: null, flags: ["readonly", "integer"] },
  { id: 1, name: "bore", unit: "m", min: 0.04, max: 0.15, flags: [] },
  { id: 2, name: "displacement", unit: "m^3", min: null, max: null, flags: ["readonly"] },
  { id: 3, name: "header_length", unit: "m", min: 0.1, max: 2.5, flags: ["per-cylinder"] },
  { id: 4, name: "idle_rpm", unit: "rpm", min: 0, max: 2000, flags: ["live"] },
  { id: 5, name: "something_new", unit: "", min: 0, max: 1, flags: [] },
];
const f = (name: string) => FIELDS.find((x) => x.name === name)!;

/** A store that describes as the core would (bore drives displacement) and records commands. */
function fakeStore(options: { refuse?: (edits: EngineEdit[]) => string | null } = {}) {
  const applied: EngineEdit[][] = [];
  const live: EngineEdit[] = [];
  const describe = (edits: EngineEdit[]): number[][] => {
    const v = [[4], [0.086], [0], [0.5, 0.5, 0.5, 0.5], [800], [0]];
    for (const e of edits) v[FIELDS.find((x) => x.name === e.field)!.id][e.index] = e.value;
    v[2] = [Math.PI / 4 * v[1][0] ** 2 * 0.086 * 4];
    return v;
  };
  const store = {
    engineFields: FIELDS,
    engineEdits: [] as EngineEdit[],
    engineValues: describe([]) as number[][] | null,
    preset: "i4" as string | null,
    async applyEdits(edits: EngineEdit[]) {
      const why = options.refuse?.(edits);
      if (why) throw Object.assign(new Error(why), { code: "RB_ERR_VALUE" });
      applied.push(edits);
      store.engineEdits = edits;
      store.engineValues = describe(edits);
    },
    async editLive(edit: EngineEdit) {
      live.push(edit);
      store.engineEdits = mergeEdits(store.engineEdits, [edit]);
      store.engineValues = describe(store.engineEdits);
    },
    async describeEdits(edits: EngineEdit[]) {
      const why = options.refuse?.(edits);
      if (why) throw Object.assign(new Error(why), { code: "RB_ERR_VALUE" });
      return describe(edits);
    },
  } satisfies EditorStore;
  return { store, applied, live, editor: new Editor(store) };
}

describe("sections", () => {
  it("leave out what has no effect: the crank position always, the bank angle on one bank", () => {
    const { editor, store } = fakeStore();
    store.engineFields = [
      ...FIELDS,
      { id: 6, name: "position", unit: "m", min: 0, max: 1, flags: ["per-cylinder"] },
      { id: 7, name: "bank_angle", unit: "deg", min: -90, max: 90, flags: ["per-bank"] },
      { id: 8, name: "banks", unit: "", min: null, max: null, flags: ["readonly", "integer"] },
    ];
    const names = () => editor.sections().flatMap((s) => s.fields.map((f) => f.name));
    store.engineValues = [...(store.engineValues ?? []), [0, 0, 0, 0], [0], [1]];
    expect(names()).not.toContain("position");
    expect(names()).not.toContain("bank_angle");
    store.engineValues = [...store.engineValues.slice(0, 6), [0, 0, 0, 0], [45, -45], [2]];
    expect(names()).toContain("bank_angle");
    expect(names()).not.toContain("position");
  });

  it("are built from the core's fields, with a field nobody placed under Other", () => {
    const { editor } = fakeStore();
    const s = editor.sections();
    expect(s.map((x) => x.title)).toEqual(["Engine", "Exhaust", "Limits and load", "Other"]);
    expect(s[0].fields.map((x) => x.name)).toEqual(["cylinders", "displacement", "bore"]);
    expect(s.at(-1)!.fields.map((x) => x.name)).toEqual(["something_new"]);
    expect(labelOf("compression_ratio")).toBe("Compression ratio");
    expect(CHOICES.muffler_type).toHaveLength(3);
  });
});

describe("the draft", () => {
  it("collects rebuild fields, previews them, and applies them as one edit set", async () => {
    const { editor, applied } = fakeStore();
    await editor.set(f("bore"), 0, 0.09);
    await editor.set(f("header_length"), 2, 0.8);
    expect(editor.draftCount).toBe(2);
    expect(applied).toEqual([]);
    expect(editor.values(f("bore"))).toEqual([0.09]);
    // the preview shows what the core works out
    expect(editor.values(f("displacement"))[0]).toBeCloseTo(Math.PI / 4 * 0.09 ** 2 * 0.086 * 4, 12);
    await editor.apply();
    expect(applied).toEqual([[{ field: "bore", index: 0, value: 0.09 }, { field: "header_length", index: 2, value: 0.8 }]]);
    expect(editor.draftCount).toBe(0);
  });

  it("is kept, with the reason, when the build is refused; the preview says so first", async () => {
    const { editor, applied } = fakeStore({ refuse: (e) => (e.some((x) => x.value === 0.149) ? "the valves don't fit" : null) });
    await editor.set(f("bore"), 0, 0.149);
    expect(editor.previewError).toContain("the valves don't fit");
    await editor.apply();
    expect(applied).toEqual([]);
    expect(editor.draftCount).toBe(1);
    expect(editor.error).toContain("RB_ERR_VALUE");
    editor.discard();
    expect(editor.draftCount).toBe(0);
    expect(editor.values(f("bore"))).toEqual([0.086]);
  });

  it("sets every cylinder at once", async () => {
    const { editor } = fakeStore();
    await editor.setAll(f("header_length"), 0.7);
    expect(editor.values(f("header_length"))).toEqual([0.7, 0.7, 0.7, 0.7]);
  });

  it("never edits a read-only field or a value that isn't a number", async () => {
    const { editor } = fakeStore();
    await editor.set(f("displacement"), 0, 0.003);
    await editor.set(f("bore"), 0, Number.NaN);
    expect(editor.draftCount).toBe(0);
  });

  it("an answer to an older preview never replaces a newer one", async () => {
    const { store, editor } = fakeStore();
    let release!: () => void;
    const slow = new Promise<void>((r) => (release = r));
    const describe = store.describeEdits;
    let first = true;
    store.describeEdits = async (edits) => {
      if (first) {
        first = false;
        await slow;
      }
      return describe(edits);
    };
    const a = editor.set(f("bore"), 0, 0.07);
    await editor.set(f("bore"), 0, 0.1);
    release();
    await a;
    expect(editor.values(f("displacement"))[0]).toBeCloseTo(Math.PI / 4 * 0.1 ** 2 * 0.086 * 4, 12);
  });
});

describe("live fields", () => {
  it("change the running engine at once, never the draft", async () => {
    const { editor, live, applied } = fakeStore();
    await editor.set(f("idle_rpm"), 0, 950);
    expect(live).toEqual([{ field: "idle_rpm", index: 0, value: 950 }]);
    expect(applied).toEqual([]);
    expect(editor.draftCount).toBe(0);
  });
});

describe("undo and redo", () => {
  it("step back and forth through what was applied, live edits included", async () => {
    const { editor, store } = fakeStore();
    await editor.set(f("bore"), 0, 0.09);
    await editor.apply();
    await editor.set(f("idle_rpm"), 0, 950);
    expect(store.engineEdits).toHaveLength(2);
    expect(editor.canUndo).toBe(true);
    await editor.undo();
    expect(store.engineEdits).toEqual([{ field: "bore", index: 0, value: 0.09 }]);
    await editor.undo();
    expect(store.engineEdits).toEqual([]);
    expect(editor.canUndo).toBe(false);
    await editor.redo();
    await editor.redo();
    expect(store.engineEdits).toHaveLength(2);
    expect(editor.canRedo).toBe(false);
  });

  it("a new change clears what could be redone", async () => {
    const { editor } = fakeStore();
    await editor.set(f("bore"), 0, 0.09);
    await editor.apply();
    await editor.undo();
    expect(editor.canRedo).toBe(true);
    await editor.set(f("bore"), 0, 0.1);
    await editor.apply();
    expect(editor.canRedo).toBe(false);
  });

  it("a refused undo keeps the history as it was", async () => {
    let refuse = false;
    const { editor, store } = fakeStore({ refuse: () => (refuse ? "no" : null) });
    await editor.set(f("bore"), 0, 0.09);
    await editor.apply();
    refuse = true;
    await editor.undo();
    expect(editor.error).toContain("no");
    expect(store.engineEdits).toHaveLength(1);
    refuse = false;
    expect(editor.canUndo).toBe(true);
  });

  it("another preset starts its own history and drops the draft", async () => {
    const { editor, store } = fakeStore();
    await editor.set(f("bore"), 0, 0.09);
    await editor.apply();
    await editor.set(f("bore"), 0, 0.1);
    store.preset = "v8-cross";
    expect(editor.canUndo).toBe(false);
    expect(editor.draftCount).toBe(0);
  });
});
