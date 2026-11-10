<!--
  CentrePanel - the middle region: the instruments now, the 3D view later. It shows the engine
  process's messages (restarting, stopped) because that is what the middle of the window is for
  when there is no engine to show.
-->
<script lang="ts">
  import Button from "../components/Button.svelte";
  import Notice from "../components/Notice.svelte";
  import Panel from "../components/Panel.svelte";
  import type { EngineStore } from "../lib/engine-store.svelte";

  interface Props {
    store: EngineStore;
  }

  let { store }: Props = $props();
</script>

<Panel title="Engine">
  <div class="stack">
    {#if store.summary === "no-shell"}
      <Notice tone="warning">This page is running without the Rev-Bench application, so there is no engine to connect to.</Notice>
    {:else if store.summary === "restarting"}
      <Notice tone="warning">{store.status?.message ?? "The engine stopped and is restarting."}</Notice>
    {:else if store.summary === "failed"}
      <Notice tone="danger">{store.status?.message ?? "The engine stopped."}</Notice>
      <div>
        <Button variant="primary" onclick={() => store.restartEngine()}>Restart the engine</Button>
      </div>
    {:else}
      <p class="empty">The instruments and the 3D view come with later stages.</p>
    {/if}
    {#if store.lastError}
      <Notice tone="warning">{store.lastError}</Notice>
    {/if}
  </div>
</Panel>

<style>
  .stack {
    display: flex;
    flex-direction: column;
    gap: var(--rb-space-4);
  }

  .empty {
    margin: 0;
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-sm);
  }
</style>
