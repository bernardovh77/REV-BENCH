/*
 * Stage 131's engine side: the engine rebuilt as a layout (set-layout) as one transaction, the
 * edits a new layout keeps and drops, descriptions of a layout, the autosave and the trace keep
 * it, and a saved layout the core refuses doesn't stop the engine (against the fake add-on,
 * which follows the contract; addon-contract.ts runs the layout rules against the real one).
 */
import { mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { describe, expect, it } from "vitest";
import { EngineControl, PRESET_TRIM, type EnginePort } from "../src/engine/control";
import { createFakeAddon } from "../src/engine/fake-addon";
import { TraceRecorder } from "../src/engine/trace-recorder";
import { Autosave } from "../src/main/autosave";
import type { Addon, BuildSpec, LayoutSpec } from "../src/shared/addon";
import { editsForLayout, layoutName, sameLayout } from "../src/shared/layout-edits";
import type { BuildChoice, EngineEdit } from "../src/shared/process-messages";
import type { Command, FromEngine } from "../src/shared/protocol";

class Recorder implements EnginePort {
  messages: FromEngine[] = [];
  post(message: FromEngine): void {
    this.messages.push(message);
  }
  of<T extends FromEngine["type"]>(type: T): Extract<FromEngine, { type: T }>[] {
    return this.messages.filter((m) => m.type === type) as Extract<FromEngine, { type: T }>[];
  }
  reply(id: number) {
    return this.of("reply").find((r) => r.id === id);
  }
}

function setup(build: BuildChoice = { preset: "i6" }) {
  let t = 1000;
  const real = createFakeAddon({ now: () => t });
  const specs: BuildSpec[] = [];
  const addon: Addon = {
    ...real,
    createHost: (config) => {
      specs.push(config.initial);
      const host = real.createHost(config);
      const prepare = host.prepare.bind(host);
      host.prepare = (spec) => (specs.push(spec), prepare(spec));
      return host;
    },
  };
  const builds: BuildChoice[] = [];
  const control = new EngineControl({
    addon,
    config: { addon: "fake", backend: "null", build },
    onBuild: (b) => builds.push(b),
    log: () => {},
    now: () => t,
  });
  control.start();
  const port = new Recorder();
  control.attach(port);
  const send = (id: number, command: Command) => control.handle({ type: "command", id, command });
  const run = (ms: number) => {
    for (let i = 0; i < ms; i += 10) {
      t += 10;
      control.pump();
    }
  };
  const fieldId = (name: string) => addon.engineFields().find((f) => f.name === name)!.id;
  return { control, port, send, run, specs, builds, fieldId, addon };
}

const bore = (value: number): EngineEdit => ({ field: "bore", index: 0, value });
const header = (index: number, value: number): EngineEdit => ({ field: "header_length", index, value });
const V10: LayoutSpec = { family: "v", cylinders: 10, crank: "even", bankAngle: 72 };
const I3: LayoutSpec = { family: "inline", cylinders: 3, crank: "even", bankAngle: 0 };

describe("set-layout: one transaction", () => {
  it("rebuilds the preset as the layout, at the layout's tier, keeping the edits that still mean the same", () => {
    const s = setup({ preset: "v8-flat", edits: [bore(0.09), header(5, 0.7)] });
    s.send(1, { kind: "set-layout", layout: V10 });
    s.run(200);
    expect(s.port.reply(1)?.ack?.outcome).toBe("ok");
    const spec = s.specs.at(-1)!;
    expect(spec.layout).toEqual(V10);
    expect(spec.quality).toBe("lean"); // a V10's tier, said outright for the trace
    expect(spec.edits).toEqual([{ field: s.fieldId("bore"), index: 0, value: 0.09 }]); // the header was the V8's sixth
    expect(s.builds.at(-1)).toMatchObject({ preset: "v8-flat", layout: V10, edits: [bore(0.09)] });
    const told = s.port.of("description").at(-1)!;
    expect(told.layout).toEqual(V10);
    expect(told.values[s.fieldId("cylinders")]).toEqual([10]);
  });

  it("null, or the preset's own layout, is the preset as it comes; the same layout again keeps every edit", () => {
    const s = setup({ preset: "i4", layout: I3, edits: [bore(0.08), header(1, 0.6)] });
    s.send(1, { kind: "set-layout", layout: I3 });
    s.run(200);
    expect(s.builds.at(-1)).toMatchObject({ layout: I3, edits: [bore(0.08), header(1, 0.6)] });
    s.send(2, { kind: "set-layout", layout: { family: "inline", cylinders: 4, crank: "even", bankAngle: 0 } });
    s.run(200);
    expect(s.builds.at(-1)!.layout).toBeUndefined();
    expect(s.builds.at(-1)!.edits).toEqual([bore(0.08)]);
    expect(s.specs.at(-1)!.layout).toBeUndefined();
    s.send(3, { kind: "set-layout", layout: V10 });
    s.run(200);
    expect(s.builds.at(-1)!.layout).toEqual(V10);
    s.send(4, { kind: "set-layout", layout: null });
    s.run(200);
    expect(s.builds.at(-1)!.layout).toBeUndefined();
  });

  it("refuses a layout of the wrong shape, or one the core doesn't build, and nothing is prepared", () => {
    const s = setup({ preset: "i4" });
    const before = s.specs.length;
    s.send(1, { kind: "set-layout", layout: { family: "w", cylinders: 12, crank: "even", bankAngle: 0 } as unknown as LayoutSpec });
    expect(s.port.reply(1)?.error?.code).toBe("BAD_LAYOUT");
    s.send(2, { kind: "set-layout", layout: { family: "v", cylinders: 9, crank: "even", bankAngle: 90 } });
    expect(s.port.reply(2)?.error?.code).toBe("RB_ERR_VALUE");
    s.send(3, { kind: "set-layout", layout: { family: "inline", cylinders: 2.5, crank: "even", bankAngle: 0 } });
    expect(s.port.reply(3)?.error?.code).toBe("BAD_LAYOUT");
    expect(s.specs.length).toBe(before);
  });

  it("is BUSY while another rebuild is on its way, and the first lands whole", () => {
    const s = setup({ preset: "i4" });
    s.send(1, { kind: "set-layout", layout: V10 });
    s.send(2, { kind: "set-layout", layout: I3 });
    expect(s.port.reply(2)?.error?.code).toBe("BUSY");
    s.run(200);
    expect(s.builds.at(-1)!.layout).toEqual(V10);
  });

  it("a quality change keeps the layout; another preset starts as its own", () => {
    const s = setup({ preset: "i4" });
    s.send(1, { kind: "set-layout", layout: V10 });
    s.run(200);
    s.send(2, { kind: "load-preset", preset: "i4", quality: "low" });
    s.run(200);
    expect(s.builds.at(-1)).toMatchObject({ preset: "i4", quality: "low", layout: V10 });
    s.send(3, { kind: "load-preset", preset: "i6" });
    s.run(200);
    expect(s.builds.at(-1)!.layout).toBeUndefined();
  });

  it("describes a layout without building it: the preview's values, and the core's refusal", () => {
    const s = setup({ preset: "i4", edits: [bore(0.09)] });
    const before = s.specs.length;
    s.send(1, { kind: "describe", edits: [bore(0.09)], layout: V10 });
    const told = s.port.of("description").find((d) => d.request === 1)!;
    expect(told.layout).toEqual(V10);
    expect(told.values[s.fieldId("cylinders")]).toEqual([10]);
    s.send(2, { kind: "describe", edits: [], layout: { family: "boxer", cylinders: 10, crank: "even", bankAngle: 0 } });
    expect(s.port.reply(2)?.error?.code).toBe("RB_ERR_VALUE");
    expect(s.specs.length).toBe(before);
  });

  it("greets with the layouts the core builds", () => {
    const s = setup();
    expect(s.port.of("hello")[0].layouts).toEqual(s.addon.layouts());
  });
});

describe("a layout's level and its start", () => {
  it("is trimmed by its size: four cylinders or fewer as the four, more untrimmed", () => {
    expect(EngineControl.trimFor({ preset: "i4" })).toBe(PRESET_TRIM.i4);
    expect(EngineControl.trimFor({ preset: "i4", layout: V10 })).toBe(1);
    expect(EngineControl.trimFor({ preset: "v8-cross", layout: I3 })).toBe(PRESET_TRIM.i4);
    expect(EngineControl.trimFor({ preset: "v8-cross" })).toBe(1);
  });

  it("starts the preset as it comes when the saved layout is refused, and says so", () => {
    const logs: string[] = [];
    const real = createFakeAddon({ now: () => 0 });
    const control = new EngineControl({
      addon: real,
      config: {
        addon: "fake",
        backend: "null",
        build: { preset: "i4", layout: { family: "v", cylinders: 18, crank: "even", bankAngle: 90 }, edits: [bore(0.09)] },
      },
      onBuild: () => {},
      log: (_level, message) => logs.push(message),
      now: () => 0,
    });
    control.start();
    const port = new Recorder();
    control.attach(port);
    expect(port.of("hello")[0].build).toEqual({ preset: "i4" });
    expect(logs.some((m) => m.includes("refused"))).toBe(true);
  });
});

describe("the shared rule", () => {
  it("keeps whole-engine edits and drops the ones indexed by cylinder, bank or firing place", () => {
    const fields = createFakeAddon().engineFields();
    const { kept, dropped } = editsForLayout([bore(0.09), header(2, 0.7), { field: "rev_limit", index: 0, value: 7000 }], fields);
    expect(kept.map((e) => e.field)).toEqual(["bore", "rev_limit"]);
    expect(dropped.map((e) => e.field)).toEqual(["header_length"]);
    expect(editsForLayout([{ field: "nothing", index: 0, value: 1 }], fields).dropped).toHaveLength(1);
  });

  it("names a layout and compares two, the missing one as the preset's own", () => {
    expect(layoutName(V10)).toBe("V10");
    expect(layoutName(I3)).toBe("Inline 3");
    expect(layoutName({ family: "boxer", cylinders: 6, crank: "even", bankAngle: 0 })).toBe("Boxer 6");
    expect(sameLayout(undefined, I3, I3)).toBe(true);
    expect(sameLayout(V10, { ...V10, bankAngle: 90 }, I3)).toBe(false);
  });
});

describe("the autosave and the trace keep the layout", () => {
  it("saves a layout and gives it back; one of the wrong shape is dropped, the rest kept", () => {
    const dir = mkdtempSync(join(tmpdir(), "rb-layout-"));
    try {
      const file = join(dir, "build.json");
      new Autosave(file).save({ preset: "v8-cross", layout: V10, edits: [bore(0.09)] });
      expect(new Autosave(file).load()).toEqual({ preset: "v8-cross", layout: V10, edits: [bore(0.09)] });
      new Autosave(file).save({ preset: "v8-cross", layout: { family: "v", cylinders: 10, crank: "zigzag", bankAngle: 72 } as unknown as LayoutSpec });
      expect(new Autosave(file).load()).toEqual({ preset: "v8-cross" });
    } finally {
      rmSync(dir, { recursive: true, force: true });
    }
  });

  it("writes a build's layout and edits as rb_cli's trace lines", () => {
    const addon = createFakeAddon();
    const fields = addon.engineFields();
    const bid = fields.find((f) => f.name === "bore")!.id;
    const recorder = new TraceRecorder(addon.params(), addon.presets(), 48000,
      { preset: "i4", quality: "lean", layout: V10, edits: [{ field: bid, index: 0, value: 0.09 }] }, fields);
    const text = recorder.text(0);
    expect(text).toContain("preset i4\n");
    expect(text).toContain("layout 1 10 0 72\n");
    const edit = /^edit bore 0 (\S+)$/m.exec(text);
    expect(Number(edit?.[1])).toBe(0.09); // written to 17 digits, as rb_cli writes it
  });
});

describe("a saved build loaded (the build saver)", () => {
  it("is another preset with its layout and edits, as one rebuild; null is that preset's own", () => {
    const s = setup({ preset: "i4", edits: [bore(0.09)] });
    s.send(1, { kind: "load-preset", preset: "v8-cross", edits: [bore(0.1)], layout: V10 });
    s.run(200);
    expect(s.port.reply(1)?.ack?.outcome).toBe("ok");
    expect(s.builds.at(-1)).toMatchObject({ preset: "v8-cross", layout: V10, edits: [bore(0.1)] });
    expect(s.specs.at(-1)!.layout).toEqual(V10);
    s.send(2, { kind: "load-preset", preset: "i6", edits: [], layout: { family: "inline", cylinders: 6, crank: "even", bankAngle: 0 } });
    s.run(200);
    expect(s.builds.at(-1)!.layout).toBeUndefined();   // the six's own: the six as it comes
    s.send(3, { kind: "load-preset", preset: "i6", edits: [], layout: { family: "v", cylinders: 9, crank: "even", bankAngle: 90 } });
    expect(s.port.reply(3)?.error?.code).toBe("RB_ERR_VALUE");
    expect(s.builds.at(-1)!.preset).toBe("i6");
  });
});
