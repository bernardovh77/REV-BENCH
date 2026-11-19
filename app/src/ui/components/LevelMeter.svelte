<!--
  LevelMeter - the output meter (stage 068): a bar from FLOOR_DB to 0 dB below full scale, the
  RMS as the fill, the peak as a line, the held peak as a tick, and CLIP lit while the output
  reached full scale lately. Its ballistics are lib/meter.ts's; this only draws a reading. SVG,
  so its sizes are attributes (the page's policy forbids inline styles).
-->
<script lang="ts">
  import { FLOOR_DB, type MeterReading } from "../lib/meter";

  interface Props {
    label: string;
    reading: MeterReading;
  }

  let { label, reading }: Props = $props();

  const W = 200;
  const H = 12;
  const x = (db: number) => (W * (Math.max(FLOOR_DB, Math.min(0, db)) - FLOOR_DB)) / -FLOOR_DB;
  let text = $derived(`${label}: peak ${reading.peakDb.toFixed(1)} dB, RMS ${reading.rmsDb.toFixed(1)} dB below full scale${reading.clip ? ", clipping" : ""}`);
</script>

<div class="level-meter" role="meter" aria-label={label} aria-valuemin={FLOOR_DB} aria-valuemax={0} aria-valuenow={reading.peakDb} aria-valuetext={text} title={text}>
  <span class="label">{label}</span>
  <span class="line">
    <svg class="bar" viewBox={`0 0 ${W} ${H}`} preserveAspectRatio="none">
      <rect class="track" x="0" y="0" width={W} height={H} />
      <rect class="rms" x="0" y="0" width={x(reading.rmsDb)} height={H} />
      <rect class="peak" x={Math.max(0, x(reading.peakDb) - 1)} y="0" width="2" height={H} />
      <rect class="hold" x={Math.max(0, x(reading.holdDb) - 1)} y="0" width="2" height={H} />
      <!-- -6 and -18 dB marks -->
      <rect class="mark" x={x(-6)} y="0" width="1" height={H} />
      <rect class="mark" x={x(-18)} y="0" width="1" height={H} />
    </svg>
    <span class="value">{reading.peakDb <= FLOOR_DB ? "–" : reading.peakDb.toFixed(1)}</span>
    <span class="unit">dBFS</span>
    <span class="clip" class:on={reading.clip}>CLIP</span>
  </span>
</div>

<style>
  .level-meter {
    display: flex;
    flex-direction: column;
    gap: var(--rb-space-1);
  }

  .label {
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-xs);
  }

  .line {
    display: inline-flex;
    align-items: center;
    gap: var(--rb-space-2);
  }

  .bar {
    width: var(--rb-meter-width);
    height: var(--rb-meter-height);
  }

  .track {
    fill: var(--rb-color-surface-raised);
  }

  .rms {
    fill: var(--rb-color-success);
  }

  .peak {
    fill: var(--rb-color-accent);
  }

  .hold {
    fill: var(--rb-color-text);
  }

  .mark {
    fill: var(--rb-color-border-strong);
  }

  .value {
    font-size: var(--rb-font-size-md);
    font-weight: var(--rb-weight-semibold);
    font-variant-numeric: tabular-nums;
  }

  .unit {
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-xs);
  }

  .clip {
    padding: 0 var(--rb-space-2);
    border-radius: var(--rb-radius-sm);
    color: var(--rb-color-text-disabled);
    font-size: var(--rb-font-size-xs);
    font-weight: var(--rb-weight-semibold);
  }

  .clip.on {
    background: var(--rb-color-danger);
    color: var(--rb-color-accent-text);
  }
</style>
