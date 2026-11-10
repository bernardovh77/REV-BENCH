// @vitest-environment jsdom
/*
 * The whole interface, mounted, against the real control loop and the fake add-on over a real
 * MessageChannel: what a user sees and does, with only Electron left out. The fake bridge plays
 * the preload's part (a status feed and a port).
 */
import { MessageChannel } from "node:worker_threads";
import { afterEach, describe, expect, it, vi } from "vitest";
import { flushSync, mount, unmount } from "svelte";
import { EngineControl } from "../src/engine/control";
import { createFakeAddon } from "../src/engine/fake-addon";
import { ENGINE_PORT_MESSAGE, type EngineStatus, type RevBenchBridge } from "../src/shared/bridge";
import App from "../src/ui/App.svelte";
import { EngineStore, type PortEvent } from "../src/ui/lib/engine-store.svelte";

const cleanups: (() => void)[] = [];
afterEach(() => {
  while (cleanups.length) cleanups.pop()!();
  document.body.innerHTML = "";
  delete document.documentElement.dataset.theme;
});

async function launch(options: { bridge?: boolean; preset?: string } = {}) {
  let t = 1000;
  const control = new EngineControl({
    addon: createFakeAddon({ now: () => t }),
    config: { addon: "fake", backend: "null", build: { preset: options.preset ?? "i4" } },
    onBuild: () => {},
    log: () => {},
    now: () => t,
  });
  control.start();
  const channel = new MessageChannel();
  channel.port1.on("message", (m) => control.handle(m));

  let pageListener: ((e: PortEvent) => void) | null = null;
  const fakeWindow = {
    addEventListener: (_t: "message", l: (e: PortEvent) => void) => void (pageListener = l),
    removeEventListener: () => void (pageListener = null),
  };
  let setStatus!: (s: EngineStatus) => void;
  const asked: string[] = [];
  const bridge: RevBenchBridge = {
    platform: "win32",
    versions: { electron: "45", chrome: "140" },
    onEngineStatus: (cb) => {
      setStatus = cb;
      cb({ state: "running", restarts: 0 });
      return () => {};
    },
    requestEnginePort: () => {
      asked.push("port");
      // the main process answers with one end of the channel, as a window message
      queueMicrotask(() => {
        control.attach({ post: (m, tr) => channel.port1.postMessage(m, tr as never) });
        pageListener?.({ data: { channel: ENGINE_PORT_MESSAGE, restarts: 0 }, source: fakeWindow, ports: [channel.port2 as unknown as MessagePort] });
      });
    },
    restartEngine: () => void asked.push("restart"),
  };
  const frames: (() => void)[] = [];
  const store = new EngineStore({
    bridge: options.bridge === false ? undefined : bridge,
    window: fakeWindow,
    requestFrame: (cb) => frames.push(cb),
    cancelFrame: () => {},
  });
  store.start();
  const target = document.createElement("div");
  document.body.appendChild(target);
  const app = mount(App, { target, props: { store } });
  cleanups.push(() => {
    unmount(app);
    store.dispose();
    channel.port1.close();
    control.shutdown();
  });

  /** time passes for the engine (ms), the loop runs, the window asks for a snapshot, messages arrive */
  async function settle(ms = 20, step = 10) {
    for (let i = 0; i < ms; i += step) {
      t += step;
      control.pump();
      frames.shift()?.();
      await new Promise((r) => setTimeout(r, 2));
      flushSync();
    }
    await new Promise((r) => setTimeout(r, 5));
    flushSync();
  }
  const button = (name: string) => [...target.querySelectorAll("button")].find((b) => b.textContent?.trim() === name)!;
  return { target, store, settle, button, asked, setStatus: (s: EngineStatus) => { setStatus(s); flushSync(); }, control };
}

