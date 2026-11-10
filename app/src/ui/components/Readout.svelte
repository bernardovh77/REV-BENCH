<!--
  Readout - a label, a number and its unit. Every number in the interface shows its unit.
  `value` null means "no reading": it shows a dash.
-->
<script lang="ts">
  interface Props {
    label: string;
    value: number | string | null;
    unit?: string;
    /** digits after the point, for a number */
    digits?: number;
  }

  let { label, value, unit = "", digits = 0 }: Props = $props();

  let text = $derived(
    value === null ? "–" : typeof value === "number" ? value.toFixed(digits) : value,
  );
</script>

<div class="readout">
  <span class="label">{label}</span>
  <span class="line">
    <span class="value">{text}</span>
    {#if unit}<span class="unit">{unit}</span>{/if}
  </span>
</div>

<style>
  .readout {
    display: inline-flex;
    flex-direction: column;
    min-width: var(--rb-readout-width);
  }

  .label {
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-xs);
  }

  .line {
    display: inline-flex;
    align-items: baseline;
    gap: var(--rb-space-2);
  }

  .value {
    font-size: var(--rb-font-size-xl);
    font-weight: var(--rb-weight-semibold);
    line-height: var(--rb-line-height-tight);
    font-variant-numeric: tabular-nums;
  }

  .unit {
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-sm);
  }
</style>
