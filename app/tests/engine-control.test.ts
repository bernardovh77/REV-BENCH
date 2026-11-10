/*
 * The engine process's control loop, against the fake add-on: commands -> acknowledgments, a
 * rebuild, snapshots forwarded, errors.
 */
import { describe, expect, it } from "vitest";
import { EngineControl, PREPARE_TIMEOUT_MS, type EnginePort } from "../src/engine/control";
import { createFakeAddon, addonError } from "../src/engine/fake-addon";
import type { Addon, Host } from "../src/shared/addon";
import type { BuildChoice, EngineInit } from "../src/shared/process-messages";
import { PROTOCOL_VERSION, type Command, type FromEngine } from "../src/shared/protocol";
import { decodeSnapshot } from "../src/shared/snapshot-v2.generated";

class Recorder implements EnginePort {
  messages: FromEngine[] = [];
  transfers: (ArrayBuffer[] | undefined)[] = [];
  post(message: FromEngine, transfer?: ArrayBuffer[]): void {
    this.messages.push(message);
    this.transfers.push(transfer);
  }
  of<T extends FromEngine["type"]>(type: T): Extract<FromEngine, { type: T }>[] {
    return this.messages.filter((m) => m.type === type) as Extract<FromEngine, { type: T }>[];
  }
  replies(id: number) {
    return this.of("reply").filter((r) => r.id === id);
  }
}

const config = (build: BuildChoice = { preset: "i4" }): EngineInit => ({ addon: "fake", backend: "null", build });

function setup(options: { build?: BuildChoice; addon?: Addon; failPresets?: string[] } = {}) {
  let t = 1000;
  const clock = { now: () => t, step: (ms: number) => void (t += ms) };
  const addon = options.addon ?? createFakeAddon({ now: clock.now, failPresets: options.failPresets });
  const builds: BuildChoice[] = [];
  const logs: string[] = [];
  const control = new EngineControl({
    addon,
    config: config(options.build),
    onBuild: (b) => builds.push(b),
    log: (level, message) => logs.push(`${level}: ${message}`),
    now: clock.now,
  });
  control.start();
  const port = new Recorder();
  control.attach(port);
  const send = (id: number, command: Command) => control.handle({ type: "command", id, command });
  /** lets time pass in 10 ms steps, pumping the loop */
  const run = (ms: number) => {
    for (let i = 0; i < ms; i += 10) {
      clock.step(10);
      control.pump();
    }
  };
  return { control, port, clock, send, run, builds, logs, addon };
}

describe("hello", () => {
  it("is the first message, and tells the window what the engine offers", () => {
    const { port, addon } = setup({ build: { preset: "i6" } });
    const hello = port.of("hello")[0];
    expect(port.messages[0]).toBe(hello);
    expect(hello.protocol).toBe(PROTOCOL_VERSION);
    expect(hello.preset).toBe("i6");
    expect(hello.presets.map((p) => p.key)).toContain("v8-cross");
    expect(hello.params.map((p) => p.name)).toContain("throttle");
    expect(hello.telemetry.map((t) => t.name)).toContain("rpm");
    expect(hello.devices.length).toBeGreaterThan(0);
    expect(hello.sampleRate).toBe(48000);
    expect(hello.generation).toBe(1);
    expect(hello.addonVersion).toBe(addon.version().addon);
    expect(hello.snapshotVersion).toBe(2);
  });

  it("is followed by the state: running", () => {
    const { port } = setup();
    expect(port.of("state")).toEqual([{ type: "state", state: "running" }]);
  });

  it("is sent again to a new port (a reloaded window), which replaces the old", () => {
    const { control, port } = setup();
    const second = new Recorder();
    control.attach(second);
    expect(second.of("hello")).toHaveLength(1);
    control.handle({ type: "stats-request" });
    expect(second.of("stats")).toHaveLength(1);
    expect(port.of("stats")).toHaveLength(0);
  });
});

