// bundle-node.mjs - builds the Node-side programs with Vite, one CommonJS file each:
//
//   dist/main/main.cjs          the Electron main process
//   dist/preload/preload.cjs    the sandboxed window's preload (a sandboxed preload can only
//                               be one CommonJS file, so everything it imports is bundled in)
//   dist/engine/engine.cjs      the engine utility process
//   dist/engine/fake-addon.cjs  the test double for the add-on, in a file of its own so a
//                               package can leave it out (the engine requires it only when
//                               RB_FAKE_ADDON=1)
//
// Why Vite and not tsc: the same TypeScript, the same resolver (".ts" imports, no ".js"
// suffixes) and the same pinned tool as the window's build, and the preload needs a bundle
// anyway. Types are checked apart (npm run typecheck). "electron" and Node's own modules stay
// outside the bundles.
//
// Run as `node scripts/bundle-node.mjs`; tests import `buildProgram` to build one into a folder.
import { builtinModules } from "node:module";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { build } from "vite";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");

export const programs = [
  { entry: "src/main/main.ts", dir: "main", file: "main.cjs" },
  { entry: "src/preload/preload.ts", dir: "preload", file: "preload.cjs" },
  { entry: "src/engine/main.ts", dir: "engine", file: "engine.cjs" },
  { entry: "src/engine/fake-addon.ts", dir: "engine", file: "fake-addon.cjs" },
];

/** Builds one program into `<outRoot>/<dir>/<file>` (default outRoot: dist). */
export async function buildProgram({ entry, dir, file }, outRoot = resolve(root, "dist"), logLevel = "warn") {
  await build({
    root,
    configFile: false,
    logLevel,
    build: {
      outDir: resolve(outRoot, dir),
      // each program writes its own file into the folder; a clean build removes dist first
      emptyOutDir: false,
      target: "es2022",
      minify: false,
      sourcemap: true,
      lib: { entry: resolve(root, entry), formats: ["cjs"], fileName: () => file },
      rollupOptions: {
        external: ["electron", ...builtinModules, ...builtinModules.map((m) => `node:${m}`)],
      },
    },
  });
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  for (const program of programs) {
    await buildProgram(program);
    console.log(`built dist/${program.dir}/${program.file}`);
  }
}
