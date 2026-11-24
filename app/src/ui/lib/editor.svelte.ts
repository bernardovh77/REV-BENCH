/*
 * editor.svelte.ts - the engine editor's state (stage 067). The fields, their units, ranges,
 * sizes and whether they are live all come from the core (addon.engineFields, the playing
 * engine's description); nothing here knows the physics. What it keeps:
 *
 *   draft     the changes to fields that need a rebuild, not applied yet; applied together as
 *             one transaction (apply-edits), or discarded. A preview of the engine they would
 *             make (describe) shows worked-out fields (the displacement) and any refusal first.
 *   live      a live field (idle speed, rev limit, ...) changes the running engine at once.
 *   layout    (stage 131) the engine's family, cylinders, crank and a V's bank angle: a change
 *             joins the draft, the preview shows what it makes and which edits it drops (those
 *             indexed by the old layout's cylinders, layout-edits.ts), and it lands with the rest.
 *   history   each applied change, live or not, can be undone and redone: undo rebuilds the
 *             preset as the layout and with the edits it had before. A new preset starts a new
 *             history.
 *   units     metric or imperial, for display only (units.ts).
 *
 * What fields go in which section, their labels and the names of a few integer choices are
 * presentation, kept here; a field the core adds later still shows, under "Other".
 */
import type { EngineFieldInfo, LayoutInfo, LayoutSpec } from "../../shared/addon";
import { editsForLayout, keptAcrossLayouts, sameLayout } from "../../shared/layout-edits";
import type { EngineEdit } from "../../shared/process-messages";
import type { UnitSystem } from "./units";

/** What the editor needs of the engine store. */
export interface EditorStore {
  readonly engineFields: EngineFieldInfo[];
  readonly engineValues: number[][] | null;
  readonly engineEdits: EngineEdit[];
  readonly preset: string | null;
  /** stage 131: the layouts the core builds, the playing one (null: the preset's own) and the preset's own */
  readonly layouts: LayoutInfo[];
  readonly layout: LayoutSpec | null;
  readonly presetLayout: LayoutSpec | null;
  /** `layout`: absent keeps the one playing, null is the preset's own */
  applyEdits(edits: EngineEdit[], layout?: LayoutSpec | null): Promise<void>;
  editLive(edit: EngineEdit): Promise<void>;
  describeEdits(edits: EngineEdit[], layout?: LayoutSpec | null): Promise<number[][]>;
}

/** One step of the history: the edits and the layout (null: the preset's own) a build had. */
interface Built {
  edits: EngineEdit[];
  layout: LayoutSpec | null;
}

/** The editor's sections, in order, by the core's field names. */
export const SECTIONS: readonly { title: string; fields: readonly string[] }[] = [
  {
    title: "Engine",
    fields: ["cylinders", "displacement", "bore", "stroke", "rod", "compression_ratio", "inertia", "banks", "bank_angle", "bank", "pin", "position", "firing_order", "firing_angle"],
  },
  {
    title: "Cams and valves",
    fields: [
      "intake_valves", "intake_diameter", "intake_port", "intake_lift", "intake_duration", "intake_center", "intake_opens", "intake_closes",
      "exhaust_valves", "exhaust_diameter", "exhaust_port", "exhaust_lift", "exhaust_duration", "exhaust_center", "exhaust_opens", "exhaust_closes",
    ],
  },
  {
    title: "Intake",
    fields: ["throttle_diameter", "plenum_volume", "runner_length", "runner_diameter", "airbox_volume", "snorkel_length", "snorkel_diameter"],
  },
  {
    title: "Exhaust",
    fields: [
      "header_length", "header_diameter", "collector", "exhaust_pipe_length", "exhaust_pipe_diameter", "exhaust_systems", "catalyst_laminar",
      "muffler_type", "muffler_volume", "muffler_length", "muffler_resolution", "tailpipe_length", "tailpipe_diameter",
      "outlet_radiation",
    ],
  },
  { title: "Limits and load", fields: ["idle_rpm", "accessory_power"] },
  // stage 133: how the revs bounce at the limit, and the 2-step (held on L)
  { title: "Rev limiter", fields: ["rev_limit", "limiter_gap", "limiter_band", "limiter_cut", "launch_rpm", "launch_gap", "launch_band"] },
];

