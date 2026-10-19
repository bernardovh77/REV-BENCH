/**
 * Unit tests for the WebAssembly build of the core, run in Node by Vitest.
 * They exercise the same wrapper (rb-core.ts) the AudioWorklet uses.
 */
import { createHash } from 'node:crypto';
import { readdirSync, readFileSync } from 'node:fs';
import { describe, expect, it } from 'vitest';
import {
  RB_CLAMPED, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE, RB_EF_PER_BANK, RB_EF_PER_CYLINDER,
  RB_EF_READONLY, RB_ERR_ARG, RB_ERR_ID, RB_ERR_READONLY, RB_ERR_VALUE, RB_OK, RbCore,
  REQUIRED_EXPORTS, type RbSim,
} from '../../src/core/rb-core';

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
    const block0 = new Float64Array(64);
    {
      const { sim } = setup(440, 0.5);
      sim.destroy();
      expect(sim.readCylinders(block0)).toBe(0);   // a freed sim has no engine to read
      expect(sim.getEngine(0)).toBeNaN();
      expect(sim.loadPreset(0)).toBe(RB_ERR_ARG);
    }
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

describe('the engine through the API (stage 8)', () => {
  const core = RbCore.loadSync(wasm);
  const field = (name: string): number => core.engineFieldId(name);
  const cylinderField = (name: string): number => {
    const id = core.cylinderFields().findIndex((f) => f.name === name);
    if (id < 0) throw new Error(`no cylinder field ${name}`);
    return id;
  };

  it('lists the presets and the engine and cylinder fields, with units', () => {
    const info = core.info();
    expect(info.presets.map((p) => p.key)).toEqual(['i4', 'i6', 'v8-cross', 'v8-flat', 'boxer4']);
    expect(info.presets[2]?.name).toBe('5.0 L cross-plane V8');
    const f = (name: string) => info.engineFields.find((x) => x.name === name);
    expect(f('bore')).toMatchObject({ unit: 'm', flags: 0 });
    expect(f('displacement')).toMatchObject({ unit: 'm^3', flags: RB_EF_READONLY });
    expect(f('bank_angle')?.flags).toBe(RB_EF_PER_BANK);
    expect(f('pin')).toMatchObject({ unit: 'deg', flags: RB_EF_PER_CYLINDER });
    expect(f('firing_order')?.flags).toBe(RB_EF_IN_ORDER | RB_EF_INTEGER);
    expect(f('rev_limit')).toMatchObject({ unit: 'rpm', flags: RB_EF_LIVE });
    expect(new Set(info.engineFields.map((x) => x.name)).size).toBe(info.engineFields.length);
    expect(info.cylinderFields.map((x) => x.name)).toEqual([
      'angle', 'piston', 'pressure', 'temperature', 'burn', 'burned_gas', 'intake_lift', 'exhaust_lift',
    ]);
    expect(info.cylinderFields[2]?.unit).toBe('Pa');
    expect(() => core.engineFieldId('wheelbase')).toThrow(/no engine field/);
  });

  it('starts as the 2.0 L inline-4 and loads the other presets', () => {
    const sim = core.createSim(FS, 4);
    expect(sim.preset).toBe(0);
    let eng = sim.engine();
    expect(eng.cylinders).toBe(4);
    expect(eng.banks).toBe(1);
    expect(eng.bore).toBeCloseTo(0.086, 12);
    expect(eng.displacement as number).toBeCloseTo(1.998e-3, 6);
    expect(eng.firing_order).toEqual([0, 2, 3, 1]);   // 1-3-4-2
    expect(eng.firing_angle).toEqual([0, 540, 180, 360]);
    expect(eng.bank_angle).toEqual([0]);              // one value per bank, even for one bank

    expect(sim.loadPreset(core.presetId('v8-cross'))).toBe(RB_OK);
    expect(sim.preset).toBe(2);
    eng = sim.engine();
    expect(eng.cylinders).toBe(8);
    expect(eng.bank_angle).toEqual([45, -45]);
    expect(eng.bank).toEqual([0, 1, 0, 1, 0, 1, 0, 1]);
    expect((eng.firing_angle as number[]).slice().sort((a, b) => a - b))
      .toEqual([0, 90, 180, 270, 360, 450, 540, 630]);   // a firing every 90 degrees
    expect(sim.loadPreset(99)).toBe(RB_ERR_ID);
    expect(sim.preset).toBe(2);                          // unchanged
    expect(core.presetId('w16')).toBe(-1);
    sim.destroy();
  });

  it('changes fields, and refuses what would make an impossible engine', () => {
    const sim = core.createSim(FS, 4);
    const d0 = sim.getEngine(field('displacement'));
    expect(sim.setEngine(field('bore'), 0, 0.09)).toBe(RB_OK);
    expect(sim.getEngine(field('displacement'))).toBeCloseTo(d0 * (90 / 86) ** 2, 12);
    expect(sim.setEngine(field('displacement'), 0, 1e-3)).toBe(RB_ERR_READONLY);
    expect(sim.setEngine(field('rod'), 0, 0.03)).toBe(RB_ERR_VALUE);   // shorter than the crank throw
    expect(sim.getEngine(field('rod'))).toBeCloseTo(0.143, 12);       // unchanged
    expect(sim.setEngine(field('bore'), 0, Number.NaN)).toBe(RB_ERR_VALUE);
    expect(sim.setEngine(field('intake_valves'), 0, 2.5)).toBe(RB_ERR_VALUE);
    expect(sim.setEngine(field('pin'), 4, 0)).toBe(RB_ERR_ARG);        // cylinders 0..3
    expect(sim.setEngine(999, 0, 0)).toBe(RB_ERR_ID);
    expect(sim.getEngine(field('pin'), 4)).toBeNaN();
    expect(sim.setEngine(field('rev_limit'), 0, 6000)).toBe(RB_OK);
    expect(sim.getEngine(field('rev_limit'))).toBe(6000);
    sim.destroy();
  });

  it('keeps the firing order a list of every cylinder, and the firing angles follow it', () => {
    const sim = core.createSim(FS, 4);
    const order = field('firing_order');
    expect(sim.setEngine(order, 1, 1)).toBe(RB_OK);   // 1-3-4-2 -> 1-2-4-3: 2 and 3 swap places
    let eng = sim.engine();
    expect(eng.firing_order).toEqual([0, 1, 3, 2]);
    expect(eng.firing_angle).toEqual([0, 180, 540, 360]);
    expect(sim.setEngine(order, 0, 1)).toBe(RB_ERR_VALUE);   // cylinder 1 must fire first
    expect(sim.setEngine(order, 1, 3)).toBe(RB_ERR_VALUE);   // 1-4-2-3 doesn't fit this crank
    eng = sim.engine();
    expect(eng.firing_order).toEqual([0, 1, 3, 2]);          // unchanged
    sim.destroy();
  });

  it("reports every cylinder's state (here the engine at rest)", () => {
    const sim = core.createSim(FS, 4);
    const fields = core.cylinderFields().length;
    const into = new Float64Array(16 * fields);
    expect(sim.readCylinders(into)).toBe(4);
    const at = (c: number, name: string) => into[c * fields + cylinderField(name)] as number;
    const firing = sim.engine().firing_angle as number[];
    for (let c = 0; c < 4; c++) {
      expect(at(c, 'angle')).toBe((720 - (firing[c] as number)) % 720);
      expect(at(c, 'pressure')).toBeCloseTo(101325, 6);
      expect(at(c, 'temperature')).toBeCloseTo(298.15, 9);
      expect(at(c, 'burn')).toBe(0);
      expect(at(c, 'burned_gas')).toBe(0);
      expect(at(c, 'piston')).toBe(sim.getCylinder(c, cylinderField('piston')));
    }
    expect(at(0, 'piston')).toBeCloseTo(0, 12);                 // cylinder 1 at its firing TDC ...
    expect(at(0, 'intake_lift') + at(0, 'exhaust_lift')).toBe(0);   // ... with its valves shut
    expect(at(3, 'angle')).toBe(360);                            // cylinder 4 at overlap TDC:
    expect(at(3, 'intake_lift')).toBeGreaterThan(0);             // both valves open
    expect(at(3, 'exhaust_lift')).toBeGreaterThan(0);
    expect(at(1, 'piston')).toBeCloseTo(0.086, 12);              // cylinder 2 at BDC: a stroke down
    expect(sim.getCylinder(4, 0)).toBeNaN();
    expect(sim.readCylinders(new Float64Array(fields * 2))).toBe(2);   // only as many as fit
    sim.destroy();
  });
});
