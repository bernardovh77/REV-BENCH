/*
 * The restart policy: what the main process does when the engine process dies, starts, says it
 * is ready, or is asked to stop. All with fakes: no Electron, no real process, no real timers.
 */
import { describe, expect, it } from "vitest";
import type { EngineStatus } from "../src/shared/bridge";
import type { BuildChoice, EngineInit, EngineToMain, MainToEngine } from "../src/shared/process-messages";
import { DEFAULT_BUILD } from "../src/main/autosave";
import { EngineSupervisor, type EngineProcess, type RestartPolicy, type WindowEndpoint } from "../src/main/supervisor";

type Port = { name: string };

class FakeProcess implements EngineProcess<Port> {
  sent: { message: MainToEngine; ports: Port[] }[] = [];
  killed = false;
  private messageListeners: ((m: EngineToMain) => void)[] = [];
  private exitListeners: ((code: number | null) => void)[] = [];
  constructor(readonly pid: number) {}
  postMessage(message: MainToEngine, ports: Port[] = []): void {
    this.sent.push({ message, ports });
  }
  kill(): void {
    this.killed = true;
    this.die(null);
  }
  onMessage(l: (m: EngineToMain) => void): void {
    this.messageListeners.push(l);
  }
  onExit(l: (code: number | null) => void): void {
    this.exitListeners.push(l);
  }
  // the test's side
  say(m: EngineToMain): void {
    this.messageListeners.forEach((l) => l(m));
  }
  die(code: number | null): void {
    this.exitListeners.forEach((l) => l(code));
  }
  get init(): EngineInit | undefined {
    const m = this.sent.find((s) => s.message.type === "init")?.message;
    return m?.type === "init" ? m.config : undefined;
  }
  get connects(): Port[][] {
    return this.sent.filter((s) => s.message.type === "connect").map((s) => s.ports);
  }
}

function setup(options: { saved?: BuildChoice | null; policy?: Partial<RestartPolicy> } = {}) {
  return makeSupervisor(options.saved === undefined ? null : options.saved, options.policy);
}

function makeSupervisor(saved: BuildChoice | null, policy: Partial<RestartPolicy> = {}) {
  let time = 0;
  let pids = 100;
  const processes: FakeProcess[] = [];
  const timers: { fn: () => void; at: number; cancelled: boolean }[] = [];
  const saves: BuildChoice[] = [];
  const logs: string[] = [];
  const statuses: EngineStatus[] = [];
  const ports: { port: Port; restarts: number }[] = [];
  let saveFile: BuildChoice | null = saved;
  let channels = 0;
  const supervisor = new EngineSupervisor<Port>({
    spawn: () => {
      const p = new FakeProcess(++pids);
      processes.push(p);
      return p;
    },
    createChannel: () => ({ port1: { name: `engine-end-${++channels}` }, port2: { name: `window-end-${channels}` } }),
    autosave: {
      load: () => saveFile,
      save: (c) => {
        saves.push(c);
        saveFile = c;
      },
    },
    engineInit: (build) => ({ addon: "fake", backend: "null", build }),
    logger: { log: (level, source, message) => void logs.push(`${level} ${source}: ${message}`) },
    schedule: (fn, ms) => {
      const t = { fn, at: time + ms, cancelled: false };
      timers.push(t);
      return () => void (t.cancelled = true);
    },
    now: () => time,
    policy,
  });
  const window: WindowEndpoint<Port> = {
    sendPort: (port, restarts) => void ports.push({ port, restarts }),
    sendStatus: (s) => void statuses.push(s),
  };
  /** moves the clock and runs the timers that are due */
  const advance = (ms: number) => {
    time += ms;
    for (const t of timers.filter((x) => !x.cancelled && x.at <= time)) {
      t.cancelled = true;
      t.fn();
    }
  };
  return { supervisor, processes, saves, logs, statuses, ports, window, advance, timers, clock: () => time, current: () => processes.at(-1)! };
}

describe("starting", () => {
  it("starts the engine with the default build when nothing is saved", () => {
    const { supervisor, current } = setup();
    supervisor.start();
    expect(current().init?.build).toEqual(DEFAULT_BUILD);
  });

  it("starts it with the saved build (the current preset) when there is one", () => {
    const { supervisor, current } = setup({ saved: { preset: "v8-cross", quality: "low" } });
    supervisor.start();
    expect(current().init?.build).toEqual({ preset: "v8-cross", quality: "low" });
  });

  it("is 'starting' until the engine says ready, then 'running' with its pid", () => {
    const { supervisor, current, window, statuses } = setup();
    supervisor.attachWindow(window);
    supervisor.start();
    expect(statuses.at(-1)).toMatchObject({ state: "starting", restarts: 0 });
    current().say({ type: "ready", sampleRate: 48000, preset: "i4" });
    expect(statuses.at(-1)).toEqual({ state: "running", restarts: 0, pid: current().pid });
  });

  it("tells a window that attaches the current status at once", () => {
    const { supervisor, window, statuses } = setup();
    supervisor.start();
    supervisor.attachWindow(window);
    expect(statuses).toEqual([{ state: "starting", restarts: 0, pid: 101 }]);
  });
});

