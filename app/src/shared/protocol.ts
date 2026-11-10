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
import type { Ack, DeviceInfo, HostStats, ParamInfo, PresetInfo, Quality, TelemetryInfo } from "./addon";

export const PROTOCOL_VERSION = 1;

/* ---- window -> engine ------------------------------------------------ */

export type Command =
  /** a driver's control or a setting, by the core's parameter name */
  | { kind: "set-param"; name: string; value: number }
  /** a rebuild: prepared off the render thread, swapped in with a fade */
  | { kind: "load-preset"; preset: string; quality?: Quality }
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
  | { type: "state"; state: EngineState; message?: string };