describe("commands", () => {
  it("acknowledges set-param with the window's own id, after the host's ack arrives", () => {
    const { send, port, run } = setup();
    send(41, { kind: "set-param", name: "throttle", value: 0.25 });
    expect(port.replies(41)).toHaveLength(0); // the render thread hasn't taken it yet
    run(10);
    const [reply] = port.replies(41);
    expect(reply.error).toBeUndefined();
    expect(reply.ack).toMatchObject({ kind: "set-param", outcome: "ok", generation: 1 });
  });

  it("really sets the parameter: the next snapshot shows it", () => {
    const { send, run, control, port, clock } = setup();
    send(1, { kind: "set-param", name: "throttle", value: 0.6 });
    run(10);
    clock.step(10);
    control.handle({ type: "snapshot-request" });
    const snap = decodeSnapshot(port.of("snapshot")[0].bytes);
    expect(snap.instruments.throttle).toBeCloseTo(0.6, 12);
  });

  it("echoes each command's id even when several are in flight", () => {
    const { send, port, run } = setup();
    send(7, { kind: "set-param", name: "ignition", value: 1 });
    send(3, { kind: "set-param", name: "throttle", value: 0.2 });
    send(99, { kind: "set-param", name: "starter", value: 1 });
    run(10);
    expect(port.of("reply").map((r) => r.id).sort((a, b) => a - b)).toEqual([3, 7, 99]);
    for (const r of port.of("reply")) expect(r.ack?.kind).toBe("set-param");
  });

  it("replies with an error, at once, for a parameter that doesn't exist", () => {
    const { send, port } = setup();
    send(5, { kind: "set-param", name: "no_such", value: 1 });
    expect(port.replies(5)[0].error).toMatchObject({ code: "UNKNOWN_PARAM" });
    expect(port.replies(5)[0].ack).toBeUndefined();
  });

  it("refuses a value that is not a finite number before the add-on sees it", () => {
    const { send, port } = setup();
    send(6, { kind: "set-param", name: "throttle", value: Number.NaN });
    send(8, { kind: "set-param", name: "throttle", value: "0.5" as unknown as number });
    expect(port.replies(6)[0].error?.code).toBe("BAD_VALUE");
    expect(port.replies(8)[0].error?.code).toBe("BAD_VALUE");
  });

  it("passes a host's error up with the C name as its code", () => {
    const { send, port } = setup();
    for (let i = 0; i < 256; i++) send(1000 + i, { kind: "set-param", name: "throttle", value: 0.1 });
    send(2000, { kind: "set-param", name: "throttle", value: 0.1 });
    expect(port.replies(2000)[0].error).toMatchObject({ code: "RB_HOST_ERR_FULL" });
  });

  it("reports an unknown command kind as an error", () => {
    const { send, port } = setup();
    send(9, { kind: "explode" } as unknown as Command);
    expect(port.replies(9)[0].error?.code).toBe("BAD_COMMAND");
  });

  it("selects a device, replying without an ack, and refuses one that doesn't exist", () => {
    const { send, port } = setup();
    send(10, { kind: "select-device", index: 0 });
    expect(port.replies(10)).toEqual([{ type: "reply", id: 10 }]);
    send(11, { kind: "select-device", index: 4242 });
    expect(port.replies(11)[0].error?.code).toBe("RB_HOST_ERR_ARG");
    send(12, { kind: "select-device", index: 1.5 });
    expect(port.replies(12)[0].error?.code).toBe("BAD_VALUE");
  });

  it("ignores malformed messages and says so in the log, without throwing", () => {
    const { control, logs, port } = setup();
    const before = port.messages.length;
    for (const junk of [null, 5, "x", {}, { type: "nope" }, { type: "command", id: "a", command: null }]) {
      expect(() => control.handle(junk)).not.toThrow();
    }
    expect(port.messages.length).toBe(before);
    expect(logs.length).toBeGreaterThanOrEqual(5);
  });
});

