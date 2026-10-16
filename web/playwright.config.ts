import { defineConfig, devices } from '@playwright/test';

// npm run test:e2e                 test against the dev server
// E2E_PREVIEW=1 npm run test:e2e   test the production build (vite build + preview)
// PW_CHROMIUM=/path/to/chrome      use an already installed Chromium
//                                  (otherwise run `npx playwright install chromium` once)
const preview = Boolean(process.env.E2E_PREVIEW);
const port = preview ? 4173 : 5173;

export default defineConfig({
  testDir: 'tests/e2e',
  timeout: 60_000,
  reporter: 'list',
  use: {
    baseURL: `http://localhost:${port}`,
    trace: 'retain-on-failure',
  },
  projects: [
    {
      name: 'chromium',
      use: {
        ...devices['Desktop Chrome'],
        launchOptions: {
          executablePath: process.env.PW_CHROMIUM || undefined,
          args: ['--autoplay-policy=no-user-gesture-required'],
        },
      },
    },
  ],
  webServer: {
    command: preview
      ? `npm run build && npx vite preview --port ${port} --strictPort`
      : `npx vite --port ${port} --strictPort`,
    url: `http://localhost:${port}`,
    reuseExistingServer: !process.env.CI,
    timeout: 120_000,
  },
});
