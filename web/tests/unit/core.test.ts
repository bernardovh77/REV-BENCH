/**
 * Unit tests for the WebAssembly build of the core, run in Node by Vitest.
 * They exercise the same wrapper (rb-core.ts) the AudioWorklet uses.
 */
import { createHash } from 'node:crypto';
import { readdirSync, readFileSync } from 'node:fs';
import { describe, expect, it } from 'vitest';
import { RB_CLAMPED, RB_ERR_VALUE, RB_OK, RbCore, REQUIRED_EXPORTS, type RbSim } from '../../src/core/rb-core';

const wasm = new Uint8Array(readFileSync(new URL('../../src/wasm/rb_core.wasm', import.meta.url)));
const FS = 48000;

function setup(hz: number, level: number) {
  const core = RbCore.loadSync(wasm);
  const { params } = core.info();
  const id = (name: string): number => {
    const param = params.find((p) => p.name === name);
    if (!param) throw new Error(`no parameter ${name}`);
    return param.id;
  };
  const sim = core.createSim(FS, 4);
  sim.setParam(id('test_tone_hz'), hz);
  sim.setParam(id('test_tone_level'), level);
  return { core, sim, id };
}

/** Renders whole 128-frame blocks, like the AudioWorklet does. */
function renderSeconds(sim: RbSim, seconds: number): Float32Array {
  const out = new Float32Array(Math.round((seconds * FS) / 128) * 128);
  const block = new Float32Array(128);
  for (let i = 0; i < out.length; i += 128) {
    sim.render(block);
    out.set(block, i);
  }
  return out;
}

describe('rb_core.wasm', () => {
  it('was rebuilt after the last change to the C sources (if not: make wasm)', () => {
    // Same file list and order as the Makefile: sorted core/include/*.h, core/src/*.{c,h}.
    const root = new URL('../../../', import.meta.url);
    const list = (dir: string, exts: string[]) =>
      readdirSync(new URL(dir, root))
        .filter((name) => exts.some((ext) => name.endsWith(ext)))
        .map((name) => dir + name);
    const files = [...list('core/include/', ['.h']), ...list('core/src/', ['.c', '.h'])].sort();
    const hash = createHash('sha256');
    for (const file of files) hash.update(readFileSync(new URL(file, root)));
    const recorded = readFileSync(new URL('../../src/wasm/rb_core.sources.sha256', import.meta.url), 'utf8');
    expect(hash.digest('hex')).toBe(recorded.trim());
  });

  it('needs no imports and exports everything the wrapper uses', () => {
    const module = new WebAssembly.Module(wasm);
    expect(WebAssembly.Module.imports(module)).toEqual([]);
    const names = WebAssembly.Module.exports(module).map((e) => e.name);
    for (const name of REQUIRED_EXPORTS) expect(names).toContain(name);
  });

  it('reports its version, parameters and telemetry channels', async () => {
    const info = (await RbCore.load(wasm)).info();
    expect(info.version).toMatch(/^\d+\.\d+\.\d+$/);
    expect(info.params.find((p) => p.name === 'test_tone_hz')).toMatchObject({
      unit: 'Hz', min: 20, max: 20000, def: 220,
    });
    expect(info.telemetry).toEqual(
      expect.arrayContaining(['time_s', 'steps', 'out_peak', 'out_rms', 'nonfinite']),
    );
  });

  it('renders a 440 Hz tone at full level', () => {
    const { core, sim } = setup(440, 1);
    const warmup = renderSeconds(sim, 0.25);  // let the fade-in settle (94 whole blocks)
    const x = renderSeconds(sim, 1);          // 375 blocks = exactly 1 s
    let crossings = 0;
    let peak = 0;
    for (let i = 1; i < x.length; i++) if ((x[i - 1] as number) < 0 && (x[i] as number) >= 0) crossings++;
    for (const v of x) peak = Math.max(peak, Math.abs(v));
    expect(crossings).toBeGreaterThanOrEqual(439);
    expect(crossings).toBeLessThanOrEqual(441);
    expect(peak).toBeGreaterThan(0.998);
    expect(peak).toBeLessThanOrEqual(1);
    const timeId = core.telemetryNames().indexOf('time_s');
    // Simulated time = frames rendered / sample rate, exactly.
    expect(sim.telemetry(timeId)).toBeCloseTo((warmup.length + x.length) / FS, 9);
  });

  it('clamps out-of-range values and rejects NaN', () => {
    const { sim, id } = setup(440, 0.5);
    expect(sim.setParam(id('test_tone_hz'), 1e9)).toBe(RB_CLAMPED);
    expect(sim.getParam(id('test_tone_hz'))).toBe(20000);
    expect(sim.setParam(id('test_tone_hz'), Number.NaN)).toBe(RB_ERR_VALUE);
    expect(sim.getParam(id('test_tone_hz'))).toBe(20000);
    expect(sim.setParam(id('test_tone_level'), 0.3)).toBe(RB_OK);
  });

  it('renders much faster than real time', () => {
    const { sim } = setup(440, 0.5);
    const t0 = performance.now();
    renderSeconds(sim, 10);
    const speed = 10 / ((performance.now() - t0) / 1000);
    console.log(`wasm core renders ${speed.toFixed(0)}x faster than real time`);
    expect(speed).toBeGreaterThan(10);
  });

  it('frees a simulation, then renders silence, and can create another', () => {
    const { core, sim } = setup(440, 0.5);
    renderSeconds(sim, 0.1);
    sim.destroy();
    sim.destroy();                            // twice is harmless
    const block = new Float32Array(128).fill(1);
    sim.render(block);
    expect(block.every((v) => v === 0)).toBe(true);
    const again = core.createSim(44100, 2);
    expect(again.getParam(0)).toBeGreaterThan(0);
    again.destroy();
  });
});
