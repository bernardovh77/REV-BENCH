/**
 * Browser smoke test: the page loads the core, plays it through an
 * AudioWorklet, and the pitch measured from the real audio output follows the
 * controls. Run with `npm run test:e2e`.
 */
import { expect, test, type Page } from '@playwright/test';

/** The pitch the page measured from its own audio output (AnalyserNode). */
async function measuredHz(page: Page): Promise<number> {
  const value = await page.locator('#pitch').getAttribute('data-hz');
  return value ? Number(value) : Number.NaN;
}

async function typePitch(page: Page, hz: number): Promise<void> {
  const input = page.locator('#freq-num');
  await input.fill(String(hz));
  await input.press('Enter');
}

async function setRange(page: Page, selector: string, value: number): Promise<void> {
  await page.locator(selector).evaluate((el: HTMLInputElement, v: number) => {
    el.value = String(v);
    el.dispatchEvent(new Event('input', { bubbles: true }));
  }, value);
}

test('the C core plays in an AudioWorklet and follows the controls', async ({ page }, testInfo) => {
  const problems: string[] = [];
  page.on('pageerror', (err) => problems.push(`page error: ${err.message}`));
  page.on('console', (msg) => {
    if (msg.type() === 'error') problems.push(`console error: ${msg.text()}`);
  });
  page.on('response', (res) => {
    if (res.status() >= 400) problems.push(`HTTP ${res.status()}: ${res.url()}`);
  });

  await page.goto('/');
  await expect(page.locator('#start')).toBeEnabled({ timeout: 15_000 });
  await expect(page.locator('#version')).toHaveText(/^\d+\.\d+\.\d+$/);

  await page.locator('#start').click();
  await expect(page.locator('#status')).toContainText('running', { timeout: 15_000 });

  // The pitch measured from the actual audio must follow the typed value.
  for (const hz of [440, 880, 150, 1234]) {
    await typePitch(page, hz);
    await expect
      .poll(async () => Math.abs((await measuredHz(page)) - hz), {
        message: `measured pitch should settle at ${hz} Hz`,
        timeout: 10_000,
      })
      .toBeLessThan(1 + hz * 0.003);
  }

  // The slider is logarithmic: its middle is sqrt(40 * 2000) = 282.8 Hz.
  await setRange(page, '#freq', 500);
  await expect(page.locator('#freq-num')).toHaveValue('283');
  await expect
    .poll(async () => Math.abs((await measuredHz(page)) - 282.84), { timeout: 10_000 })
    .toBeLessThan(2);

  // The core keeps up with real time and never produces NaN/inf.
  await page.waitForTimeout(3_000);
  const speed = Number.parseFloat((await page.locator('#t-speed').textContent()) ?? '');
  expect(speed).toBeGreaterThan(0.9);
  expect(speed).toBeLessThan(1.1);
  await expect(page.locator('#t-nonfinite')).toHaveText('0');
  await page.screenshot({ path: testInfo.outputPath('running.png'), fullPage: true });

  // Level 0 means silence: no pitch can be found any more.
  await setRange(page, '#level', 0);
  await expect(page.locator('#pitch')).toHaveText('—', { timeout: 5_000 });

  await page.locator('#start').click();
  await expect(page.locator('#status')).toContainText('Stopped');

  // Starting again builds a fresh AudioContext and engine.
  await setRange(page, '#level', 25);
  await page.locator('#start').click();
  await expect(page.locator('#status')).toContainText('running', { timeout: 15_000 });
  await typePitch(page, 660);
  await expect
    .poll(async () => Math.abs((await measuredHz(page)) - 660), { timeout: 10_000 })
    .toBeLessThan(3);
  await page.locator('#start').click();
  await expect(page.locator('#status')).toContainText('Stopped');
  expect(problems).toEqual([]);
});
