/*
 * qualities.ts - the qualities' names in the interface, and which is the next cheaper one (the
 * one the "can't keep up" warning offers; app-design section 11: never switched silently).
 */
import type { Quality } from "../../shared/addon";

export const QUALITY_NAMES: Record<Quality, string> = {
  low: "Low",
  medium: "Medium",
  high: "High (the reference)",
  ultra: "Ultra",
  balanced: "Balanced",
  lean: "Lean",
};

/** From the cheapest to the dearest to run. */
export const QUALITY_COST: readonly Quality[] = ["lean", "low", "balanced", "medium", "high", "ultra"];

/** The next cheaper quality than `quality`, or null for the cheapest. */
export function cheaperQuality(quality: Quality | null): Quality | null {
  if (!quality) return null;
  const i = QUALITY_COST.indexOf(quality);
  return i > 0 ? QUALITY_COST[i - 1] : null;
}
