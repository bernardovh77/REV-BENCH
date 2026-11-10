/*
 * preload.ts - runs in the sandboxed window before the page (built to dist/preload/preload.cjs,
 * one CommonJS file: a sandboxed preload can't load others). It exposes `window.revbench`
 * through the context bridge. All the logic is in bridge-impl.ts.
 */
import { contextBridge, ipcRenderer } from "electron";
import { makeBridge, type IpcLike } from "./bridge-impl";

const bridge = makeBridge({
  ipc: ipcRenderer as unknown as IpcLike,
  platform: process.platform,
  versions: { electron: process.versions.electron ?? "", chrome: process.versions.chrome ?? "" },
  // A MessagePort can't be an argument across the context bridge; posted to the page, it arrives
  // as a "message" event with the port in `ports` (Electron's documented way).
  postToPage: (message, ports) => window.postMessage(message, "*", ports),
});

contextBridge.exposeInMainWorld("revbench", bridge);
