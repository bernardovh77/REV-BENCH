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
import { SCENES, type Ack, type Addon, type BuildSpec, type Host, type ParamInfo, type PresetInfo, type Quality, type Scene } from "../shared/addon";
import type { BuildChoice, EngineInit } from "../shared/process-messages";
import { PROTOCOL_VERSION, type Command, type FromEngine, type ToEngine } from "../shared/protocol";
import { ThrottleGuard } from "./throttle-guard";
import { TraceRecorder } from "./trace-recorder";

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
  /** writes a capture-dump's file, by name, into the user-data folder; returns its path */
  writeFile?: (name: string, data: Uint8Array | string) => string;
}

/**
 * Quitting fades the sound out first (stage 066): master_gain goes to 0, which the core smooths
 * with a 10 ms time constant, and the ring holds about 20 ms more; after this long the sound is
 * more than 50 dB down when the host stops.
 */
export const QUIT_FADE_MS = 90;
/** The latency setting's range, ms (rb_host_config.target_fill_ms). */
export const LATENCY_MIN_MS = 10;
export const LATENCY_MAX_MS = 80;
/** How often the engine process checks whether the host moved to another device. */
export const DEVICE_CHECK_MS = 500;

/** A rebuild that takes longer than this is given up (the running engine plays on). */
export const PREPARE_TIMEOUT_MS = 30_000;
/**
 * What the app wants of every engine it builds, by parameter name: the core's default is a crank
 * held at a speed by a dyno (hold 1, for the offline renders); the app's engine runs free, so the
 * driver's ignition, starter and throttle are what make it turn. Applied in the build itself, so
 * the first block already has it, for the first engine and for every swap.
 */
export const APP_BUILD_PARAMS: Record<string, number> = { hold: 0 };
/**
 * The app's level (stage 066): master_gain at volume 1. The core's output is the pressure at the
 * listener, so engines keep their real relative loudness (the flat-plane V8 about 20 dB louder
 * than the four); this lowers it all so the loudest rev, the flat-plane V8's at the 3 m listener,
 * peaks at about -4 dBFS, under the soft limiter's knee (0.8) with 1.5 dB to spare. Measured
 * with rb_cli sound --rev at each preset's tier, three seeds: unlimited peaks of 2.17 (v8-flat),
 * 1.46 (v8-cross), 1.07 (i6) at gain 1 (docs/stage-066.md, "Levels").
 */
export const APP_HEADROOM = 0.3;
/**
 * Settings the driver changes live that every later build keeps (stage 066): where the mono
 * listener stands, and the A/B switch's variant. A rebuild would otherwise put them back to the
 * core's defaults without anyone asking.
 */
export const STICKY_PARAMS: readonly string[] = ["listener", "variant"];
/** The volume's top: twice the app's level, +6 dB (the loudest engines then reach the limiter). */
export const VOLUME_MAX = 2;
/**
 * Level trims on top of the app's level, by preset and by stereo scene (Bernardo, 2026-11-16,
 * after playing 0.2.0). They are the app's, not the core's: the reference renders don't change.
 *   i4     +10 dB: its rev was 17 dB under the cross-plane V8's (90 against 107 dB SPL at 3 m),
 *          too quiet to hear even at full volume; real 2.0 L fours sit nearer 8-12 dB under a V8.
 *          A stopgap until stage 128 ("the quiet four") fixes the four's own sound.
 *   cabin  +6 dB: the cabin scene was 6 to 15 dB under the 3 m listener (13 on the V8).
 */
