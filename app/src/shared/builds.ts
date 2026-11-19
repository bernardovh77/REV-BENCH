/*
 * builds.ts - the saved builds (the build saver): an engine the user made, kept under a name to
 * come back to. The main process keeps each in its own JSON file in the user-data folder's
 * `builds` (main/library.ts); the window lists, saves, loads and deletes them through the
 * bridge. A saved build is the engine, not the listening: the preset, its layout and edits, and
 * the engine computer's air-fuel ratio and spark offset; the quality (the machine's), the scene
 * and the listener stay as they are.
 */
import type { LayoutSpec } from "./addon";
import { badLayout } from "./layout-edits";
import type { EngineEdit } from "./process-messages";

export const SAVED_BUILD_VERSION = 1;

/** The engine part of a build, as saved. */
export interface SavedEngine {
  preset: string;
  layout?: LayoutSpec;
  edits?: EngineEdit[];
}

/** The engine computer's live settings a saved build keeps. */
export interface SavedSettings {
  afr?: number;
  spark_offset?: number;
}

/** One saved build, as its file holds it. */
export interface SavedBuild {
  version: typeof SAVED_BUILD_VERSION;
  name: string;
  /** ISO 8601, when it was saved */
  savedAt: string;
  build: SavedEngine;
  settings?: SavedSettings;
}

/** A saved build as the list shows it: its file's id (the name made safe), name and what it is. */
export interface SavedBuildSummary {
  id: string;
  name: string;
  savedAt: string;
  preset: string;
  layout?: LayoutSpec;
  edits: number;
}

/** The longest name kept. */
export const NAME_MAX = 60;

/** A name as kept: trimmed, inner runs of space made one, at most NAME_MAX; "" if nothing is left. */
export function cleanName(name: string): string {
  return name.replace(/\s+/g, " ").trim().slice(0, NAME_MAX);
}

/** The file id of a name: lower case letters and digits, words joined by "-"; "" if none. */
export function idOf(name: string): string {
  return cleanName(name)
    .normalize("NFKD")
    .replace(/[̀-ͯ]/g, "")
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, "-")
    .replace(/^-+|-+$/g, "")
    .slice(0, NAME_MAX);
}

/** 1 if an id is one idOf could make: never a path. */
export function isId(id: unknown): id is string {
  return typeof id === "string" && /^[a-z0-9]+(-[a-z0-9]+)*$/.test(id) && id.length <= NAME_MAX;
}

function isEdit(e: unknown): e is EngineEdit {
  const x = e as Partial<EngineEdit> | null;
  return typeof x === "object" && x !== null && typeof x.field === "string" && x.field !== ""
    && Number.isInteger(x.index) && (x.index as number) >= 0 && typeof x.value === "number" && Number.isFinite(x.value);
}

/** The engine part checked: a preset key, a layout of the right shape, edits well formed; null if not. */
export function checkEngine(value: unknown): SavedEngine | null {
  const b = value as Partial<SavedEngine> | null;
  if (typeof b !== "object" || b === null || typeof b.preset !== "string" || !/^[a-z0-9][a-z0-9-]*$/.test(b.preset)) return null;
  if (b.layout !== undefined && badLayout(b.layout) !== null) return null;
  if (b.edits !== undefined && !(Array.isArray(b.edits) && b.edits.every(isEdit))) return null;
  return {
    preset: b.preset,
    ...(b.layout ? { layout: { ...b.layout } } : {}),
    ...(b.edits && b.edits.length ? { edits: b.edits.map((e) => ({ field: e.field, index: e.index, value: e.value })) } : {}),
  };
}

/** The settings checked: each a finite number, unknown ones dropped. */
export function checkSettings(value: unknown): SavedSettings | undefined {
  if (typeof value !== "object" || value === null) return undefined;
  const s = value as Record<string, unknown>;
  const out: SavedSettings = {};
  if (typeof s.afr === "number" && Number.isFinite(s.afr)) out.afr = s.afr;
  if (typeof s.spark_offset === "number" && Number.isFinite(s.spark_offset)) out.spark_offset = s.spark_offset;
  return Object.keys(out).length ? out : undefined;
}

/** A file's contents as a saved build, or null if it isn't one (whether the core still builds
 *  it is the engine's to say when it is loaded). */
export function parseSaved(text: string): SavedBuild | null {
  let parsed: unknown;
  try {
    parsed = JSON.parse(text);
  } catch {
    return null;
  }
  const s = parsed as Partial<SavedBuild> | null;
  if (typeof s !== "object" || s === null || s.version !== SAVED_BUILD_VERSION) return null;
  const name = typeof s.name === "string" ? cleanName(s.name) : "";
  const build = checkEngine(s.build);
  if (!name || !idOf(name) || !build || typeof s.savedAt !== "string" || Number.isNaN(Date.parse(s.savedAt))) return null;
  const settings = checkSettings(s.settings);
  return { version: SAVED_BUILD_VERSION, name, savedAt: s.savedAt, build, ...(settings ? { settings } : {}) };
}
