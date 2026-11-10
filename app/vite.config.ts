/*
 * vite.config.ts - the renderer's build (the Svelte interface in src/ui/).
 * The main process, the preload script and the engine process are built by
 * scripts/bundle-node.mjs (docs/stage-124.md says why there are two builds).
 */
import { defineConfig, type Plugin } from "vite";
import { svelte } from "@sveltejs/vite-plugin-svelte";
import { fileURLToPath } from "node:url";
import { CONTENT_SECURITY_POLICY } from "./src/shared/csp.ts";

/** Puts the one content security policy (src/shared/csp.ts) into index.html. */
function contentSecurityPolicy(): Plugin {
  return {
    name: "rb-csp",
    transformIndexHtml: () => [
      {
        tag: "meta",
        attrs: { "http-equiv": "Content-Security-Policy", content: CONTENT_SECURITY_POLICY },
        injectTo: "head-prepend",
      },
    ],
  };
}

export default defineConfig({
  root: "src/ui",
  // Relative URLs: the window loads the build from a file, from any folder.
  base: "./",
  // svelte.config.js sits in app/, one level above the Vite root
  plugins: [svelte({ configFile: fileURLToPath(new URL("./svelte.config.js", import.meta.url)) }), contentSecurityPolicy()],
  build: {
    outDir: "../../dist/ui",
    emptyOutDir: true,
    target: "es2022",
    // No data: URIs for fonts or images; the policy allows only 'self'.
    assetsInlineLimit: 0,
    // The module preload polyfill is an inline script; the policy has none.
    modulePreload: { polyfill: false },
  },
});