describe("the window", () => {
  it("has the four regions of the layout: top bar, editor, centre and charts", async () => {
    const { target } = await launch();
    expect(target.querySelector("header.topbar")).not.toBeNull();
    expect(target.querySelector("aside")?.querySelector("section[aria-label='Editor']")).not.toBeNull();
    expect(target.querySelector("main")?.querySelector("section[aria-label='Engine']")).not.toBeNull();
    expect(target.querySelector("section[aria-label='Charts']")).not.toBeNull();
  });

  it("says the engine is running once it is connected, with its presets to choose from", async () => {
    const { target, settle } = await launch();
    await settle();
    expect(target.querySelector(".state")?.textContent?.trim()).toBe("Engine running");
    const select = target.querySelector("select")!;
    expect(select.value).toBe("i4");
    expect([...select.options].map((o) => o.value)).toEqual(["i4", "i6", "v8-cross", "v8-flat", "boxer4"]);
    expect(select.disabled).toBe(false);
  });

  it("asks the main process for the engine's port when it starts", async () => {
    const { asked } = await launch();
    expect(asked).toEqual(["port"]);
  });

  it("starts the engine with ignition and the starter, and shows the engine speed rising and settling at idle", async () => {
    const { target, settle, button, store } = await launch();
    await settle();
    expect(button("Ignition").getAttribute("aria-pressed")).toBe("false");
    button("Ignition").click();
    await settle();
    expect(button("Ignition").getAttribute("aria-pressed")).toBe("true");
    button("Starter").dispatchEvent(new Event("pointerdown", { bubbles: true }));
    await settle(800, 100);
    button("Starter").dispatchEvent(new Event("pointerup", { bubbles: true }));
    await settle(2500, 100);
    expect(store.snapshot!.instruments.rpm).toBeGreaterThan(700);
    const shown = target.querySelector(".readout .value")!.textContent;
    expect(Number(shown)).toBeGreaterThan(700);
    expect(target.querySelector(".readout .unit")!.textContent).toBe("rpm");
    expect(button("Starter").getAttribute("aria-pressed")).toBe("false");
  });

  it("turns ignition off again", async () => {
    const { settle, button } = await launch();
    await settle();
    button("Ignition").click();
    await settle();
    button("Ignition").click();
    await settle();
    expect(button("Ignition").getAttribute("aria-pressed")).toBe("false");
  });

  it("sets the throttle from the slider", async () => {
    const { target, settle, store } = await launch();
    await settle();
    const slider = target.querySelector("input[type=range]") as HTMLInputElement;
    slider.value = "0.6";
    slider.dispatchEvent(new Event("input", { bubbles: true }));
    await settle();
    expect(store.snapshot!.instruments.throttle).toBeCloseTo(0.6, 12);
  });

  it("changes the engine: disabled while it rebuilds, then the new engine's cylinders", async () => {
    const { target, settle, store } = await launch();
    await settle();
    const select = target.querySelector("select")!;
    select.value = "v8-cross";
    select.dispatchEvent(new Event("change", { bubbles: true }));
    flushSync();
    expect(store.rebuilding).toBe(true);
    expect(select.disabled).toBe(true);
    await settle(200);
    expect(store.rebuilding).toBe(false);
    expect(select.disabled).toBe(false);
    expect(select.value).toBe("v8-cross");
    expect(store.snapshot!.cylinders).toBe(8);
    expect(store.snapshot!.generation).toBe(2);
  });

  it("shows why a command was refused", async () => {
    const { target, settle, store } = await launch();
    await settle();
    await store.setParam("no_such_parameter", 1);
    flushSync();
    expect(target.textContent).toContain("UNKNOWN_PARAM");
  });

  it("switches between the dark and the light theme", async () => {
    const { settle, button } = await launch();
    await settle();
    expect(document.documentElement.dataset.theme).toBe("dark");
    button("Light theme").click();
    flushSync();
    expect(document.documentElement.dataset.theme).toBe("light");
    button("Dark theme").click();
    flushSync();
    expect(document.documentElement.dataset.theme).toBe("dark");
  });
});

describe("when the engine process dies", () => {
  it("says it is restarting, with why, and locks the controls", async () => {
    const { target, settle, setStatus, button } = await launch();
    await settle();
    setStatus({ state: "restarting", restarts: 0, message: "the engine process exited (code 3221225477). Restarting." });
    await settle(0);
    expect(target.querySelector(".state")?.textContent?.trim()).toBe("Engine restarting");
    expect(target.textContent).toContain("code 3221225477");
    expect(button("Ignition").disabled).toBe(true);
    expect(button("Starter").disabled).toBe(true);
    expect(target.querySelector("select")!.disabled).toBe(true);
    expect((target.querySelector("input[type=range]") as HTMLInputElement).disabled).toBe(true);
  });

  it("offers to restart it when the main process has given up, and asks for that", async () => {
    const { target, settle, setStatus, button, asked } = await launch();
    await settle();
    setStatus({ state: "failed", restarts: 5, message: "the engine process exited. It stopped 6 times in 60 seconds." });
    expect(target.querySelector(".state")?.textContent?.trim()).toBe("Engine stopped");
    expect(target.querySelector("[role=alert]")?.textContent).toContain("stopped 6 times");
    button("Restart the engine").click();
    expect(asked).toContain("restart");
  });
});

describe("without the application", () => {
  it("says there is no engine to connect to", async () => {
    const { target } = await launch({ bridge: false });
    expect(target.querySelector(".state")?.textContent?.trim()).toBe("No application shell");
    expect(target.textContent).toContain("running without the Rev-Bench application");
  });
});

describe("rendering", () => {
  it("does not throw or warn", async () => {
    const warn = vi.spyOn(console, "warn").mockImplementation(() => {});
    const error = vi.spyOn(console, "error").mockImplementation(() => {});
    const { settle } = await launch();
    await settle();
    expect(warn).not.toHaveBeenCalled();
    expect(error).not.toHaveBeenCalled();
    warn.mockRestore();
    error.mockRestore();
  });
});
