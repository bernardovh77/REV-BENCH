/*
 * platform.ts - everything in the shell that depends on the operating system: where files
 * live, what the menu looks like, how windows behave when they are all closed. Nowhere else in
 * app/src/main (or the rest of app/src) tests process.platform. Windows is the full target until
 * stage 126; the macOS and Linux branches are the conventions, unverified on those systems.
 *
 * Only types come from "electron" here, so this file runs under Vitest.
 */
import { join } from "node:path";
import type { MenuItemConstructorOptions } from "electron";

export type PlatformName = NodeJS.Platform;

/** What the app needs from Electron's `app` to find its folders. */
export interface PathSource {
  getPath(name: "userData"): string;
}

export interface AppFolders {
  /** %APPDATA%\Rev-Bench on Windows, ~/Library/Application Support/Rev-Bench, ~/.config/Rev-Bench */
  userData: string;
  logs: string;
  logFile: string;
  /** the folder of the current session: the autosaved build */
  session: string;
  autosaveFile: string;
}

/** The user-data folder and the files in it, built with the path module, never by hand. */
export function appFolders(source: PathSource): AppFolders {
  const userData = source.getPath("userData");
  const logs = join(userData, "logs");
  const session = join(userData, "session");
  return {
    userData,
    logs,
    logFile: join(logs, "rev-bench.log"),
    session,
    autosaveFile: join(session, "build.json"),
  };
}

/** Where the built files are, from the app's root folder (the one holding package.json). */
export function buildPaths(appRoot: string) {
  return {
    mainDir: join(appRoot, "dist", "main"),
    preload: join(appRoot, "dist", "preload", "preload.cjs"),
    engine: join(appRoot, "dist", "engine", "engine.cjs"),
    rendererIndex: join(appRoot, "dist", "ui", "index.html"),
  };
}

export const platformName = (): PlatformName => process.platform;

export const isMac =(platform: PlatformName = process.platform): boolean => platform === "darwin";

/** On macOS an app stays open with no windows; everywhere else closing the last window quits. */
export const quitWhenAllWindowsClosed = (platform: PlatformName = process.platform): boolean => !isMac(platform);

/**
 * The menu, a stub: quit, copy and paste, the window, and the developer tools only outside a
 * package. The real menu (file, builds, view) comes with the stages that have things to put in it.
 */
export function menuTemplate(options: {
  platform?: PlatformName;
  appName: string;
  packaged: boolean;
}): MenuItemConstructorOptions[] {
  const mac = isMac(options.platform);
  const template: MenuItemConstructorOptions[] = [];
  if (mac) {
    template.push({
      label: options.appName,
      submenu: [{ role: "about" }, { type: "separator" }, { role: "hide" }, { role: "hideOthers" }, { role: "unhide" }, { type: "separator" }, { role: "quit" }],
    });
  } else {
    template.push({ label: "&File", submenu: [{ role: "quit" }] });
  }
  template.push({
    label: mac ? "Edit" : "&Edit",
    submenu: [{ role: "undo" }, { role: "redo" }, { type: "separator" }, { role: "cut" }, { role: "copy" }, { role: "paste" }, { role: "selectAll" }],
  });
  const view: MenuItemConstructorOptions[] = [{ role: "togglefullscreen" }];
  if (!options.packaged) view.push({ type: "separator" }, { role: "reload" }, { role: "toggleDevTools" });
  template.push({ label: mac ? "View" : "&View", submenu: view });
  template.push({ label: mac ? "Window" : "&Window", submenu: [{ role: "minimize" }, { role: "close" }] });
  return template;
}
