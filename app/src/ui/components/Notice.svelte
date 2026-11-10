<!--
  Notice - a message the user should see: the engine restarting, or stopped. `tone` picks how
  loud it is; a "danger" one is announced to screen readers at once (role="alert").
-->
<script lang="ts">
  import type { Snippet } from "svelte";

  interface Props {
    tone?: "info" | "warning" | "danger";
    children?: Snippet;
  }

  let { tone = "info", children }: Props = $props();
</script>

<div class="notice {tone}" role={tone === "danger" ? "alert" : "status"}>
  {@render children?.()}
</div>

<style>
  .notice {
    max-width: var(--rb-notice-max-width);
    padding: var(--rb-space-3) var(--rb-space-4);
    border: var(--rb-border-width) solid var(--rb-color-border-strong);
    border-radius: var(--rb-radius-md);
    background: var(--rb-color-surface-raised);
    font-size: var(--rb-font-size-sm);
  }

  .warning {
    border-color: var(--rb-color-warning);
  }

  .danger {
    border-color: var(--rb-color-danger);
  }
</style>
