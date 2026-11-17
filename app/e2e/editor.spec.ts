/*
 * Stage 067's end-to-end test: the engine editor in the built app, on the real add-on and the
 * null audio backend (never a sound device). A draft of rebuild fields applied as one rebuild,
 * a live field changed at once, undo back to the preset, and a change the core refuses shown
 * and kept out, with the engine playing on throughout.
 */
import { expect, test, type Page } from "@playwright/test";
import { autosaved, launch, until, waitForState } from "./app";

const field = (page: Page, name: string) => page.locator(`aside [data-field="${name}"]`);
const entry = (page: Page, name: string, i = 0) => field(page, name).locator("input").nth(i);
const displacement = async (page: Page) => Number((await field(page, "displacement").locator(".readonly").innerText()).split(" ")[0]);

async function type(page: Page, name: string, value: string, i = 0) {
  await entry(page, name, i).fill(value);
  await entry(page, name, i).press("Enter");
}

test("the editor: a draft applied as one rebuild, a live change, undo, and a refusal", async () => {
  const run = await launch();
  try {
    const { page } = run;
    await waitForState(page, "Engine running");
    // the core's own values for the four: an 86 mm bore, 2.0 L
    await expect(entry(page, "bore")).toHaveValue("86");
    const before = await displacement(page);
    expect(before).toBeCloseTo(1.998, 2);

    // two rebuild fields into the draft: the preview works out the new displacement first
    await type(page, "bore", "90");
    await page.locator("aside details summary", { hasText: "Exhaust" }).click();
    await type(page, "header_length", "700", 2);
    await expect(page.locator("aside")).toContainText("2 changes wait for a rebuild");
    await until("the preview's displacement", () => displacement(page), (d) => d > 2.18, 10_000);
    await page.locator("aside").getByRole("button", { name: "Apply" }).click();
    await expect(page.locator("aside")).not.toContainText("wait for a rebuild", { timeout: 30_000 });
    await expect(entry(page, "bore")).toHaveValue("90");
    expect(autosaved(run.userData)?.edits).toEqual([
      { field: "bore", index: 0, value: 0.09 },
      { field: "header_length", index: 2, value: 0.7 },
    ]);

    // a live field: the running engine takes it, no draft
    await page.locator("aside details summary", { hasText: "Limits and load" }).click();
    await type(page, "idle_rpm", "950");
    await until("the idle speed kept", () => Promise.resolve(autosaved(run.userData)?.edits?.length ?? 0), (n) => n === 3, 10_000);
    await expect(page.locator("aside")).not.toContainText("wait for a rebuild");

    // undo the idle speed and the draft: the preset as it comes
    await page.locator("aside").getByRole("button", { name: "Undo" }).click();
    await until("one undo", () => Promise.resolve(autosaved(run.userData)?.edits?.length ?? 0), (n) => n === 2, 30_000);
    await page.locator("aside").getByRole("button", { name: "Undo" }).click();
    await expect(entry(page, "bore")).toHaveValue("86", { timeout: 30_000 });
    expect(await displacement(page)).toBeCloseTo(before, 6);

    // a twin exhaust on a four with one collector: the core refuses it, the preview says so,
    // Apply can't be pressed, and the engine is untouched
    await field(page, "exhaust_systems").locator("select").selectOption("2");
    await expect(page.locator("aside")).toContainText("This engine can't be built", { timeout: 10_000 });
    await expect(page.locator("aside").getByRole("button", { name: "Apply" })).toBeDisabled();
    await page.locator("aside").getByRole("button", { name: "Discard" }).click();
    await expect(page.locator(".topbar .state")).toContainText("Engine running");
    expect(run.problems).toEqual([]);
  } finally {
    await run.dispose();
  }
});
