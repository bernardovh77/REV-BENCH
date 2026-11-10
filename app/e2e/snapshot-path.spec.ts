/*
 * The snapshot path, measured: engine process -> MessagePortMain -> page's MessagePort, as the
 * app really uses it (the real add-on, the largest preset, the page asking once per animation
 * frame), against runtime-v1's budget (contracts/runtime-v1.md): at most 64 KiB a snapshot, 60 a
 * second, 3.75 MiB/s. Electron's MessagePortMain can't transfer an ArrayBuffer, so every
 * snapshot is copied once on the way; this measures what that costs.
 *
 * The harness runs in the page: it asks the main process for a second port (the engine closes
 * the app's own) and takes it before the app's store sees it (a capturing listener), so what is
 * measured is the real channel, with no instrument in the way. The numbers go to the console and
 * to $RB_E2E_OUTPUT/snapshot-path.json.
 */
import { expect, test } from "@playwright/test";
import { mkdirSync, writeFileSync } from "node:fs";
import { join } from "node:path";

const BLASTER = join(appRoot, "e2e", "fixtures", "blaster.cjs");
import { appRoot, launch, until, waitForState } from "./app";

interface Measured {
  preset: string;
  snapshotBytes: number;
  rtt: { samples: number; roundTripsPerSecond: number; p50Ms: number; p95Ms: number; p99Ms: number; maxMs: number };
  perFrame: {
    seconds: number;
    requests: number;
    snapshots: number;
    snapshotsPerSecond: number;
    bytesPerSecond: number;
    gapP50Ms: number;
    gapP95Ms: number;
    gapMaxMs: number;
  };
  flatOut: { seconds: number; snapshots: number; snapshotsPerSecond: number; bytesPerSecond: number };
}

// The runtime-v1 budget (contracts/runtime-v1.md)
const BUDGET = { snapshotBytes: 64 * 1024, perSecond: 60, bytesPerSecond: 3.75 * 1024 * 1024 };

