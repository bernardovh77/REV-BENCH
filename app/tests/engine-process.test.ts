/*
 * The engine process's wiring (process.ts) with fake ports, and the add-on loader.
 */
import { describe, expect, it } from "vitest";
import { ADDON_FILE, loadAddon, nativeAddonCandidates, type LoaderDeps } from "../src/engine/addon-loader";
import { runEngineProcess, type ParentPortLike, type PortLike } from "../src/engine/process";
import { createFakeAddon } from "../src/engine/fake-addon";
import type { EngineInit, EngineToMain } from "../src/shared/process-messages";
import type { FromEngine } from "../src/shared/protocol";

class FakePort implements PortLike {
  sent: FromEngine[] = [];
  started = false;
  closed = false;
  private listeners: { message: ((e: { data: unknown }) => void)[]; close: (() => void)[] } = { message: [], close: [] };
  on(event: "message" | "close", listener: never): void {
    (this.listeners[event] as unknown[]).push(listener);
  }
  postMessage(message: unknown): void {
    this.sent.push(message as FromEngine);
  }
  start(): void {
    this.started = true;
  }
  close(): void {
    this.closed = true;
  }
  deliver(data: unknown): void {
    this.listeners.message.forEach((l) => l({ data }));
  }
  hangUp(): void {
    this.listeners.close.forEach((l) => l());
  }
}

class FakeParent implements ParentPortLike {
  sent: EngineToMain[] = [];
  private listener: ((e: { data: unknown; ports: PortLike[] }) => void) | null = null;
  on(_event: "message", listener: (e: { data: unknown; ports: PortLike[] }) => void): void {
    this.listener = listener;
  }
  postMessage(message: EngineToMain): void {
    this.sent.push(message);
  }
  deliver(data: unknown, ports: PortLike[] = []): void {
    this.listener!({ data, ports });
  }
}

const init: EngineInit = { addon: "fake", backend: "null", build: { preset: "i4" } };

function setup(loadAddon?: ProcessLoad) {
  const parent = new FakeParent();
  let tick: (() => void) | null = null;
  const exits: number[] = [];
  const cleared: unknown[] = [];
  let t = 0;
  const process = runEngineProcess({
    parentPort: parent,
    loadAddon: loadAddon ?? (() => createFakeAddon({ now: () => t })),
    setInterval: (cb) => {
      tick = cb;
      return "timer";
    },
    clearInterval: (h) => void cleared.push(h),
    exit: (code) => void exits.push(code),
  });
  return { parent, process, exits, cleared, pump: () => tick?.(), step: (ms: number) => void (t += ms) };
}
type ProcessLoad = Parameters<typeof runEngineProcess>[0]["loadAddon"];

