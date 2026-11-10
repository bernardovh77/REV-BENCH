/*
 * Electron's security checklist, as far as it applies (the list is in src/main/window.ts).
 * Stage 124's exit evidence: "the security settings checked by a test".
 */
import { execFileSync } from "node:child_process";
import { mkdtempSync, readFileSync, readdirSync, rmSync, statSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join, resolve } from "node:path";
import { pathToFileURL } from "node:url";
import { build } from "vite";
import { describe, expect, it } from "vitest";
import { CONTENT_SECURITY_POLICY, CSP_DIRECTIVES } from "../src/shared/csp";
import { WINDOW_BACKGROUND, isAppPage, isTrustedSender, mainWindowOptions, secureWebContents, webPreferences, type SecurableContents } from "../src/main/window";

const INDEX = resolve("/app/dist/ui/index.html");

describe("the window's web preferences (checklist 1 and 2)", () => {
  const prefs = webPreferences("/p/preload.cjs");

  it("sandboxes the renderer, isolates its context and gives it no Node, anywhere", () => {
    expect(prefs.sandbox).toBe(true);
    expect(prefs.contextIsolation).toBe(true);
    expect(prefs.nodeIntegration).toBe(false);
    expect(prefs.nodeIntegrationInWorker).toBe(false);
    expect(prefs.nodeIntegrationInSubFrames).toBe(false);
  });

  it("keeps web security on and allows no insecure or experimental content", () => {
    expect(prefs.webSecurity).toBe(true);
    expect(prefs.allowRunningInsecureContent).toBe(false);
    expect(prefs.experimentalFeatures).toBe(false);
  });

  it("has no <webview> and no navigation by dropping files", () => {
    expect(prefs.webviewTag).toBe(false);
    expect(prefs.navigateOnDragDrop).toBe(false);
  });

  it("loads the one preload script it is given", () => {
    expect(prefs.preload).toBe("/p/preload.cjs");
  });

  it("is what the main window is made with, hidden until it is ready", () => {
    const options = mainWindowOptions("/p/preload.cjs", "Rev-Bench");
    expect(options.webPreferences).toEqual(prefs);
    expect(options.show).toBe(false);
    expect(options.title).toBe("Rev-Bench");
  });
});

describe("the content security policy (checklist 3)", () => {
  const allowedSources = new Set(["'self'", "'none'"]);

  it("starts from nothing: default-src 'none'", () => {
    expect(CSP_DIRECTIVES["default-src"]).toEqual(["'none'"]);
  });

  it("allows only the app's own files, from 'self', and no network", () => {
    for (const [name, sources] of Object.entries(CSP_DIRECTIVES)) {
      for (const source of sources) expect(allowedSources, `${name} ${source}`).toContain(source);
    }
    expect(CSP_DIRECTIVES["connect-src"]).toEqual(["'none'"]);
  });

  it("has no inline script or style, no eval, no wildcard and no remote host", () => {
    for (const bad of ["unsafe-inline", "unsafe-eval", "unsafe-hashes", "wasm-unsafe-eval", "*", "http:", "https:", "data:", "blob:", "ws:"]) {
      expect(CONTENT_SECURITY_POLICY).not.toContain(bad);
    }
  });

  it("shuts the doors a script policy leaves: plugins, frames, base URI, form targets", () => {
    for (const name of ["object-src", "frame-src", "base-uri", "form-action", "worker-src"]) {
      expect(CSP_DIRECTIVES[name]).toEqual(["'none'"]);
    }
  });

  it("restricts scripts to the page's own files", () => {
    expect(CSP_DIRECTIVES["script-src"]).toEqual(["'self'"]);
    expect(CSP_DIRECTIVES["style-src"]).toEqual(["'self'"]);
  });
});

function fakeContents() {
  const handlers: Record<string, (event: { preventDefault(): void; url?: string }, url?: string) => void> = {};
  const prevented: string[] = [];
  let openHandler: ((d: { url: string }) => { action: "deny" }) | null = null;
  let permissionRequest: ((c: unknown, p: string, cb: (g: boolean) => void) => void) | null = null;
  let permissionCheck: (() => boolean) | null = null;
  const contents: SecurableContents = {
    on: (event, listener) => void (handlers[event] = listener),
    setWindowOpenHandler: (h) => void (openHandler = h),
    session: {
      setPermissionRequestHandler: (h) => void (permissionRequest = h),
      setPermissionCheckHandler: (h) => void (permissionCheck = h),
    },
  };
  const navigate = (event: string, url: string) => {
    const e = { url, preventDefault: () => void prevented.push(url) };
    handlers[event](e);
    return prevented.includes(url);
  };
  return { contents, navigate, prevented, open: (url: string) => openHandler!({ url }), request: (p: string) => { let granted: boolean | null = null; permissionRequest!(null, p, (g) => (granted = g)); return granted; }, check: () => permissionCheck!(), handlers };
}

