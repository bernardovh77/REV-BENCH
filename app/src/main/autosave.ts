/*
 * autosave.ts - the current build, kept in the user-data folder so a restarted engine (and the
 * next launch) comes back with it. Today a build is the preset and its quality; it grows with
 * the editor (stage 124 keeps only what the protocol can rebuild: `load-preset`).
 *
 * The file is replaced atomically (written beside it, then renamed), and a file that is
 * missing, unreadable or not what we wrote is no autosave, never an error.
 */
import { mkdirSync, readFileSync, renameSync, writeFileSync } from "node:fs";
import { dirname } from "node:path";
import type { Quality } from "../shared/addon";
import type { BuildChoice } from "../shared/process-messages";

const QUALITIES: readonly string[] = ["low", "medium", "high", "ultra", "balanced"] satisfies Quality[];

export const DEFAULT_BUILD: BuildChoice = { preset: "i4" };

export class Autosave {
  constructor(private readonly file: string) {}

  load(): BuildChoice | null {
    let parsed: unknown;
    try {
      parsed = JSON.parse(readFileSync(this.file, "utf8"));
    } catch {
      return null;
    }
    if (typeof parsed !== "object" || parsed === null) return null;
    const { version, preset, quality } = parsed as Record<string, unknown>;
    if (version !== 1 || typeof preset !== "string" || preset === "") return null;
    if (quality === undefined) return { preset };
    if (typeof quality !== "string" || !QUALITIES.includes(quality)) return { preset };
    return { preset, quality: quality as Quality };
  }

  save(choice: BuildChoice): void {
    mkdirSync(dirname(this.file), { recursive: true });
    const tmp = `${this.file}.tmp`;
    writeFileSync(tmp, JSON.stringify({ version: 1, ...choice }, null, 2) + "\n", "utf8");
    renameSync(tmp, this.file);
  }
}
