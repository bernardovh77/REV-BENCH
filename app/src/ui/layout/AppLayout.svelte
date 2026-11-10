<!--
  AppLayout - the window's four regions (docs/app-design.md section 11): a top bar, an editor on
  the left, the centre and a bottom panel. It only places them; what they hold is theirs.
-->
<script lang="ts">
  import type { Snippet } from "svelte";

  interface Props {
    top: Snippet;
    left: Snippet;
    centre: Snippet;
    bottom: Snippet;
  }

  let { top, left, centre, bottom }: Props = $props();
</script>

<div class="layout">
  <div class="top">{@render top()}</div>
  <aside class="left">{@render left()}</aside>
  <main class="centre">{@render centre()}</main>
  <div class="bottom">{@render bottom()}</div>
</div>

<style>
  .layout {
    display: grid;
    grid-template-columns: var(--rb-editor-width) 1fr;
    grid-template-rows: var(--rb-topbar-height) 1fr var(--rb-bottom-height);
    grid-template-areas:
      "top top"
      "left centre"
      "left bottom";
    gap: var(--rb-space-3);
    height: 100%;
    padding: 0 var(--rb-space-3) var(--rb-space-3);
  }

  .top {
    grid-area: top;
    margin: 0 calc(-1 * var(--rb-space-3));
    min-width: 0;
  }

  .left {
    grid-area: left;
    min-width: 0;
    min-height: 0;
  }

  .centre {
    grid-area: centre;
    min-width: 0;
    min-height: 0;
  }

  .bottom {
    grid-area: bottom;
    min-width: 0;
    min-height: 0;
  }
</style>