test("the snapshot path costs a small fraction of runtime-v1's budget, and a frame is never late", async () => {
  const run = await launch();
  try {
    const { page, app } = run;
    await waitForState(page, "Engine running");
    await page.locator(".topbar select").selectOption("v8-cross");
    await until("the V8", () => page.locator(".topbar select").inputValue(), (p) => p === "v8-cross", 20_000);
    await expect(page.locator(".topbar select")).toBeEnabled();
    // a running engine: ignition and the starter
    await page.getByRole("button", { name: "Ignition" }).click();
    await page.getByRole("button", { name: "Starter" }).dispatchEvent("pointerdown");
    await new Promise((r) => setTimeout(r, 2500));
    await page.getByRole("button", { name: "Starter" }).dispatchEvent("pointerup");

    const metrics = async () =>
      (await app.evaluate(({ app }) => app.getAppMetrics())).filter((m) => m.type === "Utility").map((m) => m.cpu.percentCPUUsage);

    const cpuBefore = await metrics();
    const measured: Measured = await page.evaluate(async () => {
      // take a port of our own, ahead of the app's store
      const port = await new Promise<MessagePort>((resolve) => {
        const take = (event: MessageEvent) => {
          const data = event.data as { channel?: string } | null;
          if (event.source !== globalThis.window || data?.channel !== "rev-bench:engine-port" || !event.ports[0]) return;
          event.stopImmediatePropagation();
          globalThis.window.removeEventListener("message", take, true);
          resolve(event.ports[0]);
        };
        globalThis.window.addEventListener("message", take, true);
        globalThis.window.revbench!.requestEnginePort();
      });
      type Message = { type: string; bytes?: ArrayBuffer; preset?: string };
      let waiter: ((m: Message) => void) | null = null;
      let onSnapshot: ((bytes: number) => void) | null = null;
      let preset = "";
      port.onmessage = (event) => {
        const m = event.data as Message;
        if (m.type === "hello") preset = m.preset!;
        if (m.type === "snapshot") onSnapshot?.(m.bytes!.byteLength);
        if (m.type === "stats") {
          const w = waiter;
          waiter = null;
          w?.(m);
        }
      };
      port.start();
      const stats = () => new Promise<Message>((resolve) => ((waiter = resolve), port.postMessage({ type: "stats-request" })));
      await stats(); // the channel is up (hello has come before it)
      const quantile = (sorted: number[], q: number) => sorted[Math.min(sorted.length - 1, Math.floor(q * sorted.length))];

      // 1. the channel's round trip: a request that is always answered, one at a time
      const rtts: number[] = [];
      for (let i = 0; i < 2000; i++) {
        const t0 = performance.now();
        await stats();
        rtts.push(performance.now() - t0);
      }
      rtts.sort((a, b) => a - b);

      // 2. as the app uses it: one snapshot request per animation frame for 10 s
      const gaps: number[] = [];
      let snapshots = 0;
      let bytes = 0;
      let size = 0;
      let last = 0;
      onSnapshot = (n) => {
        const now = performance.now();
        if (last) gaps.push(now - last);
        last = now;
        snapshots++;
        bytes += n;
        size = n;
      };
      let requests = 0;
      const start = performance.now();
      await new Promise<void>((resolve) => {
        const frame = () => {
          if (performance.now() - start >= 10_000) return resolve();
          port.postMessage({ type: "snapshot-request" });
          requests++;
          requestAnimationFrame(frame);
        };
        requestAnimationFrame(frame);
      });
      await stats(); // everything asked for has been answered (the channel is in order)
      const seconds = (performance.now() - start) / 1000;
      gaps.sort((a, b) => a - b);
      const perFrame = {
        seconds,
        requests,
        snapshots,
        snapshotsPerSecond: snapshots / seconds,
        bytesPerSecond: bytes / seconds,
        gapP50Ms: quantile(gaps, 0.5),
        gapP95Ms: quantile(gaps, 0.95),
        gapMaxMs: gaps[gaps.length - 1],
      };

      // 3. flat out: as many requests as the channel carries for 5 s (what the host can publish
      //    and the channel can move, at most)
      snapshots = 0;
      bytes = 0;
      onSnapshot = (n) => {
        snapshots++;
        bytes += n;
      };
      const flatStart = performance.now();
      while (performance.now() - flatStart < 5000) {
        for (let i = 0; i < 20; i++) port.postMessage({ type: "snapshot-request" });
        await stats();
      }
      const flatSeconds = (performance.now() - flatStart) / 1000;
      onSnapshot = null;
      port.close();

      return {
        preset,
        snapshotBytes: size,
        rtt: { samples: rtts.length, roundTripsPerSecond: rtts.length / (rtts.reduce((a, b) => a + b, 0) / 1000), p50Ms: quantile(rtts, 0.5), p95Ms: quantile(rtts, 0.95), p99Ms: quantile(rtts, 0.99), maxMs: rtts[rtts.length - 1] },
        perFrame,
        flatOut: { seconds: flatSeconds, snapshots, snapshotsPerSecond: snapshots / flatSeconds, bytesPerSecond: bytes / flatSeconds },
      };
    });
    const cpuAfter = await metrics();

    const report = { ...measured, utilityCpuPercentAtStart: cpuBefore, utilityCpuPercentAfter: cpuAfter, budget: BUDGET };
    const dir = process.env.RB_E2E_OUTPUT ?? "test-results";
    mkdirSync(dir, { recursive: true });
    writeFileSync(join(dir, "snapshot-path.json"), JSON.stringify(report, null, 2));
    console.log("snapshot path:", JSON.stringify(report, null, 2));

    // the V8's snapshot: 304 + 40 * 8 bytes, far under the 64 KiB a snapshot may have
    expect(measured.preset).toBe("v8-cross");
    expect(measured.snapshotBytes).toBe(624);
    expect(measured.snapshotBytes).toBeLessThanOrEqual(BUDGET.snapshotBytes);
    // at the page's frame rate: within the 60 a second, and within the bytes a second
    expect(measured.perFrame.snapshotsPerSecond).toBeLessThanOrEqual(BUDGET.perSecond * 1.1);
    expect(measured.perFrame.snapshotsPerSecond).toBeGreaterThan(BUDGET.perSecond * 0.8);
    expect(measured.perFrame.bytesPerSecond).toBeLessThan(BUDGET.bytesPerSecond / 20);
    // a round trip is a small part of a 16.7 ms frame
    expect(measured.rtt.p95Ms).toBeLessThan(8);
    // and even flat out the channel stays within the budget's bytes
    expect(measured.flatOut.bytesPerSecond).toBeLessThan(BUDGET.bytesPerSecond * 4);
  } finally {
    await run.dispose();
  }
});

