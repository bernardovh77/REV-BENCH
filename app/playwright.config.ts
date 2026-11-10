import { defineConfig } from "@playwright/test";

// The end-to-end tests drive the real application with Playwright's Electron driver: the
// built app (npm run build), the real add-on on the null audio backend (never a sound device),
// a fresh user-data folder each. They run one at a time: each starts a real-time audio thread.
export default defineConfig({
  testDir: "e2e",
  timeout: 120_000,
  expect: { timeout: 20_000 },
  workers: 1,
  fullyParallel: false,
  retries: 0,
  reporter: [["list"]],
  outputDir: process.env.RB_E2E_OUTPUT ?? "test-results",
});
