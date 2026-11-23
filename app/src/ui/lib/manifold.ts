/*
 * manifold.ts - the boost-or-vacuum readout (stage 068, steadied in 069): the manifold's pressure
 * against standard sea-level air, every preset's surroundings.
 *
 * Bernardo saw it flick to "Boost" with the engine off (2026-11-22): a stopped engine's manifold
 * sits at the surrounding pressure, give or take a few pascals of the gas settling, so the sign
 * of the difference flipped on nothing. Within BOOST_DEADBAND_PA of the surroundings it reads
 * "Vacuum" 0; only a real boost, past the deadband, says "Boost".
 */

/** standard sea-level air, Pa */
export const AMBIENT_PA = 101325;
/** how far above the surroundings the manifold must be to read as boost, Pa (0.5 kPa) */
export const BOOST_DEADBAND_PA = 500;

export interface ManifoldReading {
  label: "Boost" | "Vacuum";
  /** Pa, never negative */
  value: number;
}

/** The readout for the manifold's absolute pressure `map` (Pa). */
export function manifoldReading(map: number): ManifoldReading {
  const above = map - AMBIENT_PA;
  if (above > BOOST_DEADBAND_PA) return { label: "Boost", value: above };
  return { label: "Vacuum", value: Math.max(0, -above) };
}