/** What a stand-in engine process sends (see fixtures/blaster.cjs) and the page receives. */
interface Blast {
  size: number;
  hz: number;
  sent: number;
  received: number;
  seconds: number;
  messagesPerSecond: number;
  bytesPerSecond: number;
  latencyP50Ms: number;
  latencyP95Ms: number;
  latencyMaxMs: number;
}

test("a utility process sending snapshots of 624 bytes, 8 KiB and runtime-v1's 64 KiB at 60 a second, and flat out", async () => {
  const run = await launch();
  try {
    const { page, app } = run;
    await waitForState(page, "Engine running");
    const cases = [
      { size: 624, hz: 60, seconds: 5 },
      { size: 8192, hz: 60, seconds: 5 },
      { size: 65536, hz: 60, seconds: 5 },
      { size: 65536, hz: 0, seconds: 3 },
    ];
    const results: Blast[] = [];
    for (const c of cases) {
      // the page listens first; then the main process starts the sender and gives the page its port
      const receiving = page.evaluate(async (): Promise<Omit<Blast, "size" | "hz" | "sent">> => {
        const port = await new Promise<MessagePort>((resolve) => {
          const take = (event: MessageEvent) => {
            const data = event.data as { channel?: string } | null;
            if (event.source !== globalThis.window || data?.channel !== "rev-bench:engine-port" || !event.ports[0]) return;
            event.stopImmediatePropagation();
            globalThis.window.removeEventListener("message", take, true);
            resolve(event.ports[0]);
          };
          globalThis.window.addEventListener("message", take, true);
        });
        const latencies: number[] = [];
        let received = 0;
        let bytes = 0;
        let first = 0;
        let last = 0;
        await new Promise<void>((resolve) => {
          port.onmessage = (event) => {
            const m = event.data as { type: string; sentAt?: number; bytes?: ArrayBuffer };
            const now = performance.timeOrigin + performance.now();
            if (m.type === "done") return resolve();
            if (!first) first = now;
            last = now;
            received++;
            bytes += m.bytes!.byteLength;
            latencies.push(now - m.sentAt!);
          };
          port.start();
        });
        port.close();
        latencies.sort((a, b) => a - b);
        const q = (x: number) => latencies[Math.min(latencies.length - 1, Math.floor(x * latencies.length))];
        const seconds = (last - first) / 1000;
        return { received, seconds, messagesPerSecond: received / seconds, bytesPerSecond: bytes / seconds, latencyP50Ms: q(0.5), latencyP95Ms: q(0.95), latencyMaxMs: latencies[latencies.length - 1] };
      });
      await app.evaluate(async ({ utilityProcess, MessageChannelMain, BrowserWindow }, args) => {
        const child = utilityProcess.fork(args.script);
        (globalThis as { __blaster?: typeof child }).__blaster = child;
        const { port1, port2 } = new MessageChannelMain();
        child.postMessage({ size: args.size, hz: args.hz, seconds: args.seconds }, [port1]);
        BrowserWindow.getAllWindows()[0].webContents.postMessage("rb:engine-port", { restarts: 99 }, [port2]);
      }, { script: BLASTER, ...c });
      const got = await receiving;
      await app.evaluate(() => (globalThis as { __blaster?: { kill(): void } }).__blaster?.kill());
      results.push({ ...c, sent: 0, ...got });
    }
    const dir = process.env.RB_E2E_OUTPUT ?? "test-results";
    mkdirSync(dir, { recursive: true });
    writeFileSync(join(dir, "snapshot-path-blast.json"), JSON.stringify(results, null, 2));
    console.log("blast:", JSON.stringify(results, null, 2));

    const at60 = results.filter((r) => r.hz === 60);
    for (const r of at60) {
      expect(r.messagesPerSecond, `${r.size} B`).toBeGreaterThan(BUDGET.perSecond * 0.8);
      // 64 KiB at 60 a second is the budget's own ceiling (3.75 MiB/s): it must be carried, with a delay under a frame
      expect(r.latencyP95Ms, `${r.size} B`).toBeLessThan(16.7);
    }
    const big = at60.find((r) => r.size === 65536)!;
    expect(big.bytesPerSecond).toBeGreaterThan(BUDGET.bytesPerSecond * 0.8);
  } finally {
    await run.dispose();
  }
});
