/*
 * meter.ts - the output meter's ballistics (stage 068). Its input is the host's: each snapshot
 * carries the peak and RMS of what the device was given since the one before (both channels,
 * after the swaps' fades, full scale 1), so no block is missed between snapshots. What it shows,
 * in dB below full scale:
 *
 *   peak   rises at once to a louder snapshot's peak and falls at FALL_DB_PER_S (a peak
 *          programme meter's return, IEC 60268-10's type II: 20 dB in 1.7 s, about 12 dB/s;
 *          here a round 20 dB/s, quicker to read on an engine)
 *   hold   the highest peak of the last HOLD_S, then it falls with the peak
 *   rms    the snapshot's RMS, smoothed over RMS_TAU_S (a VU-like averaging)
 *   clip   the peak reached full scale in the last CLIP_HOLD_S
 *
 * Time is the audio's: the host's audio frames over the sample rate, never the window's clock.
 */

export const FLOOR_DB = -60;
export const FALL_DB_PER_S = 20;
export const HOLD_S = 1.5;
export const RMS_TAU_S = 0.3;
export const CLIP_HOLD_S = 2;

/** A level, full scale 1, in dB below full scale; FLOOR_DB at most quiet. */
export function toDb(level: number): number {
  return level > 0 ? Math.max(FLOOR_DB, 20 * Math.log10(level)) : FLOOR_DB;
}

export interface MeterReading {
  peakDb: number;
  holdDb: number;
  rmsDb: number;
  clip: boolean;
}

export class Meter {
  private peakDb = FLOOR_DB;
  private holdDb = FLOOR_DB;
  private holdAt = -Infinity;
  private rmsDb = FLOOR_DB;
  private clipAt = -Infinity;
  private last: number | null = null;

  /** A snapshot's peak and RMS (full scale 1), at `seconds` of audio. */
  update(peak: number, rms: number, seconds: number): MeterReading {
    const dt = this.last === null ? 0 : Math.max(0, seconds - this.last);
    this.last = seconds;
    const p = toDb(peak);
    this.peakDb = Math.max(p, this.peakDb - FALL_DB_PER_S * dt, FLOOR_DB);
    if (p >= this.holdDb || seconds - this.holdAt > HOLD_S) {
      if (p >= this.holdDb) {
        this.holdDb = p;
        this.holdAt = seconds;
      } else {
        this.holdDb = Math.max(this.peakDb, this.holdDb - FALL_DB_PER_S * dt);
      }
    }
    const r = toDb(rms);
    const k = dt > 0 ? 1 - Math.exp(-dt / RMS_TAU_S) : 1;
    this.rmsDb = this.rmsDb + (r - this.rmsDb) * k;
    if (peak >= 1) this.clipAt = seconds;
    return this.reading(seconds);
  }

  reading(seconds = this.last ?? 0): MeterReading {
    return { peakDb: this.peakDb, holdDb: this.holdDb, rmsDb: this.rmsDb, clip: seconds - this.clipAt <= CLIP_HOLD_S };
  }

  /** A new engine or device: quiet again. */
  reset(): void {
    this.peakDb = this.holdDb = this.rmsDb = FLOOR_DB;
    this.holdAt = this.clipAt = -Infinity;
    this.last = null;
  }
}
