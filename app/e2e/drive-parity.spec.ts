/*
 * Stage 066's exit evidence, end to end: the built app on the real add-on and the null audio
 * backend (never a sound device), with the host's capture tap (RB_CAPTURE_SECONDS).
 *
 *   1. A drive through the app's interface (the keys, the starter, the ramped throttle, a
 *      preset change with its swap, a scene, the slider) gives, through the capture tap, the
 *      same audio as rb_host_render's offline render of the trace the engine process recorded
 *      of it. The budget: bit for bit, at 44.1 and at 48 kHz (the same compiler built both, so
 *      nothing may differ; docs/stage-066.md).
 *   2. The dead-man rule: a ramped key held and the window hidden, and the window's messages
 *      stopped mid-pull; the engine's throttle reaches zero both times, and an on/off throttle
 *      stays where it was.
 *
 * RB_HOST_RENDER names rb_host_render from the add-on's own build (tools/run_tests.py app-e2e).
 */
import { expect, test, type Page } from "@playwright/test";
import { execFileSync } from "node:child_process";
import { copyFileSync, existsSync, mkdirSync, readFileSync } from "node:fs";
import { join } from "node:path";
import { launch, rpmShown, until, waitForState } from "./app";

interface Capture {
  audio: string;
  trace: string;
  frames: number;
  rate: number;
  dropped: number;
  interrupted: boolean;
}

const sleep = (ms: number) => new Promise((r) => setTimeout(r, ms));
const dump = (page: Page) => page.evaluate(() => (window as unknown as { __rbCaptureDump(): Promise<Capture> }).__rbCaptureDump());

/** The float samples of a WAV's data chunk (rb_host_render --float). */
function wavFloats(path: string): { channels: number; samples: Float32Array } {
  const b = readFileSync(path);
  const channels = b.readUInt16LE(22);
  let at = 12;
  while (at + 8 <= b.length) {
    const id = b.toString("ascii", at, at + 4);
    const size = b.readUInt32LE(at + 4);
    if (id === "data") {
      const copy = new Uint8Array(b.subarray(at + 8, at + 8 + size));
      return { channels, samples: new Float32Array(copy.buffer, 0, size / 4) };
    }
    at += 8 + size + (size & 1);
  }
  throw new Error(`${path}: no data chunk`);
}

/** The engine's throttle now, from the newest snapshot the page has. */
const throttleNow = (page: Page) => page.locator(".readouts .readout").nth(1).locator(".value").innerText().then(Number);

for (const rate of [44100, 48000]) {
  test(`a drive through the interface sounds the same as the offline render of its trace, at ${rate} Hz`, async () => {
    // never a silent skip: the suite (tools/run_tests.py app-e2e) always builds and names it
    const renderer = process.env.RB_HOST_RENDER;
    if (!renderer || !existsSync(renderer)) throw new Error(`RB_HOST_RENDER must name rb_host_render from the add-on's build: ${String(renderer)}`);
    const run = await launch({ extraEnv: { RB_CAPTURE_SECONDS: "60", RB_SAMPLE_RATE: String(rate) } });
    try {
      const { page } = run;
      await waitForState(page, "Engine running");
      await page.locator("body").click({ position: { x: 5, y: 5 } });
      // the keys: ignition, the starter held, then the ramped throttle
      await page.keyboard.press("KeyI");
      await page.keyboard.down("KeyS");
      await until("cranking", () => rpmShown(page), (rpm) => rpm > 100, 15_000);
      await sleep(1200);
      await page.keyboard.up("KeyS");
      await until("running", () => rpmShown(page), (rpm) => rpm > 500, 20_000);
      await page.keyboard.down("KeyW");
      await sleep(700);
      await page.keyboard.up("KeyW");
      await sleep(800);
      // another engine (a swap with its fades), heard in a stereo scene, then the slider
      await page.locator(".topbar select").first().selectOption("v8-cross");
      await until("the V8", () => page.locator(".topbar select").first().inputValue(), (v) => v === "v8-cross", 30_000);
      await expect(page.locator(".topbar select").first()).toBeEnabled();
      await page.getByLabel("Scene").selectOption("garage");
      await expect(page.getByLabel("Scene")).toBeEnabled();
      await page.locator(".topbar input[type=range]").fill("0.35");
      await sleep(1500);

      const capture = await dump(page);
      expect(capture.rate).toBe(rate);
      expect(capture.dropped).toBe(0);
      expect(capture.interrupted).toBe(false);
      expect(capture.frames).toBeGreaterThan(rate * 4);
      const trace = readFileSync(capture.trace, "utf8");
      expect(trace).toMatch(/set ignition 1/);
      expect(trace).toMatch(/set throttle /);
      expect(trace).toMatch(/swap b\d/);

      const out = join(run.userData, "offline.wav");
      execFileSync(renderer!, [capture.trace, "-o", out, "--float"], { stdio: "pipe" });
      const offline = wavFloats(out);
      const live = new Float32Array(new Uint8Array(readFileSync(capture.audio)).buffer);
      expect(live.length).toBe(capture.frames * 2);
      // a trace that ends in a stereo scene is rendered in two channels throughout
      expect(offline.channels).toBe(2);
      expect(offline.samples.length).toBe(live.length);
      let first = -1;
      for (let i = 0; i < live.length; i++) {
        if (live[i] !== offline.samples[i]) {
          first = i;
          break;
        }
      }
      if (first >= 0 && process.env.RB_E2E_OUTPUT) {
        mkdirSync(process.env.RB_E2E_OUTPUT, { recursive: true });
        // keep what failed: the trace and both renders, for a look
        for (const [from, name] of [[capture.trace, "trace"], [capture.audio, "live.f32"], [out, "offline.wav"]]) {
          copyFileSync(from, join(process.env.RB_E2E_OUTPUT, `parity-${rate}-${name}`));
        }
      }
      expect(first, first < 0 ? "" : `first difference at frame ${Math.floor(first / 2)}: ${live[first]} live, ${offline.samples[first]} offline`).toBe(-1);
      expect(run.problems).toEqual([]);
    } finally {
      await run.dispose();
    }
  });
}

