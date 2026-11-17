/*
 * editor.svelte.ts - the engine editor's state (stage 067). The fields, their units, ranges,
 * sizes and whether they are live all come from the core (addon.engineFields, the playing
 * engine's description); nothing here knows the physics. What it keeps:
 *
 *   draft     the changes to fields that need a rebuild, not applied yet; applied together as
 *             one transaction (apply-edits), or discarded. A preview of the engine they would
 *             make (describe) shows worked-out fields (the displacement) and any refusal first.
 *   live      a live field (idle speed, rev limit, ...) changes the running engine at once.
 *   history   each applied change, live or not, can be undone and redone: undo rebuilds the
 *             preset with the edits it had before. A new preset starts a new history.
 *   units     metric or imperial, for display only (units.ts).
 *
 * What fields go in which section, their labels and the names of a few integer choices are
 * presentation, kept here; a field the core adds later still shows, under "Other".
 */
import type { EngineFieldInfo } from "../../shared/addon";
import type { EngineEdit } from "../../shared/process-messages";
import type { UnitSystem } from "./units";

/** What the editor needs of the engine store. */
export interface EditorStore {
  readonly engineFields: EngineFieldInfo[];
  readonly engineValues: number[][] | null;
  readonly engineEdits: EngineEdit[];
  readonly preset: string | null;
  applyEdits(edits: EngineEdit[]): Promise<void>;
  editLive(edit: EngineEdit): Promise<void>;
  describeEdits(edits: EngineEdit[]): Promise<number[][]>;
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
      "muffler_type", "muffler_volume", "muffler_length", "tailpipe_length", "tailpipe_diameter", "outlet_radiation",
    ],
  },
  { title: "Limits and load", fields: ["idle_rpm", "rev_limit", "accessory_power"] },
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
  private undoStack = $state.raw<EngineEdit[][]>([]);
  private redoStack = $state.raw<EngineEdit[][]>([]);
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
    return this.samePreset ? Object.keys(this.draft).length : 0;
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
    return base.map((v, i) => this.draft[key(field.name, i)]?.value ?? v);
  }

  /** The user set a field's value (SI): live ones change the engine now, the others join the draft. */
  async set(field: EngineFieldInfo, index: number, value: number): Promise<void> {
    if (field.flags.includes("readonly") || !Number.isFinite(value)) return;
    const edit: EngineEdit = { field: field.name, index, value };
    if (field.flags.includes("live")) {
      this.followPreset();
      const before = this.store.engineEdits;
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
    this.preview = null;
    this.previewError = null;
    this.previewSeq++;
  }

  /** The draft applied as one rebuild; kept, with the reason, if it is refused. */
  async apply(): Promise<void> {
    if (!this.draftCount || this.busy) return;
    this.followPreset();
    const before = this.store.engineEdits;
    const edits = mergeEdits(before, Object.values(this.draft));
    this.busy = true;
    try {
      await this.store.applyEdits(edits);
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

  /** Rebuilds with the top of `from`; on success the edits it replaced go to the other stack. */
  private async travel(from: EngineEdit[][], setFrom: (s: EngineEdit[][]) => void, pushOther: (e: EngineEdit[]) => void): Promise<void> {
    const target = from[from.length - 1];
    const current = this.store.engineEdits;
    this.busy = true;
    try {
      await this.store.applyEdits(target);
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

  private remember(before: EngineEdit[]): void {
    this.undoStack = [...this.undoStack, before];
    this.redoStack = [];
  }

  /** Another preset is playing: its own history and no draft (called from actions only). */
  private followPreset(): void {
    if (this.store.preset === this.historyPreset) return;
    this.historyPreset = this.store.preset;
    this.undoStack = [];
    this.redoStack = [];
    this.draft = {};
    this.preview = null;
    this.previewError = null;
  }

  private async refreshPreview(): Promise<void> {
    const seq = ++this.previewSeq;
    const edits = mergeEdits(this.store.engineEdits, Object.values(this.draft));
    try {
      const values = await this.store.describeEdits(edits);
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
