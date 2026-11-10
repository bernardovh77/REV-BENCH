/*
 * window.ts - the window's creation options and the rules that keep it closed in: Electron's
 * security checklist as far as it applies to a window that loads only its own build from a
 * file. tests/security.test.ts checks every item. Only types come from "electron" here.
 *
 *   1  no Node in the renderer: sandbox on, contextIsolation on, nodeIntegration off
 *      (also in workers and sub-frames)
 *   2  web security on, no insecure content, no experimental features
 *   3  a strict content security policy (src/shared/csp.ts, put into index.html at build time)
 *   4  no <webview>, no new windows, no navigation away from the app's own page
 *   5  every permission request (camera, location, ...) is refused
 *   6  messages from the page reach the main process only if they come from the app's own page
 *   7  nothing is loaded from the network (the window loads a file; see tests)
 */
import { realpathSync } from "node:fs";
import { basename, dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import type { BrowserWindowConstructorOptions, WebPreferences } from "electron";

/** The dark theme's background (src/ui/design/tokens.css), so the window doesn't flash white. */
export const WINDOW_BACKGROUND = "#0e1014";

export function webPreferences(preloadPath: string): WebPreferences {
  return {
    preload: preloadPath,
    sandbox: true,
    contextIsolation: true,
    nodeIntegration: false,
    nodeIntegrationInWorker: false,
    nodeIntegrationInSubFrames: false,
    webSecurity: true,
    allowRunningInsecureContent: false,
    experimentalFeatures: false,
    webviewTag: false,
    navigateOnDragDrop: false,
    spellcheck: false,
  };
}

export function mainWindowOptions(preloadPath: string, title: string): BrowserWindowConstructorOptions {
  return {
    title,
    width: 1280,
    height: 800,
    minWidth: 900,
    minHeight: 600,
    show: false,
    backgroundColor: WINDOW_BACKGROUND,
    webPreferences: webPreferences(preloadPath),
  };
}

/**
 * A file's path in one spelling: the real path of its longest existing folder (Windows expands
 * 8.3 short names such as RUNNER~1 there), with the rest appended. Folders inside app.asar
 * aren't real, so the walk stops at the asar file itself.
 */
export function canonicalPath(path: string): string {
  let head = resolve(path);
  const tail: string[] = [];
  for (;;) {
    try {
      return join(realpathSync.native(head), ...tail);
    } catch {
      const parent = dirname(head);
      if (parent === head) return resolve(path);
      tail.unshift(basename(head));
      head = parent;
    }
  }
}

/**
 * True only for the app's own page: the same file, however its path is spelt (a short name, the
 * letter case on Windows); the hash and query may differ.
 */
export function isAppPage(url: string | undefined, rendererIndex: string, canonical: (path: string) => string = canonicalPath): boolean {
  if (!url) return false;
  try {
    const given = new URL(url);
    if (given.protocol !== "file:") return false;
    const fold = (p: string) => (process.platform === "win32" ? p.toLowerCase() : p);
    return fold(canonical(fileURLToPath(given))) === fold(canonical(rendererIndex));
  } catch {
    return false;
  }
}

interface NavigationEvent {
  preventDefault(): void;
  url?: string;
}

/** The parts of a WebContents (and its session) the rules need, so tests can give a fake. */
export interface SecurableContents {
  on(event: "will-navigate" | "will-redirect" | "will-attach-webview", listener: (event: NavigationEvent, url?: string) => void): unknown;
  setWindowOpenHandler(handler: (details: { url: string }) => { action: "deny" }): void;
  session: {
    setPermissionRequestHandler(handler: (contents: unknown, permission: string, callback: (granted: boolean) => void) => void): void;
    setPermissionCheckHandler(handler: () => boolean): void;
  };
}

/** Applies items 4 and 5: nothing opens, nothing navigates away, nothing is granted. */
export function secureWebContents(contents: SecurableContents, rendererIndex: string, log: (message: string) => void = () => {}): void {
  const guard = (event: NavigationEvent, url?: string) => {
    const target = event.url ?? url;
    if (!isAppPage(target, rendererIndex)) {
      log(`blocked navigation to ${target ?? "(unknown)"}`);
      event.preventDefault();
    }
  };
  contents.on("will-navigate", guard);
  contents.on("will-redirect", guard);
  contents.on("will-attach-webview", (event) => event.preventDefault());
  contents.setWindowOpenHandler(({ url }) => {
    log(`blocked a new window for ${url}`);
    return { action: "deny" };
  });
  contents.session.setPermissionRequestHandler((_contents, permission, callback) => {
    log(`refused the permission "${permission}"`);
    callback(false);
  });
  contents.session.setPermissionCheckHandler(() => false);
}

/** Item 6: an IPC message is trusted only from the app's own page. */
export function isTrustedSender(frameUrl: string | undefined, rendererIndex: string): boolean {
  return isAppPage(frameUrl, rendererIndex);
}
