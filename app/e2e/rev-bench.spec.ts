/*
 * Stage 124's end-to-end tests: the built app, driven through Playwright's Electron driver, with
 * the real add-on in the engine utility process on the null audio backend.
 */
import { expect, test } from "@playwright/test";
import { alive, autosaved, engineStatus, launch, preset, readLog, rpmShown, stateText, until, waitForState } from "./app";

test("starts on the real add-on and plays: ignition and the starter bring up the real engine speed", async () => {
  const run = await launch();
  try {
    const { page } = run;
    await waitForState(page, "Engine running");
    expect(await stateText(page)).toContain("Engine running");
    // the first engine is the default build, from the add-on's own list
    expect(await preset(page)).toBe("i4");
    expect(await page.locator(".topbar select option").allInnerTexts()).toContain("5.0 L cross-plane V8");

    await page.getByRole("button", { name: "Ignition" }).click();
    await expect(page.getByRole("button", { name: "Ignition" })).toHaveAttribute("aria-pressed", "true");
    await page.getByRole("button", { name: "Starter" }).dispatchEvent("pointerdown");
    await until("cranking", () => rpmShown(page), (rpm) => rpm > 100, 15_000);
    await new Promise((r) => setTimeout(r, 1500));
    await page.getByRole("button", { name: "Starter" }).dispatchEvent("pointerup");
    // real snapshots from the real engine: it runs on its own after the starter
    const idle = await until("the engine running on its own", () => rpmShown(page), (rpm) => rpm > 500, 20_000);
    expect(idle).toBeLessThan(2500);

    // the throttle changes the real engine's speed
    const slider = page.locator(".topbar input[type=range]");
    await slider.fill("0.5");
    await until("the throttle opening", () => rpmShown(page), (rpm) => rpm > idle + 500, 20_000);
    expect(run.problems).toEqual([]);
  } finally {
    await run.dispose();
  }
});

test("the page is sandboxed in the real app: no Node, only the preload's few names, the policy in place", async () => {
  const run = await launch();
  try {
    const { page } = run;
    await waitForState(page, "Engine running");
    const seen = await page.evaluate(() => ({
      require: typeof (globalThis as { require?: unknown }).require,
      process: typeof (globalThis as { process?: unknown }).process,
      bridge: Object.keys(globalThis.window.revbench ?? {}).sort(),
      csp: document.querySelector('meta[http-equiv="Content-Security-Policy"]')?.getAttribute("content"),
    }));
    expect(seen.require).toBe("undefined");
    expect(seen.process).toBe("undefined");
    expect(seen.bridge).toEqual(["onEngineStatus", "platform", "requestEnginePort", "restartEngine", "versions"]);
    expect(seen.csp).toContain("default-src 'none'");
    // an inline script is refused by the policy
    const ran = await page.evaluate(() => {
      const s = document.createElement("script");
      s.textContent = "page.__inline = true";
      document.head.appendChild(s);
      return (globalThis.window as { __inline?: boolean }).__inline === true;
    });
    expect(ran).toBe(false);
  } finally {
    await run.dispose();
  }
});

test("changes the engine, and the build is autosaved", async () => {
  const run = await launch();
  try {
    const { page } = run;
    await waitForState(page, "Engine running");
    await page.locator(".topbar select").selectOption("v8-cross");
    await until("the new engine", () => preset(page), (p) => p === "v8-cross");
    await expect(page.locator(".topbar select")).toBeEnabled();
    await until("the autosaved build", () => autosaved(run.userData), (b) => b?.preset === "v8-cross", 10_000);
    expect(run.problems).toEqual([]);
  } finally {
    await run.dispose();
  }
});

test("restarts after its process is killed, with the autosaved preset", async () => {
  const run = await launch();
  try {
    const { page } = run;
    await waitForState(page, "Engine running");
    await page.locator(".topbar select").selectOption("v8-cross");
    await until("the autosaved build", () => autosaved(run.userData), (b) => b?.preset === "v8-cross", 15_000);
    const before = await engineStatus(page);
    expect(before.state).toBe("running");
    expect(before.restarts).toBe(0);
    expect(before.pid).toBeGreaterThan(0);

    process.kill(before.pid!);
    await waitForState(page, "restarting", 15_000);
    await expect(page.getByRole("button", { name: "Ignition" })).toBeDisabled();
    await waitForState(page, "Engine running");
    const after = await engineStatus(page);
    expect(after.restarts).toBe(1);
    expect(after.pid).not.toBe(before.pid);
    // the autosaved build came back, and the new engine answers
    expect(await preset(page)).toBe("v8-cross");
    await page.getByRole("button", { name: "Ignition" }).click();
    await expect(page.getByRole("button", { name: "Ignition" })).toHaveAttribute("aria-pressed", "true");
    expect(readLog(run.userData)).toMatch(/engine died: the engine process exited/);
    expect(readLog(run.userData)).toMatch(/engine started .* with preset "v8-cross"/);
  } finally {
    await run.dispose();
  }
});

test("quits cleanly: the engine process is asked to stop and exits 0", async () => {
  const run = await launch();
  let pid = 0;
  try {
    const { page, app } = run;
    await waitForState(page, "Engine running");
    pid = (await engineStatus(page)).pid!;
    expect(alive(pid)).toBe(true);
    await app.close();
    await until("the engine process gone", () => alive(pid), (a) => !a, 10_000);
    const log = readLog(run.userData);
    expect(log).toMatch(/engine stopped \(exit 0\)/);
    expect(log).not.toMatch(/\[(main|engine)\] error: /); // the engine's stderr (a Node warning) is not ours
  } finally {
    await run.dispose();
  }
});

test("says why when the add-on is missing, and never falls back to a fake", async () => {
  const run = await launch({ native: false });
  try {
    const { page } = run;
    await waitForState(page, "Engine restarting", 15_000);
    await expect(page.locator("[role=status], [role=alert]").filter({ hasText: "rb_native.node" }).first()).toBeVisible();
    expect(readLog(run.userData)).toMatch(/fatal: the add-on rb_native\.node was not found/);
    // it tries again, then gives up and offers the user a restart
    await waitForState(page, "Engine stopped", 60_000);
    await expect(page.getByRole("button", { name: "Restart the engine" })).toBeVisible();
  } finally {
    await run.dispose();
  }
});
