<!--
  Button - the base button. `pressed` makes it a toggle (aria-pressed); `variant` picks its tone.
  Other attributes (onclick, onpointerdown, disabled, title, ...) go to the <button>.
-->
<script lang="ts">
  import type { Snippet } from "svelte";
  import type { HTMLButtonAttributes } from "svelte/elements";

  interface Props extends HTMLButtonAttributes {
    variant?: "default" | "primary" | "danger";
    pressed?: boolean | undefined;
    children?: Snippet;
  }

  let { variant = "default", pressed = undefined, children, ...rest }: Props = $props();
</script>

<button type="button" class="button {variant}" aria-pressed={pressed} {...rest}>
  {@render children?.()}
</button>

<style>
  .button {
    height: var(--rb-control-height);
    padding: 0 var(--rb-space-4);
    border: var(--rb-border-width) solid var(--rb-color-border-strong);
    border-radius: var(--rb-radius-md);
    background: var(--rb-color-surface-raised);
    color: var(--rb-color-text);
    font-size: var(--rb-font-size-md);
    font-weight: var(--rb-weight-medium);
    cursor: pointer;
    transition: background var(--rb-duration-fast);
  }

  .button:hover:not(:disabled) {
    border-color: var(--rb-color-text-muted);
  }

  .button[aria-pressed="true"],
  .primary {
    background: var(--rb-color-accent);
    border-color: var(--rb-color-accent);
    color: var(--rb-color-accent-text);
  }

  .danger {
    border-color: var(--rb-color-danger);
    color: var(--rb-color-danger);
  }

  .button:disabled {
    color: var(--rb-color-text-disabled);
    border-color: var(--rb-color-border);
    cursor: not-allowed;
  }
</style>
