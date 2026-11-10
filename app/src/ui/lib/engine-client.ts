/*
 * engine-client.ts - the window's end of the engine channel (src/shared/protocol.ts). It sends
 * commands and gets their replies as promises, asks for snapshots and statistics, decodes the
 * snapshots with the generated decoder, and tells its owner what arrives. It knows nothing of
 * Svelte, Electron or the frame loop: the port is anything with postMessage and onmessage.
 */
import type { Ack, HostStats } from "../../shared/addon";
import type { Command, EngineState, FromEngine, ToEngine } from "../../shared/protocol";
import { SnapshotError, decodeSnapshot, type Snapshot } from "../../shared/snapshot-v2.generated";

/** A MessagePort, as far as the client uses it. */
export interface ClientPort {
  postMessage(message: ToEngine): void;
  onmessage: ((event: { data: unknown }) => void) | null;
  start?(): void;
  close(): void;
}

/** A DOM MessagePort as the client's port. */
export function fromMessagePort(port: MessagePort): ClientPort {
  const adapter: ClientPort = {
    postMessage: (message) => port.postMessage(message),
    get onmessage() {
      return handler;
    },
    set onmessage(next) {
      handler = next;
      port.onmessage = next ? (event) => next({ data: event.data }) : null;
    },
    start: () => port.start(),
    close: () => port.close(),
  };
  let handler: ClientPort["onmessage"] = null;
  return adapter;
}

export type Hello =Extract<FromEngine, { type: "hello" }>;

export interface SnapshotMeta {
  sequence: number;
  generation: number;
  audioFrame: number;
}

export interface ClientEvents {
  hello?(hello: Hello): void;
  snapshot?(snapshot: Snapshot, meta: SnapshotMeta): void;
  stats?(stats: HostStats): void;
  state?(state: EngineState, message?: string): void;
  /** the port went away (replaced, or the engine died) */
  detached?(reason: string): void;
  /** the engine sent bytes the decoder refuses */
  invalidSnapshot?(error: SnapshotError): void;
}

/** A command the engine refused, or that never got an answer. */
export class CommandError extends Error {
  constructor(
    readonly code: string,
    message: string,
  ) {
    super(message);
    this.name = "CommandError";
  }
}

interface Waiting {
  resolve(ack: Ack | undefined): void;
  reject(error: CommandError): void;
}

export class EngineClient {
  private port: ClientPort | null = null;
  private nextId = 1;
  private readonly waiting = new Map<number, Waiting>();

  constructor(private readonly events: ClientEvents = {}) {}

  get connected(): boolean {
    return this.port !== null;
  }

  /** Starts using a new port; commands still waiting on the old one fail with DETACHED. */
  attach(port: ClientPort): void {
    if (this.port) this.detach("replaced by a new connection");
    this.port = port;
    port.onmessage = (event) => this.receive(event.data);
    port.start?.();
  }

  detach(reason: string): void {
    const port = this.port;
    if (!port) return;
    this.port = null;
    port.onmessage = null;
    port.close();
    for (const [id, waiting] of this.waiting) {
      this.waiting.delete(id);
      waiting.reject(new CommandError("DETACHED", reason));
    }
    this.events.detached?.(reason);
  }

  /** Sends a command. Resolves with the host's ack (none for a device switch); rejects with a CommandError. */
  command(command: Command): Promise<Ack | undefined> {
    const port = this.port;
    if (!port) return Promise.reject(new CommandError("DETACHED", "the engine is not connected"));
    const id = this.nextId++;
    return new Promise((resolve, reject) => {
      this.waiting.set(id, { resolve, reject });
      port.postMessage({ type: "command", id, command });
    });
  }

  requestSnapshot(): void {
    this.port?.postMessage({ type: "snapshot-request" });
  }

  requestStats(): void {
    this.port?.postMessage({ type: "stats-request" });
  }

  private receive(data: unknown): void {
    if (typeof data !== "object" || data === null) return;
    const message = data as FromEngine;
    switch (message.type) {
      case "hello":
        this.events.hello?.(message);
        return;
      case "state":
        this.events.state?.(message.state, message.message);
        return;
      case "stats":
        this.events.stats?.(message.stats);
        return;
      case "snapshot":
        try {
          this.events.snapshot?.(decodeSnapshot(message.bytes), {
            sequence: message.sequence,
            generation: message.generation,
            audioFrame: message.audioFrame,
          });
        } catch (e) {
          if (e instanceof SnapshotError) this.events.invalidSnapshot?.(e);
          else throw e;
        }
        return;
      case "reply": {
        const waiting = this.waiting.get(message.id);
        if (!waiting) return;
        this.waiting.delete(message.id);
        if (message.error) waiting.reject(new CommandError(message.error.code, message.error.message));
        else waiting.resolve(message.ack);
        return;
      }
    }
  }
}
