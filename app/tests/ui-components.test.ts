// @vitest-environment jsdom
/*
 * The base components, mounted in jsdom: what they show and what they send back.
 */
import { afterEach, describe, expect, it, vi } from "vitest";
import { createRawSnippet, flushSync, mount, unmount, type Component } from "svelte";
import Button from "../src/ui/components/Button.svelte";
import Notice from "../src/ui/components/Notice.svelte";
import Panel from "../src/ui/components/Panel.svelte";
import Readout from "../src/ui/components/Readout.svelte";
import Select from "../src/ui/components/Select.svelte";
import Slider from "../src/ui/components/Slider.svelte";

const mounted: object[] = [];
afterEach(() => {
  while (mounted.length) unmount(mounted.pop()!);
  document.body.innerHTML = "";
});

// eslint-disable-next-line @typescript-eslint/no-explicit-any -- each component has its own props
function render(component: Component<any>, props: Record<string, unknown>) {
  const target = document.createElement("div");
  document.body.appendChild(target);
  mounted.push(mount(component, { target, props }));
  flushSync();
  return target;
}

const text = (content: string) => createRawSnippet(() => ({ render: () => `<span>${content}</span>` }));

describe("Button", () => {
  it("shows its content and sends clicks", () => {
    const onclick = vi.fn();
    const target = render(Button, { children: text("Ignition"), onclick });
    const button = target.querySelector("button")!;
    expect(button.textContent).toBe("Ignition");
    button.click();
    expect(onclick).toHaveBeenCalledTimes(1);
  });

  it("is a toggle when `pressed` is given: aria-pressed says which way", () => {
    const on = render(Button, { children: text("x"), pressed: true }).querySelector("button")!;
    const off = render(Button, { children: text("x"), pressed: false }).querySelector("button")!;
    const plain = render(Button, { children: text("x") }).querySelector("button")!;
    expect(on.getAttribute("aria-pressed")).toBe("true");
    expect(off.getAttribute("aria-pressed")).toBe("false");
    expect(plain.hasAttribute("aria-pressed")).toBe(false);
  });

  it("does not click while disabled", () => {
    const onclick = vi.fn();
    const button = render(Button, { children: text("x"), disabled: true, onclick }).querySelector("button")!;
    button.click();
    expect(onclick).not.toHaveBeenCalled();
    expect(button.disabled).toBe(true);
  });

  it("passes pointer events through (the starter is held)", () => {
    const down = vi.fn();
    const up = vi.fn();
    const button = render(Button, { children: text("x"), onpointerdown: down, onpointerup: up }).querySelector("button")!;
    button.dispatchEvent(new Event("pointerdown", { bubbles: true }));
    button.dispatchEvent(new Event("pointerup", { bubbles: true }));
    expect(down).toHaveBeenCalledTimes(1);
    expect(up).toHaveBeenCalledTimes(1);
  });

  it("never submits a form", () => {
    expect(render(Button, { children: text("x") }).querySelector("button")!.type).toBe("button");
  });

  it("takes its tone from `variant`", () => {
    expect(render(Button, { children: text("x"), variant: "danger" }).querySelector("button")!.classList.contains("danger")).toBe(true);
  });
});

describe("Select", () => {
  const options = [
    { value: "i4", label: "2.0 L inline-4" },
    { value: "v8", label: "5.0 L V8" },
  ];

  it("shows its label and options, with the value selected", () => {
    const target = render(Select, { label: "Engine", options, value: "v8" });
    expect(target.querySelector(".label")!.textContent).toBe("Engine");
    expect([...target.querySelectorAll("option")].map((o) => o.textContent)).toEqual(["2.0 L inline-4", "5.0 L V8"]);
    expect(target.querySelector("select")!.value).toBe("v8");
  });

  it("is labelled for assistive technology by its visible label", () => {
    const target = render(Select, { label: "Engine", options, value: "i4" });
    expect(target.querySelector("label")!.contains(target.querySelector("select"))).toBe(true);
  });

  it("sends the chosen value", () => {
    const onchange = vi.fn();
    const select = render(Select, { label: "Engine", options, value: "i4", onchange }).querySelector("select")!;
    select.value = "v8";
    select.dispatchEvent(new Event("change", { bubbles: true }));
    expect(onchange).toHaveBeenCalledWith("v8");
  });

  it("can be disabled", () => {
    expect(render(Select, { label: "Engine", options, value: "i4", disabled: true }).querySelector("select")!.disabled).toBe(true);
  });
});

describe("Readout", () => {
  it("shows a number with its digits and unit", () => {
    const target = render(Readout, { label: "Engine speed", value: 812.345, unit: "rpm", digits: 1 });
    expect(target.querySelector(".label")!.textContent).toBe("Engine speed");
    expect(target.querySelector(".value")!.textContent).toBe("812.3");
    expect(target.querySelector(".unit")!.textContent).toBe("rpm");
  });

  it("shows whole numbers by default and a dash for no reading", () => {
    expect(render(Readout, { label: "x", value: 812.6 }).querySelector(".value")!.textContent).toBe("813");
    expect(render(Readout, { label: "x", value: null }).querySelector(".value")!.textContent).toBe("–");
  });

  it("shows no unit element when there is no unit, and a string as it is", () => {
    const target = render(Readout, { label: "x", value: "idle" });
    expect(target.querySelector(".unit")).toBeNull();
    expect(target.querySelector(".value")!.textContent).toBe("idle");
  });
});

describe("Panel", () => {
  it("is a labelled region with a heading and its content", () => {
    const target = render(Panel, { title: "Charts", children: text("body text") });
    const region = target.querySelector("section")!;
    expect(region.getAttribute("aria-label")).toBe("Charts");
    expect(region.querySelector("h2")!.textContent).toBe("Charts");
    expect(region.textContent).toContain("body text");
  });
});

describe("Slider", () => {
  it("shows its value and range, and sends numbers as it moves", () => {
    const oninput = vi.fn();
    const target = render(Slider, { label: "Throttle", value: 0.25, oninput });
    const input = target.querySelector("input")!;
    expect(input.type).toBe("range");
    expect(Number(input.value)).toBeCloseTo(0.25);
    expect([input.min, input.max, input.step]).toEqual(["0", "1", "0.01"]);
    input.value = "0.75";
    input.dispatchEvent(new Event("input", { bubbles: true }));
    expect(oninput).toHaveBeenCalledWith(0.75);
  });

  it("can be disabled", () => {
    expect(render(Slider, { label: "Throttle", value: 0, disabled: true }).querySelector("input")!.disabled).toBe(true);
  });
});

describe("Notice", () => {
  it("is announced at once when it is a danger, politely otherwise", () => {
    expect(render(Notice, { tone: "danger", children: text("x") }).querySelector(".notice")!.getAttribute("role")).toBe("alert");
    expect(render(Notice, { tone: "warning", children: text("x") }).querySelector(".notice")!.getAttribute("role")).toBe("status");
    expect(render(Notice, { children: text("x") }).querySelector(".notice")!.getAttribute("role")).toBe("status");
  });
});
