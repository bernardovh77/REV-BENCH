/*
 * fake-addon.ts - a test double for the Node-API add-on (src/shared/addon.ts). It follows the
 * contract, so the engine process's control loop, the window and the end-to-end checks can
 * run without the C library: `RB_FAKE_ADDON=1` selects it in the app, tests build it directly.
 *
 * It is NOT an engine model and makes no sound. Its "rpm" is a first-order lag towards a
 * target set by ignition, starter and throttle, so the instruments move; what the real
 * engine does is the C core's business (CLAUDE.md: no physics or audio in JavaScript). Its
 * snapshots are real snapshot-v2 bytes, written by the generated encoder, so the window's
 * decoder is exercised on the true layout.
 *
 * Time comes from `options.now` (milliseconds), so tests step it by hand.
 */
import type {
  Ack,
  Addon,
  AddonError,
  Backend,
  BuildSpec,
  DeviceInfo,
  Host,
  HostConfig,
  HostStats,
  ParamInfo,
  PrepareState,
  PresetInfo,
  SnapshotRead,
  TelemetryInfo,
} from "../shared/addon";
import {
  INSTRUMENT_NAMES,
  encodeSnapshot,
  type CylinderState,
  type InstrumentName,
  type Snapshot,
} from "../shared/snapshot-v2.generated";

export interface FakeOptions {
  /** the clock, in milliseconds (default: performance.now) */
  now?: () => number;
  /** how long a prepare takes before it is "ready" (default 30 ms) */
  prepareMs?: number;
  /** preset keys whose prepare fails, to test a rebuild that can't be made */
  failPresets?: string[];
}

export function addonError(code: string, message: string): AddonError {
  const error = new Error(message) as AddonError;
  error.code = code;
  return error;
}

const PRESETS: PresetInfo[] = [
  { index: 0, key: "i4", name: "2.0 L inline-4", quality: "balanced" },
  { index: 1, key: "i6", name: "3.0 L inline-6", quality: "low" },
  { index: 2, key: "v8-cross", name: "5.0 L cross-plane V8", quality: "low" },
  { index: 3, key: "v8-flat", name: "4.5 L flat-plane V8", quality: "low" },
  { index: 4, key: "boxer4", name: "2.0 L flat-4 (boxer)", quality: "low" },
];
const CYLINDERS: Record<string, number> = { i4: 4, i6: 6, "v8-cross": 8, "v8-flat": 8, boxer4: 4 };

const control = ["control"];
const live: string[] = [];
const PARAM_ROWS: [string, string, number, number, number, string[]][] = [
  ["master_gain", "", 0, 4, 1, live],
  ["test_tone_hz", "Hz", 20, 20000, 220, live],
  ["test_tone_level", "", 0, 1, 0.5, live],
  ["source", "", 0, 1, 0, ["integer"]],
  ["ignition", "", 0, 1, 0, ["integer", ...control]],
  ["starter", "", 0, 1, 0, ["integer", ...control]],
  ["throttle", "", 0, 1, 0, control],
  ["load_torque", "N m", -500, 5000, 0, control],
  ["hold", "", 0, 1, 1, ["integer", ...control]],
  ["hold_rpm", "rpm", 0, 20000, 0, control],
  ["clutch", "", 0, 1, 0, ["integer", ...control]],
  ["variation", "", 0, 4, 0, live],
  ["seed", "", 0, 4294967295, 0, ["rebuild", "integer"]],
];
const PARAMS: ParamInfo[] = PARAM_ROWS.map(([name, unit, min, max, def, flags], id) => ({
  id,
  name,
  unit,
  min,
  max,
  default: def,
  flags,
}));
const TELEMETRY_UNITS: Record<string, string> = {
  rpm: "rpm", rpm_smooth: "rpm", cycle_rpm: "rpm", map: "Pa", torque: "N m", power: "W",
  spark: "deg", throttle_area: "m^2", overrun_time: "s", load_torque: "N m", crank_inertia: "kg m^2",
  air_flow: "kg/s", fuel_flow: "kg/s", exhaust_temperature: "K", tailpipe_temperature: "K",
};
const TELEMETRY: TelemetryInfo[] = INSTRUMENT_NAMES.map((name, id) => ({
  id,
  name,
  unit: TELEMETRY_UNITS[name] ?? "",
}));

