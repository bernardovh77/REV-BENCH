/*
 * The main process's pieces that need no Electron: platform paths and menus, the autosave, the
 * log file, the environment's choices.
 */
import { mkdtempSync, readFileSync, rmSync, writeFileSync, existsSync, readdirSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { Autosave, DEFAULT_BUILD } from "../src/main/autosave";
import { engineInitFromEnv } from "../src/main/config";
import { createFileLogger } from "../src/main/logger";
import { appFolders, buildPaths, menuTemplate, quitWhenAllWindowsClosed } from "../src/main/platform";

let dir: string;
beforeEach(() => {
  dir = mkdtempSync(join(tmpdir(), "rb-main-"));
});
afterEach(() => {
  rmSync(dir, { recursive: true, force: true });
});

describe("platform: folders", () => {
  it("puts logs and the session inside the user-data folder, joined with the path module", () => {
    const folders = appFolders({ getPath: () => join("base", "Rev-Bench") });
    expect(folders.userData).toBe(join("base", "Rev-Bench"));
    expect(folders.logFile).toBe(join("base", "Rev-Bench", "logs", "rev-bench.log"));
    expect(folders.autosaveFile).toBe(join("base", "Rev-Bench", "session", "build.json"));
  });

  it("finds the built files from the app's root", () => {
    const paths = buildPaths(join("root", "app"));
    expect(paths.preload).toBe(join("root", "app", "dist", "preload", "preload.cjs"));
    expect(paths.engine).toBe(join("root", "app", "dist", "engine", "engine.cjs"));
    expect(paths.rendererIndex).toBe(join("root", "app", "dist", "ui", "index.html"));
  });

  it("quits with the last window everywhere but macOS", () => {
    expect(quitWhenAllWindowsClosed("win32")).toBe(true);
    expect(quitWhenAllWindowsClosed("linux")).toBe(true);
    expect(quitWhenAllWindowsClosed("darwin")).toBe(false);
  });
});

describe("platform: the menu stub", () => {
  const labels = (t: ReturnType<typeof menuTemplate>) => t.map((m) => m.label);
  const roles = (item: { submenu?: unknown }) => ((item.submenu ?? []) as { role?: string }[]).map((s) => s.role);

  it("has the app's own menu first on macOS", () => {
    const t = menuTemplate({ platform: "darwin", appName: "Rev-Bench", packaged: true });
    expect(labels(t)[0]).toBe("Rev-Bench");
    expect(roles(t[0])).toContain("quit");
  });

  it("has Quit under File on Windows and Linux", () => {
    for (const platform of ["win32", "linux"] as const) {
      const t = menuTemplate({ platform, appName: "Rev-Bench", packaged: true });
      expect(labels(t)[0]).toBe("&File");
      expect(roles(t[0])).toEqual(["quit"]);
    }
  });

  it("offers reload and the developer tools only outside a package", () => {
    const view = (packaged: boolean) => roles(menuTemplate({ platform: "win32", appName: "Rev-Bench", packaged }).find((m) => m.label === "&View")!);
    expect(view(true)).not.toContain("toggleDevTools");
    expect(view(true)).not.toContain("reload");
    expect(view(false)).toContain("toggleDevTools");
  });
});

describe("autosave", () => {
  const file = () => join(dir, "session", "build.json");

  it("has nothing before anything is saved", () => {
    expect(new Autosave(file()).load()).toBeNull();
  });

  it("keeps the preset and the quality, and gives them back", () => {
    const autosave = new Autosave(file());
    autosave.save({ preset: "v8-cross", quality: "low" });
    expect(new Autosave(file()).load()).toEqual({ preset: "v8-cross", quality: "low" });
    autosave.save({ preset: "i6" });
    expect(new Autosave(file()).load()).toEqual({ preset: "i6" });
  });

  it("creates the folder, and leaves no temporary file behind", () => {
    new Autosave(file()).save({ preset: "i4" });
    expect(readdirSync(join(dir, "session"))).toEqual(["build.json"]);
  });

  it("treats a file that isn't ours as no autosave", () => {
    new Autosave(file()).save({ preset: "i4" });
    for (const junk of ["", "not json", "[]", "null", '{"version":2,"preset":"i4"}', '{"version":1}', '{"version":1,"preset":""}', '{"version":1,"preset":4}']) {
      writeFileSync(file(), junk);
      expect(new Autosave(file()).load()).toBeNull();
    }
  });

  it("drops a quality it doesn't know but keeps the preset", () => {
    new Autosave(file()).save({ preset: "i4" });
    writeFileSync(file(), '{"version":1,"preset":"i6","quality":"ludicrous"}');
    expect(new Autosave(file()).load()).toEqual({ preset: "i6" });
  });

  it("has a default build: the 2.0 L four", () => {
    expect(DEFAULT_BUILD).toEqual({ preset: "i4" });
  });
});

describe("the log file", () => {
  const file = () => join(dir, "logs", "rev-bench.log");
  const fixed = () => new Date("2026-11-09T12:00:00.000Z");

  it("appends one line per message, with time, source and level", () => {
    const logger = createFileLogger(file(), { now: fixed });
    logger.log("info", "main", "started");
    logger.log("error", "engine", "boom");
    expect(readFileSync(file(), "utf8")).toBe(
      "2026-11-09T12:00:00.000Z [main] info: started\n2026-11-09T12:00:00.000Z [engine] error: boom\n",
    );
  });

  it("indents the lines of a message that has several", () => {
    createFileLogger(file(), { now: fixed }).log("error", "engine", "uncaught: x\nat a\nat b");
    expect(readFileSync(file(), "utf8")).toBe("2026-11-09T12:00:00.000Z [engine] error: uncaught: x\n    at a\n    at b\n");
  });

  it("moves a big log aside to .1 and starts again", () => {
    const logger = createFileLogger(file(), { maxBytes: 100, now: fixed });
    for (let i = 0; i < 6; i++) logger.log("info", "main", `line ${i} ${"x".repeat(30)}`);
    expect(existsSync(`${file()}.1`)).toBe(true);
    expect(readFileSync(file(), "utf8")).toContain("line 5");
    const older = readFileSync(`${file()}.1`, "utf8");
    expect(older).toContain("line 3"); // the log that was full, no older than that one
    expect(older).not.toContain("line 0");
  });

  it("never throws, even if the folder can't be made", () => {
    writeFileSync(join(dir, "logs"), "a file where the folder should be");
    expect(() => createFileLogger(file()).log("info", "main", "x")).not.toThrow();
  });
});

describe("the environment's choices for the engine", () => {
  const build = { preset: "i4" };

  it("uses the real add-on and the default sound device unless told otherwise", () => {
    expect(engineInitFromEnv({}, build)).toEqual({ addon: "native", backend: "default", build });
  });

  it("uses the fake only for RB_FAKE_ADDON=1", () => {
    expect(engineInitFromEnv({ RB_FAKE_ADDON: "1" }, build).addon).toBe("fake");
    expect(engineInitFromEnv({ RB_FAKE_ADDON: "true" }, build).addon).toBe("native");
  });

  it("takes the null backend and its sample rate from RB_BACKEND and RB_SAMPLE_RATE", () => {
    expect(engineInitFromEnv({ RB_BACKEND: "null", RB_SAMPLE_RATE: "44100" }, build)).toMatchObject({ backend: "null", sampleRate: 44100 });
    expect(engineInitFromEnv({ RB_BACKEND: "null", RB_SAMPLE_RATE: "96000" }, build).sampleRate).toBeUndefined();
    expect(engineInitFromEnv({ RB_SAMPLE_RATE: "44100" }, build).sampleRate).toBeUndefined();
  });

  it("takes a device index only on the default backend, and only a whole number", () => {
    expect(engineInitFromEnv({ RB_DEVICE: "2" }, build).device).toBe(2);
    expect(engineInitFromEnv({ RB_DEVICE: "x" }, build).device).toBeUndefined();
    expect(engineInitFromEnv({ RB_DEVICE: "" }, build).device).toBeUndefined();
    expect(engineInitFromEnv({ RB_BACKEND: "null", RB_DEVICE: "2" }, build).device).toBeUndefined();
  });
});
