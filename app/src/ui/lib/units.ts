/*
 * units.ts - how the editor shows the core's SI values (stage 067): metric or imperial, for
 * display only. The core and the engine process always hold SI; a value typed in is turned back
 * into SI before it is sent. Every number shown has its unit (app-design section 11).
 */

export type UnitSystem = "metric" | "imperial";

/** A display unit: its label, and value_display = value_si * factor. */
export interface DisplayUnit {
  label: string;
  factor: number;
  /** decimals worth showing */
  digits: number;
}

const INCH = 0.0254;
const CUBIC_INCH = INCH ** 3;
const LB_FT2 = 0.45359237 * 0.3048 ** 2;

/**
 * The display unit for a core unit (rb_engine_field_unit / rb_param_unit) in a system. `field`
 * picks millimetres or litres where the size calls for them (a valve in mm, a muffler in L).
 */
export function displayUnit(siUnit: string, system: UnitSystem): DisplayUnit {
  switch (siUnit) {
    case "m":
      return system === "metric" ? { label: "mm", factor: 1000, digits: 1 } : { label: "in", factor: 1 / INCH, digits: 2 };
    case "m^3":
      return system === "metric" ? { label: "L", factor: 1000, digits: 3 } : { label: "cu in", factor: 1 / CUBIC_INCH, digits: 1 };
    case "kg m^2":
      return system === "metric" ? { label: "kg m²", factor: 1, digits: 3 } : { label: "lb ft²", factor: 1 / LB_FT2, digits: 2 };
    case "W":
      return system === "metric" ? { label: "kW", factor: 1 / 1000, digits: 2 } : { label: "hp", factor: 1 / 745.69987158227, digits: 2 };
    case "deg":
      return { label: "°", factor: 1, digits: 1 };
    case "rpm":
      return { label: "rpm", factor: 1, digits: 0 };
    default:
      return { label: siUnit, factor: 1, digits: 3 };
  }
}

/** An SI value as the system shows it. */
export function toDisplay(si: number, siUnit: string, system: UnitSystem): number {
  return si * displayUnit(siUnit, system).factor;
}

/** A shown value back in SI. */
export function fromDisplay(shown: number, siUnit: string, system: UnitSystem): number {
  return shown / displayUnit(siUnit, system).factor;
}
