/*
 * Stage 067's engine side: the editor's edits applied as one transaction, live fields changed on
 * the running engine, descriptions of a build, and the races that must not leave a half-applied
 * build (against the fake add-on, which follows the contract; addon-contract.ts runs the same
 * description rules against the real one).
 */
import { describe, expect, it } from "vitest";
import { EngineControl, type EnginePort } from "../src/engine/control";
import { createFakeAddon } from "../src/engine/fake-addon";
import type { Addon, BuildSpec } from "../src/shared/addon";
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

describe("the greeting", () => {
  it("brings the core's engine fields and the playing engine's values", () => {
    const { port, addon } = setup();
    const hello = port.of("hello")[0];
    expect(hello.engineFields).toEqual(addon.engineFields());
    expect(hello.description).toEqual(addon.describe({ preset: "i6" }));
  });
});

describe("apply-edits: one transaction", () => {
  it("rebuilds the preset with every edit at once, keeps them for later builds, and says what plays", () => {
    const s = setup({ preset: "i6", quality: "low", scene: "garage" });
    s.send(1, { kind: "apply-edits", edits: [bore(0.09), header(2, 0.8)] });
    s.run(200);
    expect(s.port.reply(1)?.ack?.outcome).toBe("ok");
    // one build with both edits, by the add-on's ids, its quality and scene kept
    expect(s.specs).toHaveLength(2);
    expect(s.specs[1]).toMatchObject({ preset: "i6", quality: "low", scene: "garage" });
    expect(s.specs[1].edits).toEqual([{ field: s.fieldId("bore"), index: 0, value: 0.09 }, { field: s.fieldId("header_length"), index: 2, value: 0.8 }]);
    expect(s.builds.at(-1)).toEqual({ preset: "i6", quality: "low", scene: "garage", edits: [bore(0.09), header(2, 0.8)] });
    const playing = s.port.of("description").at(-1)!;
    expect(playing).toMatchObject({ request: 0, preset: "i6", edits: [bore(0.09), header(2, 0.8)] });
    expect(playing.values[s.fieldId("bore")]).toEqual([0.09]);
    // a quality change keeps them; another preset starts from its own engine
    s.send(2, { kind: "load-preset", preset: "i6", quality: "high", scene: "garage" });
    s.run(200);
    expect(s.specs[2].edits).toHaveLength(2);
    s.send(3, { kind: "load-preset", preset: "v8-cross" });
    s.run(200);
    expect(s.specs[3].edits).toBeUndefined();
    expect(s.builds.at(-1)!.edits).toBeUndefined();
  });

  it("refuses the whole set when the core refuses one edit, and the engine playing is untouched", () => {
    const s = setup();
    s.send(1, { kind: "apply-edits", edits: [bore(0.09)] });
    s.run(200);
    const before = s.specs.length;
    s.send(2, { kind: "apply-edits", edits: [bore(0.1), { field: "displacement", index: 0, value: 0.003 }] });
    expect(s.port.reply(2)?.error?.code).toBe("RB_ERR_READONLY");
    s.run(200);
    expect(s.specs).toHaveLength(before); // nothing was prepared
    expect(s.builds.at(-1)!.edits).toEqual([bore(0.09)]);
  });

  it("refuses edits that aren't edits before the core sees them", () => {
    const s = setup();
    const bad: unknown[] = [[{ field: "no_such", index: 0, value: 1 }], [{ field: "bore", index: -1, value: 0.09 }], [{ field: "bore", index: 0, value: Number.NaN }], "bore"];
    bad.forEach((edits, i) => {
      s.send(10 + i, { kind: "apply-edits", edits: edits as EngineEdit[] });
      expect(s.port.reply(10 + i)?.error?.code).toBe("BAD_EDIT");
    });
  });

  it("an empty list is the preset as it comes", () => {
    const s = setup({ preset: "i6", edits: [bore(0.09)] });
    expect(s.specs[0].edits).toHaveLength(1);
    s.send(1, { kind: "apply-edits", edits: [] });
    s.run(200);
    expect(s.specs[1].edits).toBeUndefined();
    expect(s.builds.at(-1)!.edits).toBeUndefined();
  });
});

