# Third-party notices — Rev-Bench

**Draft** (stage 124). It lists what the application ships; stage 100 completes it (a generated
list of every package in the build, the licences' full texts, and a check that nothing was
missed). Installed copies carry this file and the licence files named below in their
`resources` folder.

## What an installed Rev-Bench contains

| Part | Version | Licence | Where its licence is |
| --- | --- | --- | --- |
| Electron (Chromium, Node.js, V8 and their dependencies) | 45.0.0 | MIT (Electron); Chromium's and Node's own licences | `LICENSE.electron.txt` and `LICENSES.chromium.html` next to `Rev-Bench.exe` (Electron's own files, kept by the package) |
| Inter (typeface), through `@fontsource/inter` | 5.3.0 | SIL Open Font License 1.1 | `resources/licenses/Inter-OFL.txt` |
| miniaudio (sound devices), linked into `rb_native.node` | 0.11.25 | Public domain (Unlicense) or MIT-0, as you choose | `resources/licenses/miniaudio-LICENSE.txt` |
| Svelte (compiled into the window's script) | 5.57.2 | MIT | below |
| The engine (REV-BENCH's C core and real-time host) and the application's own code | this build | the project's own | — |

Nothing else is shipped: the application has no run-time packages from npm besides what is
compiled into its own files (Svelte), and its build tools (Vite, TypeScript, Vitest, Playwright,
electron-builder, cmake-js, the Node-API headers) are not part of an installed copy.

## Notices

### Inter

Copyright 2016 The Inter Project Authors (https://github.com/rsms/inter). This Font Software is
licensed under the SIL Open Font License, Version 1.1. The full text is in
`resources/licenses/Inter-OFL.txt`. The font files are shipped as they were released (the
`latin` subset, weights 400, 500 and 600, in WOFF2 and WOFF).

### miniaudio

miniaudio is by David Reid (https://github.com/mackron/miniaudio), released into the public
domain, or under MIT No Attribution, at your choice. The text of both is in
`resources/licenses/miniaudio-LICENSE.txt`. It is used for the sound-device layer only; the repository
keeps the record of the vendored file in `third_party/miniaudio/NOTICE.md`.

### Electron

Copyright (c) Electron contributors and Copyright (c) 2013-2020 GitHub Inc. Licensed under the
MIT License. Chromium, Node.js and V8 and their dependencies carry their own licences,
collected by the Electron release in `LICENSES.chromium.html`, which is kept in the package.

### Svelte

Copyright (c) 2016-2025 Svelte Contributors (https://github.com/sveltejs/svelte), MIT License:

> Permission is hereby granted, free of charge, to any person obtaining a copy of this software
> and associated documentation files (the "Software"), to deal in the Software without
> restriction, including without limitation the rights to use, copy, modify, merge, publish,
> distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
> Software is furnished to do so, subject to the following conditions: The above copyright
> notice and this permission notice shall be included in all copies or substantial portions of
> the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
> PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
> LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
> OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
> IN THE SOFTWARE.

## Build tools (not shipped), for the record

| Package | Version | Licence |
| --- | --- | --- |
| electron-builder | 26.18.0 | MIT |
| vite | 8.3.5 | MIT |
| @sveltejs/vite-plugin-svelte | 7.3.1 | MIT |
| typescript / @typescript/native | 6.0.3 / 7.0.2 | Apache-2.0 |
| svelte-check | 4.7.7 | MIT |
| vitest, jsdom | 5.0.3, 30.1.3 | MIT |
| @playwright/test | 1.63.0 | Apache-2.0 |
| cmake-js, node-api-headers | 8.0.0, 1.9.0 | MIT (their own dependencies are listed in `app/native/build-tools/package-lock.json`) |
