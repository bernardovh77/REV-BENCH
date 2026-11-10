/*
 * addon.ts - the contract between the engine process and the Node-API add-on
 * (stage 124). Types only: the add-on (app/native/, C) implements it, the
 * engine process (app/src/engine/) uses it, and its tests use a fake that
 * follows it. Neither side changes it alone: a change is a contract change,
 * made here first, then on both sides.
 *
 * The add-on wraps host/include/rb_host.h and the parts of core/include/rb.h
 * a host needs (presets, parameter and telemetry metadata). It is loaded
 * only in the engine utility process, never in the main process or the
 * window. Every call is synchronous and fast (nothing waits on the render
 * thread); errors throw an AddonError.
 *
 * Numbers: 64-bit ids and counters from C (request ids, generations, ticks,
 * audio frames) are JavaScript numbers; they stay below 2^53 for centuries
 * at these rates.
 */

/** Thrown for a bad argument or a host error: `code` is the C name. */
export interface AddonError extends Error {
  /** e.g. "RB_HOST_ERR_FULL", "RB_HOST_ERR_ARG", "RB_ERR_ID", "DESTROYED" */
  code: string;
}

export type Backend = "null" | "default";

export interface DeviceInfo {
  index: number;
  name: string;
  isDefault: boolean;
  /** the rate the host renders at on this device: 44100 or 48000 */
  rate: number;
  /** the device's own rate, if it reports one */
  nativeRate: number | null;
}

export interface PresetInfo {
  index: number;
  /** "i4", "v8-cross", ... (rb_preset_key) */
  key: string;
  /** "2.0 L inline-4", ... (rb_preset_name) */
  name: string;
  /** its real-time default quality (rb_preset_quality): a QUALITY name */
  quality: Quality;
}

export type Quality = "low" | "medium" | "high" | "ultra" | "balanced";

export interface ParamInfo {
  id: number;
  /** rb_param_name: "throttle", "ignition", ... */
  name: string;
  unit: string;
  min: number;
  max: number;
  default: number;
  /** "control" (a driver's input), "rebuild" (needs a new build), "integer" */
  flags: string[];
}

export interface TelemetryInfo {
  id: number;
  name: string;
  unit: string;
}

/** What an engine build is (rb_host_build). */
export interface BuildSpec {
  /** a preset key */
  preset: string;
  /** the quality; default: the preset's real-time default */
  quality?: Quality;
  /** a stereo scene, RB_SCENE_* lowercased ("asphalt", ..., "passby", "cabin"), or none for the mono listener */
  scene?: string;
  /** blocks rendered and thrown away before the swap (rb_host_build.settle_blocks) */
  settleBlocks?: number;
  /** engine-field edits by rb_engine_field id */
  edits?: { field: number; index: number; value: number }[];
  /** parameters by rb_param id, applied when the engine is built */
  params?: { id: number; value: number }[];
}

export interface HostConfig {
  backend: Backend;
  /** a device index from listDevices; -1 or absent: the system's default */
  device?: number;
  /** null backend only: 44100 or 48000 (default 48000) */
  sampleRate?: number;
  /** the ring's fill target, 10..80 ms (default 20) */
  targetFillMs?: number;
  /** the first engine */
  initial: BuildSpec;
}

export type Outcome = "ok" | "stale" | "refused" | "busy";

/** One acknowledgment (rb_host_ack). */
export interface Ack {
  requestId: number;
  /** "set-param", "set-engine-live", "dyno-attach", ..., "swap" */
  kind: string;
  outcome: Outcome;
  /** the core's return code, when it has one */
  code: number;
  /** the generation after it */
  generation: number;
  /** a new engine started (a swap or a rebase): its state is fresh */
  reset: boolean;
  physicsTick: number;
  audioFrame: number;
}

export interface SnapshotRead {
  /** the core's snapshot-v2 bytes (contracts/snapshot-v2.md), a fresh copy */
  bytes: ArrayBuffer;
  sequence: number;
  generation: number;
  audioFrame: number;
  physicsTick: number;
}

export type PrepareState = "pending" | "ready" | "failed";

export interface HostStats {
  blocks: number;
  renderMsMean: number;
  renderMsMax: number;
  /** deadline histogram: share of blocks per 10 % of the block's duration, then 100-200 %, 200 %+ */
  deadline: number[];
  ringFillMs: number;
  ringTargetMs: number;
  underruns: number;
  commandsApplied: number;
  commandsRefused: number;
  swapsOk: number;
  eventsLost: number;
  /** the render thread's priority in words, and whether it got the boost */
  priority: string;
  boosted: boolean;
  /** forbidden calls on the render thread: must stay 0 */
  forbiddenCalls: number;
  processMb: number;
}

/** A running real-time host (rb_host). */
export interface Host {
  start(): void;
  stop(): void;
  /** stops it if running and frees everything; any later call throws DESTROYED */
  destroy(): void;
  sampleRate(): number;
  generation(): number;

  /** commands: each returns its request id; the ack comes through poll() */
  setParam(id: number, value: number, expectedGeneration?: number): number;
  setEngineLive(field: number, index: number, value: number, expectedGeneration?: number): number;

  /** rebuilds: prepare off the render thread, then swap with a fade */
  prepare(spec: BuildSpec): number;
  prepareState(prepareId: number): { state: PrepareState; code: number; message: string };
  swap(prepareId: number, expectedGeneration?: number): number;
  discard(prepareId: number): void;

  /** the acknowledgments that have arrived since the last poll */
  poll(): Ack[];
  /** the newest snapshot nobody has read, or null */
  readSnapshot(): SnapshotRead | null;
  stats(): HostStats;

  /** devices and power */
  selectDevice(index: number): void;
  suspend(): void;
  resume(): void;
}

/** The module `require("rb_native.node")` returns. */
export interface Addon {
  /** the add-on's, the core's and the snapshot's versions */
  version(): { addon: string; snapshot: number; nodeApi: number };
  presets(): PresetInfo[];
  params(): ParamInfo[];
  telemetry(): TelemetryInfo[];
  listDevices(backend: Backend): DeviceInfo[];
  createHost(config: HostConfig): Host;
}
