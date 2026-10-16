# REV-BENCH — Research notes (stage 0)

## How existing engine simulators make sound

| Project | Approach | Notes |
|---|---|---|
| [ange-yaghi/engine-sim](https://github.com/ange-yaghi/engine-sim) (AngeTheGreat) | Full physics sim of gas through cylinders, valves and exhaust; audio derived from it, then convolution | MIT, C++, Windows-only build. README: "designed specifically to produce engine audio and simulate engine response characteristics", **not** accurate for engineering. Reported sim rate ≈ 80,000 steps/s ([The Drive](https://www.thedrive.com/news/hear-this-absurdly-accurate-homebrew-engine-sim-rev-a-chevy-big-block)). |
| [glouw/ensim4](https://github.com/glouw/ensim4) | Isentropic flow equations + 1D CFD for exhaust pipes, real-time audio | MIT, **C23 + SDL3**. Very readable reference for us. Credits Ange for the impulse convolution idea. |
| [enginesound (Rust)](https://github.com/standardgalactic/enginesound) | Physically informed synthesis: pipe/chamber acoustics with speed of sound and end reflections; intake + exhaust + vibration mix | Includes resonance damping. Based on the paper below. |
| [Browser engine sound generator](https://github.com/SpudzzDev/en) | Same paper, Web Audio + AudioWorklet, later ported to C++ → WASM | WASM port was done to remove glitches. |
| [combustionlab.net](https://combustionlab.net) ([HN thread](https://hn.nuxt.dev/item/48795900)) | Web engine "simulator" | Main criticisms: redline unaffected by anything, impossible compression/boost combos allowed, unrealistic presets, LLM-written "how it works". Lessons: add limits (stage 22), validate numbers (stage 9), and write the docs from the real model. |

Key paper: *Physically informed car engine sound synthesis for virtual and augmented environments*
(Baldan, Lachambre, Delle Monache, Boussard). It models ignition, pistons, intake, resonance,
vibration and backfire.

Good physics explainer: [Bartosz Ciechanowski — Internal Combustion Engine](https://ciechanow.ski/internal-combustion-engine/).

## Browser audio

- **AudioWorklet** runs audio code off the main thread. The old `ScriptProcessorNode` ran on the main
  thread and blocked everything else. ([MDN](https://developer.mozilla.org/docs/Web/API/Web_Audio_API/Using_AudioWorklet))
- **Emscripten Wasm Audio Worklets** let C/C++ run in the worklet with no JS garbage (no GC pauses),
  but they're built on Wasm Workers, which use shared memory and so need COOP/COEP headers.
  ([Emscripten docs](https://emscripten.org/docs/api%5Freference/wasm_audio_worklets.html))
- **Our choice:** a standalone `.wasm` instantiated directly inside our own AudioWorklet processor,
  with no Emscripten runtime. That gives the same performance benefit and needs no special headers.
- Browsers won't start audio before a user gesture, so there needs to be a "Start" button
  ([Red Blob Games notes](https://www.redblobgames.com/x/2147-webaudio-motor/)).

## Hosting (static site, free)

- **Vercel Hobby:** personal/non-commercial use, about 100 GB/month bandwidth. Already used for the portfolio.
- **Cloudflare Pages:** unmetered bandwidth and static requests on the free plan, 25 MiB max per file.
- **GitHub Pages:** also fine.
- No backend needed. If shared memory is ever added, set COOP/COEP via `vercel.json` or `_headers`.

## 3D in the browser (for stages 17–19)

- **Three.js** (npm `three`; 0.186.1, released 2026-09-24, was current when this was written) fits
  a plain TypeScript app. Its `WebGPURenderer` uses WebGPU when the browser has it and falls back to
  WebGL 2 automatically. It needs `await renderer.init()` before the first frame, otherwise the
  canvas can stay black.
- WebGPU covers most browsers in 2026, but Firefox only ships it on some platforms, so the WebGL 2
  fallback matters. Custom shaders should be written in TSL (the Three.js Shading Language), which
  compiles to WGSL for WebGPU and to GLSL for WebGL 2. Plain GLSL `ShaderMaterial` only runs on the
  WebGL 2 path. ([migration notes](https://www.buildmvpfast.com/blog/threejs-webgl-to-webgpu-renderer-migration-2026),
  [WebGPU + Three.js guide](https://www.utsubo.com/blog/webgpu-threejs-migration-guide))
- Picture/sound sync: Web Audio reports how late the speakers are (`AudioContext.outputLatency`,
  `getOutputTimestamp()`), so the 3D view can be delayed by that much to match what's heard.
- Procedural geometry (built from bore, stroke, rod length, layout and crank pin angles) instead of
  imported models: every engine the simulator can run can be drawn, with nothing modelled by hand.

## Bernardo's machine (checked in stage 0)

`gcc`, `make`, `node`, `npm`, `python3` and `git` are available. **Not installed:** `clang`, `cmake`, `emcc`.
Stage 2 installs a WASM toolchain.
