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
import { SCENES, type Quality } from "../shared/addon";
import type { BuildChoice } from "../shared/process-messages";

const QUALITIES: readonly string[] = ["low", "medium", "high", "ultra", "balanced", "lean"] satisfies Quality[];

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
    const { version, preset, quality, scene, latencyMs } = parsed as Record<string, unknown>;
    if (version !== 1 || typeof preset !== "string" || preset === "") return null;
    // a quality or scene it doesn't know is dropped, the preset kept
    const known = <T extends string>(v: unknown, list: readonly T[]): v is T => typeof v === "string" && (list as readonly string[]).includes(v);
    return {
      preset,
      ...(known(quality, QUALITIES as readonly Quality[]) ? { quality } : {}),
      ...(known(scene, SCENES) ? { scene } : {}),
      ...(typeof latencyMs === "number" && Number.isInteger(latencyMs) && latencyMs >= 10 && latencyMs <= 80 ? { latencyMs } : {}),
    };
  }

  save(choice: BuildChoice): void {
    mkdirSync(dirname(this.file), { recursive: true });
    const tmp = `${this.file}.tmp`;
    writeFileSync(tmp, JSON.stringify({ version: 1, ...choice }, null, 2) + "\n", "utf8");
    renameSync(tmp, this.file);
  }
}
