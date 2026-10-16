import { defineConfig } from 'vitest/config';

export default defineConfig({
  // Relative asset URLs, so the built site works from any folder or host.
  base: './',
  // The AudioWorklet is loaded with audioWorklet.addModule(), which takes an
  // ES module, so worker bundles must be ES modules too.
  worker: { format: 'es' },
  build: { target: 'es2022' },
  test: { include: ['tests/unit/**/*.test.ts'] },
});
