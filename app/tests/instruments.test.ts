/*
 * Stage 068's window side without a window: the output meter's ballistics, the model's own
 * warning, and the dyno's runs (collected from the core's readings, overlaid, exported).
 */
import { readFileSync } from "node:fs";
import { describe, expect, it } from "vitest";
import { DynoRuns, RUNS_KEPT, type DynoSample } from "../src/ui/lib/dyno.svelte";
import { GUARD_RATE_WARN, GuardWatch, QUIET_S } from "../src/ui/lib/guard-watch";
import { CLIP_HOLD_S, FLOOR_DB, HOLD_S, Meter, toDb } from "../src/ui/lib/meter";

describe("the meter's ballistics", () => {
  it("reads dB below full scale, with a floor", () => {
    expect(toDb(1)).toBe(0);
    expect(toDb(0.5)).toBeCloseTo(-6.02, 2);
    expect(toDb(0)).toBe(FLOOR_DB);
    expect(toDb(1e-9)).toBe(FLOOR_DB);
  });

  it("rises at once, falls at its rate, and holds its peak for HOLD_S before falling", () => {
    const m = new Meter();
    expect(m.update(0.5, 0.35, 0).peakDb).toBeCloseTo(-6.02, 2);
    // quiet afterwards: the peak falls 20 dB a second, the hold stays HOLD_S
    let r = m.update(0.001, 0.001, 0.5);
    expect(r.peakDb).toBeCloseTo(-16.02, 2);   // 20 dB a second, the ballistics defined
    expect(r.holdDb).toBeCloseTo(-6.02, 2);
    r = m.update(0.001, 0.001, HOLD_S + 0.01);
    expect(r.holdDb).toBeLessThan(-6.02);
    // a louder one lifts both at once
    r = m.update(0.9, 0.6, 2.0);
    expect(r.peakDb).toBeCloseTo(toDb(0.9), 6);
    expect(r.holdDb).toBeCloseTo(toDb(0.9), 6);
  });

  it("smooths the RMS, lights the clip for CLIP_HOLD_S, and resets", () => {
    const m = new Meter();
    m.update(0, 0, 0);
    const first = m.update(0.5, 0.5, 0.1).rmsDb;
    expect(first).toBeGreaterThan(FLOOR_DB);
    expect(first).toBeLessThan(toDb(0.5));   // on its way, not there at once
    for (let t = 0.2; t < 3; t += 0.1) m.update(0.5, 0.5, t);
    expect(m.reading().rmsDb).toBeCloseTo(toDb(0.5), 1);
    expect(m.update(1, 0.7, 3).clip).toBe(true);
    expect(m.update(0.1, 0.07, 3 + CLIP_HOLD_S - 0.1).clip).toBe(true);
    expect(m.update(0.1, 0.07, 3 + CLIP_HOLD_S + 0.1).clip).toBe(false);
    m.reset();
    expect(m.reading()).toMatchObject({ peakDb: FLOOR_DB, holdDb: FLOOR_DB, rmsDb: FLOOR_DB, clip: false });
  });
});

describe("the model's own warning", () => {
  it("is rb.h's threshold", () => {
    const header = readFileSync(new URL("../../core/include/rb.h", import.meta.url), "utf8");
    expect(Number(/#define RB_GUARD_RATE_WARN ([\d.]+)/.exec(header)![1])).toBe(GUARD_RATE_WARN);
  });

  it("warns when a second's average is over it, not for a lift-off's burst, and stays QUIET_S after", () => {
    const w = new GuardWatch();
    // a lift-off: one cycle at 2200 a second among quiet ones, at 60 snapshots a second
    let warned = false;
    for (let i = 0; i < 120; i++) warned = w.update(i === 30 ? 2200 : 0, i / 60) || warned;
    expect(warned).toBe(false);
    // choked headers: 5000 a second, steady
    for (let i = 120; i < 240; i++) warned = w.update(5000, i / 60);
    expect(warned).toBe(true);
    expect(w.average()).toBeCloseTo(5000, 6);
    // back to normal: still said for QUIET_S after the last second over (ending at 239/60 s), then not
    const last = 239 / 60;
    expect(w.update(0, last + QUIET_S - 0.5)).toBe(true);
    expect(w.update(0, last + QUIET_S + 0.5)).toBe(false);
    w.reset();
    expect(w.average()).toBe(0);
  });
});

const at = (s: Partial<DynoSample>): DynoSample => ({ attached: 1, readings: 0, rpm: 3000, torque: 150, power: 47000, seconds: 0, ...s });

describe("the dyno's runs", () => {
  it("takes a point each time the core reads one, at the audio's time, and only then", () => {
    const d = new DynoRuns();
    d.expect("Held at 3000 rpm");
    d.update(at({ readings: 0, seconds: 1 }));
    expect(d.runs).toEqual([]);
    d.update(at({ readings: 1, seconds: 1.04, torque: 151 }));
    d.update(at({ readings: 1, seconds: 1.05 }));   // the same reading again: nothing new
    d.update(at({ readings: 3, seconds: 1.1, rpm: 3010, torque: 152 }));
    expect(d.current!.label).toBe("Held at 3000 rpm");
    expect(d.current!.points).toEqual([
      { reading: 1, t: 0, rpm: 3000, torque: 151, power: 47000 },
      { reading: 3, t: expect.closeTo(0.06, 9), rpm: 3010, torque: 152, power: 47000 },
    ]);
  });

  it("ends a run when the dyno lets go or its readings start again, and keeps the newest RUNS_KEPT", () => {
    const d = new DynoRuns();
    for (let run = 0; run < RUNS_KEPT + 2; run++) {
      d.expect(`Sweep ${run}`);
      for (let k = 1; k <= 3; k++) d.update(at({ readings: k, seconds: run * 10 + k }));
      d.update(at({ attached: 0 }));
    }
    expect(d.runs).toHaveLength(RUNS_KEPT);
    expect(d.runs.map((r) => r.label)).toEqual(Array.from({ length: RUNS_KEPT }, (_, i) => `Sweep ${i + 2}`));
    expect(d.current).toBeNull();
    // a clear's count going down starts a new run, the old one kept
    d.update(at({ readings: 5, seconds: 100 }));
    d.update(at({ readings: 1, seconds: 101 }));
    expect(d.runs.at(-1)!.points).toHaveLength(1);
    expect(d.runs.at(-2)!.points.map((p) => p.reading)).toEqual([5]);
  });

  it("drops a run that read nothing, clears all, and writes CSV in SI units", () => {
    const d = new DynoRuns();
    d.expect("Empty");
    d.update(at({ attached: 0 }));
    expect(d.runs).toEqual([]);
    d.expect('A "quoted" run');
    d.update(at({ readings: 1, seconds: 2, rpm: 2999.5, torque: 140.25, power: 44051.2 }));
    expect(d.csv()).toBe('run,label,reading,time_s,rpm,torque_Nm,power_W\n1,"A ""quoted"" run",1,0.0000,2999.50,140.250,44051.2\n');
    d.clear();
    expect(d.runs).toEqual([]);
    expect(d.csv()).toBe("run,label,reading,time_s,rpm,torque_Nm,power_W\n");
  });
});
