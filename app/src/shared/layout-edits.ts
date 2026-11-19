/*
 * Which of the editor's edits survive a change of layout (stage 131), for the engine process and
 * the window alike. An edit of a whole-engine field (bore, cams, compression, the limits) means
 * the same on any layout and is kept. One indexed by cylinder, bank or place in the firing order
 * belonged to the old layout's cylinders: its index may not exist, or be another cylinder, on the
 * new one, so it is dropped (and the editor says which).
 */
import type { EngineFieldInfo, LayoutSpec } from "./addon";
import type { EngineEdit } from "./process-messages";

const INDEXED = ["per-cylinder", "per-bank", "in-order"];

/** 1 if an edit of this field means the same whatever the layout. */
export function keptAcrossLayouts(field: EngineFieldInfo | undefined): boolean {
  return field !== undefined && !field.flags.some((f) => INDEXED.includes(f));
}

/** The edits split into those a new layout keeps and those it drops, each in order. */
export function editsForLayout(edits: EngineEdit[], fields: EngineFieldInfo[]): { kept: EngineEdit[]; dropped: EngineEdit[] } {
  const kept: EngineEdit[] = [];
  const dropped: EngineEdit[] = [];
  for (const e of edits) (keptAcrossLayouts(fields.find((f) => f.name === e.field)) ? kept : dropped).push(e);
  return { kept, dropped };
}

/** 1 if two layouts are the same engine (a missing one is the preset's own `own`). */
export function sameLayout(a: LayoutSpec | undefined, b: LayoutSpec | undefined, own: LayoutSpec): boolean {
  const x = a ?? own;
  const y = b ?? own;
  return x.family === y.family && x.cylinders === y.cylinders && x.crank === y.crank && Math.abs(x.bankAngle - y.bankAngle) < 1e-9;
}

/** What's wrong with a layout's shape, before the core sees it, or null. */
export function badLayout(layout: unknown): string | null {
  const l = layout as Partial<LayoutSpec> | null;
  if (typeof l !== "object" || l === null) return "a layout is {family, cylinders, crank, bankAngle}";
  if (!["inline", "v", "boxer"].includes(l.family as string)) return `no family "${String(l.family)}"`;
  if (typeof l.cylinders !== "number" || !Number.isInteger(l.cylinders) || l.cylinders < 1) return "a layout's cylinders are a whole number from 1";
  if (!["even", "common-pin", "cross-plane", "twin-180", "twin-270"].includes(l.crank as string)) return `no crank "${String(l.crank)}"`;
  if (typeof l.bankAngle !== "number" || !Number.isFinite(l.bankAngle)) return "a layout's bank angle is a finite number";
  return null;
}

/** "V10", "Inline 3", "Boxer 6": a layout's short name. */
export function layoutName(layout: LayoutSpec): string {
  if (layout.family === "v") return `V${layout.cylinders}`;
  return `${layout.family === "inline" ? "Inline" : "Boxer"} ${layout.cylinders}`;
}
