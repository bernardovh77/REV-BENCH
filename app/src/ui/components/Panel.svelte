<!--
  Panel - a titled region of the window: a header and a body.
-->
<script lang="ts">
  import type { Snippet } from "svelte";

  interface Props {
    title: string;
    children?: Snippet;
  }

  let { title, children }: Props = $props();
</script>

<section class="panel" aria-label={title}>
  <h2 class="title">{title}</h2>
  <div class="body">
    {@render children?.()}
  </div>
</section>

<style>
  .panel {
    display: flex;
    flex-direction: column;
    min-width: 0;
    min-height: 0;
    height: 100%;
    background: var(--rb-color-surface);
    border: var(--rb-border-width) solid var(--rb-color-border);
    border-radius: var(--rb-radius-md);
  }

  .title {
    margin: 0;
    padding: var(--rb-space-3) var(--rb-space-4);
    border-bottom: var(--rb-border-width) solid var(--rb-color-border);
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-sm);
    font-weight: var(--rb-weight-semibold);
    letter-spacing: var(--rb-letter-spacing-title);
  }

  .body {
    flex: 1;
    min-height: 0;
    padding: var(--rb-space-4);
    overflow: auto;
  }
</style>
