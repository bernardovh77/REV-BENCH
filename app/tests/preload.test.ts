/*
 * The preload's API (bridge-impl.ts) with a fake ipcRenderer.
 */
import { describe, expect, it } from "vitest";
import { ENGINE_PORT_MESSAGE, IPC, safeFileName, type EnginePortMessage, type EngineStatus } from "../src/shared/bridge";
import { makeBridge, type IpcLike } from "../src/preload/bridge-impl";

type Listener = (event: { ports: MessagePort[] }, payload: unknown) => void;

function fakeIpc(current: unknown = { state: "running", restarts: 0 }) {
  const listeners = new Map<string, Listener[]>();
  const sent: string[] = [];
  const ipc: IpcLike = {
    invoke: async (channel) => (channel === IPC.getEngineStatus ? current : null),
    send: (channel) => void sent.push(channel),
    on: (channel, l) => void listeners.set(channel, [...(listeners.get(channel) ?? []), l]),
    removeListener: (channel, l) => void listeners.set(channel, (listeners.get(channel) ?? []).filter((x) => x !== l)),
  };
  const emit = (channel: string, payload: unknown, ports: MessagePort[] = []) =>
    (listeners.get(channel) ?? []).forEach((l) => l({ ports }, payload));
  return { ipc, sent, emit, listeners };
}

function setup(current?: unknown) {
  const f = fakeIpc(current);
  const posted: { message: EnginePortMessage; ports: MessagePort[] }[] = [];
  const bridge = makeBridge({
    ipc: f.ipc,
    platform: "win32",
    versions: { electron: "45.0.0", chrome: "140" },
    postToPage: (message, ports) => void posted.push({ message, ports }),
  });
  return { ...f, bridge, posted };
}

describe("the preload's API", () => {
  it("says the platform and the versions", () => {
    const { bridge } = setup();
    expect(bridge.platform).toBe("win32");
    expect(bridge.versions).toEqual({ electron: "45.0.0", chrome: "140" });
  });

  it("exposes only the seven names the type promises, no ipc object", () => {
    expect(Object.keys(setup().bridge).sort()).toEqual(["builds", "onEngineStatus", "platform", "requestEnginePort", "restartEngine", "saveText", "versions"]);
    expect(Object.keys(setup().bridge.builds!).sort()).toEqual(["list", "read", "remove", "save"]);
  });

  it("asks for the saved builds on their own channels, and takes only what fits the type back", async () => {
    const calls: unknown[][] = [];
    const answers: Record<string, unknown> = {
      [IPC.listBuilds]: "not a list",
      [IPC.readBuild]: undefined,
      [IPC.saveBuild]: null,
      [IPC.deleteBuild]: "yes",
    };
    const ipc: IpcLike = {
      invoke: async (channel, ...args) => (calls.push([channel, ...args]), answers[channel]),
      send: () => {},
      on: () => {},
      removeListener: () => {},
    };
    const builds = makeBridge({ ipc, platform: "win32", versions: { electron: "", chrome: "" }, postToPage: () => {} }).builds!;
    expect(await builds.list()).toEqual([]);
    expect(await builds.read("ls7")).toBeNull();
    expect(await builds.save("LS7", { preset: "v8-cross" }, { afr: 14 }, false)).toEqual({ ok: false, reason: "bad-build" });
    expect(await builds.remove("ls7")).toBe(false);
    expect(calls).toEqual([
      [IPC.listBuilds],
      [IPC.readBuild, "ls7"],
      [IPC.saveBuild, "LS7", { preset: "v8-cross" }, { afr: 14 }, false],
      [IPC.deleteBuild, "ls7"],
    ]);
  });

  it("asks the main process for a port, and for a restart, on fixed channels", () => {
    const { bridge, sent } = setup();
    bridge.requestEnginePort();
    bridge.restartEngine();
    expect(sent).toEqual([IPC.requestEnginePort, IPC.restartEngine]);
  });

  it("posts a port from the main process to the page, with the restart count", () => {
    const { emit, posted } = setup();
    const port = {} as MessagePort;
    emit(IPC.enginePort, { restarts: 2 }, [port]);
    expect(posted).toEqual([{ message: { channel: ENGINE_PORT_MESSAGE, restarts: 2 }, ports: [port] }]);
  });

  it("posts a port even when the payload is malformed, with restarts 0", () => {
    const { emit, posted } = setup();
    emit(IPC.enginePort, null, [{} as MessagePort]);
    expect(posted[0].message.restarts).toBe(0);
  });

  it("gives the current status at once and then each change", async () => {
    const { bridge, emit } = setup({ state: "starting", restarts: 0 });
    const seen: EngineStatus[] = [];
    bridge.onEngineStatus((s) => seen.push(s));
    await Promise.resolve();
    await Promise.resolve();
    emit(IPC.engineStatus, { state: "running", restarts: 0, pid: 5 });
    expect(seen.map((s) => s.state)).toEqual(["starting", "running"]);
  });

  it("ignores a status that isn't one", async () => {
    const { bridge, emit } = setup(null);
    const seen: EngineStatus[] = [];
    bridge.onEngineStatus((s) => seen.push(s));
    await Promise.resolve();
    emit(IPC.engineStatus, "nonsense");
    emit(IPC.engineStatus, { state: "running" });
    expect(seen).toEqual([]);
  });

  it("stops listening when the returned function is called", async () => {
    const { bridge, emit, listeners } = setup();
    const seen: EngineStatus[] = [];
    const stop = bridge.onEngineStatus((s) => seen.push(s));
    stop();
    await Promise.resolve();
    await Promise.resolve();
    emit(IPC.engineStatus, { state: "running", restarts: 0 });
    expect(seen).toEqual([]);
    expect(listeners.get(IPC.engineStatus)).toEqual([]);
  });
});

describe("saving a text file (stage 068)", () => {
  it("asks the main process on its own channel, and is true only for a written file", async () => {
    const calls: unknown[][] = [];
    let answer: unknown = true;
    const ipc: IpcLike = {
      invoke: async (channel, ...args) => (calls.push([channel, ...args]), answer),
      send: () => {},
      on: () => {},
      removeListener: () => {},
    };
    const bridge = makeBridge({ ipc, platform: "win32", versions: { electron: "", chrome: "" }, postToPage: () => {} });
    expect(await bridge.saveText!("dyno.csv", "a,b\n")).toBe(true);
    answer = "yes";
    expect(await bridge.saveText!("dyno.csv", "a,b\n")).toBe(false);
    expect(calls).toEqual([[IPC.saveText, "dyno.csv", "a,b\n"], [IPC.saveText, "dyno.csv", "a,b\n"]]);
  });

  it("makes a suggested name safe for every system: never a path", () => {
    expect(safeFileName("dyno-i4-2026-11-19.csv")).toBe("dyno-i4-2026-11-19.csv");
    expect(safeFileName("../../etc/passwd")).toBe("_.._etc_passwd.txt");
    expect(safeFileName("C:\\Windows\\x.csv")).toBe("C__Windows_x.csv");
    expect(safeFileName("")).toBe("export.txt");
    expect(safeFileName(42)).toBe("export.txt");
    expect(safeFileName("notes")).toBe("notes.txt");
  });
});
