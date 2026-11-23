<!--
  CentrePanel - the middle region: the engine process's messages (restarting, stopped) when there
  is no engine to show, and otherwise the instruments (stage 068: speed, manifold pressure and
  boost, lambda and AFR, torque and power, spark, the engine computer's cuts, the output meter,
  the render's speed and underruns, the model's own warning), then the driving and sound
  settings. Every value is the snapshots' or the host's statistics', in the editor's units.
  Falling behind is said in the top bar's own slot (TopBar.svelte), where it moves nothing else.
-->
<script lang="ts">
  import { LATENCIES } from "../lib/keep-up";
  import { manifoldReading } from "../lib/manifold";
  import type { Quality } from "../../shared/addon";
  import Button from "../components/Button.svelte";
  import Notice from "../components/Notice.svelte";
  import Panel from "../components/Panel.svelte";
  import LevelMeter from "../components/LevelMeter.svelte";
  import Readout from "../components/Readout.svelte";
  import Select from "../components/Select.svelte";
  import Slider from "../components/Slider.svelte";
  import { VARIANT_MAX, type Driver } from "../lib/driver.svelte";
  import type { EngineStore } from "../lib/engine-store.svelte";
  import { GUARD_RATE_WARN } from "../lib/guard-watch";
  import { QUALITY_COST, QUALITY_NAMES } from "../lib/qualities";
  import { THROTTLE_STYLES, type ThrottleStyle } from "../lib/throttle-input";
  import { displayUnit, toDisplay, type UnitSystem } from "../lib/units";

  interface Props {
    store: EngineStore;
    driver: Driver;
    /** the editor's units (metric or imperial) */
    units?: UnitSystem;
  }
  let { store, driver, units = "metric" }: Props = $props();

  let ins = $derived(store.snapshot?.instruments ?? null);
  /** a value in the editor's units, and its unit's label */
  let shown = (si: number | undefined, unit: string) => (si === undefined ? null : toDisplay(si, unit, units));
  let label = (unit: string) => displayUnit(unit, units).label;
  let digits = (unit: string) => displayUnit(unit, units).digits;
  /** boost or vacuum against sea-level air, steady near it (manifold.ts) */
  let manifold = $derived(ins ? manifoldReading(ins.map) : null);
  let stoich = $derived(store.settingDefault("afr"));

  const STYLE_NAMES: Record<ThrottleStyle, string> = {
    onoff: "On/off key (W snaps 0 or 100 %)",
    ramped: "Ramped key (hold W)",
    analog: "Analog key (a learned controller input)",
    trigger: "Controller trigger",
    latched: "Mouse (the slider and the wheel)",
  };

  let ready = $derived(store.summary === "running");
  let tier = $derived(store.hello?.presets.find((p) => p.key === store.preset)?.quality ?? null);
  let qualities = $derived([
    { value: "", label: tier ? `This engine's tier (${QUALITY_NAMES[tier]})` : "This engine's tier" },
    ...QUALITY_COST.map((q) => ({ value: q, label: QUALITY_NAMES[q] })),
  ]);
  let styles = THROTTLE_STYLES.map((s) => ({ value: s, label: STYLE_NAMES[s] }));
  let devices = $derived([{ value: "-1", label: "The system's default" }, ...store.devices.map((d) => ({ value: String(d.index), label: d.name }))]);
  let latencies = LATENCIES.map((ms) => ({ value: String(ms), label: `${ms} ms` }));
  let candidates = Array.from({ length: VARIANT_MAX + 1 }, (_, i) => ({ value: String(i), label: i === 0 ? "None" : `Candidate ${i}` }));
  let renderShare = $derived(store.renderShare === null ? null : 100 * store.renderShare);
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

      {#if store.outsideModel}
        <Notice tone="warning">
          This build is outside what the model handles well: its numerical guards are stepping in more than
          {GUARD_RATE_WARN} times a second. What you hear and read may not be what such an engine does.
        </Notice>
      {/if}
      <div class="readouts instruments">
        <Readout label="Engine speed" value={ins ? ins.rpm : null} unit="rpm" />
        <Readout label="Manifold pressure" value={shown(ins?.map, "Pa")} unit={label("Pa")} digits={digits("Pa")} />
        <Readout label={manifold?.label ?? "Vacuum"} value={manifold === null ? null : toDisplay(manifold.value, "Pa", units)} unit={label("Pa")} digits={digits("Pa")} />
        <Readout label="Lambda" value={ins && ins.lambda > 0 ? ins.lambda : null} digits={2} />
        <Readout label="Air-fuel ratio" value={ins && ins.lambda > 0 ? ins.lambda * stoich : null} unit=": 1" digits={1} />
        <Readout label="Torque" value={shown(ins?.torque, "N m")} unit={label("N m")} digits={digits("N m")} />
        <Readout label="Power" value={shown(ins?.power, "W")} unit={label("W")} digits={1} />
        <Readout label="Spark advance" value={ins ? ins.spark : null} unit="°" digits={1} />
        <Readout label="Fuel cut" value={ins ? (ins.fuel_cut ? "on" : "off") : null} />
        <Readout label="Rev limiter" value={ins ? (ins.spark_cut ? "cutting" : "off") : null} />
        <Readout label="Limiter cuts" value={ins ? ins.limiter_cuts : null} />
        <LevelMeter label="Output" reading={store.meter} />
      </div>
      <div class="readouts">
        <Readout label="Throttle" value={ins ? 100 * ins.throttle : null} unit="%" />
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
