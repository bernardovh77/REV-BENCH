/*
 * Helpers for the end-to-end tests: launch the built app with the real add-on on the null
 * backend and a fresh user-data folder, and read what the page shows.
 */
import { _electron as electron, type ElectronApplication, type Page } from "@playwright/test";
import { existsSync, mkdtempSync, readFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import type { EngineStatus } from "../src/shared/bridge";

export const appRoot = join(dirname(fileURLToPath(import.meta.url)), "..");

export interface Launched {
  app: ElectronApplication;
  page: Page;
  userData: string;
  /** the page's console and uncaught errors */
  problems: string[];
  /** closes the app (if still open) and removes the user-data folder */
  dispose(): Promise<void>;
}

export function nativePath(): string {
  const path = process.env.RB_NATIVE_PATH;
  if (!path || !existsSync(path)) {
    throw new Error(`RB_NATIVE_PATH must name the built add-on (python tools/build_addon.py): ${String(path)}`);
  }
  return path;
}

/** Launches the app; with `native: false` it asks for no add-on at all (an engine that can't start). */
export async function launch(options: { native?: boolean; extraEnv?: Record<string, string> } = {}): Promise<Launched> {
  const userData = mkdtempSync(join(tmpdir(), "rb-e2e-"));
  const env: Record<string, string> = { ...(process.env as Record<string, string>) };
  delete env.RB_FAKE_ADDON;
  Object.assign(env, { RB_BACKEND: "null", RB_USER_DATA: userData, ...options.extraEnv });
  if (options.native !== false) env.RB_NATIVE_PATH = nativePath();
  else env.RB_NATIVE_PATH = join(userData, "no-such-addon.node");
  const app = await electron.launch({ args: ["."], cwd: appRoot, env });
  const page = await app.firstWindow();
  const problems: string[] = [];
  page.on("console", (m) => {
    if (m.type() === "error" || m.type() === "warning") problems.push(`${m.type()}: ${m.text()}`);
  });
  page.on("pageerror", (e) => problems.push(`pageerror: ${e.message}`));
  let open = true;
  app.on("close", () => (open = false));
  return {
    app,
    page,
    userData,
    problems,
    async dispose() {
      if (open) await app.close().catch(() => {});
      rmSync(userData, { recursive: true, force: true });
    },
  };
}

/** The top bar's words about the engine ("Engine running"). */
export const stateText = (page: Page) => page.locator(".topbar .state").innerText();

export async function waitForState(page: Page, text: string, timeout = 30_000): Promise<void> {
  await page.waitForFunction((t) => document.querySelector(".topbar .state")?.textContent?.includes(t) ?? false, text, { timeout });
}

/** The engine process's status from the main process (its pid, how often it has restarted). */
export function engineStatus(page: Page): Promise<EngineStatus> {
  return page.evaluate(
    () =>
      new Promise<EngineStatus>((resolve) => {
        const stop = globalThis.window.revbench!.onEngineStatus((s) => {
          stop();
          resolve(s);
        });
      }),
  );
}

export const rpmShown = async (page: Page): Promise<number> => Number(await page.locator(".topbar .readout .value").first().innerText());

export const preset = (page: Page) => page.locator(".topbar select").inputValue();

export function readLog(userData: string): string {
  const file = join(userData, "logs", "rev-bench.log");
  return existsSync(file) ? readFileSync(file, "utf8") : "";
}

export function autosaved(userData: string): { version: number; preset: string; quality?: string } | null {
  const file = join(userData, "session", "build.json");
  return existsSync(file) ? JSON.parse(readFileSync(file, "utf8")) : null;
}

/** Whether a process with this id is running. */
export function alive(pid: number): boolean {
  try {
    process.kill(pid, 0);
    return true;
  } catch {
    return false;
  }
}

export async function until<T>(what: string, read: () => Promise<T> | T, ok: (v: T) => boolean, timeoutMs = 20_000): Promise<T> {
  const end = Date.now() + timeoutMs;
  for (;;) {
    const v = await read();
    if (ok(v)) return v;
    if (Date.now() > end) throw new Error(`timed out waiting for ${what} (last: ${JSON.stringify(v)})`);
    await new Promise((r) => setTimeout(r, 100));
  }
}
