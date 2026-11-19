/*
 * protocol.ts - the messages between the window and the engine process
 * (stage 124), over the one MessagePort the main process hands both
 * (docs/app-design.md section 3). Types only, shared by both sides; a change
 * here is a contract change.
 *
 * The window never waits for the engine: it sends commands and asks for a
 * snapshot once per animation frame, and draws the newest it has. Snapshot
 * bytes travel as a transferred ArrayBuffer (no copy on the way).
 */
import type { Ack, DeviceInfo, EngineFieldInfo, HostStats, LayoutInfo, LayoutSpec, ParamInfo, PresetInfo, Quality, Scene, TelemetryInfo } from "./addon";
import type { EngineEdit } from "./process-messages";

export const PROTOCOL_VERSION = 1;

/* ---- window -> engine ------------------------------------------------ */

export type Command =
  /** a driver's control or a setting, by the core's parameter name */
  | { kind: "set-param"; name: string; value: number }
  /**
   * the driver's throttle, 0..1 (stage 066). `live` for the styles the dead-man rule covers (a
   * ramped key, an analog key, a trigger): the window then sends it at a steady beat, and if
   * none arrives for 250 ms the engine process ramps the throttle to zero over 100 ms. The
   * on/off and latched styles send `live: false` and hold.
   */
  | { kind: "throttle"; value: number; live: boolean }
  /**
   * a rebuild: prepared off the render thread, swapped in with a fade. `quality` absent: the
   * preset's own tier; `scene` absent or null: the mono listener (stage 066)
   */
  | { kind: "load-preset"; preset: string; quality?: Quality; scene?: Scene | null; edits?: EngineEdit[] }
  /**
   * stage 067: the current preset rebuilt with exactly these edits (its quality and scene kept),
   * as one transaction: checked first, then prepared and swapped in with the fade. If any edit is
   * refused nothing changes and the reply says why. An empty list is the preset as it comes.
   * With `layout` (stage 131) the engine is rebuilt as that layout too (null: the preset's own),
   * exactly these edits on it: how the editor's undo brings back a layout with its own edits.
   */
  | { kind: "apply-edits"; edits: EngineEdit[]; layout?: LayoutSpec | null }
  /** a live field (its flags say "live") changed on the running engine, and kept for rebuilds */
  | { kind: "edit-live"; edit: EngineEdit }
  /** the engine these edits would make, without building it: a `description` message answers;
   *  with `layout` (stage 131), on that layout instead (null: the preset's own) */
  | { kind: "describe"; edits: EngineEdit[]; layout?: LayoutSpec | null }
  /**
   * stage 131: the current preset rebuilt as this layout (null: the preset's own), its quality,
   * scene and the edits a new layout keeps (layout-edits.ts), as one transaction like apply-edits
   */
  | { kind: "set-layout"; layout: LayoutSpec | null }
  /**
   * the volume, 0..VOLUME_MAX (1 is the app's level: real relative loudness between engines,
   * with headroom). The engine process sets master_gain and keeps it for every rebuild.
   */
  | { kind: "volume"; value: number }
  /** the latency, ms (10..80): the sound restarts with it, and the engine greets the window again */
  | { kind: "set-latency"; ms: number }
  /**
   * tests only: writes the capture tap's audio and the drive's trace (EngineInit.captureSeconds),
   * then sends a `capture` message; refused when the engine was started without them
   */
  | { kind: "capture-dump" }
  | { kind: "select-device"; index: number };

export type ToEngine =
  /** `id` is the window's own, echoed in the reply */
  | { type: "command"; id: number; command: Command }
  /** the newest snapshot, if there is one newer than the last sent */
  | { type: "snapshot-request" }
  | { type: "stats-request" };

/* ---- engine -> window ------------------------------------------------ */

export type EngineState = "starting" | "running" | "stopped" | "error";

export type FromEngine =
  /** first message once the host runs: what the engine offers */
  | {
      type: "hello";
      protocol: number;
      addonVersion: string;
      snapshotVersion: number;
      presets: PresetInfo[];
      params: ParamInfo[];
      telemetry: TelemetryInfo[];
      devices: DeviceInfo[];
      sampleRate: number;
      preset: string;
      /** what is built now: the preset, and its quality and scene when chosen (stage 066) */
      build?: { preset: string; quality?: Quality; scene?: Scene; latencyMs?: number; edits?: EngineEdit[]; layout?: LayoutSpec };
      /** the core's engine fields, and the values of the engine playing (stage 067) */
      engineFields?: EngineFieldInfo[];
      description?: number[][];
      /** the layouts the core builds (stage 131) */
      layouts?: LayoutInfo[];
      /** the driver's live settings every build keeps: the volume, and the listener and variant once set */
      settings?: { volume: number; listener?: number; variant?: number; afr?: number; spark_offset?: number; layers?: number };
      generation: number;
    }
  /** a command's outcome: `ack` once the render thread took it (a rebuild's
   * when the swap is done), or `error` if it never got that far */
  | { type: "reply"; id: number; ack?: Ack; error?: { code: string; message: string } }
  | {
      type: "snapshot";
      bytes: ArrayBuffer;
      sequence: number;
      generation: number;
      audioFrame: number;
    }
  | { type: "stats"; stats: HostStats }
  /**
   * the sound moved to another device (stage 066): `lost`, the device went away or the system's
   * default changed and the host followed it, and the window says so (app-design section 15);
   * `selected`, the user chose it. `playing` names the device now playing.
   */
  | { type: "device"; reason: "lost" | "selected"; playing: string; devices: DeviceInfo[] }
  /**
   * an engine's values, field by field (addon.ts describe): `request` is the describe command's
   * id, or 0 for the engine now playing (sent after every rebuild and live edit). `edits` are the
   * ones it was described with.
   */
  | { type: "description"; request: number; preset: string; edits: EngineEdit[]; values: number[][]; layout?: LayoutSpec }
  /**
   * capture-dump's result: the files written (in the user-data folder), the frames, the blocks
   * the tap dropped, and whether a device change or sleep broke the trace
   */
  | { type: "capture"; audio: string; trace: string; frames: number; rate: number; dropped: number; interrupted: boolean }
  | { type: "state"; state: EngineState; message?: string };
