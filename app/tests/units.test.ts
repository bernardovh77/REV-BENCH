/* The editor's units (stage 067): display only, and every round trip exact enough. */
import { describe, expect, it } from "vitest";
import { displayUnit, fromDisplay, toDisplay } from "../src/ui/lib/units";

describe("units", () => {
  it("shows lengths in mm or inches, volumes in L or cubic inches", () => {
    expect(toDisplay(0.086, "m", "metric")).toBeCloseTo(86, 9);
    expect(toDisplay(0.0254, "m", "imperial")).toBeCloseTo(1, 12);
    expect(toDisplay(0.005, "m^3", "metric")).toBeCloseTo(5, 9); // a 5 L V8
    expect(toDisplay(0.005, "m^3", "imperial")).toBeCloseTo(305.1, 1); // 305 cu in
    expect(displayUnit("m", "imperial").label).toBe("in");
  });

  it("power in kW or hp, inertia in kg m² or lb ft², and the rest as the core has them", () => {
    expect(toDisplay(745.69987158227, "W", "imperial")).toBeCloseTo(1, 12);
    expect(toDisplay(1, "kg m^2", "imperial")).toBeCloseTo(23.73, 2);
    expect(displayUnit("deg", "imperial").label).toBe("°");
    expect(displayUnit("rpm", "metric").label).toBe("rpm");
    expect(displayUnit("Pa s/m", "metric")).toMatchObject({ label: "Pa s/m", factor: 1 });
  });

  it("turns a shown value back into SI", () => {
    for (const [unit, system] of [["m", "imperial"], ["m^3", "metric"], ["W", "imperial"], ["kg m^2", "imperial"]] as const) {
      expect(fromDisplay(toDisplay(0.1234, unit, system), unit, system)).toBeCloseTo(0.1234, 12);
    }
  });
});
