/*
 * The window's client for the engine channel, alone (with a hand-made port) and against the
 * real control loop over a real MessageChannel (structured clone and ArrayBuffer transfer).
 */
import { MessageChannel } from "node:worker_threads";
import { describe, expect, it } from "vitest";
import { EngineControl } from "../src/engine/control";
import { createFakeAddon } from "../src/engine/fake-addon";
import { CommandError, EngineClient, type ClientPort, type Hello } from "../src/ui/lib/engine-client";
import type { FromEngine, ToEngine } from "../src/shared/protocol";
import { INSTRUMENT_NAMES, encodeSnapshot, type Snapshot } from "../src/shared/snapshot-v2.generated";

class HandPort implements ClientPort {
  sent: ToEngine[] = [];
  closed = false;
  onmessage: ((event: { data: unknown }) => void) | null = null;
  started = false;
  postMessage(message: ToEngine): void {
    this.sent.push(message);
  }
  start(): void {
    this.started = true;
  }
  close(): void {
    this.closed = true;
  }
  emit(data: FromEngine | unknown): void {
    this.onmessage?.({ data });
  }
}

function validSnapshotBytes(overrides: Partial<Snapshot> = {}): ArrayBuffer {
  const instruments = Object.fromEntries(INSTRUMENT_NAMES.map((n) => [n, 1])) as Snapshot["instruments"];
  return encodeSnapshot({
    magic: 0, version: 0, size: 0, cylinders: 1, cylinder_fields: 0, reserved0: 0, reserved1: 0,
    build_id: 7n, generation: 1, sequence: 3, physics_tick: 10, audio_frame: 480, cycle_index: 0,
    physics_hz: 192000, audio_hz: 48000, phase_rad: 1, speed_rad_s: 100,
    instruments,
    cylinder: [{ pressure: 1e5, temperature: 300, burn: 0, intake_lift: 0, exhaust_lift: 0 }],
    ...overrides,
  });
}

describe("the engine client", () => {
  it("sends a command with its own id and resolves with the ack of the reply", async () => {
    const port = new HandPort();
    const client = new EngineClient();
    client.attach(port);
    const promise = client.command({ kind: "set-param", name: "throttle", value: 0.5 });
    const sent = port.sent[0] as Extract<ToEngine, { type: "command" }>;
    expect(sent.command).toEqual({ kind: "set-param", name: "throttle", value: 0.5 });
    const ack = { requestId: 1, kind: "set-param", outcome: "ok", code: 0, generation: 1, reset: false, physicsTick: 1, audioFrame: 1 };
    port.emit({ type: "reply", id: sent.id, ack });
    await expect(promise).resolves.toEqual(ack);
  });

  it("gives each command its own id and matches replies out of order", async () => {
    const port = new HandPort();
    const client = new EngineClient();
    client.attach(port);
    const a = client.command({ kind: "set-param", name: "ignition", value: 1 });
    const b = client.command({ kind: "set-param", name: "throttle", value: 1 });
    const [first, second] = port.sent as Extract<ToEngine, { type: "command" }>[];
    expect(first.id).not.toBe(second.id);
    port.emit({ type: "reply", id: second.id, ack: { kind: "B" } });
    port.emit({ type: "reply", id: first.id, ack: { kind: "A" } });
    expect(await a).toMatchObject({ kind: "A" });
    expect(await b).toMatchObject({ kind: "B" });
  });

  it("rejects with a CommandError carrying the engine's code", async () => {
    const port = new HandPort();
    const client = new EngineClient();
    client.attach(port);
    const promise = client.command({ kind: "load-preset", preset: "x" });
    port.emit({ type: "reply", id: (port.sent[0] as { id: number }).id, error: { code: "UNKNOWN_PRESET", message: 'no preset "x"' } });
    await expect(promise).rejects.toMatchObject({ name: "CommandError", code: "UNKNOWN_PRESET", message: 'no preset "x"' });
  });

  it("resolves with nothing for a reply that has neither ack nor error", async () => {
    const port = new HandPort();
    const client = new EngineClient();
    client.attach(port);
    const promise = client.command({ kind: "select-device", index: 0 });
    port.emit({ type: "reply", id: (port.sent[0] as { id: number }).id });
    await expect(promise).resolves.toBeUndefined();
  });

  it("ignores a reply nobody is waiting for", () => {
    const port = new HandPort();
    const client = new EngineClient();
    client.attach(port);
    expect(() => port.emit({ type: "reply", id: 999 })).not.toThrow();
  });

  it("rejects a command at once when there is no port", async () => {
    await expect(new EngineClient().command({ kind: "select-device", index: 0 })).rejects.toBeInstanceOf(CommandError);
  });

  it("fails the waiting commands with DETACHED when the port goes away, and says why", async () => {
    const port = new HandPort();
    const reasons: string[] = [];
    const client = new EngineClient({ detached: (r) => reasons.push(r) });
    client.attach(port);
    const promise = client.command({ kind: "set-param", name: "throttle", value: 1 });
    client.detach("the engine restarted");
    await expect(promise).rejects.toMatchObject({ code: "DETACHED", message: "the engine restarted" });
    expect(reasons).toEqual(["the engine restarted"]);
    expect(port.closed).toBe(true);
    expect(client.connected).toBe(false);
    port.emit({ type: "reply", id: 1 }); // a late message from the old port does nothing
  });

  it("detaches the old port when a new one is attached", async () => {
    const old = new HandPort();
    const next = new HandPort();
    const client = new EngineClient();
    client.attach(old);
    const waiting = client.command({ kind: "set-param", name: "throttle", value: 1 });
    client.attach(next);
    await expect(waiting).rejects.toMatchObject({ code: "DETACHED" });
    expect(old.closed).toBe(true);
    expect(next.started).toBe(true);
    expect(client.connected).toBe(true);
  });

  it("starts the port it is given", () => {
    const port = new HandPort();
    new EngineClient().attach(port);
    expect(port.started).toBe(true);
  });

  it("asks for snapshots and statistics", () => {
    const port = new HandPort();
    const client = new EngineClient();
    client.attach(port);
    client.requestSnapshot();
    client.requestStats();
    expect(port.sent).toEqual([{ type: "snapshot-request" }, { type: "stats-request" }]);
    expect(() => new EngineClient().requestSnapshot()).not.toThrow(); // no port: nothing
  });

  it("passes on hello, state and stats", () => {
    const port = new HandPort();
    const seen: unknown[] = [];
    const client = new EngineClient({
      hello: (h) => seen.push(["hello", h.preset]),
      state: (s, m) => seen.push(["state", s, m]),
      stats: (s) => seen.push(["stats", s.underruns]),
    });
    client.attach(port);
    port.emit({ type: "hello", preset: "i4" } as Hello);
    port.emit({ type: "state", state: "error", message: "boom" });
    port.emit({ type: "stats", stats: { underruns: 2 } });
    expect(seen).toEqual([["hello", "i4"], ["state", "error", "boom"], ["stats", 2]]);
  });

  it("decodes a snapshot and passes it on with its numbers", () => {
    const port = new HandPort();
    const got: [number, number][] = [];
    const client = new EngineClient({ snapshot: (s, meta) => got.push([s.sequence, meta.audioFrame]) });
    client.attach(port);
    port.emit({ type: "snapshot", bytes: validSnapshotBytes(), sequence: 3, generation: 1, audioFrame: 480 });
    expect(got).toEqual([[3, 480]]);
  });

  it("reports bytes the decoder refuses, and goes on", () => {
    const port = new HandPort();
    const refused: string[] = [];
    const good: number[] = [];
    const client = new EngineClient({ invalidSnapshot: (e) => refused.push(e.message), snapshot: (s) => good.push(s.sequence) });
    client.attach(port);
    const bad = validSnapshotBytes();
    new DataView(bad).setUint32(0, 0, true);
    port.emit({ type: "snapshot", bytes: bad, sequence: 1, generation: 1, audioFrame: 0 });
    port.emit({ type: "snapshot", bytes: validSnapshotBytes(), sequence: 2, generation: 1, audioFrame: 0 });
    expect(refused).toHaveLength(1);
    expect(good).toEqual([3]);
  });

  it("ignores messages that aren't objects or have no known type", () => {
    const port = new HandPort();
    const client = new EngineClient();
    client.attach(port);
    for (const junk of [null, 4, "x", { type: "nope" }, {}]) expect(() => port.emit(junk)).not.toThrow();
  });
});

