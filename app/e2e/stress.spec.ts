/*
 * Stage 069's stress drive with the window busy (the card's "problem 7", from the 2026-11-15
 * review): the built app on the real add-on and the null audio backend (never a sound device,
 * but paced in real time, so a late block is an underrun as on a real one), driven for
 * RB_E2E_STRESS_MINUTES through a 20-second cycle (an idle, a blip, wide open into the limiter,
 * the foot off) while the window draws a deliberately heavy animation every frame: a stand-in
 * for the 3D view, which will share the laptop's fast cores with the render thread.
 *
 * Run only when asked: RB_E2E_STRESS_MINUTES=15 (and RB_E2E_STRESS_PRESET, the V8 cross-plane by
 * default: the dearest preset; RB_E2E_STRESS_LATENCY, the latency in ms, the app's 20 by
 * default; RB_E2E_STRESS_HEAVY=0 to leave the window idle). Every 10 s it records the host's
 * statistics as the window shows them, and the window's frame rate; the record goes to
 * RB_E2E_OUTPUT/stress-<preset>-<latency>ms[-idle].json. It fails on any underrun.
 */
import { expect, test, type Page } from "@playwright/test";
import { mkdirSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import { launch, rpmShown, until, waitForState } from "./app";

const minutes = Number(process.env.RB_E2E_STRESS_MINUTES ?? 0);
const presetKey = process.env.RB_E2E_STRESS_PRESET ?? "v8-cross";
const latency = Number(process.env.RB_E2E_STRESS_LATENCY ?? 20);
const heavy = process.env.RB_E2E_STRESS_HEAVY !== "0";
const sleep = (ms: number) => new Promise((r) => setTimeout(r, ms));

/** A readout's value in the centre panel, by its label. */
const readout = async (page: Page, label: string): Promise<number> =>
  Number(await page.locator(`.readout:has(> .label:text-is("${label}"))`).locator(".value").first().innerText());

/**
 * The heavy animation: a canvas over the window, every frame 3000 random strokes and 6 ms of
 * arithmetic on the renderer's main thread, and a count of frames drawn. Injected by the test:
 * nothing of it is in the app.
 */
async function startHeavyAnimation(page: Page): Promise<void> {
  await page.evaluate(() => {
    const canvas = document.createElement("canvas");
    canvas.width = innerWidth;
    canvas.height = innerHeight;
    Object.assign(canvas.style, { position: "fixed", inset: "0", pointerEvents: "none", opacity: "0.15", zIndex: "9999" });
    document.body.appendChild(canvas);
    const g = canvas.getContext("2d")!;
    const w = window as unknown as { __rbFrames: number };
    w.__rbFrames = 0;
    let sink = 0;
    const frame = () => {
      g.clearRect(0, 0, canvas.width, canvas.height);
      g.beginPath();
      for (let i = 0; i < 3000; i++) {
        g.moveTo(Math.random() * canvas.width, Math.random() * canvas.height);
        g.lineTo(Math.random() * canvas.width, Math.random() * canvas.height);
      }
      g.stroke();
      const until = performance.now() + 6;
      while (performance.now() < until) sink += Math.sqrt(sink + 1);
      w.__rbFrames++;
      requestAnimationFrame(frame);
    };
    requestAnimationFrame(frame);
  });
}

const framesDrawn = (page: Page) => page.evaluate(() => (window as unknown as { __rbFrames?: number }).__rbFrames ?? 0);

test("a long drive with the window busy: no underruns", async () => {
  test.skip(!(minutes > 0), "on request only: RB_E2E_STRESS_MINUTES");
  test.setTimeout(minutes * 60_000 + 180_000);
  const run = await launch();
  try {
    const { page } = run;
    await waitForState(page, "Engine running");
    // the preset and the latency
    await page.locator(".topbar select").first().selectOption(presetKey);
    await until("the preset", () => page.locator(".topbar select").first().inputValue(), (v) => v === presetKey, 30_000);
    await sleep(2000);
    if (latency !== 20) {
      await page.getByLabel("Latency").selectOption(String(latency));
      await waitForState(page, "Engine running");
      await sleep(2000);
    }
    await page.locator("body").click({ position: { x: 5, y: 5 } });
    await page.keyboard.press("KeyI");
    await page.keyboard.down("KeyS");
    await until("cranking", () => rpmShown(page), (rpm) => rpm > 100, 15_000);
    await sleep(1200);
    await page.keyboard.up("KeyS");
    await until("running", () => rpmShown(page), (rpm) => rpm > 400, 20_000);
    await sleep(3000);
    if (heavy) await startHeavyAnimation(page);

    const startUnderruns = await readout(page, "Underruns");
    const record: { t: number; underruns: number; ringMs: number; renderPct: number; rpm: number; fps: number }[] = [];
    const t0 = Date.now();
    let frames0 = await framesDrawn(page);
    let lastSample = t0;
    const end = t0 + minutes * 60_000;
    const sample = async () => {
      const now = Date.now();
      const frames = await framesDrawn(page);
      record.push({
        t: (now - t0) / 1000,
        underruns: (await readout(page, "Underruns")) - startUnderruns,
        ringMs: await readout(page, "Ring"),
        renderPct: await readout(page, "Render time, of real time"),
        rpm: await rpmShown(page),
        fps: heavy ? ((frames - frames0) * 1000) / (now - lastSample) : 0,
      });
      frames0 = frames;
      lastSample = now;
    };
    // the 20-second cycle: idle 4 s, a blip, idle 3 s, wide open 5 s (the limiter), off 7.7 s
    while (Date.now() < end) {
      await sleep(4000);
      await page.keyboard.down("KeyW");
      await sleep(300);
      await page.keyboard.up("KeyW");
      await sleep(3000);
      await page.keyboard.down("KeyW");
      await sleep(5000);
      await page.keyboard.up("KeyW");
      await sleep(7700);
      if (Date.now() - lastSample >= 10_000) await sample();
    }
    await sample();

    const last = record.at(-1)!;
    const out = process.env.RB_E2E_OUTPUT;
    if (out) {
      mkdirSync(out, { recursive: true });
      writeFileSync(join(out, `stress-${presetKey}-${latency}ms${heavy ? "" : "-idle"}.json`),
        JSON.stringify({ preset: presetKey, latency, heavy, minutes, underruns: last.underruns, record }, null, 1));
    }
    console.log(`stress ${presetKey} ${latency} ms ${heavy ? "heavy" : "idle"}: ${minutes} min, underruns ${last.underruns}, ` +
      `render ${Math.min(...record.map((r) => r.renderPct)).toFixed(0)}-${Math.max(...record.map((r) => r.renderPct)).toFixed(0)} %, ` +
      `ring min ${Math.min(...record.map((r) => r.ringMs)).toFixed(1)} ms, fps ${heavy ? Math.min(...record.map((r) => r.fps)).toFixed(0) : "-"}`);
    expect(last.underruns).toBe(0);
  } finally {
    await run.dispose();
  }
});