describe("a rebuild (load-preset)", () => {
  it("prepares without blocking, swaps when ready, and replies with the swap's ack", () => {
    const { send, port, run, builds } = setup();
    send(20, { kind: "load-preset", preset: "v8-cross" });
    expect(port.replies(20)).toHaveLength(0);
    run(200);
    const [reply] = port.replies(20);
    expect(reply.error).toBeUndefined();
    expect(reply.ack).toMatchObject({ kind: "swap", outcome: "ok", reset: true, generation: 2 });
    expect(builds).toEqual([{ preset: "v8-cross" }]);
  });

  it("passes the quality on, and keeps it in what is autosaved", () => {
    const { send, run, builds } = setup();
    send(21, { kind: "load-preset", preset: "i6", quality: "high" });
    run(200);
    expect(builds).toEqual([{ preset: "i6", quality: "high" }]);
  });

  it("changes what the snapshots show: a new generation, the new engine's cylinders", () => {
    const { send, run, control, port, clock } = setup();
    send(22, { kind: "load-preset", preset: "v8-flat" });
    run(200);
    clock.step(10);
    control.handle({ type: "snapshot-request" });
    const snap = decodeSnapshot(port.of("snapshot").at(-1)!.bytes);
    expect(snap.generation).toBe(2);
    expect(snap.cylinder).toHaveLength(8);
  });

  it("says hello with the new preset to a window that connects after it", () => {
    const { send, run, control } = setup();
    send(23, { kind: "load-preset", preset: "boxer4" });
    run(200);
    const late = new Recorder();
    control.attach(late);
    expect(late.of("hello")[0].preset).toBe("boxer4");
    expect(late.of("hello")[0].generation).toBe(2);
  });

  it("answers a preset that doesn't exist at once, and starts nothing", () => {
    const { send, port, run, builds } = setup();
    send(24, { kind: "load-preset", preset: "no-such" });
    expect(port.replies(24)[0].error?.code).toBe("UNKNOWN_PRESET");
    run(200);
    expect(builds).toEqual([]);
  });

  it("refuses a quality it doesn't know", () => {
    const { send, port } = setup();
    send(25, { kind: "load-preset", preset: "i6", quality: "ludicrous" as never });
    expect(port.replies(25)[0].error?.code).toBe("BAD_QUALITY");
  });

  it("is busy while another rebuild is in progress", () => {
    const { send, port, run } = setup();
    send(26, { kind: "load-preset", preset: "i6" });
    send(27, { kind: "load-preset", preset: "v8-flat" });
    expect(port.replies(27)[0].error?.code).toBe("BUSY");
    run(200);
    expect(port.replies(26)[0].ack?.outcome).toBe("ok");
    send(28, { kind: "load-preset", preset: "v8-flat" });
    run(200);
    expect(port.replies(28)[0].ack?.outcome).toBe("ok");
  });

  it("leaves the running engine alone when the preparation fails, and says why", () => {
    const { send, port, run, builds, control, clock } = setup({ failPresets: ["i6"] });
    send(29, { kind: "load-preset", preset: "i6" });
    run(200);
    expect(port.replies(29)[0].error).toMatchObject({ code: "PREPARE_FAILED" });
    expect(port.replies(29)[0].error?.message).toMatch(/i6/);
    expect(builds).toEqual([]);
    clock.step(10);
    control.handle({ type: "snapshot-request" });
    expect(decodeSnapshot(port.of("snapshot")[0].bytes).generation).toBe(1);
    // and a later rebuild still works
    send(30, { kind: "load-preset", preset: "v8-flat" });
    run(200);
    expect(port.replies(30)[0].ack?.outcome).toBe("ok");
  });

  it("gives up a preparation that never finishes", () => {
    const addon = createFakeAddon({ now: () => 1000, prepareMs: 10 ** 9 });
    let t = 1000;
    const port = new Recorder();
    const control = new EngineControl({ addon, config: config(), onBuild: () => {}, log: () => {}, now: () => t });
    control.start();
    control.attach(port);
    control.handle({ type: "command", id: 31, command: { kind: "load-preset", preset: "i6" } });
    control.pump();
    expect(port.replies(31)).toHaveLength(0);
    t += PREPARE_TIMEOUT_MS + 1;
    control.pump();
    expect(port.replies(31)[0].error?.code).toBe("PREPARE_TIMEOUT");
  });

  it("does not send the rebuild's reply to a window that has gone, but still finishes it", () => {
    const { send, run, control, port, builds } = setup();
    send(32, { kind: "load-preset", preset: "i6" });
    const next = new Recorder();
    control.attach(next); // the window reloaded
    run(200);
    expect(port.replies(32)).toHaveLength(0);
    expect(next.replies(32)).toHaveLength(0);
    expect(builds).toEqual([{ preset: "i6" }]);
  });
});

