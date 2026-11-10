/*
 * control.ts - the engine process's control loop (docs/app-design.md section 3). It owns the
 * add-on's host and speaks the window's protocol (src/shared/protocol.ts):
 *
 *   commands      -> host calls; the reply carries the window's own id and the host's ack
 *                    (a rebuild's when its swap is done), or an error that never got that far;
 *   snapshot asks -> the newest snapshot the host has, if any, as a transferred ArrayBuffer;
 *   stats asks    -> the host's statistics.
 *
 * Nothing here blocks: a rebuild is `prepare`, then polled by `pump()` until it is ready, then
 * `swap`; the acks come through `poll()`. The entry (main.ts) calls `pump()` on a short timer.
 * The loop never touches physics or audio; it only moves numbers between the window and the
 * add-on. The window's port is an `EnginePort`, so Electron's MessagePortMain and the tests'
 * in-memory ports both fit.
 */
import type { Ack, Addon, BuildSpec, Host, ParamInfo, PresetInfo, Quality } from "../shared/addon";
import type { BuildChoice, EngineInit } from "../shared/process-messages";
import { PROTOCOL_VERSION, type Command, type FromEngine, type ToEngine } from "../shared/protocol";

/** Where replies go. `transfer` lists buffers the port may take rather than copy (if it can). */
export interface EnginePort {
  post(message: FromEngine, transfer?: ArrayBuffer[]): void;
}

export interface ControlDeps {
  addon: Addon;
  config: EngineInit;
  /** a rebuild took effect: what the main process autosaves */
  onBuild(choice: BuildChoice): void;
  log(level: "info" | "warn" | "error", message: string): void;
  /** milliseconds (default Date.now) */
  now?: () => number;
}

/** A rebuild that takes longer than this is given up (the running engine plays on). */
export const PREPARE_TIMEOUT_MS = 30_000;
/**
 * What the app wants of every engine it builds, by parameter name: the core's default is a crank
 * held at a speed by a dyno (hold 1, for the offline renders); the app's engine runs free, so the
 * driver's ignition, starter and throttle are what make it turn. Applied in the build itself, so
 * the first block already has it, for the first engine and for every swap.
 */
export const APP_BUILD_PARAMS: Record<string, number> = { hold: 0 };
const QUALITIES: readonly Quality[] = ["low", "medium", "high", "ultra", "balanced"];

interface Pending {
  /** the window's command id */
  id: number;
  /** which connection it came from: a reply to an earlier one is dropped */
  epoch: number;
  /** a rebuild's swap request: its ack also finishes the rebuild */
  rebuild?: BuildChoice;
}

interface Rebuild {
  id: number;
  epoch: number;
  prepareId: number;
  choice: BuildChoice;
  startedAt: number;
}

function errorOf(e: unknown): { code: string; message: string } {
  const code = (e as { code?: unknown } | null)?.code;
  return {
    code: typeof code === "string" ? code : "ADDON",
    message: e instanceof Error ? e.message : String(e),
  };
}

export class EngineControl {
  private host: Host | null = null;
  private port: EnginePort | null = null;
  private epoch = 0;
  private params: ParamInfo[] = [];
  private presets: PresetInfo[] = [];
  private choice: BuildChoice;
  private readonly pending = new Map<number, Pending>();
  private rebuild: Rebuild | null = null;
  private readonly now: () => number;

  constructor(private readonly deps: ControlDeps) {
    this.choice = deps.config.build;
    this.now = deps.now ?? (() => Date.now());
  }

  /** Makes the host and starts it. Throws if the add-on can't (the entry reports it and exits). */
  start(): void {
    const { addon, config } = this.deps;
    this.presets = addon.presets();
    this.params = addon.params();
    if (!this.presets.some((p) => p.key === config.build.preset)) {
      throw new Error(`no preset "${config.build.preset}"`);
    }
    this.host = addon.createHost({
      backend: config.backend,
      ...(config.device === undefined ? {} : { device: config.device }),
      ...(config.sampleRate === undefined ? {} : { sampleRate: config.sampleRate }),
      initial: { ...this.buildSpec(config.build) },
    });
    this.host.start();
  }

