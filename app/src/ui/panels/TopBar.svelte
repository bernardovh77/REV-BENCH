<!--
  TopBar - the engine's state, the preset, ignition, starter, the throttle, the rpm, and where
  you listen from (stage 066). Every control shows what the engine confirms (the snapshot, the
  store), not what was clicked. The throttle slider and the wheel over it are the latched style
  (throttle-input.ts); the keys are the driver's (driver.svelte.ts).
-->
<script lang="ts">
  import { OFFERED_SCENES, type Scene } from "../../shared/addon";
  import Button from "../components/Button.svelte";
  import Readout from "../components/Readout.svelte";
  import Select from "../components/Select.svelte";
  import Slider from "../components/Slider.svelte";
  import type { Driver } from "../lib/driver.svelte";
  import type { EngineStore, EngineSummary } from "../lib/engine-store.svelte";
  import { QUALITY_NAMES, cheaperQuality } from "../lib/qualities";

  interface Props {
    store: EngineStore;
    driver: Driver;
    theme: "dark" | "light";
    ontoggletheme: () => void;
  }

  let { store, driver, theme, ontoggletheme }: Props = $props();

  const SUMMARY_TEXT: Record<EngineSummary, string> = {
    "no-shell": "No application shell",
    starting: "Engine starting",
    connecting: "Connecting to the engine",
    running: "Engine running",
    restarting: "Engine restarting",
    failed: "Engine stopped",
  };

  /** RB_LISTENER_*, in order */
  const PLACES = ["1 m behind the tailpipe", "3 m behind", "Beside the car", "In front", "In the driver's seat"];
  const SCENE_NAMES: Record<Scene, string> = {
    asphalt: "Asphalt", grass: "Grass", wall: "By a wall", garage: "Garage",
    dyno: "Dyno cell", iso: "Tailpipe test, 0.5 m at 45°", passby: "Pass-by at 15 m/s", cabin: "Cabin",
  };

  let instruments = $derived(store.snapshot?.instruments ?? null);
  // the quality playing (chosen, or the preset's tier) and the next cheaper one, for the warning
  let playing = $derived(store.quality ?? store.hello?.presets.find((p) => p.key === store.preset)?.quality ?? null);
  let cheaper = $derived(cheaperQuality(playing));
  let ready = $derived(store.summary === "running");
  let presets = $derived((store.hello?.presets ?? []).map((p) => ({ value: p.key, label: p.name })));
  let places = PLACES.map((label, i) => ({ value: String(i), label }));
  let scenes = [{ value: "", label: "Mono (no scene)" }, ...OFFERED_SCENES.map((s) => ({ value: s, label: `Stereo: ${SCENE_NAMES[s]}` }))];

  function onwheel(event: WheelEvent) {
    if (!ready) return;
    event.preventDefault();
    driver.wheel(Math.sign(event.deltaY));
  }
</script>

<header class="topbar">
  <div class="state" data-state={store.summary}>
    <span class="dot" aria-hidden="true"></span>
    <span class="state-text">{SUMMARY_TEXT[store.summary]}</span>
  </div>

  <!-- The "can't keep up" warning's own slot: its space is kept while it's hidden, so it never
       moves the rest of the bar; it stays up ~10 s after the last late block (the store). -->
  <div class="keep-up" class:on={store.fallingBehind} role="status" aria-live="polite">
    {#if store.fallingBehind}
      <span class="keep-up-text" title="This computer isn't keeping up with this engine at the {playing ? QUALITY_NAMES[playing] : ''} quality: the sound may break up.">
        Can't keep up
      </span>
      {#if cheaper}
        <span class="keep-up-action"><Button onclick={() => void store.setQuality(cheaper)}>Use {QUALITY_NAMES[cheaper]}</Button></span>
      {/if}
    {/if}
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
      title="Ignition (I)"
      onclick={() => void store.setParam("ignition", instruments?.ignition === 1 ? 0 : 1)}
    >
      Ignition
    </Button>
    <Button
      pressed={instruments?.starter === 1}
      disabled={!ready}
      title="Starter (hold S)"
      onpointerdown={() => void store.setParam("starter", 1)}
      onpointerup={() => void store.setParam("starter", 0)}
      onpointerleave={() => instruments?.starter === 1 && void store.setParam("starter", 0)}
      onpointercancel={() => void store.setParam("starter", 0)}
    >
      Starter
    </Button>
    <div class="throttle" onwheel={onwheel} title="Throttle: W or Up, the wheel, a trigger">
      <Slider label="Throttle" value={instruments?.throttle ?? 0} disabled={!ready} oninput={(value) => driver.setLatched(value)} />
    </div>
  </div>

  <Readout label="Engine speed" value={instruments ? instruments.rpm : null} unit="rpm" />

  <Select
    label="Scene"
    options={scenes}
    value={store.scene ?? ""}
    disabled={!ready || store.rebuilding}
    onchange={(value) => void store.setScene(value === "" ? null : (value as Scene))}
  />
  <Select
    label="Listener"
    options={places}
    value={String(store.listener)}
    disabled={!ready || store.scene !== null}
    onchange={(value) => void store.setListener(Number(value))}
  />

  <div class="spacer"></div>

  <Button onclick={ontoggletheme}>{theme === "dark" ? "Light theme" : "Dark theme"}</Button>
</header>

<style>
  .topbar {
    display: flex;
    align-items: center;
    flex-wrap: wrap;
    gap: var(--rb-space-3) var(--rb-space-6);
    box-sizing: border-box;
    min-height: 100%;
    padding: var(--rb-space-3) var(--rb-space-5);
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

  .keep-up {
    display: inline-flex;
    align-items: center;
    gap: var(--rb-space-3);
    box-sizing: border-box;
    width: var(--rb-status-width);
    height: var(--rb-control-height);
    padding: 0 var(--rb-space-3);
    border-radius: var(--rb-radius-md);
    font-size: var(--rb-font-size-sm);
    color: var(--rb-color-warning);
    visibility: hidden;
  }

  .keep-up.on {
    visibility: visible;
    border: var(--rb-border-width) solid var(--rb-color-warning);
  }

  .keep-up-text {
    white-space: nowrap;
  }

  .keep-up-action {
    margin-left: auto;
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
