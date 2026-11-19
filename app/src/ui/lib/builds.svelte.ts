/*
 * builds.svelte.ts - the build saver's state in the window: the saved builds' list, the name
 * being typed, saving the engine playing, loading one back, deleting one (asked twice). The
 * files are the main process's (main/library.ts), reached through the bridge; loading goes to
 * the engine as one rebuild (the preset, its layout and edits), then the engine computer's
 * settings.
 */
import type { LayoutSpec } from "../../shared/addon";
import type { SaveResult } from "../../shared/bridge";
import { cleanName, idOf, NAME_MAX, type SavedBuild, type SavedBuildSummary, type SavedEngine, type SavedSettings } from "../../shared/builds";
import type { EngineEdit } from "../../shared/process-messages";

/** The bridge's builds API (bridge.ts: RevBenchBridge.builds). */
export interface BuildsBridge {
  list(): Promise<SavedBuildSummary[]>;
  read(id: string): Promise<SavedBuild | null>;
  save(name: string, build: SavedEngine, settings: SavedSettings, replace: boolean): Promise<SaveResult>;
  remove(id: string): Promise<boolean>;
}

/** What the saver needs of the engine store. */
export interface BuildsStore {
  readonly preset: string | null;
  readonly layout: LayoutSpec | null;
  readonly engineEdits: EngineEdit[];
  readonly settings: Record<string, number>;
  /** the core's defaults of the settings a saved build keeps */
  settingDefault(name: string): number;
  loadBuild(build: SavedEngine): Promise<void>;
  setParam(name: string, value: number): Promise<void>;
}

export { NAME_MAX };

export class SavedBuilds {
  list = $state.raw<SavedBuildSummary[]>([]);
  name = $state("");
  busy = $state(false);
  /** the last save's, load's or delete's outcome, said plainly */
  message = $state<string | null>(null);
  /** the build whose Delete was pressed once: a second press deletes it */
  confirmDelete = $state<string | null>(null);

  constructor(
    private readonly bridge: BuildsBridge | null,
    private readonly store: BuildsStore,
  ) {}

  /** The saver works only in the app (not in a plain browser page). */
  get available(): boolean {
    return this.bridge !== null;
  }

  /** The typed name names a build already saved: saving replaces it. */
  get replaces(): boolean {
    const id = idOf(this.name);
    return id !== "" && this.list.some((b) => b.id === id);
  }

  get canSave(): boolean {
    return this.available && !this.busy && this.store.preset !== null && idOf(this.name) !== "";
  }

  async refresh(): Promise<void> {
    if (!this.bridge) return;
    this.list = await this.bridge.list();
  }

  /** Saves the engine playing under the typed name (replacing one of that name). */
  async save(): Promise<void> {
    if (!this.bridge || !this.canSave) return;
    const name = cleanName(this.name);
    const build: SavedEngine = {
      preset: this.store.preset!,
      ...(this.store.layout ? { layout: { ...this.store.layout } } : {}),
      ...(this.store.engineEdits.length ? { edits: this.store.engineEdits.map((e) => ({ ...e })) } : {}),
    };
    const settings: SavedSettings = {
      afr: this.store.settings.afr ?? this.store.settingDefault("afr"),
      spark_offset: this.store.settings.spark_offset ?? this.store.settingDefault("spark_offset"),
    };
    this.busy = true;
    try {
      const result = await this.bridge.save(name, build, settings, this.replaces);
      if (result.ok) {
        this.message = `Saved "${name}"`;
        this.name = "";
        await this.refresh();
      } else {
        this.message = result.reason === "exists" ? `"${name}" is already saved` : `"${name}" can't be saved (${result.reason})`;
      }
    } finally {
      this.busy = false;
    }
  }

  /** Loads a saved build: its engine as one rebuild, then its settings; the old engine plays on
   *  if the core refuses it, and the message says why. */
  async load(id: string): Promise<void> {
    if (!this.bridge || this.busy) return;
    this.busy = true;
    this.confirmDelete = null;
    try {
      const saved = await this.bridge.read(id);
      if (!saved) {
        this.message = "That build's file is gone or no longer readable";
        await this.refresh();
        return;
      }
      await this.store.loadBuild(saved.build);
      for (const name of ["afr", "spark_offset"] as const) {
        await this.store.setParam(name, saved.settings?.[name] ?? this.store.settingDefault(name));
      }
      this.message = `Loaded "${saved.name}"`;
    } catch (e) {
      this.message = `"${id}" couldn't be loaded: ${e instanceof Error ? e.message : String(e)}`;
    } finally {
      this.busy = false;
    }
  }

  /** The first press asks; the second deletes. */
  async remove(id: string): Promise<void> {
    if (!this.bridge || this.busy) return;
    if (this.confirmDelete !== id) {
      this.confirmDelete = id;
      return;
    }
    this.confirmDelete = null;
    const name = this.list.find((b) => b.id === id)?.name ?? id;
    this.message = (await this.bridge.remove(id)) ? `Deleted "${name}"` : `"${name}" was already gone`;
    await this.refresh();
  }
}