describe("the window's port", () => {
  it("is made when the window asks: one end to the engine, the other to the window", () => {
    const { supervisor, current, window, ports } = setup();
    supervisor.attachWindow(window);
    supervisor.start();
    expect(current().connects).toEqual([]); // the window hasn't asked
    supervisor.requestPort();
    expect(current().connects).toEqual([[{ name: "engine-end-1" }]]);
    expect(ports).toEqual([{ port: { name: "window-end-1" }, restarts: 0 }]);
  });

  it("is made afresh for a window that asks again (a reload)", () => {
    const { supervisor, current, window, ports } = setup();
    supervisor.attachWindow(window);
    supervisor.start();
    supervisor.requestPort();
    supervisor.requestPort();
    expect(current().connects).toHaveLength(2);
    expect(ports.map((p) => p.port.name)).toEqual(["window-end-1", "window-end-2"]);
  });

  it("waits for the engine when it is asked for during a restart, and arrives with it", () => {
    const { supervisor, current, window, ports, advance } = setup();
    supervisor.attachWindow(window);
    supervisor.start();
    current().die(1);
    supervisor.requestPort();
    expect(ports).toEqual([]);
    advance(200);
    expect(current().connects).toHaveLength(1);
    expect(ports).toHaveLength(1);
  });
});

describe("a dying engine", () => {
  it("is restarted after a pause, and the window is told why and that it is restarting", () => {
    const { supervisor, processes, window, statuses, advance } = setup();
    supervisor.attachWindow(window);
    supervisor.start();
    processes[0].say({ type: "ready", sampleRate: 48000, preset: "i4" });
    processes[0].die(3221225477);
    expect(statuses.at(-1)).toMatchObject({ state: "restarting", restarts: 0 });
    expect(statuses.at(-1)?.message).toMatch(/3221225477/);
    expect(processes).toHaveLength(1);
    advance(199);
    expect(processes).toHaveLength(1);
    advance(1);
    expect(processes).toHaveLength(2);
    expect(statuses.at(-1)).toMatchObject({ state: "starting", restarts: 1 });
  });

  it("is restarted with the build it last made: the autosaved preset", () => {
    const { supervisor, processes, advance, saves } = setup({ saved: { preset: "i4" } });
    supervisor.start();
    processes[0].say({ type: "ready", sampleRate: 48000, preset: "i4" });
    processes[0].say({ type: "build", preset: "v8-flat", quality: "low" });
    expect(saves).toEqual([{ preset: "v8-flat", quality: "low" }]);
    processes[0].die(1);
    advance(200);
    expect(processes[1].init?.build).toEqual({ preset: "v8-flat", quality: "low" });
  });

  it("is told the new port after the restart, if the window had asked", () => {
    const { supervisor, processes, window, ports, advance } = setup();
    supervisor.attachWindow(window);
    supervisor.start();
    supervisor.requestPort();
    processes[0].die(1);
    advance(200);
    expect(ports.map((p) => p.restarts)).toEqual([0, 1]);
    expect(processes[1].connects).toHaveLength(1);
  });

  it("says why when the engine reported a fatal error before it exited", () => {
    const { supervisor, processes, statuses, window, logs } = setup();
    supervisor.attachWindow(window);
    supervisor.start();
    processes[0].say({ type: "fatal", message: "the add-on rb_native.node was not found" });
    processes[0].die(1);
    expect(statuses.at(-1)?.message).toMatch(/rb_native\.node was not found/);
    expect(logs.join("\n")).toMatch(/error engine: fatal: the add-on/);
  });

  it("waits longer each time in a row, up to the last delay", () => {
    const { supervisor, processes, advance, timers, clock } = setup({
      policy: { maxRestarts: 100, delaysMs: [100, 200, 400] },
    });
    supervisor.start();
    const delays: number[] = [];
    for (let i = 0; i < 5; i++) {
      processes.at(-1)!.die(1);
      delays.push(timers.at(-1)!.at - clock());
      advance(delays.at(-1)!);
    }
    expect(delays).toEqual([100, 200, 400, 400, 400]);
  });

  it("gives up after too many deaths in a minute, and says so", () => {
    const { supervisor, processes, advance, statuses, window } = setup();
    supervisor.attachWindow(window);
    supervisor.start();
    for (let i = 0; i < 5; i++) {
      processes.at(-1)!.die(1);
      advance(5000);
    }
    expect(processes).toHaveLength(6);
    processes.at(-1)!.die(1); // the sixth death within the minute
    expect(statuses.at(-1)?.state).toBe("failed");
    expect(statuses.at(-1)?.message).toMatch(/not restarted again/);
    advance(60_000);
    expect(processes).toHaveLength(6);
  });

  it("forgets deaths older than the window", () => {
    const { supervisor, processes, advance, statuses } = setup({ policy: { windowMs: 1000, maxRestarts: 2 } });
    supervisor.start();
    for (let i = 0; i < 6; i++) {
      processes.at(-1)!.die(1);
      expect(statuses.at(-1)?.state ?? "restarting").not.toBe("failed");
      advance(5000);
    }
    expect(processes).toHaveLength(7);
  });

  it("starts again on the user's request after failing, from a clean slate", () => {
    const { supervisor, processes, advance, statuses, window } = setup();
    supervisor.attachWindow(window);
    supervisor.start();
    for (let i = 0; i < 5; i++) {
      processes.at(-1)!.die(1);
      advance(5000);
    }
    processes.at(-1)!.die(1);
    expect(statuses.at(-1)?.state).toBe("failed");
    supervisor.restartNow();
    expect(processes).toHaveLength(7);
    expect(statuses.at(-1)?.state).toBe("starting");
    processes.at(-1)!.die(1);
    expect(statuses.at(-1)?.state).toBe("restarting");
  });

  it("ignores restartNow() while an engine is running", () => {
    const { supervisor, processes } = setup();
    supervisor.start();
    supervisor.restartNow();
    expect(processes).toHaveLength(1);
  });

  it("falls back to the default build when the saved one keeps the engine from starting", () => {
    const { supervisor, processes, advance, logs } = setup({ saved: { preset: "gone" } });
    supervisor.start();
    expect(processes[0].init?.build.preset).toBe("gone");
    processes[0].die(1); // never said ready
    advance(200);
    expect(processes[1].init?.build.preset).toBe("gone");
    processes[1].die(1);
    advance(500);
    expect(processes[2].init?.build).toEqual(DEFAULT_BUILD);
    expect(logs.join("\n")).toMatch(/using the default build/);
  });

  it("does not count a crash after ready as a failed start", () => {
    const { supervisor, processes, advance } = setup({ saved: { preset: "v8-flat" } });
    supervisor.start();
    for (let i = 0; i < 4; i++) {
      processes.at(-1)!.say({ type: "ready", sampleRate: 48000, preset: "v8-flat" });
      processes.at(-1)!.die(1);
      advance(5000);
    }
    expect(processes.at(-1)!.init?.build.preset).toBe("v8-flat");
  });
});