const PHYSICS_HZ = 192000;
const MAX_PENDING_ACKS = 256;
const TWO_CYCLES = 4 * Math.PI;

const paramId = (name: string): number => PARAMS.findIndex((p) => p.name === name);

class FakeHost implements Host {
  private destroyed = false;
  private running = false;
  private generationNow = 1;
  private readonly values = new Map<number, number>();
  private readonly acks: Ack[] = [];
  private nextRequest = 1;
  private nextPrepare = 1;
  private readonly prepares = new Map<number, { spec: BuildSpec; readyAt: number; fails: boolean }>();
  private current: BuildSpec;
  private sequence = 0;
  private lastSnapshotAt: number;
  private lastModelAt: number;
  private startedAt: number;
  private rpm = 0;
  private phase = 0;
  private physicsTicks = 0;
  private cycles = 0;
  private applied = 0;
  private refused = 0;
  private swaps = 0;
  private readonly rate: number;

  constructor(
    private readonly config: HostConfig,
    private readonly options: Required<Pick<FakeOptions, "now" | "prepareMs">> & { failPresets: string[] },
    private readonly devices: DeviceInfo[],
  ) {
    this.current = config.initial;
    this.rate = config.backend === "null" ? (config.sampleRate ?? 48000) : this.deviceRate(config.device);
    this.startedAt = this.lastModelAt = this.lastSnapshotAt = options.now();
    this.resetControls();
    for (const p of config.initial.params ?? []) this.values.set(p.id, p.value);
  }

  private deviceRate(index: number | undefined): number {
    const device = this.devices.find((d) => d.index === index) ?? this.devices.find((d) => d.isDefault);
    return device ? device.rate : 48000;
  }

  private live(): void {
    if (this.destroyed) throw addonError("DESTROYED", "the host is destroyed");
  }

  private resetControls(): void {
    this.values.clear();
    for (const p of PARAMS) this.values.set(p.id, p.default);
    this.rpm = 0;
  }

  private ack(kind: string, outcome: Ack["outcome"], reset = false): number {
    if (this.acks.length >= MAX_PENDING_ACKS) {
      this.refused++;
      throw addonError("RB_HOST_ERR_FULL", "the acknowledgment queue is full");
    }
    const requestId = this.nextRequest++;
    this.acks.push({
      requestId,
      kind,
      outcome,
      code: 0,
      generation: this.generationNow,
      reset,
      physicsTick: this.physicsTicks,
      audioFrame: this.audioFrame(),
    });
    if (outcome === "ok") this.applied++;
    else this.refused++;
    return requestId;
  }

  private audioFrame(): number {
    return Math.floor(((this.options.now() - this.startedAt) / 1000) * this.rate);
  }

  start(): void {
    this.live();
    this.running = true;
  }

  stop(): void {
    this.live();
    this.running = false;
  }

  destroy(): void {
    this.destroyed = true;
    this.running = false;
  }

  sampleRate(): number {
    this.live();
    return this.rate;
  }

  generation(): number {
    this.live();
    return this.generationNow;
  }

  setParam(id: number, value: number, expectedGeneration?: number): number {
    this.live();
    const info = PARAMS[id];
    if (!info) throw addonError("RB_ERR_ID", `no parameter ${id}`);
    if (!Number.isFinite(value)) throw addonError("RB_HOST_ERR_ARG", "the value is not a number");
    if (expectedGeneration !== undefined && expectedGeneration !== this.generationNow) {
      return this.ack("set-param", "stale");
    }
    this.advanceModel();
    this.values.set(id, Math.min(info.max, Math.max(info.min, value)));
    return this.ack("set-param", "ok");
  }

  setEngineLive(field: number, index: number, value: number, expectedGeneration?: number): number {
    this.live();
    if (!Number.isFinite(value) || !Number.isInteger(field) || !Number.isInteger(index)) {
      throw addonError("RB_HOST_ERR_ARG", "bad argument");
    }
    return this.ack("set-engine-live", expectedGeneration === undefined || expectedGeneration === this.generationNow ? "ok" : "stale");
  }

