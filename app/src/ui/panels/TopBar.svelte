<!--
  TopBar - the engine's state, the preset, ignition, starter, throttle and the rpm. Minimal
  (stage 124): enough to start the engine and hear it. Every control shows what the engine
  confirms (the snapshot), not what was clicked.
-->
<script lang="ts">
  import Button from "../components/Button.svelte";
  import Readout from "../components/Readout.svelte";
  import Select from "../components/Select.svelte";
  import Slider from "../components/Slider.svelte";
  import type { EngineStore, EngineSummary } from "../lib/engine-store.svelte";

  interface Props {
    store: EngineStore;
    theme: "dark" | "light";
    ontoggletheme: () => void;
  }

  let { store, theme, ontoggletheme }: Props = $props();

  const SUMMARY_TEXT: Record<EngineSummary, string> = {
    "no-shell": "No application shell",
    starting: "Engine starting",
    connecting: "Connecting to the engine",
    running: "Engine running",
    restarting: "Engine restarting",
    failed: "Engine stopped",
  };

  let instruments = $derived(store.snapshot?.instruments ?? null);
  let ready = $derived(store.summary === "running");
  let presets = $derived((store.hello?.presets ?? []).map((p) => ({ value: p.key, label: p.name })));
</script>

<header class="topbar">
  <div class="state" data-state={store.summary}>
    <span class="dot" aria-hidden="true"></span>
    <span class="state-text">{SUMMARY_TEXT[store.summary]}</span>
  </div>

  <Select
    label="Engine"
    options={presets}
    value={store.preset ?? ""}
    disabled={!ready || store.rebuilding}
    onchange={(key) => void store.loadPreset(key)}
  />

  <div class="controls">
    <Button
      pressed={instruments?.ignition === 1}
      disabled={!ready}
      onclick={() => void store.setParam("ignition", instruments?.ignition === 1 ? 0 : 1)}
    >
      Ignition
    </Button>
    <Button
      pressed={instruments?.starter === 1}
      disabled={!ready}
      onpointerdown={() => void store.setParam("starter", 1)}
      onpointerup={() => void store.setParam("starter", 0)}
      onpointerleave={() => instruments?.starter === 1 && void store.setParam("starter", 0)}
      onpointercancel={() => void store.setParam("starter", 0)}
    >
      Starter
    </Button>
    <Slider
      label="Throttle"
      value={instruments?.throttle ?? 0}
      disabled={!ready}
      oninput={(value) => void store.setParam("throttle", value)}
    />
  </div>

  <Readout label="Engine speed" value={instruments ? instruments.rpm : null} unit="rpm" />

  <div class="spacer"></div>

  <Button onclick={ontoggletheme}>{theme === "dark" ? "Light theme" : "Dark theme"}</Button>
</header>

<style>
  .topbar {
    display: flex;
    align-items: center;
    gap: var(--rb-space-6);
    height: 100%;
    padding: 0 var(--rb-space-5);
    background: var(--rb-color-surface);
    border-bottom: var(--rb-border-width) solid var(--rb-color-border);
  }

  .state {
    display: inline-flex;
    align-items: center;
    gap: var(--rb-space-3);
    font-size: var(--rb-font-size-sm);
    color: var(--rb-color-text-muted);
  }

  .dot {
    width: var(--rb-space-3);
    height: var(--rb-space-3);
    border-radius: var(--rb-radius-round);
    background: var(--rb-color-text-disabled);
  }

  .state[data-state="running"] .dot {
    background: var(--rb-color-success);
  }

  .state[data-state="starting"] .dot,
  .state[data-state="connecting"] .dot,
  .state[data-state="restarting"] .dot {
    background: var(--rb-color-warning);
  }

  .state[data-state="failed"] .dot {
    background: var(--rb-color-danger);
  }

  .controls {
    display: flex;
    align-items: center;
    gap: var(--rb-space-4);
  }

  .spacer {
    flex: 1;
  }
</style>
