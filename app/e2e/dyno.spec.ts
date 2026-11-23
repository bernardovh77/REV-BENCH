/*
 * Stage 068's exit evidence for the dyno, end to end: the built app on the real add-on and the
 * null audio backend (never a sound device), with the host's capture tap. A sweep run from the
 * dyno panel (2000 to 5000 rpm at 1000 rpm/s, wide open), exported through the panel's Export
 * CSV (the save dialog answered by the test), is compared with every reading the core's dyno
 * makes when rb_host_render replays the trace the engine process recorded of the same drive
 * (--dyno-csv). The card's gate, frozen here: matched by reading, at most 2 % torque error
 * pointwise above 10 % of the peak and 1 % of the peak below it; aligned in time, each point's
 * time since the run's first within a snapshot interval and a block of the native one's.
 *
 * RB_HOST_RENDER names rb_host_render from the add-on's own build (tools/run_tests.py app-e2e).
 */
import { expect, test, type Page } from "@playwright/test";
import { execFileSync } from "node:child_process";
import { existsSync, readFileSync } from "node:fs";
import { join } from "node:path";
import { launch, rpmShown, until, waitForState } from "./app";

const sleep = (ms: number) => new Promise((r) => setTimeout(r, ms));
const dump = (page: Page) =>
  page.evaluate(() => (window as unknown as { __rbCaptureDump(): Promise<{ trace: string; rate: number; dropped: number; interrupted: boolean }> }).__rbCaptureDump());

/** A CSV's rows as objects, by its header. */
function csvRows(text: string): Record<string, string>[] {
  const [head, ...rows] = text.trim().split("\n");
  const names = head.split(",");
  return rows.map((row) => {
    const cells = row.match(/("([^"]|"")*"|[^,]*)(,|$)/g)!.map((c) => c.replace(/,$/, ""));
    return Object.fromEntries(names.map((n, i) => [n, cells[i]]));
  });
}

