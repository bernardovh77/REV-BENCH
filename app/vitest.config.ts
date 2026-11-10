import { defineConfig } from "vitest/config";
import { svelte } from "@sveltejs/vite-plugin-svelte";

export default defineConfig({
  plugins: [svelte()],
  // Svelte's browser build: the components mount in jsdom.
  resolve: { conditions: ["browser"] },
  test: {
    include: ["tests/**/*.test.ts"],
    environment: "node",
  },
});
