<!--
  BottomPanel - the bottom region: the dyno and time charts, empty until their stages.
-->
<script lang="ts">
  import Panel from "../components/Panel.svelte";
</script>

<Panel title="Charts">
  <p class="empty">The dyno and the time charts come with later stages.</p>
</Panel>

<style>
  .empty {
    margin: 0;
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-sm);
  }
</style>
