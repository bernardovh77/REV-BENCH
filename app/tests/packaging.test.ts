/*
 * The Windows packages' configuration (electron-builder.config.cjs), checked against the rest of
 * the app: where the add-on goes is where the engine process looks, the fake add-on and the
 * source maps stay out, the fuses are the safe ones, the packages are unsigned and x64.
 * tools/package_app.py checks the same things in a real package.
 */
import { readFileSync } from "node:fs";
import { createRequire } from "node:module";
import { join, resolve } from "node:path";
import { describe, expect, it } from "vitest";
import { ADDON_FILE, nativeAddonCandidates } from "../src/engine/addon-loader";

interface Config {
  appId: string;
  productName: string;
  asar: boolean;
  files: string[];
  extraResources: { from: string; to: string }[];
  win: { icon: string; target: { target: string; arch: string[] }[] };
  nsis: { oneClick: boolean; perMachine: boolean };
  electronFuses: Record<string, boolean>;
  publish: unknown;
  directories: { buildResources: string };
  npmRebuild: boolean;
}

function load(env: Record<string, string | undefined> = {}): Config {
  const path = resolve("electron-builder.config.cjs");
  const saved = process.env.RB_NATIVE_PATH;
  if (env.RB_NATIVE_PATH === undefined) delete process.env.RB_NATIVE_PATH;
  else process.env.RB_NATIVE_PATH = env.RB_NATIVE_PATH;
  try {
    const require = createRequire(import.meta.url);
    delete require.cache[path];
    return require(path) as Config;
  } finally {
    if (saved === undefined) delete process.env.RB_NATIVE_PATH;
    else process.env.RB_NATIVE_PATH = saved;
  }
}

describe("the package's identity", () => {
  const config = load();

  it("is Rev-Bench, with the bundle id the design decided", () => {
    expect(config.productName).toBe("Rev-Bench");
    expect(config.appId).toBe("io.github.bernardovh77.revbench");
  });

  it("makes an NSIS installer and a zip, x64 only", () => {
    expect(config.win.target.map((t) => t.target).sort()).toEqual(["nsis", "zip"]);
    for (const t of config.win.target) expect(t.arch).toEqual(["x64"]);
  });

  it("installs per user, with the choice of folder", () => {
    expect(config.nsis.oneClick).toBe(false);
    expect(config.nsis.perMachine).toBe(false);
  });

  it("is unsigned and doesn't publish or update itself", () => {
    expect(config.publish).toBeNull();
    expect(JSON.stringify(config)).not.toMatch(/certificate|signtool|sign(ing)?Hash/i);
  });

  it("uses the icon that is in branding", () => {
    expect(config.win.icon).toBe("icon.ico");
    const ico = readFileSync(join(config.directories.buildResources, config.win.icon));
    expect([...ico.subarray(0, 4)]).toEqual([0, 0, 1, 0]); // an ICO file
    expect(ico.readUInt16LE(4)).toBeGreaterThanOrEqual(6); // with several sizes
  });
});

describe("what is in the package", () => {
  const config = load();

  it("packs the built files into the asar, without source maps or the fake add-on", () => {
    expect(config.asar).toBe(true);
    expect(config.files).toContain("dist/**/*");
    expect(config.files).toContain("!dist/**/*.map");
    expect(config.files).toContain("!dist/engine/fake-addon.cjs");
  });

  it("brings no node_modules: nothing is needed at run time", () => {
    expect(config.files.some((f) => f.includes("node_modules"))).toBe(false);
  });

  it("puts the add-on in resources, beside the asar, under the name the engine process looks for", () => {
    const addon = config.extraResources.find((r) => r.to === ADDON_FILE);
    expect(addon, "an extraResource named rb_native.node").toBeDefined();
    // the engine process finds it as <resources>/rb_native.node (its second candidate)
    const found = nativeAddonCandidates({ env: {}, dirname: "/x/dist/engine", resourcesPath: "/res" });
    expect(found[0].replaceAll("\\", "/")).toBe(`/res/${ADDON_FILE}`);
    expect(addon!.to).toBe(ADDON_FILE);
  });

  it("takes the add-on from RB_NATIVE_PATH when it is set (the package tool's build), else the development build's", () => {
    const dev = load().extraResources.find((r) => r.to === ADDON_FILE)!;
    expect(dev.from.replaceAll("\\", "/")).toBe("native/build/rb_native.node");
    const built = load({ RB_NATIVE_PATH: "/out/addon-build/rb_native.node" }).extraResources.find((r) => r.to === ADDON_FILE)!;
    expect(built.from).toBe("/out/addon-build/rb_native.node");
  });

  it("carries the notices and the licences of the fonts and the sound library", () => {
    const to = config.extraResources.map((r) => r.to);
    expect(to).toContain("THIRD_PARTY_NOTICES.md");
    expect(to).toContain("licenses/Inter-OFL.txt");
    expect(to).toContain("licenses/miniaudio-LICENSE.txt");
  });

  it("doesn't rebuild native modules: the add-on is built by CMake, not by electron-builder", () => {
    expect(config.npmRebuild).toBe(false);
  });
});

describe("Electron's fuses in the package", () => {
  const { electronFuses: fuses } = load();

  it("turns off running as plain Node, NODE_OPTIONS and the inspect flags", () => {
    expect(fuses.runAsNode).toBe(false);
    expect(fuses.enableNodeOptionsEnvironmentVariable).toBe(false);
    expect(fuses.enableNodeCliInspectArguments).toBe(false);
  });

  it("loads the app only from its asar, and checks the asar", () => {
    expect(fuses.onlyLoadAppFromAsar).toBe(true);
    expect(fuses.enableEmbeddedAsarIntegrityValidation).toBe(true);
  });

  it("encrypts cookies", () => {
    expect(fuses.enableCookieEncryption).toBe(true);
  });

  it("keeps file:// privileges while the window is loaded from a file (a custom protocol replaces it later)", () => {
    expect(fuses.grantFileProtocolExtraPrivileges).toBe(true);
  });
});

describe("the add-on's location convention", () => {
  it("is one: the development build, the tests' and the package's all say rb_native.node", () => {
    expect(ADDON_FILE).toBe("rb_native.node");
    const dev = nativeAddonCandidates({ env: {}, dirname: join("a", "dist", "engine") }).at(-1)!;
    expect(dev).toBe(join("a", "native", "build", ADDON_FILE));
    const pkg = readFileSync("package.json", "utf8");
    expect(pkg).toContain('"build:native": "python ../tools/build_addon.py"');
  });
});