describe("what the engine says", () => {
  it("has its log lines written to the log, as the engine's", () => {
    const { supervisor, current, logs } = setup();
    supervisor.start();
    current().say({ type: "log", level: "warn", message: "ring nearly empty" });
    expect(logs).toContain("warn engine: ring nearly empty");
  });

  it("survives a failing autosave: the engine goes on", () => {
    const { supervisor, current, logs } = makeWithBrokenSave();
    supervisor.start();
    expect(() => current().say({ type: "build", preset: "i6" })).not.toThrow();
    expect(logs.join("\n")).toMatch(/could not save the build/);
  });

  it("ignores messages from an engine that has already been replaced", () => {
    const { supervisor, processes, advance, saves } = setup();
    supervisor.start();
    const old = processes[0];
    old.die(1);
    advance(200);
    old.say({ type: "build", preset: "i6" });
    expect(saves).toEqual([]);
  });
});

function makeWithBrokenSave() {
  const logs: string[] = [];
  let process: FakeProcess | null = null;
  const supervisor = new EngineSupervisor<Port>({
    spawn: () => (process = new FakeProcess(1)),
    createChannel: () => ({ port1: { name: "a" }, port2: { name: "b" } }),
    autosave: {
      load: () => null,
      save: () => {
        throw new Error("disk full");
      },
    },
    engineInit: (build) => ({ addon: "fake", backend: "null", build }),
    logger: { log: (l, s, m) => void logs.push(`${l} ${s}: ${m}`) },
    schedule: () => () => {},
    now: () => 0,
  });
  return { supervisor, logs, current: () => process! };
}

describe("stopping", () => {
  it("asks the engine to shut down and does not restart it", () => {
    const { supervisor, processes, advance } = setup();
    supervisor.start();
    supervisor.stop();
    expect(processes[0].sent.at(-1)?.message).toEqual({ type: "shutdown" });
    processes[0].die(0);
    advance(10_000);
    expect(processes).toHaveLength(1);
  });

  it("kills an engine that doesn't shut down in time", () => {
    const { supervisor, processes, advance } = setup();
    supervisor.start();
    supervisor.stop();
    advance(1999);
    expect(processes[0].killed).toBe(false);
    advance(1);
    expect(processes[0].killed).toBe(true);
  });

  it("does not kill an engine that exited by itself", () => {
    const { supervisor, processes, advance } = setup();
    supervisor.start();
    supervisor.stop();
    processes[0].die(0);
    advance(5000);
    expect(processes[0].killed).toBe(false);
  });

  it("cancels a restart that was waiting", () => {
    const { supervisor, processes, advance } = setup();
    supervisor.start();
    processes[0].die(1);
    supervisor.stop();
    advance(10_000);
    expect(processes).toHaveLength(1);
  });
});
