/*
 * bridge.ts - what the window gets from the main process through the preload script
 * (src/preload/preload.ts): a small typed API on `window.revbench`, and the names of the
 * IPC channels behind it. The engine itself is not here: the window talks to the engine
 * process over a MessagePort (protocol.ts), which the preload hands to the page.
 */

/** The engine process, as the main process sees it. */
export type EngineProcessState = "starting" | "running" | "restarting" | "failed";

export interface EngineStatus {
  state: EngineProcessState;
  /** how many times it has been restarted since the app started */
  restarts: number;
  /** the engine process's id while it runs (the end-to-end test kills it) */
  pid?: number;
  /** what the window may say: why it restarted, or why it gave up */
  message?: string;
}

export interface RevBenchBridge {
  readonly platform: string;
  readonly versions: { readonly electron: string; readonly chrome: string };
  /** the current status now, then each change; returns the function that stops listening */
  onEngineStatus(callback: (status: EngineStatus) => void): () => void;
  /**
   * Asks the main process for a (new) port to the engine. It arrives as a window "message"
   * event whose data is { channel: ENGINE_PORT_MESSAGE, restarts } and whose `ports[0]` is the
   * port (a port can't cross the context bridge as an argument, so the preload posts it).
   */
  requestEnginePort(): void;
  /** after "failed": the user's request to try the engine again */
  restartEngine(): void;
}

declare global {
  interface Window {
    revbench?: RevBenchBridge;
  }
}

/** The data of the window message that carries the engine's port. */
export const ENGINE_PORT_MESSAGE = "rev-bench:engine-port";

export interface EnginePortMessage {
  channel: typeof ENGINE_PORT_MESSAGE;
  restarts: number;
}

/** IPC channel names, shared by the main process and the preload script. */
export const IPC = {
  /** main -> window: an EngineStatus */
  engineStatus: "rb:engine-status",
  /** window -> main (invoke): the current EngineStatus */
  getEngineStatus: "rb:get-engine-status",
  /** main -> window: a port to the engine (with the restart count) */
  enginePort: "rb:engine-port",
  /** window -> main: asks for a port */
  requestEnginePort: "rb:request-engine-port",
  /** window -> main: try the engine again */
  restartEngine: "rb:restart-engine",
} as const;