describe("the client against the real control loop over a real MessageChannel", () => {
  it("gets a hello, a reply to a command and a snapshot that decodes", async () => {
    let t = 1000;
    const control = new EngineControl({
      addon: createFakeAddon({ now: () => t }),
      config: { addon: "fake", backend: "null", build: { preset: "v8-cross" } },
      onBuild: () => {},
      log: () => {},
      now: () => t,
    });
    control.start();
    const { port1: enginePort, port2: windowPort } = new MessageChannel();
    enginePort.on("message", (m) => control.handle(m));

    let onHello!: (h: Hello) => void;
    let onSnapshot!: (s: Snapshot) => void;
    const hello = new Promise<Hello>((resolve) => (onHello = resolve));
    const snapshot = new Promise<Snapshot>((resolve) => (onSnapshot = resolve));
    const client = new EngineClient({ hello: (h) => onHello(h), snapshot: (s) => onSnapshot(s) });
    client.attach(adapt(windowPort));
    control.attach({ post: (message, transfer) => enginePort.postMessage(message, transfer as never) });
    expect((await hello).preset).toBe("v8-cross");

    const reply = client.command({ kind: "set-param", name: "throttle", value: 0.4 });
    await new Promise((r) => setTimeout(r, 5));
    control.pump();
    expect((await reply)?.outcome).toBe("ok");

    t += 20;
    client.requestSnapshot();
    const s = await snapshot;
    expect(s.cylinders).toBe(8);
    expect(s.instruments.throttle).toBeCloseTo(0.4, 12);

    client.detach("test over");
    enginePort.close();
    control.shutdown();
  });
});

/** A Node MessagePort as the client's port (the browser's has the same shape). */
function adapt(port: import("node:worker_threads").MessagePort): ClientPort {
  const adapter: ClientPort = {
    postMessage: (m) => port.postMessage(m),
    onmessage: null,
    start: () => void port.on("message", (data) => adapter.onmessage?.({ data })),
    close: () => void port.close(),
  };
  return adapter;
}
