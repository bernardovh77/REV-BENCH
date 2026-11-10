/*
 * The interface's engine store: the connection, the status, the frame loop, the commands.
 */
import { describe, expect, it } from "vitest";
import { ENGINE_PORT_MESSAGE, type EngineStatus, type RevBenchBridge } from "../src/shared/bridge";
import type { FromEngine, ToEngine } from "../src/shared/protocol";
import { INSTRUMENT_NAMES, encodeSnapshot, type Snapshot } from "../src/shared/snapshot-v2.generated";
import { EngineStore, type PortEvent, type WindowLike } from "../src/ui/lib/engine-store.svelte";

/** A DOM MessagePort made by hand. */
class FakeDomPort {
  sent: ToEngine[] = [];
  closed = false;
  started = false;
  onmessage: ((event: { data: unknown }) => void) | null = null;
  postMessage(m: ToEngine): void {
    this.sent.push(m);
  }
  start(): void {
    this.started = true;
  }
  close(): void {
    this.closed = true;
  }
  emit(data: FromEngine): void {
    this.onmessage?.({ data });
  }
}

const hello = (preset = "i4"): FromEngine => ({
  type: "hello", protocol: 1, addonVersion: "fake", snapshotVersion: 2, presets: [{ index: 0, key: "i4", name: "2.0 L inline-4", quality: "balanced" }],
  params: [], telemetry: [], devices: [], sampleRate: 48000, preset, generation: 1,
});

function snapshotBytes(rpm = 800, sequence = 1): ArrayBuffer {
  const instruments = Object.fromEntries(INSTRUMENT_NAMES.map((n) => [n, 0])) as Snapshot["instruments"];
  instruments.rpm = rpm;
  return encodeSnapshot({
    magic: 0, version: 0, size: 0, cylinders: 1, cylinder_fields: 0, reserved0: 0, reserved1: 0, build_id: 1n,
    generation: 1, sequence, physics_tick: 1, audio_frame: 1, cycle_index: 0, physics_hz: 1, audio_hz: 48000, phase_rad: 0, speed_rad_s: 0,
    instruments, cylinder: [{ pressure: 1, temperature: 1, burn: 0, intake_lift: 0, exhaust_lift: 0 }],
  });
}

function setup(withBridge = true) {
  let onMessage: ((event: PortEvent) => void) | null = null;
  const win = {
    addEventListener: (_t: "message", l: (e: PortEvent) => void) => void (onMessage = l),
    removeEventListener: () => void (onMessage = null),
  } satisfies WindowLike;
  let statusListener: ((s: EngineStatus) => void) | null = null;
  const calls: string[] = [];
  const bridge: RevBenchBridge = {
    platform: "win32",
    versions: { electron: "45", chrome: "140" },
    onEngineStatus: (cb) => {
      statusListener = cb;
      return () => void (statusListener = null);
    },
    requestEnginePort: () => void calls.push("requestEnginePort"),
    restartEngine: () => void calls.push("restartEngine"),
  };
  const frames: (() => void)[] = [];
  let cancelled = 0;
  const store = new EngineStore({
    bridge: withBridge ? bridge : undefined,
    window: win,
    requestFrame: (cb) => frames.push(cb),
    cancelFrame: () => void cancelled++,
    statsEveryFrames: 3,
  });
  const givePort = (port: FakeDomPort, source: unknown = win, channel: unknown = ENGINE_PORT_MESSAGE) =>
    onMessage!({ data: { channel, restarts: 0 }, source, ports: [port as unknown as MessagePort] });
  const givePortless = () => onMessage!({ data: { channel: ENGINE_PORT_MESSAGE, restarts: 0 }, source: win, ports: [] });
  const frame = () => frames.shift()!();
  return { store, win, calls, givePort, givePortless, frame, status: (s: EngineStatus) => statusListener!(s), frames, cancelled: () => cancelled, hasListener: () => onMessage !== null };
}

