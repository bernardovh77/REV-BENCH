/*
 * supervisor.ts - keeps the engine utility process alive (docs/app-design.md section 3). It:
 *
 *   - starts the engine with the autosaved build (the current preset), or the default one;
 *   - hands the window a MessagePort to the engine whenever the window asks and after every
 *     restart (a new process has new ports);
 *   - restarts a dead engine after a short, growing delay, with the build the engine last said
 *     it made (kept by the autosave), and tells the window each step;
 *   - gives up when the engine dies too often in a short while ("failed"), until the user asks
 *     again, and falls back to the default build when the saved one keeps the engine from starting.
 *
 * It knows nothing of Electron: the process, the channel, the timers and the window are
 * injected (engine-spawn.ts and main.ts give the real ones; the tests give fakes).
 */
import type { EngineStatus } from "../shared/bridge";
import type { BuildChoice, EngineInit, EngineToMain, MainToEngine } from "../shared/process-messages";
import { DEFAULT_BUILD } from "./autosave";
import type { Logger } from "./logger";

/** The engine process as the supervisor needs it. */
export interface EngineProcess<Port> {
  readonly pid: number | undefined;
  postMessage(message: MainToEngine, ports?: Port[]): void;
  kill(): void;
  onMessage(listener: (message: EngineToMain) => void): void;
  /** `code` is the exit code, or null if it was killed */
  onExit(listener: (code: number | null) => void): void;
}

export interface WindowEndpoint<Port> {
  sendPort(port: Port, restarts: number): void;
  sendStatus(status: EngineStatus): void;
}

export interface RestartPolicy {
  /** the pause before the n-th restart in a row (the last one repeats) */
  delaysMs: number[];
  /** more than `maxRestarts` deaths within `windowMs` is "failed" */
  maxRestarts: number;
  windowMs: number;
  /** after this many starts in a row that never got ready, the saved build is not tried again */
  failuresBeforeDefaultBuild: number;
  /** how long the engine gets to shut down before it is killed */
  shutdownMs: number;
}

export const DEFAULT_POLICY: RestartPolicy = {
  delaysMs: [200, 500, 1000, 2000, 4000],
  maxRestarts: 5,
  windowMs: 60_000,
  failuresBeforeDefaultBuild: 2,
  shutdownMs: 2000,
};

export interface SupervisorDeps<Port> {
  spawn(): EngineProcess<Port>;
  createChannel(): { port1: Port; port2: Port };
  autosave: { load(): BuildChoice | null; save(choice: BuildChoice): void };
  /** the engine's start-up message for a build (backend, add-on kind, ...) */
  engineInit(build: BuildChoice): EngineInit;
  logger: Logger;
  /** runs `fn` after `ms`; returns the function that cancels it */
  schedule(fn: () => void, ms: number): () => void;
  now(): number;
  policy?: Partial<RestartPolicy>;
}

export class EngineSupervisor<Port> {
  private readonly policy: RestartPolicy;
  private process: EngineProcess<Port> | null = null;
  private window: WindowEndpoint<Port> | null = null;
  private windowWantsPort = false;
  private status: EngineStatus = { state: "starting", restarts: 0 };
  private deaths: number[] = [];
  private startsWithoutReady = 0;
  private stopping = false;
  private cancelRestart: (() => void) | null = null;
  private lastFatal: string | null = null;

  constructor(private readonly deps: SupervisorDeps<Port>) {
    this.policy = { ...DEFAULT_POLICY, ...deps.policy };
  }

  /** The status the window sees now. */
  currentStatus(): EngineStatus {
    return { ...this.status };
  }

  start(): void {
    this.spawnEngine();
  }

  /** The window that gets the status and the ports; it is told the current status at once. */
  attachWindow(window: WindowEndpoint<Port>): void {
    this.window = window;
    window.sendStatus(this.currentStatus());
  }

  /** The window asks for a port to the engine; it gets one now, or when the engine is back. */
  requestPort(): void {
    this.windowWantsPort = true;
    this.connectWindow();
  }

  /** After "failed": try again from a clean slate. */
  restartNow(): void {
    if (this.process || this.stopping) return;
    this.cancelRestart?.();
    this.cancelRestart = null;
    this.deaths = [];
    this.startsWithoutReady = 0;
    this.spawnEngine();
  }

