/*
 * The drive as a command trace (stage 066): what the render thread acknowledged as applied, at
 * the audio frame its ack gives, in host/include/rb_host_trace.h's format, so rb_host_render can
 * play it back and the capture tap's audio can be checked against it.
 */
import { describe, expect, it } from "vitest";
import { EngineControl, type EnginePort } from "../src/engine/control";
import { createFakeAddon } from "../src/engine/fake-addon";
import { TraceRecorder, swapFadeFrames } from "../src/engine/trace-recorder";
import type { Ack, Host } from "../src/shared/addon";
import type { FromEngine } from "../src/shared/protocol";

describe("the trace recorder", () => {
  const addon = createFakeAddon({ prepareMs: 0 });
  const params = addon.params();
  const id = (name: string) => params.find((p) => p.name === name)!.id;

  function recorded() {
    const host = addon.createHost({ backend: "null", sampleRate: 44100, initial: { preset: "v8-cross" } });
    const recorder = new TraceRecorder(params, addon.presets(), 44100, {
      preset: "v8-cross",
      params: [{ id: id("hold"), value: 0 }, { id: id("master_gain"), value: 0.3 }],
    });
    return { host: recorder.wrap(host), recorder };
  }

  it("starts with the first build, at its preset's tier", () => {
    const { recorder } = recorded();
    expect(recorder.text(4410).split("\n").slice(0, 8)).toEqual([
      "rbtrace 1", "rate 44100", "preset v8-cross", "quality 5", "param hold 0", "param master_gain 0.29999999999999999", "skip 0", "frames 4410",
    ]);
  });

  it("writes each applied parameter at the frame its ack gives, with 17 digits", () => {
    const { host, recorder } = recorded();
    host.start();
    host.setParam(id("ignition"), 1);
    host.setParam(id("throttle"), 0.6);
    const acks: Ack[] = host.poll();
    expect(acks).toHaveLength(2);
    const lines = recorder.text(1000).split("\n");
    expect(lines).toContain(`at ${acks[0].audioFrame} set ignition 1`);
    expect(lines).toContain(`at ${acks[1].audioFrame} set throttle 0.59999999999999998`);
  });

  it("writes a swap and its build, and leaves out what wasn't applied", () => {
    const { host, recorder } = recorded();
    host.start();
    const prep = host.prepare({ preset: "i4", quality: "high", scene: "garage" });
    host.swap(prep);
    host.setParam(id("throttle"), 0.5, 999); // stale: another generation
    host.poll();
    const text = recorder.text(10);
    expect(text).toMatch(/^at -?\d+ swap b1$/m); // the fake swaps at once: no real fade before it
    expect(text).toContain("build b1\npreset i4\nquality 2\nscene 3\nend");
    expect(text).not.toContain("set throttle");
  });

  it("dates a swap from when its fade-out began, as the host sizes it: 5 blocks at 44.1 kHz, 6 at 48", () => {
    expect(swapFadeFrames(44100)).toBe(640);
    expect(swapFadeFrames(48000)).toBe(768);
    expect(swapFadeFrames(48000, 5)).toBe(512); // never under 4 blocks
    const { host, recorder } = recorded();
    host.start();
    host.swap(host.prepare({ preset: "i4" }));
    const [ack] = host.poll();
    expect(recorder.text(10)).toContain(`at ${ack.audioFrame - 640} swap b1`);
  });

  it("says a device change or a sleep broke the trace", () => {
    for (const act of [(h: Host) => h.selectDevice(0), (h: Host) => h.suspend()]) {
      const { host, recorder } = recorded();
      expect(recorder.interrupted).toBe(false);
      act(host);
      expect(recorder.interrupted).toBe(true);
    }
  });
});

describe("capture-dump", () => {
  function control(capture: boolean) {
    const files = new Map<string, Uint8Array | string>();
    const messages: FromEngine[] = [];
    const port: EnginePort = { post: (m) => void messages.push(m) };
    const c = new EngineControl({
      addon: createFakeAddon(),
      config: { addon: "fake", backend: "null", build: { preset: "i4" }, ...(capture ? { captureSeconds: 5 } : {}) },
      onBuild: () => {},
      log: () => {},
      writeFile: (name, data) => (files.set(name, data), `/data/${name}`),
    });
    c.start();
    c.attach(port);
    return { c, files, messages };
  }

  it("writes the audio and the trace and says where", () => {
    const { c, files, messages } = control(true);
    c.handle({ type: "command", id: 1, command: { kind: "capture-dump" } });
    expect([...files.keys()].sort()).toEqual(["capture.f32", "capture.rbtrace"]);
    expect(String(files.get("capture.rbtrace"))).toMatch(/^rbtrace 1\nrate 48000\npreset i4\nquality 4\n/);
    expect(messages.find((m) => m.type === "capture")).toMatchObject({
      audio: "/data/capture.f32", trace: "/data/capture.rbtrace", frames: 0, rate: 48000, dropped: 0, interrupted: false,
    });
  });

  it("is refused without a capture tap", () => {
    const { c, files, messages } = control(false);
    c.handle({ type: "command", id: 1, command: { kind: "capture-dump" } });
    expect(files.size).toBe(0);
    expect(messages.find((m) => m.type === "reply")).toMatchObject({ error: { code: "NO_CAPTURE" } });
  });
});
