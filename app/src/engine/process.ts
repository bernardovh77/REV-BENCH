/*
 * process.ts - the engine utility process, wired to its two kinds of port. main.ts passes the
 * real ones (Electron's `process.parentPort` and the MessagePortMain the main process hands
 * over); the tests pass fakes. The process:
 *
 *   init     -> load the add-on, make the host, start the pump timer, say "ready" (or "fatal")
 *   connect  -> the window's port: attach the control loop to it
 *   shutdown -> stop the host and exit
 */
import { EngineControl, type EnginePort } from "./control";
import type { Addon } from "../shared/addon";
import type { EngineInit, EngineToMain, MainToEngine } from "../shared/process-messages";

export interface PortLike {
  on(event: "message", listener: (event: { data: unknown }) => void): unknown;
  on(event: "close", listener: () => void): unknown;
  postMessage(message: unknown): void;
  start(): void;
  close(): void;
}

export interface ParentPortLike {
  on(event: "message", listener: (event: { data: unknown; ports: PortLike[] }) => void): unknown;
  postMessage(message: EngineToMain): void;
}

export interface ProcessDeps {
  parentPort: ParentPortLike;
  loadAddon(kind: EngineInit["addon"]): Addon;
  /** the control loop's pump, every `pumpMs` */
  setInterval(callback: () => void, ms: number): unknown;
  clearInterval(handle: unknown): void;
  exit(code: number): void;
  pumpMs?: number;
}

/** A port to the window as the control loop sees it. MessagePortMain can't transfer an ArrayBuffer, so snapshots are copied. */
export function windowPort(port: PortLike): EnginePort {
  return { post: (message) => port.postMessage(message) };
}

export function runEngineProcess(deps: ProcessDeps): { control: () => EngineControl | null } {
  let control: EngineControl | null = null;
  let timer: unknown = null;
  let window: PortLike | null = null;
  const say = (message: EngineToMain) => deps.parentPort.postMessage(message);
  const log = (level: "info" | "warn" | "error", message: string) => say({ type: "log", level, message });

  function stop(): void {
    if (timer !== null) deps.clearInterval(timer);
    timer = null;
    control?.shutdown();
  }

  function fatal(error: unknown): void {
    const message = error instanceof Error ? error.message : String(error);
    say({ type: "fatal", message });
    stop();
    deps.exit(1);
  }

  function init(config: EngineInit): void {
    if (control) return;
    try {
      const addon = deps.loadAddon(config.addon);
      const next = new EngineControl({
        addon,
        config,
        onBuild: (choice) => say({ type: "build", ...choice }),
        log,
      });
      next.start();
      control = next;
      timer = deps.setInterval(() => next.pump(), deps.pumpMs ?? 4);
      say({ type: "ready", sampleRate: next.sampleRate(), preset: config.build.preset });
    } catch (e) {
      fatal(e);
    }
  }

  function connect(port: PortLike | undefined): void {
    if (!control || !port) {
      log("warn", "a connect message arrived without a started engine or a port");
      return;
    }
    window?.close();
    window = port;
    port.on("message", (event) => control?.handle(event.data));
    port.on("close", () => {
      if (window === port) {
        window = null;
        control?.detach();
      }
    });
    port.start();
    control.attach(windowPort(port));
  }

  deps.parentPort.on("message", (event) => {
    const message = event.data as MainToEngine;
    switch (message?.type) {
      case "init":
        return init(message.config);
      case "connect":
        return connect(event.ports[0]);
      case "shutdown":
        stop();
        return deps.exit(0);
      default:
        log("warn", "ignored a message from the main process");
    }
  });

  return { control: () => control };
}
