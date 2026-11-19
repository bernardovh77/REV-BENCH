<!--
  App - the window: the layout, the four regions, the theme, and the driver's seat (stage 066:
  the keys, the wheel and a controller, driver.svelte.ts). The engine store is made in main.ts
  and given here, so tests can give a store with fake dependencies; the driver listens to this
  window and document, and runs on the store's animation frames.
-->
<script lang="ts">
  import { untrack } from "svelte";
  import AppLayout from "./layout/AppLayout.svelte";
  import { SavedBuilds } from "./lib/builds.svelte";
  import { Driver } from "./lib/driver.svelte";
  import { Editor } from "./lib/editor.svelte";
  import type { EngineStore } from "./lib/engine-store.svelte";
  import type { PadLike } from "./lib/gamepad";
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

  const driver = new Driver({
    store: {
      sendThrottle: (value, live) => store.sendThrottle(value, live),
      setParam: (name, value) => store.setParam(name, value),
      ignition: () => store.snapshot?.instruments.ignition ?? null,
    },
    window,
    document,
    // jsdom (the tests) and some systems have no Gamepad API
    getPads: () => (typeof navigator.getGamepads === "function" ? (navigator.getGamepads() as readonly (PadLike | null)[]) : []),
    now: () => performance.now(),
  });

  // the engine editor's draft, history and units (stage 067)
  const editor = new Editor({
    get engineFields() {
      return store.engineFields;
    },
    get engineValues() {
      return store.engineValues;
    },
    get engineEdits() {
      return store.engineEdits;
    },
    get preset() {
      return store.preset;
    },
    get layouts() {
      return store.layouts;
    },
    get layout() {
      return store.layout;
    },
    get presetLayout() {
      return store.presetLayout;
    },
    applyEdits: (edits, layout) => store.applyEdits(edits, layout),
    editLive: (edit) => store.editLive(edit),
    describeEdits: (edits, layout) => store.describeEdits(edits, layout),
  });

  // the build saver: through the app's bridge (none in a plain browser page)
  const builds = new SavedBuilds(window.revbench?.builds ?? null, {
    get preset() {
      return store.preset;
    },
    get layout() {
      return store.layout;
    },
    get engineEdits() {
      return store.engineEdits;
    },
    get settings() {
      return store.settings;
    },
    settingDefault: (name) => store.settingDefault(name),
    loadBuild: (build) => store.loadBuild(build),
    setParam: (name, value) => store.setParam(name, value),
  });
  $effect(() =>
    untrack(() => {
      void builds.refresh();
    }),
  );

  $effect(() => {
    document.documentElement.dataset.theme = theme;
  });

  // once, on mount: untracked, so the driver's own state (its style) never re-runs it
  $effect(() =>
    untrack(() => {
      driver.start();
      const stop = store.onFrame(() => driver.frame());
      return () => {
        stop();
        driver.dispose();
      };
    }),
  );
</script>

<AppLayout>
  {#snippet top()}
    <TopBar {store} {driver} {theme} ontoggletheme={() => (theme = theme === "dark" ? "light" : "dark")} />
  {/snippet}
  {#snippet left()}
    <EditorPanel {store} {editor} {builds} />
  {/snippet}
  {#snippet centre()}
    <CentrePanel {store} {driver} />
  {/snippet}
  {#snippet bottom()}
    <BottomPanel />
  {/snippet}
</AppLayout>
