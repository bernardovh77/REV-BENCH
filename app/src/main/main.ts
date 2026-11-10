/*
 * main.ts - the Electron main process (built to dist/main/main.cjs). It owns the window, the
 * menu and the engine's life, and is never in the sound's or the telemetry's way: the window
 * and the engine process talk over their own MessagePort. This file only wires the pieces,
 * each of which is tested apart (platform, autosave, logger, supervisor, window, config).
 */
import { app, BrowserWindow, ipcMain, Menu, MessageChannelMain, type IpcMainEvent, type IpcMainInvokeEvent, type MessagePortMain } from "electron";
import { IPC } from "../shared/bridge";
import { Autosave } from "./autosave";
import { engineInitFromEnv } from "./config";
import { spawnEngineProcess } from "./engine-spawn";
import { createFileLogger } from "./logger";
import { appFolders, buildPaths, menuTemplate, platformName, quitWhenAllWindowsClosed } from "./platform";
import { EngineSupervisor, type WindowEndpoint } from "./supervisor";
import { isTrustedSender, mainWindowOptions, secureWebContents } from "./window";

const APP_NAME = "Rev-Bench";

app.setName(APP_NAME);
// Tests (and anyone who wants a clean slate) point the user-data folder somewhere else.
if (process.env.RB_USER_DATA) app.setPath("userData", process.env.RB_USER_DATA);

const folders = appFolders(app);
const paths = buildPaths(app.getAppPath());
const logger = createFileLogger(folders.logFile);
const autosave = new Autosave(folders.autosaveFile);
const log = (message: string) => logger.log("info", "main", message);

let window: BrowserWindow | null = null;

const supervisor = new EngineSupervisor<MessagePortMain>({
  spawn: () => spawnEngineProcess(paths.engine, logger),
  createChannel: () => new MessageChannelMain(),
  autosave,
  engineInit: (build) => engineInitFromEnv(process.env, build),
  logger,
  schedule: (fn, ms) => {
    const timer = setTimeout(fn, ms);
    return () => clearTimeout(timer);
  },
  now: () => Date.now(),
});

const endpoint: WindowEndpoint<MessagePortMain> = {
  sendPort: (port, restarts) => {
    if (window && !window.isDestroyed()) window.webContents.postMessage(IPC.enginePort, { restarts }, [port]);
  },
  sendStatus: (status) => {
    if (window && !window.isDestroyed()) window.webContents.send(IPC.engineStatus, status);
  },
};

function trusted(event: IpcMainEvent | IpcMainInvokeEvent): boolean {
  const ok = isTrustedSender(event.senderFrame?.url, paths.rendererIndex);
  if (!ok) logger.log("warn", "main", `ignored an IPC message from ${event.senderFrame?.url ?? "an unknown frame"}`);
  return ok;
}

function createWindow(): void {
  window = new BrowserWindow(mainWindowOptions(paths.preload, APP_NAME));
  secureWebContents(window.webContents, paths.rendererIndex, log);
  window.once("ready-to-show", () => window?.show());
  window.webContents.on("render-process-gone", (_event, details) => logger.log("error", "main", `the window's process is gone: ${details.reason}`));
  window.on("closed", () => {
    window = null;
  });
  supervisor.attachWindow(endpoint);
  void window.loadFile(paths.rendererIndex);
}

ipcMain.handle(IPC.getEngineStatus, (event) => (trusted(event) ? supervisor.currentStatus() : null));
ipcMain.on(IPC.requestEnginePort, (event) => {
  if (trusted(event)) supervisor.requestPort();
});
ipcMain.on(IPC.restartEngine, (event) => {
  if (trusted(event)) supervisor.restartNow();
});

if (!app.requestSingleInstanceLock()) {
  app.quit();
} else {
  app.on("second-instance", () => {
    if (window) {
      if (window.isMinimized()) window.restore();
      window.focus();
    }
  });
  app.on("window-all-closed", () => {
    if (quitWhenAllWindowsClosed()) app.quit();
  });
  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
  app.on("before-quit", () => supervisor.stop());
  void app.whenReady().then(() => {
    Menu.setApplicationMenu(Menu.buildFromTemplate(menuTemplate({ appName: APP_NAME, packaged: app.isPackaged })));
    logger.log("info", "main", `${APP_NAME} ${app.getVersion()} starting (Electron ${process.versions.electron}, ${platformName()})`);
    supervisor.start();
    createWindow();
  });
}