test("a sweep's curves in the app are the core's own readings, matched by reading and aligned in time", async () => {
  const renderer = process.env.RB_HOST_RENDER;
  if (!renderer || !existsSync(renderer)) throw new Error(`RB_HOST_RENDER must name rb_host_render from the add-on's build: ${String(renderer)}`);
  const run = await launch({ extraEnv: { RB_CAPTURE_SECONDS: "60", RB_SAMPLE_RATE: "48000" } });
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

    // the sweep, from the dyno panel
    const dyno = page.locator("section[aria-label='Dyno']");
    await dyno.getByLabel("From", { exact: true }).fill("2000");
    await dyno.getByLabel("From", { exact: true }).press("Enter");
    await dyno.getByLabel("To", { exact: true }).fill("5000");
    await dyno.getByLabel("To", { exact: true }).press("Enter");
    await dyno.getByLabel("Rate", { exact: true }).fill("1000");
    await dyno.getByLabel("Rate", { exact: true }).press("Enter");
    await dyno.getByRole("button", { name: "Start" }).click();
    await expect(dyno.getByRole("button", { name: "Release" })).toBeVisible({ timeout: 10_000 });
    const target = () => dyno.locator(".readout", { hasText: "Target" }).locator(".value").innerText().then(Number);
    await until("the sweep's end", target, (t) => t >= 5000, 30_000);
    await sleep(500);
    await dyno.getByRole("button", { name: "Release" }).click();
    await expect(dyno.getByRole("button", { name: "Start" })).toBeVisible({ timeout: 10_000 });
    await expect(dyno.locator("figure").first()).toContainText("Sweep 2000-5000 rpm at 1000 rpm/s, wide open");

    // the panel's export, the save dialog answered with a path
    const csvPath = join(run.userData, "app-dyno.csv");
    await app.evaluate(({ dialog }, path) => {
      dialog.showSaveDialog = (async () => ({ canceled: false, filePath: path })) as typeof dialog.showSaveDialog;
    }, csvPath);
    await dyno.getByRole("button", { name: "Export CSV" }).click();
    await until("the CSV", () => Promise.resolve(existsSync(csvPath)), (e) => e, 10_000);
    const appRows = csvRows(readFileSync(csvPath, "utf8"));

    // the same drive replayed natively: every reading the core's dyno made
    const capture = await dump(page);
    expect(capture.dropped).toBe(0);
    expect(capture.interrupted).toBe(false);
    const trace = readFileSync(capture.trace, "utf8");
    expect(trace).toMatch(/dyno_attach 2000/);
    expect(trace).toMatch(/dyno_sweep 5000 1000/);
    expect(trace).toMatch(/dyno_release/);
    const nativeCsv = join(run.userData, "native-dyno.csv");
    execFileSync(renderer, [capture.trace, "-o", join(run.userData, "offline.wav"), "--dyno-csv", nativeCsv], { stdio: "pipe" });
    const native = new Map(csvRows(readFileSync(nativeCsv, "utf8")).map((r) => [Number(r.reading), r]));

    // the gate
    expect(appRows.length).toBeGreaterThan(40);
    const peak = Math.max(...[...native.values()].map((r) => Number(r.torque_Nm)));
    expect(peak).toBeGreaterThan(50);
    const first = appRows[0];
    const firstNative = native.get(Number(first.reading))!;
    expect(firstNative, `reading ${first.reading} in the native run`).toBeDefined();
    let worst = 0;
    let previousT = 0;
    const lates: { reading: number; late: number; gap: number }[] = [];
    for (const row of appRows) {
      const n = native.get(Number(row.reading));
      expect(n, `reading ${row.reading} in the native run`).toBeDefined();
      const appTorque = Number(row.torque_Nm);
      const nativeTorque = Number(n!.torque_Nm);
      const error = Math.abs(appTorque - nativeTorque);
      const allowed = Math.abs(nativeTorque) > 0.1 * peak ? 0.02 * Math.abs(nativeTorque) : 0.01 * peak;
      worst = Math.max(worst, error / allowed);
      expect(error, `reading ${row.reading}: ${appTorque} N m in the app, ${nativeTorque} native`).toBeLessThanOrEqual(allowed);
      expect(Math.abs(Number(row.rpm) - Number(n!.rpm))).toBeLessThanOrEqual(0.02 * Number(n!.rpm));
      lates.push({ reading: Number(row.reading), late: Number(row.time_s) - (Number(n!.frame) - Number(firstNative.frame)) / capture.rate,
                   gap: Number(row.time_s) - previousT });
      previousT = Number(row.time_s);
    }
    // aligned in time. The app stamps a point with the snapshot in which the reading's count
    // rose: never early, and late by at most the time since the snapshot before it, which a busy
    // window (CI's runner, 2026-11-23) can stretch past a 60th of a second. Its times count from
    // the run's first point, which may itself be late, so lateness is measured from the least
    // late point (on time, or nearly). Each point is then late by no more than the time since the
    // app's previous point (and a block), and nine in ten within the card's snapshot interval
    // and a block.
    const block = 128 / capture.rate;
    const base = Math.min(...lates.map((l) => l.late));
    let onTime = 0;
    for (const l of lates) {
      const late = l.late - base;
      expect(late, `reading ${l.reading}'s time`).toBeLessThanOrEqual(Math.max(1 / 60, l.gap) + block + 1e-3);
      if (late <= 1 / 60 + block + 1e-3) onTime++;
    }
    expect(Math.abs(base), "the first point's lateness").toBeLessThanOrEqual(3 / 60 + block);
    expect(onTime / lates.length, "points within a snapshot interval and a block").toBeGreaterThanOrEqual(0.9);
    console.log(`dyno parity: ${appRows.length} points, worst ${(100 * worst).toFixed(1)} % of the allowed error, peak ${peak.toFixed(1)} N m`);
    expect(run.problems).toEqual([]);
  } finally {
    await run.dispose();
  }
});
