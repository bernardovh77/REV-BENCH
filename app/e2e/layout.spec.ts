/*
 * Stage 131's end-to-end test: the layout in the built app, on the real add-on and the null
 * audio backend (never a sound device). The four rebuilt as a V6 from the editor's Layout
 * controls (previewed, then applied as one rebuild, its per-cylinder edit dropped and said so),
 * kept by the autosave, undone back to the four with that edit, and a common-pin V12 at 120
 * degrees refused in the preview and kept out, with the engine playing on throughout.
 */
import { expect, test, type Page } from "@playwright/test";
import { autosaved, launch, until, waitForState } from "./app";

const aside = (page: Page) => page.locator("aside");
const layoutSelect = (page: Page, label: string) => aside(page).locator("details.layout label", { hasText: label }).locator("select");
const field = (page: Page, name: string) => page.locator(`aside [data-field="${name}"]`);
const cylinders = async (page: Page) => Number((await field(page, "cylinders").locator(".readonly").innerText()).split(" ")[0]);

test("the layout: a V6 from the four, previewed and applied, undone, and a refusal", async () => {
  const run = await launch();
  try {
    const { page } = run;
    await waitForState(page, "Engine running");
    await expect(aside(page).locator("details.layout summary")).toHaveText("Layout");
    await expect(layoutSelect(page, "Family")).toHaveValue("inline");
    await expect(layoutSelect(page, "Cylinders")).toHaveValue("4");
    expect(await cylinders(page)).toBe(4);

    // a header made longer first: an edit per cylinder, which a new layout drops
    await page.locator("aside details summary", { hasText: "Exhaust" }).click();
    const header = field(page, "header_length").locator("input").nth(1);
    await header.fill("650");
    await header.press("Enter");
    await aside(page).getByRole("button", { name: "Apply" }).click();
    await until("the header kept", () => Promise.resolve(autosaved(run.userData)?.edits?.length ?? 0), (n) => n === 1, 30_000);

    // a V: its nearest count (the four's 4), then 6, with its usual crank and angle, in the
    // draft with a preview
    await layoutSelect(page, "Family").selectOption("v");
    await expect(layoutSelect(page, "Cylinders")).toHaveValue("4");
    await layoutSelect(page, "Cylinders").selectOption("6");
    await expect(layoutSelect(page, "Crank")).toHaveValue("even");
    await expect(aside(page)).toContainText("1 change waits for a rebuild (as V6)");
    await expect(aside(page)).toContainText("drops an edit made per cylinder: header length");
    await until("the preview's cylinders", () => cylinders(page), (n) => n === 6, 10_000);
    await aside(page).getByRole("button", { name: "Apply" }).click();
    await expect(aside(page)).not.toContainText("wait for a rebuild", { timeout: 30_000 });
    await expect(aside(page).locator("details.layout summary")).toHaveText("Layout: V6, generic");
    await until("the autosave's layout", () => Promise.resolve(autosaved(run.userData)?.layout?.cylinders ?? 0), (n) => n === 6, 10_000);
    expect(autosaved(run.userData)?.layout).toEqual({ family: "v", cylinders: 6, crank: "even", bankAngle: 60 });
    expect(autosaved(run.userData)?.edits ?? []).toEqual([]);
    await expect(page.locator(".topbar .state")).toContainText("Engine running");

    // undo: the four again, with its header
    await aside(page).getByRole("button", { name: "Undo" }).click();
    await until("back to four", () => cylinders(page), (n) => n === 4, 30_000);
    await expect(aside(page).locator("details.layout summary")).toHaveText("Layout");
    await until("the header back", () => Promise.resolve(autosaved(run.userData)?.edits?.length ?? 0), (n) => n === 1, 10_000);
    expect(autosaved(run.userData)?.layout).toBeUndefined();

    // common pins on a V12 at 120 degrees: two cylinders would fire at once; refused, kept out
    await layoutSelect(page, "Family").selectOption("v");
    await layoutSelect(page, "Cylinders").selectOption("12");
    await layoutSelect(page, "Crank").selectOption("common-pin");
    const angle = aside(page).locator("details.layout input");
    await angle.fill("120");
    await angle.press("Enter");
    await expect(aside(page)).toContainText("two cylinders would fire at once", { timeout: 10_000 });
    await expect(aside(page).getByRole("button", { name: "Apply" })).toBeDisabled();
    await aside(page).getByRole("button", { name: "Discard" }).click();
    expect(await cylinders(page)).toBe(4);
    await expect(page.locator(".topbar .state")).toContainText("Engine running");
    expect(run.problems).toEqual([]);
  } finally {
    await run.dispose();
  }
});
