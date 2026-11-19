<!--
  BottomPanel - the bottom region: the dyno (stage 068). The dyno is the core's (rb_dyno_*): it
  takes the crank at a speed and either holds it there or sweeps it up at a rate, reading the
  brake torque and power once a cycle. This panel asks for that, shows the readings as they come
  (torque and power against speed, the run being read bold, earlier runs overlaid), in the
  editor's units, and exports them as CSV. Nothing here computes a torque.
-->
<script lang="ts">
  import Button from "../components/Button.svelte";
  import LineChart from "../components/LineChart.svelte";
  import Notice from "../components/Notice.svelte";
  import NumberField from "../components/NumberField.svelte";
  import Panel from "../components/Panel.svelte";
  import Readout from "../components/Readout.svelte";
  import Select from "../components/Select.svelte";
  import type { EngineStore } from "../lib/engine-store.svelte";
  import { displayUnit, toDisplay, type UnitSystem } from "../lib/units";

  interface Props {
    store: EngineStore;
    units?: UnitSystem;
    /** the main process's save dialog (absent outside the app) */
    saveText?: (suggestedName: string, text: string) => Promise<boolean>;
  }

  let { store, units = "metric", saveText }: Props = $props();

  let mode = $state<"sweep" | "hold">("sweep");
  let throttle = $state<"wide-open" | "driven">("wide-open");
  let holdRpm = $state(3000);
  let fromRpm = $state(1500);
  let toRpm = $state(6500);
  let rate = $state(500);
  let message = $state<string | null>(null);

  let ins = $derived(store.snapshot?.instruments ?? null);
  let attached = $derived(ins?.dyno_attached === 1);
  let running = $derived(store.summary === "running");
  let torqueUnit = $derived(displayUnit("N m", units));
  let powerUnit = $derived(displayUnit("W", units));
  /** the runs, newest first, as each chart's series */
  let newestFirst = $derived([...store.dyno.runs].reverse());
  let torqueSeries = $derived(newestFirst.map((r) => ({ label: r.label, points: r.points.map((p) => ({ x: p.rpm, y: toDisplay(p.torque, "N m", units) })) })));
  let powerSeries = $derived(newestFirst.map((r) => ({ label: r.label, points: r.points.map((p) => ({ x: p.rpm, y: toDisplay(p.power, "W", units) })) })));

  async function start() {
    message = null;
    try {
      if (mode === "hold") await store.dynoHold(holdRpm, throttle === "wide-open");
      else await store.dynoSweep(fromRpm, toRpm, rate, throttle === "wide-open");
    } catch (e) {
      message = e instanceof Error ? e.message : String(e);
    }
  }

  async function release() {
    try {
      await store.dynoRelease();
    } catch (e) {
      message = e instanceof Error ? e.message : String(e);
    }
  }

  async function exportCsv() {
    if (!saveText) return;
    const stamp = new Date().toISOString().slice(0, 19).replace(/[:T]/g, "-");
    message = (await saveText(`dyno-${store.preset ?? "engine"}-${stamp}.csv`, store.dyno.csv())) ? "Saved." : null;
  }
</script>

<Panel title="Dyno">
  <div class="dyno">
    <div class="row">
      <Select label="Run" options={[{ value: "sweep", label: "Sweep" }, { value: "hold", label: "Hold a speed" }]} value={mode}
        disabled={attached} onchange={(v) => (mode = v as "sweep" | "hold")} />
      {#if mode === "hold"}
        <NumberField label="Speed" value={holdRpm} unit="rpm" min={300} max={20000} step={50} digits={0} onchange={(v) => (holdRpm = v)} />
      {:else}
        <NumberField label="From" value={fromRpm} unit="rpm" min={300} max={20000} step={50} digits={0} onchange={(v) => (fromRpm = v)} />
        <NumberField label="To" value={toRpm} unit="rpm" min={300} max={20000} step={50} digits={0} onchange={(v) => (toRpm = v)} />
        <NumberField label="Rate" value={rate} unit="rpm/s" min={10} max={5000} step={10} digits={0} onchange={(v) => (rate = v)} />
      {/if}
      <Select label="Throttle" options={[{ value: "wide-open", label: "Wide open" }, { value: "driven", label: "As driven" }]} value={throttle}
        disabled={attached} onchange={(v) => (throttle = v as "wide-open" | "driven")} />
      {#if attached}
        <Button variant="danger" onclick={() => void release()}>Release</Button>
      {:else}
        <Button variant="primary" disabled={!running || store.rebuilding} onclick={() => void start()}
          title="The engine runs into the dyno at the speed asked: start it first">Start</Button>
      {/if}
      <Button disabled={store.dyno.runs.length === 0} onclick={() => store.dyno.clear()}>Clear runs</Button>
      <Button disabled={store.dyno.runs.length === 0 || !saveText} onclick={() => void exportCsv()}>Export CSV</Button>
    </div>
    <div class="row readouts">
      <Readout label="Dyno" value={attached ? "holding" : "free"} />
      <Readout label="Target" value={attached ? ins!.dyno_target_rpm : null} unit="rpm" />
      <Readout label="Read at" value={attached && ins!.dyno_readings > 0 ? ins!.dyno_rpm : null} unit="rpm" />
      <Readout label="Brake torque" value={attached && ins!.dyno_readings > 0 ? toDisplay(ins!.dyno_torque, "N m", units) : null} unit={torqueUnit.label} digits={1} />
      <Readout label="Brake power" value={attached && ins!.dyno_readings > 0 ? toDisplay(ins!.dyno_power, "W", units) : null} unit={powerUnit.label} digits={1} />
      <Readout label="Readings" value={attached ? ins!.dyno_readings : null} />
    </div>
    {#if message}<Notice tone="info">{message}</Notice>{/if}
    <div class="charts">
      <LineChart title="Brake torque" xLabel="rpm" yLabel={torqueUnit.label} series={torqueSeries} />
      <LineChart title="Brake power" xLabel="rpm" yLabel={powerUnit.label} series={powerSeries} />
    </div>
  </div>
</Panel>

<style>
  .dyno {
    display: flex;
    flex-direction: column;
    gap: var(--rb-space-3);
  }

  .row {
    display: flex;
    flex-wrap: wrap;
    align-items: flex-end;
    gap: var(--rb-space-3);
  }

  .readouts {
    gap: var(--rb-space-5);
  }

  .charts {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: var(--rb-space-4);
  }
</style>