  /** Asks the engine to stop and doesn't restart it. */
  stop(): void {
    this.stopping = true;
    this.cancelRestart?.();
    this.cancelRestart = null;
    const process = this.process;
    if (!process) return;
    try {
      process.postMessage({ type: "shutdown" });
    } catch (e) {
      this.log("warn", `could not ask the engine to shut down: ${String(e)}`);
    }
    const cancel = this.deps.schedule(() => process.kill(), this.policy.shutdownMs);
    process.onExit(cancel);
  }

  private log(level: "info" | "warn" | "error", message: string): void {
    this.deps.logger.log(level, "main", message);
  }

  private setStatus(state: EngineStatus["state"], message?: string): void {
    this.status = {
      state,
      restarts: this.status.restarts,
      ...(this.process?.pid !== undefined && (state === "running" || state === "starting") ? { pid: this.process.pid } : {}),
      ...(message ? { message } : {}),
    };
    this.window?.sendStatus(this.currentStatus());
  }

  /** The build to start with: the saved one, unless it keeps the engine from starting. */
  private buildToStart(): BuildChoice {
    if (this.startsWithoutReady >= this.policy.failuresBeforeDefaultBuild) {
      this.log("warn", `the saved build failed ${this.startsWithoutReady} starts in a row; using the default build`);
      return DEFAULT_BUILD;
    }
    return this.deps.autosave.load() ?? DEFAULT_BUILD;
  }

  private spawnEngine(): void {
    const build = this.buildToStart();
    this.lastFatal = null;
    const process = this.deps.spawn();
    this.process = process;
    this.startsWithoutReady++;
    process.onMessage((message) => this.onMessage(process, message));
    process.onExit((code) => this.onExit(process, code));
    this.setStatus("starting");
    this.log("info", `engine started (pid ${process.pid ?? "?"}) with preset "${build.preset}"`);
    process.postMessage({ type: "init", config: this.deps.engineInit(build) });
    this.connectWindow();
  }

  private connectWindow(): void {
    const process = this.process;
    if (!process || !this.window || !this.windowWantsPort) return;
    const { port1, port2 } = this.deps.createChannel();
    process.postMessage({ type: "connect" }, [port1]);
    this.window.sendPort(port2, this.status.restarts);
  }

  private onMessage(from: EngineProcess<Port>, message: EngineToMain): void {
    if (from !== this.process) return;
    switch (message.type) {
      case "ready":
        this.startsWithoutReady = 0;
        this.setStatus("running");
        return;
      case "build": {
        const choice: BuildChoice = { preset: message.preset, ...(message.quality ? { quality: message.quality } : {}) };
        try {
          this.deps.autosave.save(choice);
        } catch (e) {
          this.log("error", `could not save the build: ${String(e)}`);
        }
        return;
      }
      case "log":
        this.deps.logger.log(message.level, "engine", message.message);
        return;
      case "fatal":
        this.lastFatal = message.message;
        this.deps.logger.log("error", "engine", `fatal: ${message.message}`);
        return;
    }
  }

  private onExit(from: EngineProcess<Port>, code: number | null): void {
    if (from !== this.process) return;
    this.process = null;
    if (this.stopping) {
      this.log("info", `engine stopped (exit ${code})`);
      return;
    }
    const why = this.lastFatal ?? `the engine process exited (code ${code === null ? "killed" : code})`;
    this.log("error", `engine died: ${why}`);
    const now = this.deps.now();
    this.deaths = [...this.deaths.filter((t) => now - t < this.policy.windowMs), now];
    if (this.deaths.length > this.policy.maxRestarts) {
      this.setStatus("failed", `${why}. It stopped ${this.deaths.length} times in ${Math.round(this.policy.windowMs / 1000)} seconds, so it was not restarted again.`);
      return;
    }
    const delays = this.policy.delaysMs;
    const delay = delays[Math.min(this.deaths.length - 1, delays.length - 1)];
    this.setStatus("restarting", `${why}. Restarting.`);
    this.cancelRestart = this.deps.schedule(() => {
      this.cancelRestart = null;
      if (this.stopping) return;
      this.status = { ...this.status, restarts: this.status.restarts + 1 };
      this.spawnEngine();
    }, delay);
  }
}