describe("the engine process", () => {
  it("says ready after init, with the sample rate and the preset", () => {
    const { parent } = setup();
    parent.deliver({ type: "init", config: init });
    expect(parent.sent).toEqual([{ type: "ready", sampleRate: 48000, preset: "i4" }]);
  });

  it("says fatal and exits 1 when the add-on can't be loaded", () => {
    const { parent, exits } = setup(() => {
      throw new Error("the add-on rb_native.node was not found");
    });
    parent.deliver({ type: "init", config: { ...init, addon: "native" } });
    expect(parent.sent).toEqual([{ type: "fatal", message: "the add-on rb_native.node was not found" }]);
    expect(exits).toEqual([1]);
  });

  it("says fatal and exits 1 when the host can't be made (a bad preset)", () => {
    const { parent, exits } = setup();
    parent.deliver({ type: "init", config: { ...init, build: { preset: "nope" } } });
    expect(parent.sent[0]).toMatchObject({ type: "fatal" });
    expect(exits).toEqual([1]);
  });

  it("answers the window's port: hello, then replies to commands", () => {
    const { parent, pump } = setup();
    parent.deliver({ type: "init", config: init });
    const port = new FakePort();
    parent.deliver({ type: "connect" }, [port]);
    expect(port.started).toBe(true);
    expect(port.sent.map((m) => m.type)).toEqual(["hello", "state"]);
    port.deliver({ type: "command", id: 1, command: { kind: "set-param", name: "throttle", value: 0.3 } });
    pump();
    expect(port.sent.at(-1)).toMatchObject({ type: "reply", id: 1, ack: { outcome: "ok" } });
  });

  it("tells the main process what build a rebuild made, for the autosave", () => {
    const { parent, pump, step } = setup();
    parent.deliver({ type: "init", config: init });
    const port = new FakePort();
    parent.deliver({ type: "connect" }, [port]);
    port.deliver({ type: "command", id: 2, command: { kind: "load-preset", preset: "i6", quality: "low" } });
    step(100);
    pump();
    pump();
    expect(parent.sent).toContainEqual({ type: "build", preset: "i6", quality: "low" });
  });

  it("closes the old window port when a new one connects", () => {
    const { parent } = setup();
    parent.deliver({ type: "init", config: init });
    const first = new FakePort();
    const second = new FakePort();
    parent.deliver({ type: "connect" }, [first]);
    parent.deliver({ type: "connect" }, [second]);
    expect(first.closed).toBe(true);
    expect(second.sent[0]).toMatchObject({ type: "hello" });
  });

  it("warns, rather than failing, about a connect before init or without a port", () => {
    const { parent } = setup();
    parent.deliver({ type: "connect" }, [new FakePort()]);
    parent.deliver({ type: "init", config: init });
    parent.deliver({ type: "connect" }, []);
    expect(parent.sent.filter((m) => m.type === "log" && m.level === "warn")).toHaveLength(2);
  });

  it("detaches when the window's port closes", () => {
    const { parent, pump } = setup();
    parent.deliver({ type: "init", config: init });
    const port = new FakePort();
    parent.deliver({ type: "connect" }, [port]);
    const before = port.sent.length;
    port.hangUp();
    port.deliver({ type: "stats-request" });
    pump();
    expect(port.sent.length).toBe(before);
  });

  it("stops the timer and exits 0 on shutdown", () => {
    const { parent, exits, cleared } = setup();
    parent.deliver({ type: "init", config: init });
    parent.deliver({ type: "shutdown" });
    expect(cleared).toEqual(["timer"]);
    expect(exits).toEqual([0]);
  });

  it("logs, and goes on, for a message it doesn't know", () => {
    const { parent, exits } = setup();
    parent.deliver({ type: "dance" });
    expect(parent.sent[0]).toMatchObject({ type: "log", level: "warn" });
    expect(exits).toEqual([]);
  });
});

describe("the add-on loader", () => {
  const fake = createFakeAddon();
  const deps = (over: Partial<LoaderDeps> = {}): LoaderDeps => ({
    env: {},
    dirname: "/app/dist/engine",
    exists: () => false,
    load: () => {
      throw new Error("not loaded");
    },
    loadFake: () => fake,
    ...over,
  });

  it("gives the fake only when asked for it", () => {
    expect(loadAddon("fake", deps())).toBe(fake);
  });

  it("looks first where RB_NATIVE_PATH says, then in the package, then in the build tree", () => {
    const list = nativeAddonCandidates({ env: { RB_NATIVE_PATH: "/x/y.node" }, dirname: "/app/dist/engine", resourcesPath: "/res" });
    expect(list[0]).toBe("/x/y.node");
    expect(list[1]).toMatch(/res.rb_native\.node$/);
    expect(list[2].replaceAll("\\", "/")).toMatch(/\/app\/native\/build\/Release\/rb_native\.node$/);
  });

  it("loads the first candidate that exists", () => {
    const loaded: string[] = [];
    const addon = loadAddon("native", deps({
      env: { RB_NATIVE_PATH: "/first.node" },
      exists: (p) => p === "/first.node",
      load: (p) => (loaded.push(p), fake),
    }));
    expect(addon).toBe(fake);
    expect(loaded).toEqual(["/first.node"]);
  });

  it("says where it looked when the add-on isn't there, and never falls back to the fake", () => {
    expect(() => loadAddon("native", deps())).toThrow(new RegExp(ADDON_FILE.replace(".", "\\.") + ".*looked in"));
  });

  it("refuses a module that isn't the add-on, saying what it lacks", () => {
    expect(() => loadAddon("native", deps({ env: { RB_NATIVE_PATH: "/p.node" }, exists: () => true, load: () => ({ version() {} }) }))).toThrow(/lacks presets/);
    expect(() => loadAddon("native", deps({ env: { RB_NATIVE_PATH: "/p.node" }, exists: () => true, load: () => null }))).toThrow(/does not look like/);
  });
});
