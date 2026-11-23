/*
 * Stage 133's 2-step, end to end: the built app on the real add-on and the null audio backend
 * (never a sound device). The four started, L held and the throttle wide open: the engine holds
 * the 2-step's limit (4000 rpm by default); L let go, it revs on to the rev limiter.
 */
import { expect, test } from "@playwright/test";
import { launch, rpmShown, until, waitForState } from "./app";

const sleep = (ms: number) => new Promise((r) => setTimeout(r, ms));

test("holding L holds the 2-step's limit wide open; letting go revs on", async () => {
  const run = await launch();
  try {
    const { page } = run;
    await waitForState(page, "Engine running");
    await page.locator("body").click({ position: { x: 5, y: 5 } });
    await page.keyboard.press("KeyI");
    await page.keyboard.down("KeyS");
    await until("cranking", () => rpmShown(page), (rpm) => rpm > 100, 15_000);
    await sleep(1200);
    await page.keyboard.up("KeyS");
    await until("running", () => rpmShown(page), (rpm) => rpm > 500, 20_000);

    await page.keyboard.down("KeyL");
    await page.keyboard.down("KeyW");
    await until("the 2-step's limit", () => rpmShown(page), (rpm) => rpm > 3700, 15_000);
    let top = 0;
    for (let i = 0; i < 20; i++) {
      top = Math.max(top, await rpmShown(page));
      await sleep(100);
    }
    expect(top).toBeLessThan(4400);
    await page.keyboard.up("KeyL");
    await until("revving past it", () => rpmShown(page), (rpm) => rpm > 5500, 15_000);
    await page.keyboard.up("KeyW");
    expect(run.problems).toEqual([]);
  } finally {
    await run.dispose();
  }
});