describe("navigation, windows and permissions (checklist 4 and 5)", () => {
  const url = pathToFileURL(INDEX).href;

  it("tells the app's own page from everything else", () => {
    expect(isAppPage(url, INDEX)).toBe(true);
    expect(isAppPage(url + "#settings", INDEX)).toBe(true);
    expect(isAppPage("https://example.com/", INDEX)).toBe(false);
    expect(isAppPage("http://localhost:5173/", INDEX)).toBe(false);
    expect(isAppPage(pathToFileURL(resolve("/app/dist/ui/other.html")).href, INDEX)).toBe(false);
    expect(isAppPage("file:///C:/Windows/system.ini", INDEX)).toBe(false);
    expect(isAppPage("not a url", INDEX)).toBe(false);
    expect(isAppPage(undefined, INDEX)).toBe(false);
  });

  it("knows its page under another spelling of the same path (an 8.3 short name, letter case)", () => {
    // CI installed the app under C:\Users\RUNNER~1\...; Chromium reported the page under the
    // long name, and the window never got its port. The same file must count, however spelt.
    const canonical = (p: string) => p.replace("RUNNER~1", "runneradmin");
    const short = "C:\\Users\\RUNNER~1\\AppData\\Local\\Temp\\rb\\resources\\app.asar\\dist\\ui\\index.html";
    const long = "file:///C:/Users/runneradmin/AppData/Local/Temp/rb/resources/app.asar/dist/ui/index.html";
    expect(isAppPage(long, short, canonical)).toBe(true);
    expect(isAppPage(long.replace("index.html", "other.html"), short, canonical)).toBe(false);
    if (process.platform === "win32") {
      expect(isAppPage(long.toUpperCase().replace("FILE:", "file:"), short, canonical)).toBe(true);
    }
  });

  it.runIf(process.platform === "win32")("resolves a real 8.3 short name on Windows", () => {
    const dir = mkdtempSync(join(tmpdir(), "rb long folder name "));
    try {
      const page = join(dir, "index.html");
      writeFileSync(page, "");
      const short = execFileSync("cmd", ["/d", "/c", "for %I in (index.html) do @echo %~sI"], { cwd: dir, encoding: "utf8" }).trim();
      expect(short).not.toBe(page);                                 // the volume makes short names
      expect(isAppPage(pathToFileURL(page).href, short)).toBe(true);
      expect(isAppPage(pathToFileURL(short).href, page)).toBe(true);
    } finally {
      rmSync(dir, { recursive: true, force: true });
    }
  });

  it("blocks navigation and redirects to anywhere but the app's page", () => {
    const f = fakeContents();
    secureWebContents(f.contents, INDEX);
    expect(f.navigate("will-navigate", "https://example.com/")).toBe(true);
    expect(f.navigate("will-redirect", "https://example.com/redirected")).toBe(true);
    expect(f.navigate("will-navigate", "file:///C:/Windows/system.ini")).toBe(true);
    expect(f.navigate("will-navigate", url)).toBe(false);
  });

  it("refuses to open new windows, whatever the URL", () => {
    const f = fakeContents();
    secureWebContents(f.contents, INDEX);
    expect(f.open("https://example.com/")).toEqual({ action: "deny" });
    expect(f.open(url)).toEqual({ action: "deny" });
  });

  it("refuses to attach a <webview>", () => {
    const f = fakeContents();
    secureWebContents(f.contents, INDEX);
    expect(f.navigate("will-attach-webview", "anything")).toBe(true);
  });

  it("refuses every permission request and answers every permission check no", () => {
    const f = fakeContents();
    secureWebContents(f.contents, INDEX);
    for (const permission of ["media", "geolocation", "notifications", "clipboard-read", "midi", "hid", "serial"]) {
      expect(f.request(permission)).toBe(false);
    }
    expect(f.check()).toBe(false);
  });

  it("logs what it blocks", () => {
    const f = fakeContents();
    const lines: string[] = [];
    secureWebContents(f.contents, INDEX, (m) => lines.push(m));
    f.navigate("will-navigate", "https://example.com/");
    f.open("https://example.com/popup");
    f.request("media");
    expect(lines.join("\n")).toMatch(/example\.com\/\n?.*popup[\s\S]*media/);
  });
});

describe("who may talk to the main process (checklist 6)", () => {
  it("trusts IPC from the app's own page only", () => {
    expect(isTrustedSender(pathToFileURL(INDEX).href, INDEX)).toBe(true);
    expect(isTrustedSender("https://example.com/", INDEX)).toBe(false);
    expect(isTrustedSender(undefined, INDEX)).toBe(false);
  });
});

