/*
 * dyno.svelte.ts - the dyno's runs in the window (stage 068). The dyno is the core's
 * (rb_dyno_*, through the host): this collects what it reads, from the snapshots, and never
 * computes a torque of its own. A point is taken each time the core's count of readings goes
 * up (its last reading: mean speed, brake torque, brake power over one cycle), at the audio's
 * time; a count that goes down (cleared, attached again) or a dyno that lets go ends the run.
 * Snapshots come about 60 times a second and readings once a cycle, so at high speed a point
 * stands for the newest of a few readings: its `reading` number says which.
 *
 * Runs are kept to overlay (the newest RUNS_KEPT), each with what was asked of it (held at a
 * speed, or swept from one to another at a rate), and written as CSV for export.
 */

export const RUNS_KEPT = 5;

export interface DynoPoint {
  /** the core's reading number, from 1 in the run */
  reading: number;
  /** s of audio since the run started */
  t: number;
  rpm: number;
  /** N m, W: brake torque and power */
  torque: number;
  power: number;
}

export interface DynoRun {
  id: number;
  label: string;
  points: DynoPoint[];
}

/** What the collector needs of a snapshot: the dyno's instruments and the audio's time. */
export interface DynoSample {
  attached: number;
  readings: number;
  rpm: number;
  torque: number;
  power: number;
  /** s of audio */
  seconds: number;
}

export class DynoRuns {
  runs = $state.raw<DynoRun[]>([]);
  /** the run being read now (also the last of `runs`), or null */
  current = $state.raw<DynoRun | null>(null);
  private nextId = 1;
  private lastReadings = 0;
  private startedAt = 0;
  private label = "";

  /** A new run is about to start (the panel asked for a hold or a sweep): its label. */
  expect(label: string): void {
    this.label = label;
    this.finish();
  }

  /** Each snapshot's dyno instruments. */
  update(s: DynoSample): void {
    if (s.attached !== 1) {
      this.finish();
      this.lastReadings = 0;
      return;
    }
    if (s.readings < this.lastReadings) {   // cleared or attached again: a new run
      this.finish();
      this.lastReadings = 0;
    }
    if (s.readings > this.lastReadings && s.readings > 0) {
      if (!this.current) this.begin(s.seconds);
      const run = this.current!;
      const point: DynoPoint = { reading: s.readings, t: s.seconds - this.startedAt, rpm: s.rpm, torque: s.torque, power: s.power };
      const updated: DynoRun = { ...run, points: [...run.points, point] };
      this.current = updated;
      this.runs = this.runs.map((r) => (r.id === run.id ? updated : r));
    }
    this.lastReadings = s.readings;
  }

  private begin(seconds: number): void {
    const run: DynoRun = { id: this.nextId++, label: this.label || `Run ${this.nextId - 1}`, points: [] };
    this.startedAt = seconds;
    this.current = run;
    this.runs = [...this.runs, run].slice(-RUNS_KEPT);
  }

  /** The run being read ends; it stays to overlay (an empty one is dropped). */
  finish(): void {
    if (this.current && this.current.points.length === 0) this.runs = this.runs.filter((r) => r.id !== this.current!.id);
    this.current = null;
  }

  /** Forgets every run. */
  clear(): void {
    this.current = null;
    this.runs = [];
  }

  /** The runs as CSV: one row per point, SI units (rpm, N m, W), the run's label quoted. */
  csv(): string {
    const rows = ["run,label,reading,time_s,rpm,torque_Nm,power_W"];
    for (const r of this.runs)
      for (const p of r.points)
        rows.push([r.id, `"${r.label.replace(/"/g, '""')}"`, p.reading, p.t.toFixed(4), p.rpm.toFixed(2), p.torque.toFixed(3), p.power.toFixed(1)].join(","));
    return rows.join("\n") + "\n";
  }
}