/**
 * Fields the editor doesn't offer, because tests/test_editor_fields.c measured that they change
 * nothing the engine does or the listener hears (its NO_EFFECT table): a control without an
 * effect would mislead. `banks` 0 for every engine, else only one with that many banks.
 */
export const HIDDEN: readonly { name: string; banks: number; why: string }[] = [
  { name: "position", banks: 0, why: "where the 3D view will draw each cylinder; nothing in the model feels it" },
  { name: "bank_angle", banks: 1, why: "on one bank it only tilts the engine" },
];

/** Integer fields whose numbers name choices (the core's enums). */
export const CHOICES: Readonly<Record<string, readonly string[]>> = {
  muffler_type: ["None (straight pipe)", "Straight-through", "Chambered"],
  outlet_radiation: ["Constant end", "Radiating outlet"],
  exhaust_systems: ["", "One (single exhaust)", "Two (twin exhaust)"],
  collector: ["First", "Second"],
  limiter_cut: ["Spark cut", "Fuel cut"],
};

/** A label from a core name: "compression_ratio" -> "Compression ratio". */
export function labelOf(name: string): string {
  const text = name.replace(/_/g, " ");
  return text.charAt(0).toUpperCase() + text.slice(1);
}

const key = (field: string, index: number) => `${field}#${index}`;

/** `base` with `changes` in place of the edits of the same field and index, in order. */
export function mergeEdits(base: readonly EngineEdit[], changes: readonly EngineEdit[]): EngineEdit[] {
  const out = base.filter((e) => !changes.some((c) => c.field === e.field && c.index === e.index));
  return [...out, ...changes];
}

export class Editor {
  units = $state<UnitSystem>("metric");
  /** the draft, by field#index */
  draft = $state.raw<Record<string, EngineEdit>>({});
  /** the engine the draft would make, or why it can't be made */
  preview = $state.raw<number[][] | null>(null);
  previewError = $state<string | null>(null);
  /** the last apply's or undo's refusal */
  error = $state<string | null>(null);
  busy = $state(false);
  /** the layout the draft would build (null: no change of layout) */
  layoutDraft = $state.raw<LayoutSpec | null>(null);
  private undoStack = $state.raw<Built[]>([]);
  private redoStack = $state.raw<Built[]>([]);
  /** the preset the history and draft belong to (state: the getters depend on it) */
  private historyPreset = $state<string | null>(null);
  private previewSeq = 0;

  constructor(private readonly store: EditorStore) {}

  /** The history and the draft belong to the preset they were made on; read-only checks. */
  private get samePreset(): boolean {
    return this.store.preset === this.historyPreset;
  }

  get canUndo(): boolean {
    return this.samePreset && this.undoStack.length > 0 && !this.busy;
  }

  get canRedo(): boolean {
    return this.samePreset && this.redoStack.length > 0 && !this.busy;
  }

  get draftCount(): number {
    return this.samePreset ? Object.keys(this.draft).length + (this.layoutChanging ? 1 : 0) : 0;
  }

  /** The layout playing: its own when the store has none. */
  get currentLayout(): LayoutSpec | null {
    return this.store.layout ?? this.store.presetLayout;
  }

  /** The layout shown: the draft's, else the one playing. */
  get shownLayout(): LayoutSpec | null {
    return this.layoutDraft ?? this.currentLayout;
  }

  /** The draft changes the layout. */
  get layoutChanging(): boolean {
    const own = this.store.presetLayout;
    return this.samePreset && own !== null && this.layoutDraft !== null && !sameLayout(this.layoutDraft, this.store.layout ?? undefined, own);
  }

  /** The edits (applied or in the draft) the draft's new layout would drop: indexed by cylinder. */
  get dropped(): EngineEdit[] {
    if (!this.layoutChanging) return [];
    return editsForLayout(mergeEdits(this.store.engineEdits, Object.values(this.draft)), this.store.engineFields).dropped;
  }

