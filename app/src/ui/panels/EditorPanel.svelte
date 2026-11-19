<!--
  EditorPanel - the left region: the engine editor (stage 067). Every field, its unit, range,
  size (one value, or one per bank or cylinder) and whether it changes the running engine come
  from the core (editor.svelte.ts); this file lays them out. Fields that need a rebuild go into
  a draft, applied together as one rebuild or discarded; live fields change the engine at once.
  The listener's audition controls (which layers are heard) sit apart from the physical tuning:
  they change what you hear, not the engine. The layout (stage 131) comes first: the family,
  the cylinders, the crank and a V's bank angle, only what the core builds; a change joins the
  draft, and the draft bar says which edits the new layout drops.
-->
<script lang="ts">
  import type { EngineFieldInfo, ParamInfo } from "../../shared/addon";
  import Button from "../components/Button.svelte";
  import Notice from "../components/Notice.svelte";
  import NumberField from "../components/NumberField.svelte";
  import Panel from "../components/Panel.svelte";
  import Select from "../components/Select.svelte";
  import { CHOICES, labelOf, type Editor } from "../lib/editor.svelte";
  import { layoutName } from "../../shared/layout-edits";
  import type { EngineStore } from "../lib/engine-store.svelte";
  import { displayUnit, fromDisplay, toDisplay, type UnitSystem } from "../lib/units";

  interface Props {
    store: EngineStore;
    editor: Editor;
  }

  let { store, editor }: Props = $props();

  /** RB_LAYER_* bits */
  const LAYERS = [
    { bit: 1, label: "Pipes" },
    { bit: 2, label: "Flow noise" },
    { bit: 4, label: "Mechanical" },
  ];
  const UNIT_CHOICES = [
    { value: "metric", label: "Metric" },
    { value: "imperial", label: "Imperial" },
  ];

  let ready = $derived(store.summary === "running" && store.engineValues !== null);

  /* the layout's choices: the families, the counts of the one shown, its cranks */
  let layoutShown = $derived(editor.shownLayout);
  let familyChoices = $derived(
    [...new Map(store.layouts.map((l) => [l.family, l.familyName])).entries()].map(([value, label]) => ({ value, label })),
  );
  let countChoices = $derived(
    store.layouts.filter((l) => l.family === layoutShown?.family).map((l) => ({ value: String(l.cylinders), label: String(l.cylinders) })),
  );
  let crankEntry = $derived(store.layouts.find((l) => l.family === layoutShown?.family && l.cylinders === layoutShown?.cylinders));
  let crankChoices = $derived(crankEntry?.cranks.map((c) => ({ value: c.crank, label: c.name })) ?? []);
  let crankShown = $derived(crankEntry?.cranks.find((c) => c.crank === layoutShown?.crank));
  let generated = $derived(store.layout !== null);
  /** which sections are open: the user's choice, kept through rebuilds (Engine at first) */
  let opened = $state<Record<string, boolean>>({ Engine: true });
  let sections = $derived(editor.sections());
  let param = (name: string): ParamInfo | undefined => store.hello?.params.find((p) => p.name === name);

  /** How a field changes the engine, in words. */
  function behaviour(field: EngineFieldInfo): string {
    if (field.flags.includes("readonly")) return "read-only";
    return field.flags.includes("live") ? "live" : "rebuild";
  }

  /** What each value of a multi-value field is, for its label. */
  function indexName(field: EngineFieldInfo, i: number): string {
    if (field.flags.includes("per-bank")) return `bank ${i + 1}`;
    if (field.flags.includes("in-order")) return `place ${i + 1}`;
    return `cylinder ${i + 1}`;
  }

  /** A value as shown, in the editor's units. */
  function shown(field: EngineFieldInfo, si: number, system: UnitSystem): number {
    return toDisplay(si, field.unit, system);
  }

  function layerOn(bit: number): boolean {
    return ((store.settings.layers ?? 7) & bit) !== 0;
  }

  function toggleLayer(bit: number) {
    const next = (store.settings.layers ?? 7) ^ bit;
    if (next !== 0) void store.setParam("layers", next); // at least one layer: silence is the volume's job
  }
</script>