test("the dead-man rule: a held ramped key lets go when the window is hidden, or its messages stop", async () => {
  // the capture's trace is the engine's own record of every throttle it applied
  const run = await launch({ extraEnv: { RB_CAPTURE_SECONDS: "60" } });
  try {
    const { page, app } = run;
    await waitForState(page, "Engine running");
    await page.locator("body").click({ position: { x: 5, y: 5 } });
    await page.keyboard.press("KeyI");
    await page.keyboard.down("KeyS");
    await until("cranking", () => rpmShown(page), (rpm) => rpm > 100, 15_000);
    await sleep(1200);
    await page.keyboard.up("KeyS");
    await until("running", () => rpmShown(page), (rpm) => rpm > 500, 20_000);

    // 1. the ramped key held, then the window hidden (its key-up never comes)
    await page.keyboard.down("KeyW");
    await until("pulling", () => throttleNow(page), (v) => v >= 99, 10_000);
    await app.evaluate(({ BrowserWindow }) => BrowserWindow.getAllWindows()[0].hide());
    await sleep(1500);
    await app.evaluate(({ BrowserWindow }) => BrowserWindow.getAllWindows()[0].show());
    await until("the throttle closed", () => throttleNow(page), (v) => v === 0, 10_000);
    await page.keyboard.up("KeyW");

    // 2. the ramped key held, then the window's timers stopped dead: no beat reaches the engine
    await page.locator("body").click({ position: { x: 5, y: 5 } });
    await page.keyboard.down("KeyW");
    await until("pulling again", () => throttleNow(page), (v) => v >= 99, 10_000);
    const before = (await dump(page)).trace;
    const heldFrom = readFileSync(before, "utf8").split("\n").filter((l) => l.includes("set throttle")).length;
    await page.evaluate(() => {
      const end = Date.now() + 1500; // the page's thread blocked: nothing is sent for 1.5 s
      while (Date.now() < end);
    });
    await sleep(500); // the page runs again, the key still held
    const after = readFileSync((await dump(page)).trace, "utf8");
    // with W held, the window only ever sent 1 before the stall: the ramp to 0 is the engine's.
    // After it the window lets go too (a frame after a long gap), and the throttle stays shut
    const throttles = after.split("\n").filter((l) => l.includes("set throttle")).slice(heldFrom).map((l) => Number(l.split(" ").at(-1)));
    const zero = throttles.indexOf(0);
    expect(zero, throttles.join(" ")).toBeGreaterThan(1);
    expect(throttles.slice(0, zero).some((v) => v > 0 && v < 1), "a ramp, not a cut").toBe(true);
    expect(throttles.slice(zero).every((v) => v === 0), `nothing reopens it: ${throttles.slice(zero).join(" ")}`).toBe(true);
    expect(await throttleNow(page)).toBe(0);
    // a fresh press opens it again
    await page.keyboard.up("KeyW");
    await page.keyboard.down("KeyW");
    await until("pulling after a fresh press", () => throttleNow(page), (v) => v >= 99, 10_000);
    await page.keyboard.up("KeyW");

    // 3. the on/off style holds through a hidden window
    await page.getByLabel("Throttle style").selectOption("onoff");
    await page.locator("body").click({ position: { x: 5, y: 5 } });
    await page.keyboard.press("KeyW");
    await until("on", () => throttleNow(page), (v) => v === 100, 10_000);
    await app.evaluate(({ BrowserWindow }) => BrowserWindow.getAllWindows()[0].hide());
    await sleep(1000);
    await app.evaluate(({ BrowserWindow }) => BrowserWindow.getAllWindows()[0].show());
    await sleep(300);
    expect(await throttleNow(page)).toBe(100);
  } finally {
    await run.dispose();
  }
});