  /**
   * The user picked part of a layout: a family, a count, a crank or a bank angle. The rest
   * follows what the core builds: a family's count kept if it has it (else its nearest), the
   * crank kept if the count has it (else the usual one), a V's angle kept in its range (else the
   * usual one). Back to the layout playing, it leaves the draft.
   */
  async chooseLayout(change: Partial<LayoutSpec>): Promise<void> {
    const from = this.shownLayout;
    if (!from) return;
    this.followPreset();
    const family = change.family ?? from.family;
    const counts = this.store.layouts.filter((l) => l.family === family);
    if (!counts.length) return;
    const wanted = change.cylinders ?? from.cylinders;
    const entry = counts.reduce((best, l) => (Math.abs(l.cylinders - wanted) < Math.abs(best.cylinders - wanted) ? l : best), counts[0]);
    const crankName = change.crank ?? from.crank;
    const crank = entry.cranks.find((c) => c.crank === crankName) ?? entry.cranks.find((c) => c.crank === entry.usualCrank) ?? entry.cranks[0];
    let bankAngle = crank.angleUsual;
    if (family === "v") {
      const asked = change.bankAngle ?? (from.family === "v" ? from.bankAngle : crank.angleUsual);
      bankAngle = asked >= crank.angleMin && asked <= crank.angleMax ? asked : crank.angleUsual;
      if (change.cylinders !== undefined && change.bankAngle === undefined) bankAngle = crank.angleUsual;
    }
    const next: LayoutSpec = { family, cylinders: entry.cylinders, crank: crank.crank, bankAngle };
    const own = this.store.presetLayout;
    this.layoutDraft = own && sameLayout(next, this.store.layout ?? undefined, own) ? null : next;
    await this.refreshPreview();
  }

  /** The fields by section; one the sections don't name goes under "Other". */
  sections(): { title: string; fields: EngineFieldInfo[] }[] {
    const banksField = this.store.engineFields.find((f) => f.name === "banks");
    const banks = banksField ? (this.store.engineValues?.[banksField.id]?.[0] ?? 0) : 0;
    const hidden = (f: EngineFieldInfo) => HIDDEN.some((h) => h.name === f.name && (h.banks === 0 || h.banks === banks));
    const all = this.store.engineFields.filter((f) => !hidden(f));
    const named = new Set(SECTIONS.flatMap((s) => s.fields));
    const out = SECTIONS.map((s) => ({
      title: s.title,
      fields: s.fields.map((n) => all.find((f) => f.name === n)).filter((f): f is EngineFieldInfo => !!f),
    }));
    const other = all.filter((f) => !named.has(f.name));
    if (other.length) out.push({ title: "Other", fields: other });
    return out.filter((s) => s.fields.length);
  }

  /** A field's values as shown: the draft's where it has one, else the preview's, else the engine's. */
  values(field: EngineFieldInfo): number[] {
    const source = (this.draftCount && this.preview) || this.store.engineValues;
    const base = source?.[field.id] ?? [];
    if (!this.samePreset) return base;
    // with a new layout, a draft value indexed by the old layout's cylinders isn't shown
    if (this.layoutChanging && !keptAcrossLayouts(field)) return base;
    return base.map((v, i) => this.draft[key(field.name, i)]?.value ?? v);
  }

  /** The user set a field's value (SI): live ones change the engine now, the others join the draft. */
  async set(field: EngineFieldInfo, index: number, value: number): Promise<void> {
    if (field.flags.includes("readonly") || !Number.isFinite(value)) return;
    const edit: EngineEdit = { field: field.name, index, value };
    if (field.flags.includes("live")) {
      this.followPreset();
      const before = this.built();
      try {
        await this.store.editLive(edit);
        this.remember(before);
        this.error = null;
      } catch (e) {
        this.error = describe(e);
      }
      return;
    }
    this.followPreset();
    this.draft = { ...this.draft, [key(field.name, index)]: edit };
    await this.refreshPreview();
  }