describe("the app's driver defaults", () => {
  const held = (s: ReturnType<typeof setup>) => {
    s.clock.step(10);
    s.control.handle({ type: "snapshot-request" });
    return decodeSnapshot(s.port.of("snapshot").at(-1)!.bytes).instruments.held;
  };

  it("builds the first engine with a free crank (hold 0), not the core's dyno-held default", () => {
    expect(held(setup())).toBe(0);
  });

  it("builds every engine after a rebuild the same way", () => {
    const s = setup();
    s.send(60, { kind: "load-preset", preset: "v8-cross" });
    s.run(200);
    expect(held(s)).toBe(0);
  });

  it("asks for nothing the add-on doesn't have", () => {
    const real = createFakeAddon();
    const addon: Addon = { ...real, params: () => real.params().filter((p) => p.name !== "hold") };
    const control = new EngineControl({ addon, config: config(), onBuild: () => {}, log: () => {} });
    expect(() => control.start()).not.toThrow();
  });
});

describe("snapshots and statistics", () => {
  it("sends the newest snapshot as a transferred ArrayBuffer, with its numbers", () => {
    const { control, port, clock } = setup();
    clock.step(16);
    control.handle({ type: "snapshot-request" });
    const [msg] = port.of("snapshot");
    const index = port.messages.indexOf(msg);
    expect(port.transfers[index]).toEqual([msg.bytes]);
    const decoded = decodeSnapshot(msg.bytes);
    expect(msg.sequence).toBe(decoded.sequence);
    expect(msg.generation).toBe(decoded.generation);
    expect(msg.audioFrame).toBe(decoded.audio_frame);
  });

  it("sends nothing when there is no snapshot newer than the last sent", () => {
    const { control, port, clock } = setup();
    clock.step(16);
    control.handle({ type: "snapshot-request" });
    control.handle({ type: "snapshot-request" });
    expect(port.of("snapshot")).toHaveLength(1);
  });

  it("sends the host's statistics", () => {
    const { control, port } = setup();
    control.handle({ type: "stats-request" });
    expect(port.of("stats")[0].stats.forbiddenCalls).toBe(0);
  });

  it("sends nothing to a window that isn't there", () => {
    const { control, port, clock } = setup();
    control.detach();
    clock.step(16);
    control.handle({ type: "snapshot-request" });
    expect(port.of("snapshot")).toHaveLength(0);
  });
});

describe("failing and stopping", () => {
  it("throws from start() when the host can't be made, for the entry to report", () => {
    const real = createFakeAddon();
    const addon: Addon = {
      ...real,
      createHost: () => {
        throw addonError("RB_HOST_ERR_DEVICE", "no sound device");
      },
    };
    const control = new EngineControl({ addon, config: config(), onBuild: () => {}, log: () => {} });
    expect(() => control.start()).toThrow(/no sound device/);
    expect(control.running).toBe(false);
  });

  it("throws from start() for a preset the add-on doesn't have", () => {
    const control = new EngineControl({
      addon: createFakeAddon(),
      config: config({ preset: "nope" }),
      onBuild: () => {},
      log: () => {},
    });
    expect(() => control.start()).toThrow(/nope/);
  });

  it("keeps going when a host call throws inside pump()", () => {
    const real = createFakeAddon();
    let host: Host;
    const addon: Addon = {
      ...real,
      createHost: (c) => {
        host = real.createHost(c);
        return host;
      },
    };
    const logs: string[] = [];
    const control = new EngineControl({ addon, config: config(), onBuild: () => {}, log: (_l, m) => logs.push(m) });
    control.start();
    host!.destroy(); // everything now throws DESTROYED
    expect(() => control.pump()).not.toThrow();
    expect(logs.join()).toMatch(/pump failed/);
  });

  it("destroys the host on shutdown, once", () => {
    const real = createFakeAddon();
    let host: Host;
    let destroyed = 0;
    const addon: Addon = {
      ...real,
      createHost: (c) => {
        host = real.createHost(c);
        const destroy = host.destroy.bind(host);
        host.destroy = () => {
          destroyed++;
          destroy();
        };
        return host;
      },
    };
    const control = new EngineControl({ addon, config: config(), onBuild: () => {}, log: () => {} });
    control.start();
    control.shutdown();
    control.shutdown();
    expect(destroyed).toBe(1);
    expect(control.running).toBe(false);
    expect(() => control.handle({ type: "stats-request" })).not.toThrow();
  });
});
