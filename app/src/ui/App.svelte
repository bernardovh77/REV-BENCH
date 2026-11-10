<!--
  App - the window: the layout, the four regions, the theme. The engine store is made in main.ts
  and given here, so tests can give a store with fake dependencies.
-->
<script lang="ts">
  import AppLayout from "./layout/AppLayout.svelte";
  import type { EngineStore } from "./lib/engine-store.svelte";
  import BottomPanel from "./panels/BottomPanel.svelte";
  import CentrePanel from "./panels/CentrePanel.svelte";
  import EditorPanel from "./panels/EditorPanel.svelte";
  import TopBar from "./panels/TopBar.svelte";

  interface Props {
    store: EngineStore;
  }

  let { store }: Props = $props();

  // Dark is the default. The choice is not kept yet: settings come with their own stage.
  let theme = $state<"dark" | "light">("dark");

  $effect(() => {
    document.documentElement.dataset.theme = theme;
  });
</script>

<AppLayout>
  {#snippet top()}
    <TopBar {store} {theme} ontoggletheme={() => (theme = theme === "dark" ? "light" : "dark")} />
  {/snippet}
  {#snippet left()}
    <EditorPanel />
  {/snippet}
  {#snippet centre()}
    <CentrePanel {store} />
  {/snippet}
  {#snippet bottom()}
    <BottomPanel />
  {/snippet}
</AppLayout>