  get running(): boolean {
    return this.host !== null;
  }

  /** The rate the host renders at (the device's, or the null backend's). */
  sampleRate(): number {
    return this.requireHost().sampleRate();
  }

  /** The add-on's build for a choice: the preset and quality, and the app's driver defaults. */
  private buildSpec(choice: BuildChoice): BuildSpec {
    const params = this.params.filter((p) => p.name in APP_BUILD_PARAMS).map((p) => ({ id: p.id, value: APP_BUILD_PARAMS[p.name] }));
    return { preset: choice.preset, ...(choice.quality ? { quality: choice.quality } : {}), ...(params.length ? { params } : {}) };
  }

  /** The window's port: says hello on it. A later attach replaces it (a reloaded window). */
  attach(port: EnginePort): void {
    const host = this.requireHost();
    this.epoch++;
    this.port = port;
    const { addon } = this.deps;
    port.post({
      type: "hello",
      protocol: PROTOCOL_VERSION,
      addonVersion: addon.version().addon,
      snapshotVersion: addon.version().snapshot,
      presets: this.presets,
      params: this.params,
      telemetry: addon.telemetry(),
      devices: addon.listDevices(this.deps.config.backend),
      sampleRate: host.sampleRate(),
      preset: this.choice.preset,
      generation: host.generation(),
    });
    port.post({ type: "state", state: "running" });
  }

  detach(): void {
    this.port = null;
  }

  /** One message from the window. Whatever it holds, this never throws. */
  handle(message: unknown): void {
    try {
      this.dispatch(message as ToEngine);
    } catch (e) {
      this.deps.log("error", `a window message failed: ${errorOf(e).message}`);
    }
  }

  private dispatch(message: ToEngine): void {
    if (typeof message !== "object" || message === null) {
      this.deps.log("warn", "ignored a message that is not an object");
      return;
    }
    switch (message.type) {
      case "command":
        this.command(message.id, message.command);
        return;
      case "snapshot-request":
        this.snapshot();
        return;
      case "stats-request":
        this.stats();
        return;
      default:
        this.deps.log("warn", `ignored a window message of unknown type ${JSON.stringify((message as { type?: unknown }).type)}`);
    }
  }

  private requireHost(): Host {
    if (!this.host) throw new Error("the engine has not started");
    return this.host;
  }

  private reply(id: number, body: { ack?: Ack; error?: { code: string; message: string } }): void {
    this.port?.post({ type: "reply", id, ...body });
  }

  private command(id: number, command: Command): void {
    const host = this.requireHost();
    if (!Number.isInteger(id) || typeof command !== "object" || command === null) {
      this.deps.log("warn", "ignored a command without an id or a body");
      return;
    }
    try {
      switch (command.kind) {
        case "set-param": {
          const param = this.params.find((p) => p.name === command.name);
          if (!param) return this.reply(id, { error: { code: "UNKNOWN_PARAM", message: `no parameter "${String(command.name)}"` } });
          if (typeof command.value !== "number" || !Number.isFinite(command.value)) {
            return this.reply(id, { error: { code: "BAD_VALUE", message: `"${param.name}" needs a finite number` } });
          }
          const request = host.setParam(param.id, command.value);
          this.pending.set(request, { id, epoch: this.epoch });
          return;
        }
        case "load-preset":
          return this.loadPreset(id, command.preset, command.quality);
        case "select-device": {
          if (!Number.isInteger(command.index)) {
            return this.reply(id, { error: { code: "BAD_VALUE", message: "a device index is an integer" } });
          }
          host.selectDevice(command.index);
          return this.reply(id, {});
        }
        default:
          return this.reply(id, {
            error: { code: "BAD_COMMAND", message: `unknown command ${JSON.stringify((command as { kind?: unknown }).kind)}` },
          });
      }
    } catch (e) {
      this.reply(id, { error: errorOf(e) });
    }
  }