describe("starting", () => {
  it("listens for the engine's port, asks the main process for one and starts the frame loop", () => {
    const { store, calls, frames, hasListener } = setup();
    store.start();
    expect(hasListener()).toBe(true);
    expect(calls).toEqual(["requestEnginePort"]);
    expect(frames).toHaveLength(1);
  });

  it("is 'no-shell' without the application's bridge (a plain browser)", () => {
    const { store } = setup(false);
    store.start();
    expect(store.summary).toBe("no-shell");
  });

  it("is 'starting' until the main process says more", () => {
    const { store } = setup();
    store.start();
    expect(store.summary).toBe("starting");
  });
});

describe("the connection", () => {
  it("is 'connecting' when the process runs but has not said hello, then 'running'", () => {
    const { store, status, givePort } = setup();
    store.start();
    status({ state: "running", restarts: 0 });
    expect(store.summary).toBe("connecting");
    const port = new FakeDomPort();
    givePort(port);
    port.emit(hello("i4"));
    expect(store.summary).toBe("running");
    expect(store.preset).toBe("i4");
    expect(store.hello?.presets).toHaveLength(1);
  });

  it("takes a port only from the page itself, and only with the right channel name", () => {
    const { store, givePort } = setup();
    store.start();
    const port = new FakeDomPort();
    givePort(port, {} /* another window */);
    givePort(port, undefined, "something-else");
    expect(port.started).toBe(false);
    expect(store.connected).toBe(false);
  });

  it("ignores a port message without a port", () => {
    const { store, givePortless } = setup();
    store.start();
    expect(() => givePortless()).not.toThrow();
    expect(store.connected).toBe(false);
  });

  it("drops the connection when the engine process restarts, and says so", () => {
    const { store, status, givePort } = setup();
    store.start();
    status({ state: "running", restarts: 0 });
    const port = new FakeDomPort();
    givePort(port);
    port.emit(hello());
    status({ state: "restarting", restarts: 0, message: "the engine process exited (code 1). Restarting." });
    expect(store.connected).toBe(false);
    expect(store.summary).toBe("restarting");
    expect(port.closed).toBe(true);
    expect(store.status?.message).toMatch(/exited/);
  });

  it("connects again to the port the restarted engine brings", () => {
    const { store, status, givePort } = setup();
    store.start();
    const first = new FakeDomPort();
    givePort(first);
    first.emit(hello());
    status({ state: "restarting", restarts: 0 });
    status({ state: "starting", restarts: 1 });
    const second = new FakeDomPort();
    givePort(second);
    second.emit(hello("i4"));
    status({ state: "running", restarts: 1 });
    expect(store.summary).toBe("running");
  });

  it("is 'failed' when the main process gives up", () => {
    const { store, status } = setup();
    store.start();
    status({ state: "failed", restarts: 5, message: "stopped 6 times" });
    expect(store.summary).toBe("failed");
  });

  it("asks the main process to try again", () => {
    const { store, calls } = setup();
    store.start();
    store.restartEngine();
    expect(calls).toContain("restartEngine");
  });
});

describe("the frame loop", () => {
  it("asks for a snapshot every frame and for statistics every few", () => {
    const { store, givePort, frame } = setup();
    store.start();
    const port = new FakeDomPort();
    givePort(port);
    frame();
    frame();
    frame();
    expect(port.sent.map((m) => m.type)).toEqual(["snapshot-request", "snapshot-request", "snapshot-request", "stats-request"]);
  });

  it("keeps the newest snapshot it has, and counts those the decoder refuses", () => {
    const { store, givePort } = setup();
    store.start();
    const port = new FakeDomPort();
    givePort(port);
    port.emit({ type: "snapshot", bytes: snapshotBytes(900), sequence: 1, generation: 1, audioFrame: 1 });
    expect(store.snapshot?.instruments.rpm).toBe(900);
    port.emit({ type: "snapshot", bytes: new ArrayBuffer(10), sequence: 2, generation: 1, audioFrame: 2 });
    expect(store.refusedSnapshots).toBe(1);
    expect(store.snapshot?.instruments.rpm).toBe(900);
  });

  it("keeps the statistics", () => {
    const { store, givePort } = setup();
    store.start();
    const port = new FakeDomPort();
    givePort(port);
    port.emit({ type: "stats", stats: { underruns: 3 } as never });
    expect(store.stats?.underruns).toBe(3);
  });

  it("stops on dispose: no listener, no frame, no port", () => {
    const { store, givePort, hasListener, cancelled } = setup();
    store.start();
    const port = new FakeDomPort();
    givePort(port);
    store.dispose();
    expect(hasListener()).toBe(false);
    expect(cancelled()).toBe(1);
    expect(port.closed).toBe(true);
  });
});

