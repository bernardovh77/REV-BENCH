<!--
  NumberField - the base number entry (stage 067), controlled like Select: `value` is what the
  parent shows (what the engine confirms, or the draft), and onchange fires when the user commits
  a number (Enter or leaving the field), never on each keystroke. A blank or non-number entry is
  dropped and the shown value comes back. `label` names it for screen readers; `unit` is shown
  after it. `min` and `max` are the range offered, also given in its tooltip.
-->
<script lang="ts">
  interface Props {
    label: string;
    value: number | null;
    unit?: string;
    /** a short caption before the entry: which cylinder or bank it is */
    caption?: string;
    min?: number;
    max?: number;
    step?: number | "any";
    /** decimals shown */
    digits?: number;
    disabled?: boolean;
    onchange?: (value: number) => void;
  }

  let { label, value, unit = "", caption = "", min, max, step = "any", digits = 3, disabled = false, onchange }: Props = $props();

  let shown = $derived(value === null || !Number.isFinite(value) ? "" : String(Number(value.toFixed(digits))));
  let range = $derived(min !== undefined && max !== undefined ? `${label}: ${Number(min.toFixed(digits))} to ${Number(max.toFixed(digits))}${unit ? " " + unit : ""}` : label);

  function commit(input: HTMLInputElement) {
    // a number input's value is always written with a dot, whatever the language
    const v = input.value.trim() === "" ? Number.NaN : Number(input.value);
    if (Number.isFinite(v)) onchange?.(v);
    else input.value = shown;
  }
</script>

<span class="number-field" title={range}>
  {#if caption}<span class="caption" aria-hidden="true">{caption}</span>{/if}
  <input
    type="number"
    aria-label={label}
    value={shown}
    {min}
    {max}
    {step}
    {disabled}
    onchange={(event) => commit(event.currentTarget)}
    onkeydown={(event) => event.key === "Enter" && event.currentTarget.blur()}
  />
  {#if unit}<span class="unit">{unit}</span>{/if}
</span>

<style>
  .number-field {
    display: inline-flex;
    align-items: center;
    gap: var(--rb-space-1);
  }

  input {
    width: var(--rb-number-width);
    height: var(--rb-control-height);
    padding: 0 var(--rb-space-2);
    border: var(--rb-border-width) solid var(--rb-color-border-strong);
    border-radius: var(--rb-radius-md);
    background: var(--rb-color-surface-raised);
    color: var(--rb-color-text);
    font-size: var(--rb-font-size-sm);
  }

  input:disabled {
    color: var(--rb-color-text-disabled);
  }

  .caption {
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-xs);
  }

  .unit {
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-xs);
  }
</style>