describe("edit-live", () => {
  it("changes a live field on the running engine, with no rebuild, and keeps it for the next", () => {
    const s = setup();
    s.send(1, { kind: "edit-live", edit: { field: "idle_rpm", index: 0, value: 950 } });
    s.run(10);
    expect(s.port.reply(1)?.ack).toMatchObject({ kind: "set-engine-live", outcome: "ok" });
    expect(s.specs).toHaveLength(1); // no rebuild
    expect(s.builds.at(-1)!.edits).toEqual([{ field: "idle_rpm", index: 0, value: 950 }]);
    // a second edit of the same field replaces the first
    s.send(2, { kind: "edit-live", edit: { field: "idle_rpm", index: 0, value: 900 } });
    s.run(10);
    expect(s.builds.at(-1)!.edits).toEqual([{ field: "idle_rpm", index: 0, value: 900 }]);
    s.send(3, { kind: "load-preset", preset: "i6", quality: "high" });
    s.run(200);
    expect(s.specs[1].edits).toEqual([{ field: s.fieldId("idle_rpm"), index: 0, value: 900 }]);
  });

  it("refuses a field that needs a rebuild", () => {
    const s = setup();
    s.send(1, { kind: "edit-live", edit: bore(0.09) });
    expect(s.port.reply(1)?.error?.code).toBe("NOT_LIVE");
  });
});

describe("rapid mixed edits never leave a half-applied build", () => {
  it("a live edit while a rebuild is under way is refused (it would miss the new engine)", () => {
    const s = setup();
    s.send(1, { kind: "apply-edits", edits: [bore(0.09)] });
    s.send(2, { kind: "edit-live", edit: { field: "idle_rpm", index: 0, value: 950 } });
    expect(s.port.reply(2)?.error?.code).toBe("BUSY");
    s.run(200);
    expect(s.builds.at(-1)!.edits).toEqual([bore(0.09)]);
  });

  it("a rebuild while a live edit is on its way is refused (it would lose the edit)", () => {
    const s = setup();
    s.send(1, { kind: "edit-live", edit: { field: "idle_rpm", index: 0, value: 950 } });
    s.send(2, { kind: "apply-edits", edits: [bore(0.09)] });
    expect(s.port.reply(2)?.error?.code).toBe("BUSY");
    s.run(10);
    s.send(3, { kind: "apply-edits", edits: [{ field: "idle_rpm", index: 0, value: 950 }, bore(0.09)] });
    s.run(200);
    expect(s.port.reply(3)?.ack?.outcome).toBe("ok");
  });

  it("a second set while one is being built is refused; the first lands whole", () => {
    const s = setup();
    s.send(1, { kind: "apply-edits", edits: [bore(0.09), header(0, 0.7)] });
    s.send(2, { kind: "apply-edits", edits: [bore(0.12)] });
    expect(s.port.reply(2)?.error?.code).toBe("BUSY");
    s.run(200);
    expect(s.builds.at(-1)!.edits).toEqual([bore(0.09), header(0, 0.7)]);
    expect(s.specs).toHaveLength(2);
  });
});

describe("the engine computer's settings (stage 067)", () => {
  it("keeps the air-fuel ratio and the spark offset for every later build", () => {
    const s = setup();
    s.send(1, { kind: "set-param", name: "afr", value: 12.5 });
    s.send(2, { kind: "set-param", name: "spark_offset", value: -4 });
    s.run(10);
    s.send(3, { kind: "apply-edits", edits: [bore(0.09)] });
    s.run(200);
    const params = s.specs.at(-1)!.params!;
    const id = (name: string) => s.addon.params().find((p) => p.name === name)!.id;
    expect(params.find((p) => p.id === id("afr"))?.value).toBe(12.5);
    expect(params.find((p) => p.id === id("spark_offset"))?.value).toBe(-4);
  });
});

describe("describe", () => {
  it("answers with the values of a set of edits, building nothing", () => {
    const s = setup();
    s.send(7, { kind: "describe", edits: [bore(0.1)] });
    const d = s.port.of("description").find((m) => m.request === 7)!;
    expect(d.values[s.fieldId("bore")]).toEqual([0.1]);
    expect(s.specs).toHaveLength(1);
    expect(s.port.reply(7)?.error).toBeUndefined();
  });

  it("answers a set the core refuses with its reason", () => {
    const s = setup();
    s.send(8, { kind: "describe", edits: [{ field: "displacement", index: 0, value: 1 }] });
    expect(s.port.reply(8)?.error?.code).toBe("RB_ERR_READONLY");
    expect(s.port.of("description").some((m) => m.request === 8)).toBe(false);
  });
});
