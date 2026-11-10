<!--
  Slider - the base range control, controlled like Select: `value` is what the parent shows.
-->
<script lang="ts">
  interface Props {
    label: string;
    value: number;
    min?: number;
    max?: number;
    step?: number;
    disabled?: boolean;
    oninput?: (value: number) => void;
  }

  let { label, value, min = 0, max = 1, step = 0.01, disabled = false, oninput }: Props = $props();
</script>

<label class="slider">
  <span class="label">{label}</span>
  <input
    type="range"
    {min}
    {max}
    {step}
    {value}
    {disabled}
    oninput={(event) => oninput?.(event.currentTarget.valueAsNumber)}
  />
</label>

<style>
  .slider {
    display: inline-flex;
    flex-direction: column;
    gap: var(--rb-space-1);
  }

  .label {
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-xs);
  }

  input {
    width: var(--rb-slider-width);
    height: var(--rb-control-height);
    margin: 0;
    accent-color: var(--rb-color-accent);
  }
</style>
