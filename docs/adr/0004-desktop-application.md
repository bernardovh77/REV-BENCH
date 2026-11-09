---
status: accepted
---

# A desktop application, not a browser page

On 2026-11-08 Bernardo chose a downloadable app for Windows, Linux and macOS
instead of the browser bench planned since 2026-10-23, and asked for the
interface to be chosen for what is best for the project, its look and its craft.

**Context.** The browser plan ran the core as WebAssembly in an AudioWorklet: one
thread with a hard deadline every 2.7 ms, no link-time optimisation of the host,
a narrower instruction set, and the browser's autoplay and lifecycle rules. The
core already misses real time natively (the four: 0.66x on Bernardo's machine,
engine step only). The 3D view, editor, instruments, dyno, turbo and catalog make
the app large, and the sound is the product: its delivery has to be reliable.

**Options considered.**
- **Dear ImGui** (one native C/C++ program). Fast and simple, but always looks
  like a developer tool, and the 3D renderer would be written from scratch.
- **Tauri** (a Rust shell and the OS's own web view). Small, but macOS's and
  Linux's web engines lag on WebGPU and WebGL, so the 3D view could break or
  differ by system, and it adds Rust.
- **Qt.** Native widgets and Qt Quick 3D, but large, C++-heavy and LGPL-bound.
- **Electron with a native core.** Its own Chromium on every system, the web's
  tools for typography, layout and animation, Three.js as planned, mature
  packaging and updates. It costs about 150 MB per installer and two languages.

**Decision.** Electron, with the C core and a new C real-time host in a Node-API
add-on, running in an Electron utility process. The sound is rendered on its own
high-priority C thread into a ring that miniaudio plays, never in JavaScript. The
interface (Svelte 5, TypeScript, Three.js) reads the engine through snapshots and
sends it commands. The full design is in [docs/app-design.md](../app-design.md).

**Consequences.**
- Stages 066–069 become the desktop app's stages and 095–102 its persistence and
  release.
- New stages 120–124 come first: upkeep, the core's public engine API,
  cross-platform builds and CI, the real-time host, and the app's foundation.
- Stage 118 targets the native render thread; WebAssembly no longer gates
  anything, and a browser demo is optional stage 125.
- Contract runtime-v1's ownership, generation and clock rules still hold; its
  Web Audio parts are replaced by the host's device clock, and snapshot-v2 adds
  the binary layout.
- Stage IDs don't change. Finished stages' records keep their browser-era
  wording as dated history.
