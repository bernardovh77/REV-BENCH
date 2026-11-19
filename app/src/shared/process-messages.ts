/*
 * process-messages.ts - the messages between the main process and the engine process
 * over the utility process's own channel (the window's messages are protocol.ts).
 * The main process never reads a snapshot or a command: it starts the engine, hands it
 * the window's port, learns what build is running (for the autosave) and hears its logs.
 */
import type { Backend, LayoutSpec, Quality, Scene } from "./addon";

/** One change to the engine's description, by the core's field name (stage 067): names stay
 *  meaningful across versions, where ids may not. `index` is the bank, cylinder or place in the
 *  firing order for the fields that have one value each, else 0. */
export interface EngineEdit {
  field: string;
  index: number;
  value: number;
}

/** What the engine process builds first (and what the autosave keeps). */
export interface BuildChoice {
  preset: string;
  quality?: Quality;
  /** a stereo scene; absent: the mono listener */
  scene?: Scene;
  /** the latency setting, ms (the host's ring target); absent: the host's 20. An audio setting
   *  rather than part of the engine, kept with the build so a restart comes back with it. */
  latencyMs?: number;
  /** the editor's changes to the preset's engine, in the order they were made (stage 067) */
  edits?: EngineEdit[];
  /** the engine as this layout, started from the preset (stage 131); absent: the preset's own */
  layout?: LayoutSpec;
}

export interface EngineInit {
  /** "native": the real rb_native.node; "fake": the test double (src/engine/fake-addon.ts) */
  addon: "native" | "fake";
  /** "default": the system's sound device; "null": no device (tests) */
  backend: Backend;
  /** null backend only: 44100 or 48000 */
  sampleRate?: number;
  /** a device index, or absent for the system's default */
  device?: number;
  build: BuildChoice;
  /**
   * tests only (RB_CAPTURE_SECONDS): the host's capture tap, this many seconds of it, and the
   * drive recorded as a trace; the window's `capture-dump` writes both (stage 066's evidence)
   */
  captureSeconds?: number;
  /** where capture-dump writes (the user-data folder), with captureSeconds */
  captureDir?: string;
}

export type MainToEngine =
  | { type: "init"; config: EngineInit }
  /** the window's end of a new MessagePort arrives with this message as ports[0]; it replaces any earlier one */
  | { type: "connect" }
  /** fades the sound out (QUIT_FADE_MS), then stops the host and exits */
  | { type: "shutdown" }
  /** the system is going to sleep, and has woken (Electron's powerMonitor, stage 066) */
  | { type: "suspend" }
  | { type: "resume" };

export type EngineToMain =
  /** the host runs and the engine process answers on its port */
  | { type: "ready"; sampleRate: number; preset: string }
  /** a rebuild took effect: this is what to autosave */
  | ({ type: "build" } & BuildChoice)
  | { type: "log"; level: "info" | "warn" | "error"; message: string }
  /** it can't go on (no add-on, no host): it exits right after */
  | { type: "fatal"; message: string };