  /** Every index of a per-bank or per-cylinder field at once. */
  async setAll(field: EngineFieldInfo, value: number): Promise<void> {
    const n = this.values(field).length;
    if (field.flags.includes("live")) {
      for (let i = 0; i < n; i++) await this.set(field, i, value);
      return;
    }
    this.followPreset();
    const next = { ...this.draft };
    for (let i = 0; i < n; i++) next[key(field.name, i)] = { field: field.name, index: i, value };
    this.draft = next;
    await this.refreshPreview();
  }

  discard(): void {
    this.draft = {};
    this.layoutDraft = null;
    this.preview = null;
    this.previewError = null;
    this.previewSeq++;
  }

  /** The draft applied as one rebuild; kept, with the reason, if it is refused. */
  async apply(): Promise<void> {
    if (!this.draftCount || this.busy) return;
    this.followPreset();
    const before = this.built();
    const { edits, layout } = this.drafted();
    this.busy = true;
    try {
      await this.store.applyEdits(edits, layout);
      this.remember(before);
      this.discard();
      this.error = null;
    } catch (e) {
      this.error = describe(e);
    } finally {
      this.busy = false;
    }
  }

  async undo(): Promise<void> {
    this.followPreset();
    if (!this.canUndo) return;
    await this.travel(this.undoStack, (s) => (this.undoStack = s), (e) => (this.redoStack = [...this.redoStack, e]));
  }

  async redo(): Promise<void> {
    this.followPreset();
    if (!this.canRedo) return;
    await this.travel(this.redoStack, (s) => (this.redoStack = s), (e) => (this.undoStack = [...this.undoStack, e]));
  }

  /** Rebuilds with the top of `from`; on success the build it replaced goes to the other stack. */
  private async travel(from: Built[], setFrom: (s: Built[]) => void, pushOther: (e: Built) => void): Promise<void> {
    const target = from[from.length - 1];
    const current = this.built();
    this.busy = true;
    try {
      await this.store.applyEdits(target.edits, target.layout);
      setFrom(from.slice(0, -1));
      pushOther(current);
      this.discard();
      this.error = null;
    } catch (e) {
      this.error = describe(e);
    } finally {
      this.busy = false;
    }
  }

  private remember(before: Built): void {
    this.undoStack = [...this.undoStack, before];
    this.redoStack = [];
  }

  /** What plays now, as a step of the history. */
  private built(): Built {
    return { edits: this.store.engineEdits, layout: this.store.layout };
  }

  /** The build the draft makes: its edits on the playing ones (without those a new layout drops),
   *  and the layout (absent: unchanged; null: the preset's own). */
  private drafted(): { edits: EngineEdit[]; layout?: LayoutSpec | null } {
    const edits = mergeEdits(this.store.engineEdits, Object.values(this.draft));
    if (!this.layoutChanging) return { edits };
    const own = this.store.presetLayout!;
    const layout = sameLayout(this.layoutDraft!, undefined, own) ? null : this.layoutDraft;
    return { edits: editsForLayout(edits, this.store.engineFields).kept, layout };
  }

  /** Another preset is playing: its own history and no draft (called from actions only). */
  private followPreset(): void {
    if (this.store.preset === this.historyPreset) return;
    this.historyPreset = this.store.preset;
    this.undoStack = [];
    this.redoStack = [];
    this.draft = {};
    this.layoutDraft = null;
    this.preview = null;
    this.previewError = null;
  }

  private async refreshPreview(): Promise<void> {
    const seq = ++this.previewSeq;
    const { edits, layout } = this.drafted();
    if (!this.draftCount) {
      this.preview = null;
      this.previewError = null;
      return;
    }
    try {
      const values = await this.store.describeEdits(edits, layout);
      if (seq !== this.previewSeq) return; // a newer change asked again meanwhile
      this.preview = values;
      this.previewError = null;
    } catch (e) {
      if (seq !== this.previewSeq) return;
      this.preview = null;
      this.previewError = describe(e);
    }
  }
}

function describe(e: unknown): string {
  const code = (e as { code?: unknown } | null)?.code;
  const message = e instanceof Error ? e.message : String(e);
  return typeof code === "string" && !message.includes(code) ? `${message} (${code})` : message;
}
