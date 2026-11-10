/*
 * process-messages.ts - the messages between the main process and the engine process
 * over the utility process's own channel (the window's messages are protocol.ts).
 * The main process never reads a snapshot or a command: it starts the engine, hands it
 * the window's port, learns what build is running (for the autosave) and hears its logs.
 */
import type { Backend, Quality } from "./addon";

/** What the engine process builds first (and what the autosave keeps). */
export interface BuildChoice {
  preset: string;
  quality?: Quality;
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
}

export type MainToEngine =
  | { type: "init"; config: EngineInit }
  /** the window's end of a new MessagePort arrives with this message as ports[0]; it replaces any earlier one */
  | { type: "connect" }
  | { type: "shutdown" };

export type EngineToMain =
  /** the host runs and the engine process answers on its port */
  | { type: "ready"; sampleRate: number; preset: string }
  /** a rebuild took effect: this is what to autosave */
  | ({ type: "build" } & BuildChoice)
  | { type: "log"; level: "info" | "warn" | "error"; message: string }
  /** it can't go on (no add-on, no host): it exits right after */
  | { type: "fatal"; message: string };