export const PRESET_TRIM: Readonly<Record<string, number>> = { i4: 10 ** (10 / 20) };
export const SCENE_TRIM: Readonly<Record<string, number>> = { cabin: 2 };
const QUALITIES: readonly Quality[] = ["low", "medium", "high", "ultra", "balanced", "lean"];

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
  /** the dead-man rule for a live throttle (throttle-guard.ts) */
  private readonly guard = new ThrottleGuard();
  /** the volume, 0..VOLUME_MAX: master_gain is APP_HEADROOM times it, in every build */
  private volume = 1;
  /** the host's device events last seen, and when they were checked */
  private deviceEvents = 0;
  private deviceCheckedAt = -Infinity;
  /** the drive's trace, when the engine runs with a capture tap (tests) */
  private recorder: TraceRecorder | null = null;
  /** the sticky parameters' values, by name, once set */
  private readonly sticky: Record<string, number> = {};
  /** the device chosen (absent: the system's default) */
  private device: number | undefined;

  constructor(private readonly deps: ControlDeps) {
    this.choice = deps.config.build;
    this.device = deps.config.device;
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
    this.host = this.createHost(this.choice);
    this.host.start();
  }

  private createHost(choice: BuildChoice): Host {
    const { addon, config } = this.deps;
    const initial = this.buildSpec(choice);
    // the capture tap: the next power of two above the seconds asked for, at 48 kHz
    const captureFrames = config.captureSeconds ? 2 ** Math.ceil(Math.log2(config.captureSeconds * 48000)) : 0;
    const host = addon.createHost({
      backend: config.backend,
      ...(this.device === undefined ? {} : { device: this.device }),
      ...(config.sampleRate === undefined ? {} : { sampleRate: config.sampleRate }),
      ...(choice.latencyMs === undefined ? {} : { targetFillMs: choice.latencyMs }),
      ...(captureFrames ? { captureFrames } : {}),
      initial,
    });
    if (!captureFrames) {
      this.recorder = null;
      return host;
    }
    this.recorder = new TraceRecorder(this.params, this.presets, host.sampleRate(), initial);
    return this.recorder.wrap(host);
  }

  /**
   * Tests only: the capture tap's audio (float32, stereo interleaved, from frame 0) and the
   * drive's trace, written into the user-data folder; then a `capture` message says where.
   */
  private captureDump(id: number): void {
    const host = this.requireHost();
    const recorder = this.recorder;
    if (!recorder || !this.deps.writeFile) {
      return this.reply(id, { error: { code: "NO_CAPTURE", message: "the engine was started without a capture tap" } });
    }
    const chunks: Float32Array[] = [];
    let frames = 0;
    let dropped = 0;
    for (;;) {
      const read = host.captureRead(1 << 16);
      dropped = read.dropped;
      if (read.samples.length === 0) break;
      chunks.push(read.samples);
      frames += read.samples.length / 2;
    }
    const all = new Float32Array(frames * 2);
    let at = 0;
    for (const chunk of chunks) {
      all.set(chunk, at);
      at += chunk.length;
    }
    const audio = this.deps.writeFile("capture.f32", new Uint8Array(all.buffer));
    const trace = this.deps.writeFile("capture.rbtrace", recorder.text(frames));
    this.reply(id, {});
    this.port?.post({ type: "capture", audio, trace, frames, rate: host.sampleRate(), dropped, interrupted: recorder.interrupted });
  }

  /**
   * The latency (the ring's fill target, LATENCY_MIN_MS..LATENCY_MAX_MS): the host takes it only
   * when it is made, so the host is made again with it, the engine rebuilt as it was and the
   * window greeted again; the sound stops for a moment. Commands in flight are answered with
   * an error (the engine they were for is gone).
   */
  private setLatency(id: number, ms: number): void {
    if (typeof ms !== "number" || !Number.isInteger(ms) || ms < LATENCY_MIN_MS || ms > LATENCY_MAX_MS) {
      return this.reply(id, { error: { code: "BAD_VALUE", message: `the latency is a whole number of ms from ${LATENCY_MIN_MS} to ${LATENCY_MAX_MS}` } });
    }
    if (this.rebuild) return this.reply(id, { error: { code: "BUSY", message: "a rebuild is in progress" } });
    const choice: BuildChoice = { ...this.choice, latencyMs: ms };
    const next = this.createHost(choice);
    const old = this.host;
    this.host = next;
    for (const p of this.pending.values()) {
      if (p.epoch === this.epoch) this.reply(p.id, { error: { code: "RESTARTED", message: "the sound restarted for the new latency" } });
    }
    this.pending.clear();
    try {
      old?.destroy();
    } catch (e) {
      this.deps.log("warn", `destroy failed: ${errorOf(e).message}`);
    }
    next.start();
    this.choice = choice;
    this.deviceEvents = 0;
    this.deps.onBuild(choice);
    this.reply(id, {});
    if (this.port) this.attach(this.port);
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
    const wanted: Record<string, number> = { ...APP_BUILD_PARAMS, ...this.sticky, master_gain: this.gainFor(choice, this.volume) };
    const params = this.params.filter((p) => p.name in wanted).map((p) => ({ id: p.id, value: wanted[p.name] }));
    return {
      preset: choice.preset,
      ...(choice.quality ? { quality: choice.quality } : {}),
      ...(choice.scene ? { scene: choice.scene } : {}),
      ...(params.length ? { params } : {}),
    };
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
      build: { ...this.choice },
      settings: { volume: this.volume, ...this.sticky },
      generation: host.generation(),
    });
    port.post({ type: "state", state: "running" });
  }

  /** The window is gone: a live throttle falls now (the dead-man rule), not in 250 ms. */
  detach(): void {
    this.port = null;
    this.guard.release(this.now());
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
          if (STICKY_PARAMS.includes(param.name)) this.sticky[param.name] = command.value;
          // a throttle set this way (a slider) holds, as the latched style does
          if (param.name === "throttle" && command.value >= 0 && command.value <= 1) this.guard.beat(command.value, false, this.now());
          return;
        }
        case "throttle": {
          const param = this.params.find((p) => p.name === "throttle");
          if (!param) return this.reply(id, { error: { code: "UNKNOWN_PARAM", message: 'no parameter "throttle"' } });
          const { value, live } = command;
          if (typeof value !== "number" || !Number.isFinite(value) || value < 0 || value > 1 || typeof live !== "boolean") {
            return this.reply(id, { error: { code: "BAD_VALUE", message: "a throttle is a number from 0 to 1, and live or not" } });
          }
          this.guard.beat(value, live, this.now());
          const request = host.setParam(param.id, value);
          this.pending.set(request, { id, epoch: this.epoch });
          return;
        }
        case "load-preset":
          return this.loadPreset(id, command.preset, command.quality, command.scene);
        case "set-latency":
          return this.setLatency(id, command.ms);
        case "capture-dump":
          return this.captureDump(id);
        case "volume": {
          const param = this.params.find((p) => p.name === "master_gain");
          const { value } = command;
          if (typeof value !== "number" || !Number.isFinite(value) || value < 0 || value > VOLUME_MAX) {
            return this.reply(id, { error: { code: "BAD_VALUE", message: `the volume is a number from 0 to ${VOLUME_MAX}` } });
          }
          if (!param) return this.reply(id, { error: { code: "UNKNOWN_PARAM", message: 'no parameter "master_gain"' } });
          this.volume = value;
          const request = host.setParam(param.id, this.gainFor(this.choice, value));
          this.pending.set(request, { id, epoch: this.epoch });
          return;
        }
        case "select-device": {
          if (!Number.isInteger(command.index)) {
            return this.reply(id, { error: { code: "BAD_VALUE", message: "a device index is an integer" } });
          }
          host.selectDevice(command.index);
          this.device = command.index < 0 ? undefined : command.index;
          this.reply(id, {});
          return this.postDevice("selected", command.index);
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

  private loadPreset(id: number, preset: string, quality: Quality | undefined, scene: Scene | null | undefined): void {
    if (this.rebuild) {
      return this.reply(id, { error: { code: "BUSY", message: "another rebuild is in progress" } });
    }
    if (!this.presets.some((p) => p.key === preset)) {
      return this.reply(id, { error: { code: "UNKNOWN_PRESET", message: `no preset "${String(preset)}"` } });
    }
    if (quality !== undefined && !QUALITIES.includes(quality)) {
      return this.reply(id, { error: { code: "BAD_QUALITY", message: `no quality "${String(quality)}"` } });
    }
    if (scene !== undefined && scene !== null && !SCENES.includes(scene)) {
      return this.reply(id, { error: { code: "BAD_SCENE", message: `no scene "${String(scene)}"` } });
    }
    const { latencyMs } = this.choice;
    const choice: BuildChoice = { preset, ...(quality ? { quality } : {}), ...(scene ? { scene } : {}), ...(latencyMs === undefined ? {} : { latencyMs }) };
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
      this.guardThrottle(host);
      this.watchDevice(host);
    } catch (e) {
      this.deps.log("error", `pump failed: ${errorOf(e).message}`);
    }
  }

  /**
   * A lost device (unplugged headphones) or a changed system default: the host has already moved
   * to the new default device; the window is told which (app-design section 15).
   */
  private watchDevice(host: Host): void {
    const now = this.now();
    if (now - this.deviceCheckedAt < DEVICE_CHECK_MS) return;
    this.deviceCheckedAt = now;
    const events = host.stats().deviceEvents;
    if (events === this.deviceEvents) return;
    this.deviceEvents = events;
    this.postDevice("lost", -1);
  }

  private postDevice(reason: "lost" | "selected", index: number): void {
    const devices = this.deps.addon.listDevices(this.deps.config.backend);
    const playing = devices.find((d) => (index >= 0 ? d.index === index : d.isDefault)) ?? devices[0];
    this.port?.post({ type: "device", reason, playing: playing?.name ?? "no device", devices });
  }

  /** master_gain for a build at a volume: the app's level, and the preset's and scene's trims. */
  private gainFor(choice: BuildChoice, volume: number): number {
    return APP_HEADROOM * volume * (PRESET_TRIM[choice.preset] ?? 1) * (choice.scene ? (SCENE_TRIM[choice.scene] ?? 1) : 1);
  }

  /** Quitting: the sound fades out; how long to wait before stopping the host (QUIT_FADE_MS). */
  fadeOut(): number {
    const host = this.host;
    const param = this.params.find((p) => p.name === "master_gain");
    if (!host || !param) return 0;
    try {
      host.setParam(param.id, 0);
    } catch (e) {
      this.deps.log("warn", `the quit's fade could not be sent: ${errorOf(e).message}`);
      return 0;
    }
    return QUIT_FADE_MS;
  }

  /** The system sleeps and wakes: the host stops its device, and restarts it after a flush. */
  suspend(): void {
    try {
      this.host?.suspend();
    } catch (e) {
      this.deps.log("warn", `suspend failed: ${errorOf(e).message}`);
    }
  }

  resume(): void {
    try {
      this.host?.resume();
    } catch (e) {
      this.deps.log("warn", `resume failed: ${errorOf(e).message}`);
    }
  }

  /** The dead-man rule: a live throttle whose beat stopped is ramped to zero (no reply: nobody asked). */
  private guardThrottle(host: Host): void {
    const value = this.guard.tick(this.now());
    if (value === null) return;
    const param = this.params.find((p) => p.name === "throttle");
    if (!param) return;
    try {
      host.setParam(param.id, value);
    } catch (e) {
      // a full queue: the next pump sends the ramp's next value
      this.deps.log("warn", `the dead-man ramp could not set the throttle: ${errorOf(e).message}`);
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
