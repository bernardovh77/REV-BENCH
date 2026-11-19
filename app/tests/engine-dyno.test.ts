/*
 * Stage 068's engine side of the dyno: its four commands passed to the host and acknowledged,
 * their numbers checked first, refused during a rebuild, recorded in the trace as rb_cli's
 * lines; and each snapshot message carries the host's output meter.
 */
import { describe, expect, it } from "vitest";
import { EngineControl, type EnginePort } from "../src/engine/control";
import { createFakeAddon } from "../src/engine/fake-addon";
import { TraceRecorder } from "../src/engine/trace-recorder";
import type { Addon, BuildSpec } from "../src/shared/addon";
import type { BuildChoice } from "../src/shared/process-messages";
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

describe("the dyno's commands", () => {
  it("are passed to the host and acknowledged once the render thread took them", () => {
    const s = setup({ preset: "i4" });
    s.send(1, { kind: "dyno-attach", rpm: 3000 });
    s.send(2, { kind: "dyno-sweep", toRpm: 6000, rate: 500 });
    s.send(3, { kind: "dyno-clear" });
    s.send(4, { kind: "dyno-release" });
    s.run(50);
    expect([1, 2, 3, 4].map((id) => [s.port.reply(id)?.ack?.kind, s.port.reply(id)?.ack?.outcome])).toEqual([
      ["dyno-attach", "ok"], ["dyno-sweep", "ok"], ["dyno-clear", "ok"], ["dyno-release", "ok"],
    ]);
  });

  it("refuse numbers outside the dyno's range, and wait for no rebuild", () => {
    const s = setup({ preset: "i4" });
    for (const [id, command] of [
      [1, { kind: "dyno-attach", rpm: 100 }],
      [2, { kind: "dyno-attach", rpm: Number.NaN }],
      [3, { kind: "dyno-sweep", toRpm: 25000, rate: 500 }],
      [4, { kind: "dyno-sweep", toRpm: 6000, rate: -1 }],
    ] as [number, Command][]) {
      s.send(id, command);
      expect(s.port.reply(id)?.error?.code, JSON.stringify(command)).toBe("BAD_VALUE");
    }
    s.send(5, { kind: "load-preset", preset: "i6" });
    s.send(6, { kind: "dyno-attach", rpm: 3000 });
    expect(s.port.reply(6)?.error?.code).toBe("BUSY");
  });

  it("are written in the trace as rb_cli writes them, at the frame each took effect", () => {
    const addon = createFakeAddon();
    const recorder = new TraceRecorder(addon.params(), addon.presets(), 48000, { preset: "i4" }, addon.engineFields());
    const host = recorder.wrap(addon.createHost({ backend: "null", sampleRate: 48000, initial: { preset: "i4" } }));
    host.dynoAttach(2000);
    host.dynoSweep(5000, 1000);
    host.dynoClear();
    host.dynoRelease();
    host.poll();
    const text = recorder.text(0);
    for (const line of [/^at \d+ dyno_attach 2000$/m, /^at \d+ dyno_sweep 5000 1000$/m, /^at \d+ dyno_clear$/m, /^at \d+ dyno_release$/m]) {
      expect(text).toMatch(line);
    }
    host.destroy();
  });

  it("each snapshot message carries the host's output meter", () => {
    const s = setup({ preset: "i4" });
    s.run(100);
    s.control.handle({ type: "snapshot-request" });
    const snap = s.port.of("snapshot").at(-1)!;
    expect(snap.outPeak).toBeGreaterThanOrEqual(0);
    expect(snap.outRms).toBeLessThanOrEqual(snap.outPeak + 1e-9);
    expect(typeof snap.outPeak).toBe("number");
  });
});
