/*
 * engine-store.svelte.ts - what the interface knows about the engine, as Svelte state: the
 * process's status (from the main process), the connection (from the engine client), the
 * engine's offer (hello), the newest snapshot and statistics. Panels read it and call its
 * methods; nothing else talks to the engine.
 *
 * The window never waits for the engine: it asks for a snapshot every animation frame and shows
 * the newest it has (docs/app-design.md section 4).
 */
import type { HostStats } from "../../shared/addon";
import { ENGINE_PORT_MESSAGE, type EngineStatus, type RevBenchBridge } from "../../shared/bridge";
import type { Snapshot } from "../../shared/snapshot-v2.generated";
import { CommandError, EngineClient, fromMessagePort, type Hello } from "./engine-client";

/** What the top bar says about the engine. */
export type EngineSummary = "no-shell" | "starting" | "connecting" | "running" | "restarting" | "failed";

/** The parts of `window` the store uses, so tests can give their own. */
export interface WindowLike {
  addEventListener(type: "message", listener: (event: PortEvent) => void): void;
  removeEventListener(type: "message", listener: (event: PortEvent) => void): void;
}

export interface PortEvent {
  data: unknown;
  source: unknown;
  ports: readonly MessagePort[];
}

export interface StoreDeps {
  bridge?: RevBenchBridge;
  window: WindowLike & object;
  requestFrame(callback: () => void): number;
  cancelFrame(handle: number): void;
  /** one statistics request per this many frames */
  statsEveryFrames?: number;
}

export class EngineStore {
  status = $state.raw<EngineStatus | null>(null);
  hello = $state.raw<Hello | null>(null);
  snapshot = $state.raw<Snapshot | null>(null);
  stats = $state.raw<HostStats | null>(null);
  connected = $state(false);
  /** the preset being played (what the engine last confirmed) */
  preset = $state<string | null>(null);
  rebuilding = $state(false);
  lastError = $state<string | null>(null);
  /** snapshots the decoder refused */
  refusedSnapshots = $state(0);

  summary: EngineSummary = $derived.by(() => {
    if (!this.deps.bridge) return "no-shell";
    const state = this.status?.state;
    if (!state) return "starting";
    if (state === "running") return this.connected ? "running" : "connecting";
    return state;
  });

  private readonly client: EngineClient;
  private stopStatus: (() => void) | null = null;
  private frame: number | null = null;
  private frames = 0;
  private readonly onWindowMessage = (event: PortEvent) => this.receivePort(event);

  constructor(private readonly deps: StoreDeps) {
    this.client = new EngineClient({
      hello: (hello) => {
        this.hello = hello;
        this.preset = hello.preset;
        this.connected = true;
      },
      snapshot: (snapshot) => {
        this.snapshot = snapshot;
      },
      stats: (stats) => {
        this.stats = stats;
      },
      detached: () => {
        this.connected = false;
      },
      invalidSnapshot: () => {
        this.refusedSnapshots++;
      },
    });
  }

  /** Starts listening for the engine's port, asks for it, and begins the frame loop. */
  start(): void {
    const { bridge, window } = this.deps;
    window.addEventListener("message", this.onWindowMessage);
    if (bridge) {
      this.stopStatus = bridge.onEngineStatus((status) => this.receiveStatus(status));
      bridge.requestEnginePort();
    }
    const tick = () => {
      this.client.requestSnapshot();
      if (++this.frames % (this.deps.statsEveryFrames ?? 60) === 0) this.client.requestStats();
      this.frame = this.deps.requestFrame(tick);
    };
    this.frame = this.deps.requestFrame(tick);
  }

  dispose(): void {
    this.deps.window.removeEventListener("message", this.onWindowMessage);
    this.stopStatus?.();
    this.stopStatus = null;
    if (this.frame !== null) this.deps.cancelFrame(this.frame);
    this.frame = null;
    this.client.detach("the window closed");
  }

  /** After "failed": asks the main process to try the engine again. */
  restartEngine(): void {
    this.deps.bridge?.restartEngine();
  }

  /** Sets a driver's control or a setting by the core's parameter name. */
  async setParam(name: string, value: number): Promise<void> {
    try {
      await this.client.command({ kind: "set-param", name, value });
      this.lastError = null;
    } catch (e) {
      this.lastError = describe(e);
    }
  }

  /** Rebuilds with another preset: the old engine plays until the new one is swapped in. */
  async loadPreset(preset: string): Promise<void> {
    if (this.rebuilding) return;
    this.rebuilding = true;
    try {
      await this.client.command({ kind: "load-preset", preset });
      this.preset = preset;
      this.lastError = null;
    } catch (e) {
      this.lastError = describe(e);
    } finally {
      this.rebuilding = false;
    }
  }

  private receiveStatus(status: EngineStatus): void {
    this.status = status;
    if (status.state !== "running") {
      // a dead or restarting engine: its port is no good any more
      if (this.client.connected) this.client.detach(status.message ?? `the engine is ${status.state}`);
    }
  }

  private receivePort(event: PortEvent): void {
    const data = event.data as { channel?: unknown } | null;
    if (event.source !== this.deps.window || data?.channel !== ENGINE_PORT_MESSAGE) return;
    const port = event.ports[0];
    if (!port) return;
    this.client.attach(fromMessagePort(port));
  }
}

function describe(e: unknown): string {
  if (e instanceof CommandError) return e.code === "DETACHED" ? e.message : `${e.message} (${e.code})`;
  return e instanceof Error ? e.message : String(e);
}
