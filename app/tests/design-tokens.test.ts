/*
 * The design system's rule (src/ui/design/lint.ts): outside tokens.css, no colour, font or size
 * of the interface's own. The linter is tested on small samples, then run over every file.
 */
import { existsSync, readFileSync, readdirSync, statSync } from "node:fs";
import { join, relative, resolve } from "node:path";
import { describe, expect, it } from "vitest";
import { declarations, definedTokens, lintSource } from "../src/ui/design/lint";

const UI = resolve("src/ui");
const tokensCss = readFileSync(join(UI, "design", "tokens.css"), "utf8");
const tokens = definedTokens(tokensCss);

function filesUnder(dir: string, extensions: string[]): string[] {
  return readdirSync(dir).flatMap((name) => {
    const path = join(dir, name);
    if (statSync(path).isDirectory()) return filesUnder(path, extensions);
    return extensions.some((e) => name.endsWith(e)) ? [path] : [];
  });
}

const svelte = (css: string, markup = "") => `<script lang="ts"></script>${markup}<style>${css}</style>`;
const lint = (css: string, markup = "") => lintSource(svelte(css, markup), "svelte", tokens);
const rules = (css: string, markup = "") => lint(css, markup).map((v) => v.rule);

describe("the linter finds what the rule forbids", () => {
  it("accepts a style that uses only tokens", () => {
    expect(
      lint(`.a { color: var(--rb-color-text); padding: 0 var(--rb-space-4); font-size: var(--rb-font-size-md); border: var(--rb-border-width) solid var(--rb-color-border); width: 100%; transition: background var(--rb-duration-fast); }`),
    ).toEqual([]);
  });

  it("finds a hex colour, in any of its lengths", () => {
    for (const hex of ["#fff", "#ff0000", "#FF000080", "#f00a"]) expect(rules(`.a { color: ${hex}; }`)).toEqual(["colour"]);
  });

  it("finds a colour function", () => {
    for (const fn of ["rgb(1 2 3)", "rgba(0,0,0,.5)", "hsl(10 20% 30%)", "oklch(0.7 0.1 200)", "color-mix(in srgb, red, blue)"]) {
      expect(rules(`.a { background: ${fn}; }`)).toContain("colour");
    }
  });

  it("finds a colour name in a property that carries a colour", () => {
    expect(rules(`.a { color: red; }`)).toEqual(["colour"]);
    expect(rules(`.a { border: 1px solid tomato; }`)).toContain("colour");
    expect(rules(`.a { box-shadow: var(--rb-space-1) black; }`)).toEqual(["colour"]);
  });

  it("lets transparent, currentColor and inherit be", () => {
    expect(lint(`.a { color: inherit; background: transparent; border-color: currentColor; }`)).toEqual([]);
  });

  it("does not take a word in another property for a colour", () => {
    expect(lint(`.a { white-space: nowrap; display: grid; cursor: pointer; content: "red"; }`)).toEqual([]);
  });

  it("finds a font family and any font-size, weight or line height that isn't a token", () => {
    expect(rules(`.a { font-family: Arial, sans-serif; }`)).toEqual(["font"]);
    expect(rules(`.a { font-family: "Inter"; }`)).toEqual(["font"]);
    expect(rules(`.a { font-size: 14px; }`)).toEqual(["size"]);
    expect(rules(`.a { font-size: larger; }`)).toEqual(["size"]);
    expect(rules(`.a { font-weight: 600; }`)).toEqual(["size"]);
    expect(rules(`.a { line-height: 1.5; }`)).toEqual(["size"]);
    expect(rules(`.a { font: 12px Arial; }`)).toEqual(["font"]);
  });

  it("finds a length with a unit, in any property, but not 0 or a percentage", () => {
    for (const length of ["12px", "1rem", "0.5em", "10vh", "2.5pt", "-4px"]) {
      expect(rules(`.a { margin: ${length}; }`), length).toEqual(["size"]);
    }
    expect(rules(`.a { padding: 0 var(--rb-space-3) 4px; }`)).toEqual(["size"]);
    expect(lint(`.a { margin: 0; width: 50%; flex: 1; gap: 1fr; transition: opacity 200ms; z-index: 3; }`)).toEqual([]);
  });

  it("finds a length inside calc, but not a token's", () => {
    expect(rules(`.a { margin: calc(-1 * var(--rb-space-3)); }`)).toEqual([]);
    expect(rules(`.a { margin: calc(100% - 8px); }`)).toEqual(["size"]);
  });

  it("looks inside media queries and nested rules, and ignores selectors", () => {
    expect(rules(`@media (min-width: 600px) { .a:hover { color: #fff; } }`)).toEqual(["colour"]);
    expect(lint(`.a:not(.b)::after, .c[data-x="1"] { color: var(--rb-color-text); }`)).toEqual([]);
  });

  it("ignores comments", () => {
    expect(lint(`/* color: #fff; font-size: 12px; */ .a { color: var(--rb-color-text); }`)).toEqual([]);
  });

  it("finds a variable that no token defines", () => {
    const v = lint(`.a { color: var(--rb-color-txet); }`);
    expect(v).toEqual([{ rule: "unknown-token", text: "--rb-color-txet" }]);
  });

  it("finds an inline style attribute, and a style: directive with a literal", () => {
    expect(rules(``, `<div style="color: red"></div>`)).toContain("inline");
    expect(rules(``, `<div style={"width: 4px"}></div>`)).toContain("inline");
    expect(rules(``, `<div style:color="#123456"></div>`)).toContain("colour");
    expect(rules(``, `<div style:width="12px"></div>`)).toContain("size");
    expect(rules(``, `<div style:width={w}></div>`)).toEqual([]);
  });

  it("finds a colour written as a string in the script", () => {
    const source = `<script lang="ts">const c = '#ff00ff'; const d = "rgb(1,2,3)";</script>`;
    expect(lintSource(source, "svelte", tokens).map((v) => v.rule)).toEqual(["colour", "colour"]);
  });

  it("checks a plain CSS file as a whole", () => {
    expect(lintSource(`body { color: #000; }`, "css", tokens).map((v) => v.rule)).toEqual(["colour"]);
  });

  it("reads declarations, selectors apart", () => {
    expect(declarations(`.a { color: red; margin: 0 }`)).toEqual([["color", "red"], ["margin", "0"]]);
  });
});