  prepare(spec: BuildSpec): number {
    this.live();
    if (!PRESETS.some((p) => p.key === spec.preset)) {
      throw addonError("RB_HOST_ERR_ARG", `no preset "${spec.preset}"`);
    }
    const id = this.nextPrepare++;
    this.prepares.set(id, {
      spec,
      readyAt: this.options.now() + this.options.prepareMs,
      fails: this.options.failPresets.includes(spec.preset),
    });
    return id;
  }

  prepareState(prepareId: number): { state: PrepareState; code: number; message: string } {
    this.live();
    const p = this.prepares.get(prepareId);
    if (!p) throw addonError("RB_HOST_ERR_ARG", `no preparation ${prepareId}`);
    if (this.options.now() < p.readyAt) return { state: "pending", code: 0, message: "" };
    if (p.fails) return { state: "failed", code: -3, message: `the fake could not build "${p.spec.preset}"` };
    return { state: "ready", code: 0, message: "" };
  }

  swap(prepareId: number, expectedGeneration?: number): number {
    this.live();
    const p = this.prepares.get(prepareId);
    if (!p) throw addonError("RB_HOST_ERR_ARG", `no preparation ${prepareId}`);
    if (this.prepareState(prepareId).state !== "ready") return this.ack("swap", "refused");
    if (expectedGeneration !== undefined && expectedGeneration !== this.generationNow) {
      return this.ack("swap", "stale");
    }
    this.prepares.delete(prepareId);
    this.current = p.spec;
    this.generationNow++;
    this.swaps++;
    this.resetControls();
    for (const q of p.spec.params ?? []) this.values.set(q.id, q.value);
    this.physicsTicks = 0;
    this.cycles = 0;
    this.phase = 0;
    this.lastModelAt = this.options.now();
    return this.ack("swap", "ok", true);
  }

  discard(prepareId: number): void {
    this.live();
    this.prepares.delete(prepareId);
  }

  poll(): Ack[] {
    this.live();
    return this.acks.splice(0, this.acks.length);
  }

  /** The stand-in for the engine: rpm lags towards a target; nothing here is physics. */
  private advanceModel(): void {
    const now = this.options.now();
    const dt = Math.max(0, now - this.lastModelAt) / 1000;
    this.lastModelAt = now;
    const ignition = this.values.get(paramId("ignition")) === 1;
    const starter = this.values.get(paramId("starter")) === 1;
    const throttle = this.values.get(paramId("throttle")) ?? 0;
    let target = 0;
    let tau = 0.8;
    if (starter) {
      target = 300;
      tau = 0.25;
    }
    if (ignition && this.rpm >= 200) {
      target = 850 + 5150 * throttle;
      tau = 0.4;
    }
    this.rpm += (target - this.rpm) * (1 - Math.exp(-dt / tau));
    if (this.rpm < 0.01) this.rpm = 0;
    const speed = (this.rpm * 2 * Math.PI) / 60;
    this.phase += speed * dt;
    while (this.phase >= TWO_CYCLES) {
      this.phase -= TWO_CYCLES;
      this.cycles++;
    }
    this.physicsTicks += Math.round(dt * PHYSICS_HZ);
  }

