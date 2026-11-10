<!--
  EditorPanel - the left region: the engine editor (engine, intake, exhaust, fuel and spark),
  empty until its stage.
-->
<script lang="ts">
  import Panel from "../components/Panel.svelte";
</script>

<Panel title="Editor">
  <p class="empty">The engine editor comes with a later stage.</p>
</Panel>

<style>
  .empty {
    margin: 0;
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-sm);
  }
</style>
