/*
 * The build saver's end-to-end test: in the built app, on the real add-on and the null audio
 * backend (never a sound device). The four saved as it comes, then with a bigger bore; each
 * loaded back and checked in the editor and on disk; a save over an existing name replaces it;
 * a delete asks twice and removes the file; the engine plays throughout.
 */
import { existsSync, readFileSync } from "node:fs";
import { join } from "node:path";
import { expect, test, type Page } from "@playwright/test";
import { launch, waitForState } from "./app";

const aside = (page: Page) => page.locator("aside");
const saved = (page: Page) => aside(page).locator("details.saved");
const nameBox = (page: Page) => saved(page).getByRole("textbox", { name: "Name this build" });
const item = (page: Page, id: string) => saved(page).locator(`[data-build="${id}"]`);
const bore = (page: Page) => page.locator('aside [data-field="bore"] input').first();

async function saveAs(page: Page, name: string) {
  await nameBox(page).fill(name);
  await saved(page).getByRole("button", { name: /^(Save|Replace)$/ }).click();
  await expect(saved(page)).toContainText(`Saved "${name}"`, { timeout: 10_000 });
}

test("the build saver: save, load back, replace and delete", async () => {
  const run = await launch();
  try {
    const { page } = run;
    await waitForState(page, "Engine running");
    await expect(saved(page)).toContainText("No builds saved yet.");
    const folder = join(run.userData, "builds");

    await saveAs(page, "Plain four");
    await expect(item(page, "plain-four")).toContainText("Inline 4, 0 changes");
    expect(JSON.parse(readFileSync(join(folder, "plain-four.json"), "utf8"))).toMatchObject({ version: 1, name: "Plain four", build: { preset: "i4" } });

    // a bigger bore, applied, then saved
    await bore(page).fill("90");
    await bore(page).press("Enter");
    await aside(page).getByRole("button", { name: "Apply" }).click();
    await expect(aside(page)).not.toContainText("wait for a rebuild", { timeout: 30_000 });
    await saveAs(page, "Big bore");
    await expect(item(page, "big-bore")).toContainText("1 change");
    expect(JSON.parse(readFileSync(join(folder, "big-bore.json"), "utf8")).build.edits).toEqual([{ field: "bore", index: 0, value: 0.09 }]);
    await expect(saved(page).locator("[data-build]").first()).toHaveAttribute("data-build", "big-bore");   // newest first

    // each loaded back
    await item(page, "plain-four").getByRole("button", { name: "Load" }).click();
    await expect(saved(page)).toContainText('Loaded "Plain four"', { timeout: 30_000 });
    await expect(bore(page)).toHaveValue("86");
    await item(page, "big-bore").getByRole("button", { name: "Load" }).click();
    await expect(saved(page)).toContainText('Loaded "Big bore"', { timeout: 30_000 });
    await expect(bore(page)).toHaveValue("90");

    // the same name again: the button says Replace, and the file is replaced
    await nameBox(page).fill("big BORE");
    await expect(saved(page).getByRole("button", { name: "Replace" })).toBeVisible();
    await saved(page).getByRole("button", { name: "Replace" }).click();
    await expect(saved(page).locator("[data-build]")).toHaveCount(2);
    expect(JSON.parse(readFileSync(join(folder, "big-bore.json"), "utf8")).name).toBe("big BORE");

    // delete asks twice
    await item(page, "plain-four").getByRole("button", { name: "Delete" }).click();
    await expect(item(page, "plain-four").getByRole("button", { name: "Sure?" })).toBeVisible();
    expect(existsSync(join(folder, "plain-four.json"))).toBe(true);
    await item(page, "plain-four").getByRole("button", { name: "Sure?" }).click();
    await expect(item(page, "plain-four")).toHaveCount(0);
    expect(existsSync(join(folder, "plain-four.json"))).toBe(false);
    await expect(page.locator(".topbar .state")).toContainText("Engine running");
    expect(run.problems).toEqual([]);
  } finally {
    await run.dispose();
  }
});
