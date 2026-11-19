/*
 * library.ts - the saved builds on disk (the build saver): one JSON file per build in the
 * user-data folder's `builds`, named by its id (shared/builds.ts: idOf). Only the main process
 * touches the files; the window asks through the bridge, by id, never by path. A file that
 * isn't a saved build (hand-edited, from another version) is left alone and not listed.
 */
import { mkdirSync, readdirSync, readFileSync, renameSync, unlinkSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import {
  SAVED_BUILD_VERSION,
  checkEngine,
  checkSettings,
  cleanName,
  idOf,
  isId,
  parseSaved,
  type SavedBuild,
  type SavedBuildSummary,
  type SavedEngine,
  type SavedSettings,
} from "../shared/builds";

import type { SaveResult } from "../shared/bridge";

export class BuildLibrary {
  constructor(
    private readonly folder: string,
    private readonly now: () => Date = () => new Date(),
  ) {}

  private file(id: string): string {
    return join(this.folder, `${id}.json`);
  }

  /** The saved builds, newest first. */
  list(): SavedBuildSummary[] {
    let names: string[];
    try {
      names = readdirSync(this.folder);
    } catch {
      return [];
    }
    const out: SavedBuildSummary[] = [];
    for (const name of names) {
      const id = name.endsWith(".json") ? name.slice(0, -5) : "";
      if (!isId(id)) continue;
      const saved = this.read(id);
      if (!saved) continue;
      out.push({
        id,
        name: saved.name,
        savedAt: saved.savedAt,
        preset: saved.build.preset,
        ...(saved.build.layout ? { layout: saved.build.layout } : {}),
        edits: saved.build.edits?.length ?? 0,
      });
    }
    return out.sort((a, b) => (a.savedAt < b.savedAt ? 1 : a.savedAt > b.savedAt ? -1 : a.name.localeCompare(b.name)));
  }

  /** One saved build by id, or null (no such file, or not a saved build). */
  read(id: unknown): SavedBuild | null {
    if (!isId(id)) return null;
    try {
      return parseSaved(readFileSync(this.file(id), "utf8"));
    } catch {
      return null;
    }
  }

  /** Saves a build under a name; one of the same id is replaced only when `replace` says so. */
  save(name: unknown, build: unknown, settings: unknown, replace: boolean): SaveResult {
    const clean = typeof name === "string" ? cleanName(name) : "";
    const id = idOf(clean);
    if (!clean || !id) return { ok: false, reason: "bad-name" };
    const engine: SavedEngine | null = checkEngine(build);
    if (!engine) return { ok: false, reason: "bad-build" };
    if (!replace && this.read(id)) return { ok: false, reason: "exists" };
    const kept: SavedSettings | undefined = checkSettings(settings);
    const saved: SavedBuild = {
      version: SAVED_BUILD_VERSION,
      name: clean,
      savedAt: this.now().toISOString(),
      build: engine,
      ...(kept ? { settings: kept } : {}),
    };
    mkdirSync(this.folder, { recursive: true });
    const tmp = `${this.file(id)}.tmp`;
    writeFileSync(tmp, JSON.stringify(saved, null, 2) + "\n", "utf8");
    renameSync(tmp, this.file(id));
    return { ok: true, id };
  }

  /** Deletes a saved build; 1 if there was one. */
  remove(id: unknown): boolean {
    if (!isId(id) || !this.read(id)) return false;
    try {
      unlinkSync(this.file(id));
      return true;
    } catch {
      return false;
    }
  }
}
