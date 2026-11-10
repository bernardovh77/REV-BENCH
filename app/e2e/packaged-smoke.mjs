// packaged-smoke.mjs - starts a PACKAGED Rev-Bench (the unpacked folder, the portable zip's
// Rev-Bench.exe or an installed copy) and checks it plays:
//
//   node e2e/packaged-smoke.mjs <path to Rev-Bench.exe>
//
// The real add-on from the package's resources, on the null audio backend (RB_BACKEND=null: no
// sound device is opened) and a fresh user-data folder. Playwright's Electron driver can't drive
// a package (its fuses turn off the inspect flags it needs), so the app is started by hand with
// --remote-debugging-port, which no fuse governs, and Playwright connects to the window over
// that. It checks: the window reaches "Engine running", ignition and the starter bring the real
// engine to a speed of its own, another engine can be chosen (and is the autosaved one), no
// Node in the window, and that quitting stops the engine process with exit 0.
import { chromium } from "@playwright/test";
import { spawn } from "node:child_process";
import { existsSync, mkdtempSync, readFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

const exe = process.argv[2];
if (!exe || !existsSync(exe)) {
  console.error(`usage: node e2e/packaged-smoke.mjs <Rev-Bench.exe> (not found: ${exe})`);
  process.exit(2);
}
const port = 9333 + Math.floor(Math.random() * 500);
const userData = mkdtempSync(join(tmpdir(), "rb-packed-"));
const env = { ...process.env, RB_BACKEND: "null", RB_USER_DATA: userData };
for (const name of ["RB_FAKE_ADDON", "RB_NATIVE_PATH", "ELECTRON_RUN_AS_NODE"]) delete env[name];
const child = spawn(exe, [`--remote-debugging-port=${port}`], { env, stdio: "ignore" });
const exited = new Promise((resolve) => child.once("exit", (code) => resolve(code)));
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
let failed = null;
const check = (ok, what) => {
  if (!ok) throw new Error(what);
  console.log(`ok: ${what}`);
};

try {
  let browser;
  for (let i = 0; i < 100 && !browser; i++) {
    try {
      browser = await chromium.connectOverCDP(`http://127.0.0.1:${port}`);
    } catch {
      await sleep(200);
    }
  }
  check(browser, "connected to the window over the remote-debugging port");
  let page;
  for (let i = 0; i < 100 && !page; i++) {
    page = browser.contexts().flatMap((c) => c.pages()).find((p) => p.url().startsWith("file:"));
    if (!page) await sleep(200);
  }
  check(page, "the window is open");
  const state = () => page.locator(".topbar .state").innerText();
  await page.waitForFunction(() => document.querySelector(".topbar .state")?.textContent?.includes("Engine running"), null, { timeout: 30_000 });
  check((await state()).includes("Engine running"), "the window says Engine running");

  const seen = await page.evaluate(() => ({ require: typeof globalThis.require, process: typeof globalThis.process }));
  check(seen.require === "undefined" && seen.process === "undefined", "the window has no Node");

  await page.getByRole("button", { name: "Ignition" }).click();
  await page.getByRole("button", { name: "Starter" }).dispatchEvent("pointerdown");
  await sleep(2500);
  await page.getByRole("button", { name: "Starter" }).dispatchEvent("pointerup");
  let rpm = 0;
  for (let i = 0; i < 200 && rpm <= 500; i++) {
    rpm = Number(await page.locator(".topbar .readout .value").first().innerText());
    await sleep(100);
  }
  check(rpm > 500, `the real engine runs on its own (${rpm} rpm)`);

  await page.locator(".topbar select").selectOption("v8-cross");
  for (let i = 0; i < 200 && (await page.locator(".topbar select").inputValue()) !== "v8-cross"; i++) await sleep(100);
  check((await page.locator(".topbar select").inputValue()) === "v8-cross", "another engine was chosen");
  const saved = join(userData, "session", "build.json");
  for (let i = 0; i < 100 && !(existsSync(saved) && readFileSync(saved, "utf8").includes("v8-cross")); i++) await sleep(100);
  check(existsSync(saved) && readFileSync(saved, "utf8").includes("v8-cross"), "the build was autosaved");

  await browser.close().catch(() => {});
  // quit as a user would: close the window (the app quits with its last window on Windows)
  const quit = spawn("taskkill", ["/PID", String(child.pid)], { stdio: "ignore" });
  await new Promise((r) => quit.once("exit", r));
  const code = await Promise.race([exited, sleep(15_000).then(() => "timeout")]);
  check(code !== "timeout", "the app quit");
  const log = readFileSync(join(userData, "logs", "rev-bench.log"), "utf8");
  check(/engine stopped \(exit 0\)/.test(log), "the engine process stopped with exit 0");
  check(!/\[(main|engine)\] error: /.test(log), "the log has no errors");
} catch (e) {
  failed = e;
  console.error(`FAILED: ${e.message}`);
  try {
    console.error(readFileSync(join(userData, "logs", "rev-bench.log"), "utf8"));
  } catch {
    // no log
  }
} finally {
  child.kill();
  rmSync(userData, { recursive: true, force: true });
}
process.exit(failed ? 1 : 0);