describe("the built page (checklist 3 and 7)", () => {
  it("carries the policy in a meta tag, with no inline script or style and no remote URL", async () => {
    const out = mkdtempSync(join(tmpdir(), "rb-ui-build-"));
    try {
      await build({
        configFile: resolve("vite.config.ts"),
        root: resolve("src/ui"),
        logLevel: "silent",
        build: { outDir: out, emptyOutDir: true },
      });
      const html = readFileSync(join(out, "index.html"), "utf8");
      const meta = /<meta http-equiv="Content-Security-Policy" content="([^"]*)"/.exec(html);
      // the build writes the quotes as HTML entities, which the parser reads back as quotes
      expect(meta?.[1].replaceAll("&#39;", "'")).toBe(CONTENT_SECURITY_POLICY);
      expect(html).not.toMatch(/<script(?![^>]*\bsrc=)[^>]*>/); // every script has a src
      expect(html).not.toMatch(/<style/i);
      expect(html).not.toMatch(/\sstyle=/i);
      expect(html).not.toMatch(/\son[a-z]+=/i);
      expect(html).not.toMatch(/(https?:)?\/\/[a-z0-9.-]+\.[a-z]{2,}/i);
      // nothing in the build asks for the network either
      for (const file of readdirSync(join(out, "assets"))) {
        if (!file.endsWith(".js") && !file.endsWith(".css")) continue;
        const text = readFileSync(join(out, "assets", file), "utf8");
        expect(text, file).not.toMatch(/url\(\s*["']?https?:/);
        expect(text, file).not.toMatch(/\bfetch\(["']https?:/);
      }
    } finally {
      rmSync(out, { recursive: true, force: true });
    }
  }, 60_000);

  it("has a preload that is one file needing nothing but electron (all a sandbox allows)", async () => {
    const out = mkdtempSync(join(tmpdir(), "rb-preload-build-"));
    try {
      const script = pathToFileURL(resolve("scripts/bundle-node.mjs")).href;
      const { buildProgram, programs } = (await import(/* @vite-ignore */ script)) as {
        buildProgram(p: unknown, outRoot: string, logLevel: string): Promise<void>;
        programs: { dir: string; file: string }[];
      };
      const preload = programs.find((p) => p.dir === "preload")!;
      await buildProgram(preload, out, "silent");
      const code = readFileSync(join(out, "preload", preload.file), "utf8");
      const required = [...code.matchAll(/require\(\s*["']([^"']+)["']\s*\)/g)].map((m) => m[1]);
      expect(required.length).toBeGreaterThan(0);
      expect([...new Set(required)]).toEqual(["electron"]);
      expect(code).not.toMatch(/\bimport\s*\(/);
    } finally {
      rmSync(out, { recursive: true, force: true });
    }
  }, 60_000);

  it("uses a window background that is the dark theme's (so nothing flashes white)", () => {
    const tokens = readFileSync(resolve("src/ui/design/tokens.css"), "utf8");
    expect(tokens.toLowerCase()).toContain(WINDOW_BACKGROUND.toLowerCase());
  });
});

describe("the source never turns a protection off", () => {
  function sources(dir: string): string[] {
    return readdirSync(dir).flatMap((name) => {
      const path = join(dir, name);
      if (statSync(path).isDirectory()) return sources(path);
      return /\.(ts|svelte|html|css)$/.test(name) ? [path] : [];
    });
  }
  const files = sources(resolve("src")).filter((f) => !f.endsWith("snapshot-v2.generated.ts"));

  it("finds the source files to scan", () => {
    expect(files.length).toBeGreaterThan(20);
  });

  it.each([
    ["nodeIntegration on", /nodeIntegration\w*:\s*true/],
    ["the sandbox off", /sandbox:\s*false/],
    ["context isolation off", /contextIsolation:\s*false/],
    ["web security off", /webSecurity:\s*false/],
    ["insecure content allowed", /allowRunningInsecureContent:\s*true/],
    ["a webview tag", /webviewTag:\s*true/],
    ["remote module", /enableRemoteModule/],
    ["a window loading a URL from the network", /loadURL\(\s*["'`]https?:/],
    ["eval", /\beval\(/],
    ["new Function", /new Function\(/],
    ["raw HTML in Svelte", /\{@html/],
    ["innerHTML", /\.innerHTML\s*=/],
    ["document.write", /document\.write\(/],
    ["an unsafe CSP source", /unsafe-(inline|eval)/],
  ])("has no %s", (_name, pattern) => {
    for (const file of files) {
      // the tests of these very rules live in tests/, not in src/
      expect(readFileSync(file, "utf8"), file).not.toMatch(pattern);
    }
  });
});
