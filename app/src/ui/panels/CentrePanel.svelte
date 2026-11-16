<!--
  CentrePanel - the middle region: the engine process's messages (restarting, stopped) when there
  is no engine to show, and otherwise the driving and sound settings and a minimal readout
  (stage 066) until the instruments and the 3D view take the space. Nothing here changes the
  quality by itself: falling behind is said, and a lower quality offered (app-design section 11).
-->
<script lang="ts">
  import type { Quality } from "../../shared/addon";
  import Button from "../components/Button.svelte";
  import Notice from "../components/Notice.svelte";
  import Panel from "../components/Panel.svelte";
  import Readout from "../components/Readout.svelte";
  import Select from "../components/Select.svelte";
  import Slider from "../components/Slider.svelte";
  import { VARIANT_MAX, type Driver } from "../lib/driver.svelte";
  import type { EngineStore } from "../lib/engine-store.svelte";
  import { THROTTLE_STYLES, type ThrottleStyle } from "../lib/throttle-input";

  interface Props {
    store: EngineStore;
    driver: Driver;
  }

  let { store, driver }: Props = $props();

  const STYLE_NAMES: Record<ThrottleStyle, string> = {
    onoff: "On/off key (W snaps 0 or 100 %)",
    ramped: "Ramped key (hold W)",
    analog: "Analog key (a learned controller input)",
    trigger: "Controller trigger",
    latched: "Mouse (the slider and the wheel)",
  };
  const QUALITY_NAMES: Record<Quality, string> = {
    low: "Low", medium: "Medium", high: "High (the reference)", ultra: "Ultra", balanced: "Balanced", lean: "Lean",
  };
  /** from the cheapest to the dearest, for "offer a lower quality" */
  const COST: Quality[] = ["lean", "low", "balanced", "medium", "high", "ultra"];
  const LATENCIES = [10, 20, 30, 40, 60, 80];

  let ready = $derived(store.summary === "running");
  let tier = $derived(store.hello?.presets.find((p) => p.key === store.preset)?.quality ?? null);
  let playingQuality = $derived(store.quality ?? tier);
  let lower = $derived.by(() => {
    if (!playingQuality) return null;
    const i = COST.indexOf(playingQuality);
    return i > 0 ? COST[i - 1] : null;
  });
  let qualities = $derived([
    { value: "", label: tier ? `This engine's tier (${QUALITY_NAMES[tier]})` : "This engine's tier" },
    ...COST.map((q) => ({ value: q, label: QUALITY_NAMES[q] })),
  ]);
  let styles = THROTTLE_STYLES.map((s) => ({ value: s, label: STYLE_NAMES[s] }));
  let devices = $derived([{ value: "-1", label: "The system's default" }, ...store.devices.map((d) => ({ value: String(d.index), label: d.name }))]);
  let latencies = LATENCIES.map((ms) => ({ value: String(ms), label: `${ms} ms` }));
  let candidates = Array.from({ length: VARIANT_MAX + 1 }, (_, i) => ({ value: String(i), label: i === 0 ? "None" : `Candidate ${i}` }));
  /** the host renders RB_HOST_BLOCK_FRAMES (128) frames a block */
  const BLOCK_FRAMES = 128;
  let renderShare = $derived.by(() => {
    const rate = store.hello?.sampleRate;
    if (!store.stats || !rate) return null;
    return (100 * store.stats.renderMsMean) / ((1000 * BLOCK_FRAMES) / rate);
  });
  /** after a blind session: which side was the candidate */
  let revealed = $state<"A" | "B" | null>(null);
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
      {#if store.deviceNotice}
        <Notice tone="info">
          {store.deviceNotice}
          <Button onclick={() => store.dismissDeviceNotice()}>Dismiss</Button>
        </Notice>
      {/if}
      {#if store.fallingBehind}
        <Notice tone="warning">
          This computer isn't keeping up with this engine at the {playingQuality ? QUALITY_NAMES[playingQuality] : ""} quality: the sound may break up.
          {#if lower}
            <Button onclick={() => void store.setQuality(lower)}>Use {QUALITY_NAMES[lower]}</Button>
          {/if}
        </Notice>
      {/if}

      <div class="readouts">
        <Readout label="Engine speed" value={store.snapshot ? store.snapshot.instruments.rpm : null} unit="rpm" />
        <Readout label="Throttle" value={store.snapshot ? 100 * store.snapshot.instruments.throttle : null} unit="%" />
        <Readout label="Render time, of real time" value={renderShare} unit="%" />
        <Readout label="Ring" value={store.stats ? store.stats.ringFillMs : null} unit="ms" digits={1} />
        <Readout label="Underruns" value={store.stats ? store.stats.underruns : null} />
        <Readout label="Playing on" value={store.playing} />
      </div>

      <div class="row">
        <Select label="Throttle style" options={styles} value={driver.style} disabled={!ready} onchange={(v) => driver.setStyle(v as ThrottleStyle)} />
        {#if driver.style === "analog"}
          <Button pressed={driver.learning} disabled={!ready} onclick={() => driver.learnAnalog()}>
            {driver.learning ? "Press the analog key now" : "Learn the analog key"}
          </Button>
        {/if}
        {#if (driver.style === "analog" || driver.style === "trigger") && driver.pads.length === 0}
          <span class="hint">No controller seen yet: press a button on it (or, for an analog keyboard, check its controller emulation is on).</span>
        {/if}
      </div>

      <div class="row">
        <Select
          label="Quality"
          options={qualities}
          value={store.quality ?? ""}
          disabled={!ready || store.rebuilding}
          onchange={(v) => void store.setQuality(v === "" ? null : (v as Quality))}
        />
        <Slider label="Volume" value={store.volume} min={0} max={2} step={0.05} disabled={!ready} oninput={(v) => void store.setVolume(v)} />
        <Select label="Output" options={devices} value="-1" disabled={!ready} onchange={(v) => void store.selectDevice(Number(v))} />
        <Select label="Latency" options={latencies} value={String(store.latencyMs)} disabled={!ready || store.rebuilding} onchange={(v) => void store.setLatency(Number(v))} />
      </div>

      <details class="developer">
        <summary>Developer: the A/B switch</summary>
        <div class="row">
          <Select label="Candidate" options={candidates} value={String(driver.candidate)} disabled={!ready} onchange={(v) => driver.setCandidate(Number(v))} />
          <Button pressed={driver.blind} disabled={!ready} onclick={() => ((revealed = null), driver.setBlind(!driver.blind))}>Blind</Button>
          <span class="hint">B flips A and B. Now: {driver.side}{driver.blind ? "" : driver.side === "B" ? ` (candidate ${driver.candidate})` : " (the accepted sound)"}.</span>
        </div>
        {#if driver.candidate === 0}
          <p class="hint">No candidate chosen: A and B are the same sound.</p>
        {:else if driver.blind}
          <Button onclick={() => (revealed = driver.reveal())}>Reveal</Button>
          {#if revealed}<span class="hint">The candidate was {revealed}.</span>{/if}
        {/if}
      </details>
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

  .row,
  .readouts {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: var(--rb-space-5);
  }

  .hint {
    margin: 0;
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-sm);
  }

  .developer summary {
    cursor: pointer;
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-sm);
  }
</style>