  private loadPreset(id: number, preset: string, quality: Quality | undefined): void {
    if (this.rebuild) {
      return this.reply(id, { error: { code: "BUSY", message: "another rebuild is in progress" } });
    }
    if (!this.presets.some((p) => p.key === preset)) {
      return this.reply(id, { error: { code: "UNKNOWN_PRESET", message: `no preset "${String(preset)}"` } });
    }
    if (quality !== undefined && !QUALITIES.includes(quality)) {
      return this.reply(id, { error: { code: "BAD_QUALITY", message: `no quality "${String(quality)}"` } });
    }
    const choice: BuildChoice = { preset, ...(quality ? { quality } : {}) };
    const prepareId = this.requireHost().prepare(this.buildSpec(choice));
    this.rebuild = { id, epoch: this.epoch, prepareId, choice, startedAt: this.now() };
  }

  private snapshot(): void {
    const read = this.requireHost().readSnapshot();
    if (!read) return;
    this.port?.post(
      { type: "snapshot", bytes: read.bytes, sequence: read.sequence, generation: read.generation, audioFrame: read.audioFrame },
      [read.bytes],
    );
  }

  private stats(): void {
    this.port?.post({ type: "stats", stats: this.requireHost().stats() });
  }

  /** Collects the host's acknowledgments and moves a rebuild on; call it every few milliseconds. */
  pump(): void {
    const host = this.host;
    if (!host) return;
    try {
      this.collectAcks(host);
      this.advanceRebuild(host);
    } catch (e) {
      this.deps.log("error", `pump failed: ${errorOf(e).message}`);
    }
  }

  private collectAcks(host: Host): void {
    for (const ack of host.poll()) {
      const pending = this.pending.get(ack.requestId);
      if (!pending) continue;
      this.pending.delete(ack.requestId);
      if (pending.rebuild) {
        this.rebuild = null;
        if (ack.outcome === "ok") {
          this.choice = pending.rebuild;
          this.deps.onBuild(pending.rebuild);
        }
      }
      if (pending.epoch === this.epoch) this.reply(pending.id, { ack });
    }
  }

  private advanceRebuild(host: Host): void {
    const rebuild = this.rebuild;
    if (!rebuild || this.pendingSwap(rebuild)) return;
    const state = host.prepareState(rebuild.prepareId);
    if (state.state === "ready") {
      try {
        const request = host.swap(rebuild.prepareId);
        this.pending.set(request, { id: rebuild.id, epoch: rebuild.epoch, rebuild: rebuild.choice });
      } catch (e) {
        this.failRebuild(host, rebuild, errorOf(e));
      }
    } else if (state.state === "failed") {
      this.failRebuild(host, rebuild, { code: "PREPARE_FAILED", message: state.message || "the engine could not be built" });
    } else if (this.now() - rebuild.startedAt > PREPARE_TIMEOUT_MS) {
      this.failRebuild(host, rebuild, { code: "PREPARE_TIMEOUT", message: "the engine took too long to build" });
    }
  }

  /** True once the swap has been asked for (its ack finishes the rebuild). */
  private pendingSwap(rebuild: Rebuild): boolean {
    for (const p of this.pending.values()) if (p.rebuild === rebuild.choice) return true;
    return false;
  }

  private failRebuild(host: Host, rebuild: Rebuild, error: { code: string; message: string }): void {
    this.rebuild = null;
    try {
      host.discard(rebuild.prepareId);
    } catch (e) {
      this.deps.log("warn", `discard failed: ${errorOf(e).message}`);
    }
    this.deps.log("warn", `rebuild of "${rebuild.choice.preset}" failed: ${error.message}`);
    if (rebuild.epoch === this.epoch) this.reply(rebuild.id, { error });
  }

  /** Stops the host and frees it. Safe to call twice. */
  shutdown(): void {
    const host = this.host;
    this.host = null;
    this.port = null;
    if (!host) return;
    try {
      host.destroy();
    } catch (e) {
      this.deps.log("warn", `destroy failed: ${errorOf(e).message}`);
    }
  }
}
