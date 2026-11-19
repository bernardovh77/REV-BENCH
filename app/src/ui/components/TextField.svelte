<!--
  TextField - the base text entry (the build saver's name). `value` is the parent's, kept as
  the user types (oninput, each keystroke); Enter fires onenter. `label` names it for screen
  readers and is shown as its placeholder.
-->
<script lang="ts">
  interface Props {
    label: string;
    value: string;
    maxlength?: number;
    disabled?: boolean;
    oninput?: (value: string) => void;
    onenter?: () => void;
  }

  let { label, value, maxlength, disabled = false, oninput, onenter }: Props = $props();
</script>

<input
  class="text-field"
  type="text"
  aria-label={label}
  placeholder={label}
  {value}
  {maxlength}
  {disabled}
  oninput={(event) => oninput?.(event.currentTarget.value)}
  onkeydown={(event) => event.key === "Enter" && onenter?.()}
/>

<style>
  .text-field {
    flex: 1;
    min-width: 0;
    height: var(--rb-control-height);
    padding: 0 var(--rb-space-3);
    border: var(--rb-border-width) solid var(--rb-color-border-strong);
    border-radius: var(--rb-radius-md);
    background: var(--rb-color-surface-raised);
    color: var(--rb-color-text);
    font-size: var(--rb-font-size-sm);
  }

  .text-field:disabled {
    color: var(--rb-color-text-disabled);
  }
</style>
