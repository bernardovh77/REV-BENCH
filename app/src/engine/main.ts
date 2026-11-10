/*
 * main.ts - the engine utility process's entry (built to dist/engine/engine.cjs). Electron
 * starts it from the main process with utilityProcess.fork; everything that matters is in
 * process.ts (testable) and control.ts.
 */
import { existsSync } from "node:fs";
import { createRequire } from "node:module";
import { loadAddon } from "./addon-loader";
import { runEngineProcess, type ParentPortLike } from "./process";

// Electron adds `parentPort` to the utility process's `process`
const parentPort = (process as unknown as { parentPort?: ParentPortLike }).parentPort;
if (!parentPort) throw new Error("rev-bench engine: not running as an Electron utility process");

const nodeRequire = createRequire(__filename);

runEngineProcess({
  parentPort,
  loadAddon: (kind) =>
    loadAddon(kind, {
      env: process.env,
      dirname: __dirname,
      resourcesPath: (process as unknown as { resourcesPath?: string }).resourcesPath,
      exists: existsSync,
      load: (path) => nodeRequire(path),
      // its own file in the build, so a package may leave it out
      loadFake: () => (nodeRequire("./fake-addon.cjs") as { createFakeAddon(): ReturnType<typeof loadAddon> }).createFakeAddon(),
    }),
  setInterval: (callback, ms) => setInterval(callback, ms),
  clearInterval: (handle) => clearInterval(handle as NodeJS.Timeout),
  exit: (code) => process.exit(code),
});

process.on("uncaughtException", (error) => {
  parentPort.postMessage({ type: "fatal", message: `uncaught: ${error.stack ?? error.message}` });
  process.exit(1);
});
process.on("unhandledRejection", (reason) => {
  parentPort.postMessage({ type: "fatal", message: `unhandled rejection: ${String(reason)}` });
  process.exit(1);
});
