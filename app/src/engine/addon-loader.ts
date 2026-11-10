/*
 * addon-loader.ts - finds and loads the add-on in the engine process.
 *
 * "native" is the real rb_native.node (the C add-on over the host); "fake" is the test double
 * (fake-addon.ts), chosen only when asked (RB_FAKE_ADDON=1 in the main process, or a test). The
 * app never falls back to the fake on its own: a missing add-on is an error the window shows.
 */
import { join } from "node:path";
import type { Addon } from "../shared/addon";

export interface LoaderDeps {
  env: Record<string, string | undefined>;
  /** the folder of the running script (dist/engine in a build) */
  dirname: string;
  /** process.resourcesPath, in a packaged app */
  resourcesPath?: string;
  exists(path: string): boolean;
  load(path: string): unknown;
  /** the fake add-on's factory; the build keeps it in its own file so a package can leave it out */
  loadFake(): Addon;
}

export const ADDON_FILE = "rb_native.node";

/** Where the real add-on may be, in the order tried. */
export function nativeAddonCandidates(deps: Pick<LoaderDeps, "env" | "dirname" | "resourcesPath">): string[] {
  const out: string[] = [];
  if (deps.env.RB_NATIVE_PATH) out.push(deps.env.RB_NATIVE_PATH);
  if (deps.resourcesPath) out.push(join(deps.resourcesPath, ADDON_FILE));
  // a development build: app/dist/engine -> app/native/build/Release (cmake-js's output)
  out.push(join(deps.dirname, "..", "..", "native", "build", "Release", ADDON_FILE));
  return out;
}

const REQUIRED = ["version", "presets", "params", "telemetry", "listDevices", "createHost"] as const;

function looksLikeTheAddon(module: unknown): module is Addon {
  return typeof module === "object" && module !== null && REQUIRED.every((name) => typeof (module as Record<string, unknown>)[name] === "function");
}

export function loadAddon(kind: "native" | "fake", deps: LoaderDeps): Addon {
  if (kind === "fake") return deps.loadFake();
  const candidates = nativeAddonCandidates(deps);
  const found = candidates.find((path) => deps.exists(path));
  if (!found) {
    throw new Error(`the add-on ${ADDON_FILE} was not found; looked in: ${candidates.join("; ")}`);
  }
  const module = deps.load(found);
  if (!looksLikeTheAddon(module)) {
    throw new Error(`${found} does not look like the add-on (it lacks ${REQUIRED.filter((n) => typeof (module as Record<string, unknown> | null)?.[n] !== "function").join(", ")})`);
  }
  return module;
}