describe("commands", () => {
  async function connected() {
    const s = setup();
    s.store.start();
    const port = new FakeDomPort();
    s.givePort(port);
    port.emit(hello());
    const reply = (index: number, body: Partial<Extract<FromEngine, { type: "reply" }>>) =>
      port.emit({ type: "reply", id: (port.sent[index] as { id: number }).id, ...body });
    return { ...s, port, reply };
  }

  it("sets a parameter by name", async () => {
    const { store, port, reply } = await connected();
    const done = store.setParam("throttle", 0.5);
    expect(port.sent[0]).toMatchObject({ type: "command", command: { kind: "set-param", name: "throttle", value: 0.5 } });
    reply(0, { ack: {} as never });
    await done;
    expect(store.lastError).toBeNull();
  });

  it("keeps the engine's refusal as the last error, with its code", async () => {
    const { store, reply } = await connected();
    const done = store.setParam("nope", 1);
    reply(0, { error: { code: "UNKNOWN_PARAM", message: 'no parameter "nope"' } });
    await done;
    expect(store.lastError).toBe('no parameter "nope" (UNKNOWN_PARAM)');
  });

  it("clears the error with the next command that works", async () => {
    const { store, reply } = await connected();
    const first = store.setParam("nope", 1);
    reply(0, { error: { code: "UNKNOWN_PARAM", message: "x" } });
    await first;
    const second = store.setParam("throttle", 0);
    reply(1, {});
    await second;
    expect(store.lastError).toBeNull();
  });

  it("loads a preset: rebuilding until the swap, then the new preset is the engine's", async () => {
    const { store, reply, port } = await connected();
    const done = store.loadPreset("v8-cross");
    expect(store.rebuilding).toBe(true);
    expect(port.sent[0]).toMatchObject({ command: { kind: "load-preset", preset: "v8-cross" } });
    reply(0, { ack: {} as never });
    await done;
    expect(store.rebuilding).toBe(false);
    expect(store.preset).toBe("v8-cross");
  });

  it("keeps the old preset when the rebuild fails, and says why", async () => {
    const { store, reply } = await connected();
    const done = store.loadPreset("v8-cross");
    reply(0, { error: { code: "PREPARE_FAILED", message: "no memory" } });
    await done;
    expect(store.preset).toBe("i4");
    expect(store.lastError).toContain("PREPARE_FAILED");
    expect(store.rebuilding).toBe(false);
  });

  it("does not start a second rebuild while one is going", async () => {
    const { store, port, reply } = await connected();
    const first = store.loadPreset("v8-cross");
    await store.loadPreset("i6");
    expect(port.sent).toHaveLength(1);
    reply(0, {});
    await first;
  });

  it("says plainly that the engine isn't there when a command is sent to nobody", async () => {
    const { store } = setup();
    store.start();
    await store.setParam("throttle", 1);
    expect(store.lastError).toBe("the engine is not connected");
  });

  it("fails a command in flight, without a code, when the engine restarts", async () => {
    const { store, status } = await connected();
    const done = store.setParam("throttle", 1);
    status({ state: "restarting", restarts: 0, message: "the engine process exited. Restarting." });
    await done;
    expect(store.lastError).toBe("the engine process exited. Restarting.");
  });
});