<Panel title="Editor">
  <div class="editor">
    <div class="row">
      <Select label="Units" options={UNIT_CHOICES} value={editor.units} onchange={(v) => (editor.units = v as UnitSystem)} />
      <Button disabled={!ready || !editor.canUndo} onclick={() => void editor.undo()} title="Undo the last change">Undo</Button>
      <Button disabled={!ready || !editor.canRedo} onclick={() => void editor.redo()} title="Redo what was undone">Redo</Button>
    </div>


    {#if !ready}
      <p class="empty">The editor opens once the engine runs.</p>
    {:else}
      {#if layoutShown && store.layouts.length}
        <details class="section layout" open={opened.Layout ?? true} ontoggle={(e) => (opened.Layout = e.currentTarget.open)}>
          <summary>Layout{generated ? `: ${layoutName(store.layout!)}, generic` : ""}</summary>
          <div class="row">
            <Select label="Family" options={familyChoices} value={layoutShown.family} disabled={store.rebuilding}
              onchange={(v) => void editor.chooseLayout({ family: v as typeof layoutShown.family })} />
            <Select label="Cylinders" options={countChoices} value={String(layoutShown.cylinders)} disabled={store.rebuilding}
              onchange={(v) => void editor.chooseLayout({ cylinders: Number(v) })} />
            <Select label="Crank" options={crankChoices} value={layoutShown.crank} disabled={store.rebuilding || crankChoices.length < 2}
              onchange={(v) => void editor.chooseLayout({ crank: v as typeof layoutShown.crank })} />
            {#if layoutShown.family === "v" && crankShown}
              <NumberField label="Bank angle" value={layoutShown.bankAngle} unit="°" min={crankShown.angleMin} max={crankShown.angleMax}
                digits={1} onchange={(v) => void editor.chooseLayout({ bankAngle: v })} />
            {/if}
          </div>
          <p class="note">
            {#if generated || editor.layoutChanging}
              A generic engine worked out from {store.hello?.presets.find((p) => p.key === store.preset)?.name ?? "the preset"}: its
              cylinder, cams and limits, with the crank, exhaust and idle made for this layout. Not any real engine.
            {:else}
              The preset's own layout. Another builds a generic engine from this preset's cylinder, cams and limits.
            {/if}
          </p>
        </details>
      {/if}
      {#each sections as section (section.title)}
        <details class="section" open={opened[section.title] ?? false} ontoggle={(e) => (opened[section.title] = e.currentTarget.open)}>
          <summary>{section.title}</summary>
          {#each section.fields as field (field.id)}
            {@const values = editor.values(field)}
            {@const unit = displayUnit(field.unit, editor.units)}
            <div class="field" data-field={field.name}>
              <div class="field-head">
                <span class="field-name">{labelOf(field.name)}</span>
                <span class="tag" data-behaviour={behaviour(field)}>{behaviour(field)}</span>
              </div>
              <div class="values">
                {#each values as value, i (i)}
                  {#if field.flags.includes("readonly")}
                    <span class="readonly">{Number(shown(field, value, editor.units).toFixed(unit.digits))}{unit.label ? " " + unit.label : ""}</span>
                  {:else if CHOICES[field.name]}
                    <Select
                      label={values.length > 1 ? labelOf(indexName(field, i)) : labelOf(field.name)}
                      options={CHOICES[field.name].map((label, n) => ({ value: String(n), label })).filter((o) => o.label !== "" && Number(o.value) >= (field.min ?? 0) && Number(o.value) <= (field.max ?? 99))}
                      value={String(value)}
                      disabled={store.rebuilding}
                      onchange={(v) => void editor.set(field, i, Number(v))}
                    />
                  {:else}
                    <NumberField
                      label={values.length > 1 ? `${labelOf(field.name)}, ${indexName(field, i)}` : labelOf(field.name)}
                      value={shown(field, value, editor.units)}
                      unit={values.length > 1 ? "" : unit.label}
                      caption={values.length > 1 ? String(i + 1) : ""}
                      min={field.min === null ? undefined : shown(field, field.min, editor.units)}
                      max={field.max === null ? undefined : shown(field, field.max, editor.units)}
                      step={field.flags.includes("integer") ? 1 : "any"}
                      digits={unit.digits}
                      disabled={store.rebuilding}
                      onchange={(v) => void editor.set(field, i, fromDisplay(v, field.unit, editor.units))}
                    />
                  {/if}
                {/each}
                {#if values.length > 1 && field.flags.includes("per-cylinder") && !field.flags.includes("readonly") && !CHOICES[field.name]}
                  <NumberField
                    label={`${labelOf(field.name)}, every one`}
                    value={null}
                    unit={`${unit.label} (all)`}
                    disabled={store.rebuilding}
                    onchange={(v) => void editor.setAll(field, fromDisplay(v, field.unit, editor.units))}
                  />
                {/if}
              </div>
            </div>
          {/each}
        </details>
      {/each}

      <details class="section" open={opened["Fuel and spark"] ?? false} ontoggle={(e) => (opened["Fuel and spark"] = e.currentTarget.open)}>
        <summary>Fuel and spark</summary>
        {#each ["afr", "spark_offset"] as name (name)}
          {@const p = param(name)}
          {#if p}
            <div class="field" data-field={name}>
              <div class="field-head">
                <span class="field-name">{name === "afr" ? "Air-fuel ratio" : "Spark offset"}</span>
                <span class="tag" data-behaviour="live">live</span>
              </div>
              <NumberField
                label={name === "afr" ? "Air-fuel ratio" : "Spark offset"}
                value={store.settings[name] ?? p.default}
                unit={name === "afr" ? ": 1" : "°"}
                min={p.min}
                max={p.max}
                digits={2}
                onchange={(v) => void store.setParam(name, v)}
              />
            </div>
          {/if}
        {/each}
      </details>

      <details class="section audition" open={opened.Listen ?? false} ontoggle={(e) => (opened.Listen = e.currentTarget.open)}>
        <summary>Listen (what you hear, not the engine)</summary>
        <div class="row">
          {#each LAYERS as layer (layer.bit)}
            <Button pressed={layerOn(layer.bit)} onclick={() => toggleLayer(layer.bit)}>{layer.label}</Button>
          {/each}
        </div>
      </details>
    {/if}

    <!-- The draft's bar: stuck to the panel's bottom edge, over the fields, so it moves nothing
         when it comes and goes. -->
    {#if editor.draftCount || editor.previewError || editor.error}
      <div class="draft" role="status">
        {#if editor.draftCount}
          <div class="draft-line">
            <span>{editor.draftCount} {editor.draftCount === 1 ? "change waits" : "changes wait"} for a rebuild{editor.layoutChanging
                ? ` (as ${layoutName(editor.layoutDraft!)})` : ""}</span>
            <span class="draft-actions">
              <Button size="sm" disabled={editor.busy} onclick={() => editor.discard()}>Discard</Button>
              <Button size="sm" variant="primary" disabled={!ready || editor.busy || store.rebuilding || editor.previewError !== null} onclick={() => void editor.apply()}>Apply</Button>
            </span>
          </div>
        {/if}
        {#if editor.dropped.length}
          <p class="dropped">
            The new layout drops {editor.dropped.length === 1 ? "an edit" : `${editor.dropped.length} edits`} made per cylinder:
            {[...new Set(editor.dropped.map((e) => labelOf(e.field).toLowerCase()))].join(", ")}.
          </p>
        {/if}
        {#if editor.previewError}
          <Notice tone="warning">This engine can't be built: {editor.previewError}</Notice>
        {/if}
        {#if editor.error}
          <Notice tone="warning">{editor.error}</Notice>
        {/if}
      </div>
    {/if}
  </div>
</Panel>

<style>
  .editor {
    display: flex;
    flex-direction: column;
    gap: var(--rb-space-4);
  }

  .row {
    display: flex;
    flex-wrap: wrap;
    align-items: flex-end;
    gap: var(--rb-space-3);
  }

  .draft {
    position: sticky;
    bottom: 0;
    display: flex;
    flex-direction: column;
    gap: var(--rb-space-3);
    padding: var(--rb-space-3);
    border: var(--rb-border-width) solid var(--rb-color-accent);
    border-radius: var(--rb-radius-md);
    background: var(--rb-color-surface-raised);
    /* the panel's colour all round it, so the fields scrolling behind never peek out under it */
    box-shadow: 0 0 0 var(--rb-space-4) var(--rb-color-surface);
    font-size: var(--rb-font-size-sm);
  }

  .draft-line {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: var(--rb-space-3);
  }

  .note,
  .dropped {
    margin: 0;
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-xs);
  }

  .dropped {
    color: var(--rb-color-warning);
  }

  .draft-actions {
    display: flex;
    flex: none;
    gap: var(--rb-space-2);
  }

  .section summary {
    cursor: pointer;
    font-size: var(--rb-font-size-sm);
    font-weight: var(--rb-weight-semibold);
    padding: var(--rb-space-2) 0;
  }

  .field {
    display: flex;
    flex-direction: column;
    gap: var(--rb-space-1);
    padding: var(--rb-space-2) 0;
  }

  .field-head {
    display: flex;
    justify-content: space-between;
    gap: var(--rb-space-3);
    font-size: var(--rb-font-size-sm);
  }

  .tag {
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-xs);
  }

  .tag[data-behaviour="live"] {
    color: var(--rb-color-success);
  }

  .values {
    display: flex;
    flex-wrap: wrap;
    gap: var(--rb-space-2);
  }

  .readonly {
    font-size: var(--rb-font-size-sm);
  }

  .empty {
    margin: 0;
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-sm);
  }
</style>
