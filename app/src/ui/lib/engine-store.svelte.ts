/*
 * engine-store.svelte.ts - what the interface knows about the engine, as Svelte state: the
 * process's status (from the main process), the connection (from the engine client), the
 * engine's offer (hello), the newest snapshot and statistics. Panels read it and call its
 * methods; nothing else talks to the engine.
 *
 * The window never waits for the engine: it asks for a snapshot every animation frame and shows
 * the newest it has (docs/app-design.md section 4).
 */
import type { DeviceInfo, HostStats, Quality, Scene } from "../../shared/addon";
import { ENGINE_PORT_MESSAGE, type EngineStatus, type RevBenchBridge } from "../../shared/bridge";
import type { Snapshot } from "../../shared/snapshot-v2.generated";
import type { FromEngine } from "../../shared/protocol";
import { CommandError, EngineClient, fromMessagePort, type Hello } from "./engine-client";

/** Statistics in a row with nothing late before the "can't keep up" warning goes (one a second). */
export const KEEP_UP_QUIET_STATS = 10;

/** A capture-dump's result (protocol.ts). */
export type CaptureInfo = Omit<Extract<FromEngine, { type: "capture" }>, "type">;

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
  /** the quality chosen for it, or null: the preset's own tier */
  quality = $state<Quality | null>(null);
  /** the stereo scene, or null: the mono listener */
  scene = $state<Scene | null>(null);
  rebuilding = $state(false);
  lastError = $state<string | null>(null);
  /** snapshots the decoder refused */
  refusedSnapshots = $state(0);
  /** the playback devices, the one playing, and a notice when the sound moved by itself */
  devices = $state.raw<DeviceInfo[]>([]);
  playing = $state<string | null>(null);
  deviceNotice = $state<string | null>(null);
  /** the latency setting, ms */
  latencyMs = $state(20);
  /** the computer missed a block's deadline or the device ran dry lately: the window says so
   *  and offers a lower quality, never switching by itself (app-design 11). It stays up until
   *  KEEP_UP_QUIET_STATS statistics in a row (about 10 s) have nothing late, so a computer that
   *  is late now and then gets one steady warning, not one that blinks every second */
  fallingBehind = $state(false);
  private lastLate = -1;
  private quietStats = 0;
  private captured: ((capture: CaptureInfo) => void) | null = null;

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
  private readonly frameListeners = new Set<() => void>();
  /** the mono listener's place (RB_LISTENER_*, 1: 3 m behind, the core's default) */
  listener = $state(1);
  /** the volume, 0..2 (1: the app's level) */
  volume = $state(1);

  constructor(private readonly deps: StoreDeps) {
    this.client = new EngineClient({
      hello: (hello) => {
        this.hello = hello;
        this.preset = hello.preset;
        this.quality = hello.build?.quality ?? null;
        this.scene = hello.build?.scene ?? null;
        this.latencyMs = hello.build?.latencyMs ?? 20;
        this.listener = hello.settings?.listener ?? 1;
        this.volume = hello.settings?.volume ?? 1;
        this.devices = hello.devices;
        this.playing = hello.devices.find((d) => d.isDefault)?.name ?? hello.devices[0]?.name ?? null;
        this.lastLate = -1;
        this.quietStats = 0;
        this.fallingBehind = false;
        this.connected = true;
      },
      snapshot: (snapshot) => {
        this.snapshot = snapshot;
      },
      stats: (stats) => {
        this.stats = stats;
        const late = stats.deadlineMissed + stats.underruns;
        if (this.lastLate >= 0 && late > this.lastLate) {
          this.fallingBehind = true;
          this.quietStats = 0;
        } else if (this.fallingBehind && ++this.quietStats >= KEEP_UP_QUIET_STATS) {
          this.fallingBehind = false;
        }
        this.lastLate = late;
      },
      capture: ({ type: _type, ...capture }) => {
        this.captured?.(capture);
        this.captured = null;
      },
      device: (device) => {
        this.devices = device.devices;
        this.playing = device.playing;
        if (device.reason === "lost") this.deviceNotice = `The sound device changed: now playing on ${device.playing}.`;
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
      for (const listener of this.frameListeners) listener();
      this.client.requestSnapshot();
      if (++this.frames % (this.deps.statsEveryFrames ?? 60) === 0) this.client.requestStats();
      this.frame = this.deps.requestFrame(tick);
    };
    this.frame = this.deps.requestFrame(tick);
  }

  /** Runs `listener` every animation frame (the driver's keys and controller); returns its remover. */
  onFrame(listener: () => void): () => void {
    this.frameListeners.add(listener);
    return () => this.frameListeners.delete(listener);
  }

  /** Where the mono listener stands (RB_LISTENER_*): a live change, no rebuild. */
  async setListener(place: number): Promise<void> {
    await this.setParam("listener", place);
    if (this.lastError === null) this.listener = place;
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

  /**
   * The driver's throttle (stage 066), sent without waiting: the beat of a live style sends it
   * every 50 ms, and only a failure is worth saying.
   */
  sendThrottle(value: number, live: boolean): void {
    this.client.command({ kind: "throttle", value, live }).catch((e: unknown) => {
      this.lastError = describe(e);
    });
  }

  /** Moves the sound to a device (-1: the system's default). */
  async selectDevice(index: number): Promise<void> {
    this.deviceNotice = null;
    try {
      await this.client.command({ kind: "select-device", index });
      this.lastError = null;
    } catch (e) {
      this.lastError = describe(e);
    }
  }

  /** The latency: the sound restarts with it (the engine greets the window again). */
  async setLatency(ms: number): Promise<void> {
    try {
      await this.client.command({ kind: "set-latency", ms });
      this.lastError = null;
    } catch (e) {
      this.lastError = describe(e);
    }
  }

  /**
   * Tests only: the engine writes its capture tap's audio and the drive's trace (when it was
   * started with RB_CAPTURE_SECONDS) and says where. The end-to-end test calls it through
   * window.__rbCaptureDump (main.ts).
   */
  async captureDump(): Promise<CaptureInfo> {
    const arrived = new Promise<CaptureInfo>((resolve) => (this.captured = resolve));
    await this.client.command({ kind: "capture-dump" });
    return arrived;
  }

  dismissDeviceNotice(): void {
    this.deviceNotice = null;
  }

  /** The volume, 0..2: 1 is the app's level (control.ts, APP_HEADROOM). */
  async setVolume(value: number): Promise<void> {
    try {
      await this.client.command({ kind: "volume", value });
      this.volume = value;
      this.lastError = null;
    } catch (e) {
      this.lastError = describe(e);
    }
  }

  /**
   * Rebuilds with another preset: the old engine plays until the new one is swapped in. A new
   * preset starts at its own tier, never at the last one's chosen quality; the scene is kept.
   */
  async loadPreset(preset: string): Promise<void> {
    await this.rebuild(preset, null, this.scene);
  }

  /** Rebuilds at a chosen quality (null: the preset's tier). Never done without being asked. */
  async setQuality(quality: Quality | null): Promise<void> {
    if (this.preset) await this.rebuild(this.preset, quality, this.scene);
  }

  /** Rebuilds with a stereo scene, or the mono listener (null). */
  async setScene(scene: Scene | null): Promise<void> {
    if (this.preset) await this.rebuild(this.preset, this.quality, scene);
  }

  private async rebuild(preset: string, quality: Quality | null, scene: Scene | null): Promise<void> {
    if (this.rebuilding) return;
    this.rebuilding = true;
    try {
      await this.client.command({ kind: "load-preset", preset, ...(quality ? { quality } : {}), scene });
      this.preset = preset;
      this.quality = quality;
      this.scene = scene;
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