describe("the interface obeys it", () => {
  const svelteFiles = filesUnder(UI, [".svelte"]);
  const cssFiles = filesUnder(UI, [".css"]).filter((f) => !f.endsWith("tokens.css"));

  it("finds the panels, the layout and the base components to check", () => {
    const names = svelteFiles.map((f) => relative(UI, f).replaceAll("\\", "/"));
    for (const expected of ["App.svelte", "layout/AppLayout.svelte", "panels/TopBar.svelte", "panels/EditorPanel.svelte", "panels/CentrePanel.svelte", "panels/BottomPanel.svelte", "components/Button.svelte", "components/Panel.svelte"]) {
      expect(names).toContain(expected);
    }
  });

  it.each(svelteFiles.map((f) => [relative(UI, f).replaceAll("\\", "/"), f]))("%s uses only tokens", (_name, file) => {
    expect(lintSource(readFileSync(file, "utf8"), "svelte", tokens)).toEqual([]);
  });

  it.each(cssFiles.map((f) => [relative(UI, f).replaceAll("\\", "/"), f]))("%s uses only tokens", (_name, file) => {
    expect(lintSource(readFileSync(file, "utf8"), "css", tokens)).toEqual([]);
  });

  it("builds every panel from the base components, not from raw controls", () => {
    for (const file of filesUnder(join(UI, "panels"), [".svelte"])) {
      const markup = readFileSync(file, "utf8").replace(/<style[^>]*>[\s\S]*?<\/style>/g, "").replace(/<script[^>]*>[\s\S]*?<\/script>/g, "");
      expect(markup, file).not.toMatch(/<(button|select|input|textarea)\b/); // the components are capitalised
    }
  });
});

describe("the tokens", () => {
  /** the --rb-color-* names each theme defines */
  function themeColours(selectorPattern: RegExp): Set<string> {
    const names = new Set<string>();
    for (const block of tokensCss.split("}")) {
      const [selector, body] = block.split("{");
      if (body !== undefined && selectorPattern.test(selector)) {
        for (const m of body.matchAll(/(--rb-color-[\w-]+)\s*:/g)) names.add(m[1]);
      }
    }
    return names;
  }
  const dark = themeColours(/data-theme="dark"/);
  const light = themeColours(/data-theme="light"/);

  it("define the same colours in the dark theme and the light one", () => {
    expect(dark.size).toBeGreaterThan(8);
    expect([...dark].sort()).toEqual([...light].sort());
  });

  it("make dark the default: the plain :root has the dark colours", () => {
    expect(tokensCss).toMatch(/:root,\s*:root\[data-theme="dark"\]/);
  });

  it("have a type scale, a spacing scale and Inter first in the font stack", () => {
    for (const name of ["--rb-font-size-xs", "--rb-font-size-xxl", "--rb-space-1", "--rb-space-8", "--rb-font-ui"]) {
      expect(tokens).toContain(name);
    }
    expect(tokensCss).toMatch(/--rb-font-ui:\s*"Inter"/);
  });

  it("have Inter's files installed for the weights the interface imports", () => {
    const main = readFileSync(join(UI, "main.ts"), "utf8");
    const imports = [...main.matchAll(/import "(@fontsource\/inter\/[^"]+)"/g)].map((m) => m[1]);
    expect(imports.length).toBeGreaterThanOrEqual(3);
    for (const path of imports) expect(existsSync(resolve("node_modules", path)), path).toBe(true);
  });
});
