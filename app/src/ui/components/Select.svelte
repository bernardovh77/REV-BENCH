<!--
  Select - the base drop-down. `value` is controlled: the parent changes it in response to
  onchange (so the displayed choice is always what the engine confirms).
-->
<script lang="ts">
  export interface SelectOption {
    value: string;
    label: string;
  }

  interface Props {
    label: string;
    options: SelectOption[];
    value: string;
    disabled?: boolean;
    onchange?: (value: string) => void;
  }

  let { label, options, value, disabled = false, onchange }: Props = $props();
</script>

<label class="select">
  <span class="label">{label}</span>
  <select {value} {disabled} onchange={(event) => onchange?.(event.currentTarget.value)}>
    {#each options as option (option.value)}
      <option value={option.value} selected={option.value === value}>{option.label}</option>
    {/each}
  </select>
</label>

<style>
  .select {
    display: inline-flex;
    flex-direction: column;
    gap: var(--rb-space-1);
  }

  .label {
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-xs);
  }

  select {
    height: var(--rb-control-height);
    min-width: var(--rb-select-width);
    padding: 0 var(--rb-space-3);
    border: var(--rb-border-width) solid var(--rb-color-border-strong);
    border-radius: var(--rb-radius-md);
    background: var(--rb-color-surface-raised);
    font-size: var(--rb-font-size-md);
  }

  select:disabled {
    color: var(--rb-color-text-disabled);
  }
</style>
