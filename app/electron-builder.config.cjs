/*
 * electron-builder.config.cjs - the Windows packages (stage 124): an unsigned NSIS installer and
 * a portable zip, x64. tools/package_app.py builds them (in a copy of app/ outside the repo,
 * with the add-on built first); tests/packaging.test.ts checks this file.
 *
 *  - The built files only (dist/), no source maps and never the fake add-on: a package has the
 *    real add-on or none.
 *  - rb_native.node goes beside the asar, in the app's `resources` folder, not inside it: a
 *    native library can't be loaded from an archive, and that is where the engine process looks
 *    second (src/engine/addon-loader.ts: resourcesPath/rb_native.node). `RB_NATIVE_PATH` says
 *    which build to take (tools/package_app.py sets it); by default the development build's.
 *  - Electron's fuses, set in the package: no ELECTRON_RUN_AS_NODE, no NODE_OPTIONS, no
 *    --inspect flags, the app only from its asar, and the asar's integrity checked. The window is
 *    loaded from a file, so the file:// privileges fuse stays on until the window is served from
 *    a custom protocol (a later stage). Playwright's Electron driver needs the inspect flag, so
 *    the end-to-end tests drive the unpackaged app; the packaged one is checked by
 *    tools/package_app.py over the remote-debugging port, which no fuse governs.
 *  - Unsigned (docs/app-design.md section 15: signing waits until the app works), no auto-update.
 */
module.exports = {
  appId: "io.github.bernardovh77.revbench",
  productName: "Rev-Bench",
  copyright: "Copyright (c) 2026 bernardovh77",
  artifactName: "Rev-Bench-${version}-${os}-${arch}.${ext}",
  directories: { output: "release", buildResources: "branding" },
  asar: true,
  // no packages at run time: everything is compiled into dist/
  npmRebuild: false,
  nodeGypRebuild: false,
  buildDependenciesFromSource: false,
  files: ["package.json", "dist/**/*", "!dist/**/*.map", "!dist/engine/fake-addon.cjs"],
  extraResources: [
    { from: process.env.RB_NATIVE_PATH || "native/build/rb_native.node", to: "rb_native.node" },
    { from: "../THIRD_PARTY_NOTICES.md", to: "THIRD_PARTY_NOTICES.md" },
    { from: "node_modules/@fontsource/inter/LICENSE", to: "licenses/Inter-OFL.txt" },
    { from: "../third_party/miniaudio/LICENSE", to: "licenses/miniaudio-LICENSE.txt" },
  ],
  win: {
    icon: "icon.ico",
    target: [
      { target: "nsis", arch: ["x64"] },
      { target: "zip", arch: ["x64"] },
    ],
  },
  nsis: {
    oneClick: false,
    perMachine: false,
    allowToChangeInstallationDirectory: true,
    artifactName: "Rev-Bench-Setup-${version}-${arch}.${ext}",
    shortcutName: "Rev-Bench",
    deleteAppDataOnUninstall: false,
  },
  electronFuses: {
    runAsNode: false,
    enableCookieEncryption: true,
    enableNodeOptionsEnvironmentVariable: false,
    enableNodeCliInspectArguments: false,
    enableEmbeddedAsarIntegrityValidation: true,
    onlyLoadAppFromAsar: true,
    loadBrowserProcessSpecificV8Snapshot: false,
    grantFileProtocolExtraPrivileges: true,
  },
  publish: null,
};