  readSnapshot(): SnapshotRead | null {
    this.live();
    const now = this.options.now();
    if (now - this.lastSnapshotAt < 1) return null;
    this.lastSnapshotAt = now;
    this.advanceModel();
    const n = CYLINDERS[this.current.preset] ?? 4;
    const throttle = this.values.get(paramId("throttle")) ?? 0;
    const value = (name: string): number => this.values.get(paramId(name)) ?? 0;
    const instruments = Object.fromEntries(INSTRUMENT_NAMES.map((name) => [name, 0])) as Record<InstrumentName, number>;
    Object.assign(instruments, {
      rpm: this.rpm,
      rpm_smooth: this.rpm,
      cycle_rpm: this.rpm,
      map: 30000 + 70000 * throttle,
      lambda: this.rpm > 0 ? 1 : 0,
      throttle,
      ignition: value("ignition"),
      starter: value("starter"),
      load_torque: value("load_torque"),
      crank_inertia: 0.16,
      exhaust_temperature: 600,
      tailpipe_temperature: 400,
    });
    const cylinder: CylinderState[] = Array.from({ length: n }, (_, c) => {
      const through = ((this.phase / TWO_CYCLES + c / n) % 1 + 1) % 1;
      return {
        pressure: 101325 * (1 + 20 * through * through),
        temperature: 300 + 900 * through,
        burn: through,
        intake_lift: 0,
        exhaust_lift: 0,
      };
    });
    this.sequence++;
    const snapshot: Snapshot = {
      magic: 0, version: 0, size: 0, cylinders: n, cylinder_fields: 0, reserved0: 0, reserved1: 0,
      build_id: BigInt(PRESETS.findIndex((p) => p.key === this.current.preset) + 1),
      generation: this.generationNow,
      sequence: this.sequence,
      physics_tick: this.physicsTicks,
      audio_frame: this.audioFrame(),
      cycle_index: this.cycles,
      physics_hz: PHYSICS_HZ,
      audio_hz: this.rate,
      phase_rad: this.phase,
      speed_rad_s: (this.rpm * 2 * Math.PI) / 60,
      instruments,
      cylinder,
    };
    return {
      bytes: encodeSnapshot(snapshot),
      sequence: this.sequence,
      generation: this.generationNow,
      audioFrame: snapshot.audio_frame,
      physicsTick: snapshot.physics_tick,
    };
  }

  stats(): HostStats {
    this.live();
    const seconds = Math.max(0, this.options.now() - this.startedAt) / 1000;
    return {
      blocks: this.running ? Math.floor((seconds * this.rate) / 256) : 0,
      renderMsMean: 0.9,
      renderMsMax: 1.4,
      deadline: [0.7, 0.3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
      ringFillMs: this.config.targetFillMs ?? 20,
      ringTargetMs: this.config.targetFillMs ?? 20,
      underruns: 0,
      commandsApplied: this.applied,
      commandsRefused: this.refused,
      swapsOk: this.swaps,
      eventsLost: 0,
      priority: "fake",
      boosted: false,
      forbiddenCalls: 0,
      processMb: 0,
    };
  }

  selectDevice(index: number): void {
    this.live();
    if (!this.devices.some((d) => d.index === index)) {
      throw addonError("RB_HOST_ERR_ARG", `no device ${index}`);
    }
  }

  suspend(): void {
    this.live();
    this.running = false;
  }

  resume(): void {
    this.live();
    this.running = true;
  }
}

function devicesFor(backend: Backend): DeviceInfo[] {
  if (backend === "null") {
    return [{ index: 0, name: "Null device", isDefault: true, rate: 48000, nativeRate: null }];
  }
  return [
    { index: 0, name: "Fake speakers", isDefault: true, rate: 48000, nativeRate: 48000 },
    { index: 1, name: "Fake headphones", isDefault: false, rate: 44100, nativeRate: 44100 },
  ];
}

/** The fake add-on. Several hosts may exist (as with the real one); each has its own state. */
export function createFakeAddon(options: FakeOptions = {}): Addon {
  const resolved = {
    now: options.now ?? (() => performance.now()),
    prepareMs: options.prepareMs ?? 30,
    failPresets: options.failPresets ?? [],
  };
  return {
    version: () => ({ addon: "fake-0.1.0", snapshot: 2, nodeApi: 9 }),
    presets: () => PRESETS.map((p) => ({ ...p })),
    params: () => PARAMS.map((p) => ({ ...p, flags: [...p.flags] })),
    telemetry: () => TELEMETRY.map((t) => ({ ...t })),
    listDevices: (backend) => devicesFor(backend).map((d) => ({ ...d })),
    createHost(config: HostConfig): Host {
      if (!PRESETS.some((p) => p.key === config.initial.preset)) {
        throw addonError("RB_HOST_ERR_ARG", `no preset "${config.initial.preset}"`);
      }
      return new FakeHost(config, resolved, devicesFor(config.backend));
    },
  };
}
