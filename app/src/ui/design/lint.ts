/*
 * lint.ts - the design system's rule, as code: outside tokens.css, no colour, no font and no
 * size of the interface's own. Panels, the layout, App.svelte and the base components say
 * `var(--rb-...)`; a new look changes tokens.css (and the base components), not them.
 * tests/design-tokens.test.ts runs this over every .svelte and .css file of src/ui and fails on
 * the first violation; it is not part of the build, so a violation can't ship.
 *
 * What counts:
 *   colour    a hex colour, rgb()/rgba()/hsl()/hsla()/hwb()/lab()/lch()/oklab()/oklch()/color(),
 *             or a CSS colour name, in a style block, a style attribute or a string in a script
 *   font      a `font-family` or `font` shorthand that isn't a var() (or inherit)
 *   size      a length with a unit (px, rem, em, pt, vh, vw, ...) other than 0, in a style block,
 *             and `font-size`, `font-weight`, `line-height`, `letter-spacing` that aren't var()
 *   inline    a `style="..."` attribute (also forbidden by the content security policy)
 *   unknown   a var(--rb-...) that tokens.css does not define
 */

export interface Violation {
  rule: "colour" | "font" | "size" | "inline" | "unknown-token";
  /** the offending text */
  text: string;
}

const COLOUR_NAMES = new Set(
  (
    "aliceblue antiquewhite aqua aquamarine azure beige bisque black blanchedalmond blue blueviolet brown " +
    "burlywood cadetblue chartreuse chocolate coral cornflowerblue cornsilk crimson cyan darkblue darkcyan " +
    "darkgoldenrod darkgray darkgreen darkgrey darkkhaki darkmagenta darkolivegreen darkorange darkorchid " +
    "darkred darksalmon darkseagreen darkslateblue darkslategray darkslategrey darkturquoise darkviolet " +
    "deeppink deepskyblue dimgray dimgrey dodgerblue firebrick floralwhite forestgreen fuchsia gainsboro " +
    "ghostwhite gold goldenrod gray green greenyellow grey honeydew hotpink indianred indigo ivory khaki " +
    "lavender lavenderblush lawngreen lemonchiffon lightblue lightcoral lightcyan lightgoldenrodyellow " +
    "lightgray lightgreen lightgrey lightpink lightsalmon lightseagreen lightskyblue lightslategray " +
    "lightslategrey lightsteelblue lightyellow lime limegreen linen magenta maroon mediumaquamarine " +
    "mediumblue mediumorchid mediumpurple mediumseagreen mediumslateblue mediumspringgreen mediumturquoise " +
    "mediumvioletred midnightblue mintcream mistyrose moccasin navajowhite navy oldlace olive olivedrab " +
    "orange orangered orchid palegoldenrod palegreen paleturquoise palevioletred papayawhip peachpuff peru " +
    "pink plum powderblue purple rebeccapurple red rosybrown royalblue saddlebrown salmon sandybrown " +
    "seagreen seashell sienna silver skyblue slateblue slategray slategrey snow springgreen steelblue tan " +
    "teal thistle tomato turquoise violet wheat white whitesmoke yellow yellowgreen"
  ).split(" "),
);

const COLOUR_FUNCTION = /\b(rgb|rgba|hsl|hsla|hwb|lab|lch|oklab|oklch|color|color-mix)\(/i;
const HEX_COLOUR = /#[0-9a-f]{3,8}\b/i;
const LENGTH = /(^|[^\w#.-])-?(\d*\.)?\d+(px|rem|em|pt|pc|cm|mm|in|vh|vw|vmin|vmax|ch|ex|lh|rlh|cqw|cqh)\b/i;
/** The properties that carry a colour, where a bare colour name is one (and a local --custom one). */
const COLOUR_PROPERTY =
  /^(color|background(-color|-image)?|border(-[\w]+)*|outline(-color)?|fill|stroke|box-shadow|text-shadow|accent-color|caret-color|text-decoration(-color)?|column-rule(-color)?|scrollbar-color|stop-color|flood-color|--[\w-]+)$/;
const VAR_ONLY =/^var\(--[\w-]+\)$/;

/** Properties whose value must be a token, whatever it looks like. */
const TOKEN_ONLY = new Set(["font-family", "font-size", "font-weight", "line-height", "letter-spacing"]);
const WORD_OK = new Set(["inherit", "initial", "unset", "revert", "currentcolor", "transparent", "none", "normal"]);

function stripComments(text: string): string {
  return text.replace(/\/\*[\s\S]*?\*\//g, "");
}

/** The declarations (property, value) in a block of CSS, skipping selectors and at-rule preludes. */
export function declarations(css: string): [string, string][] {
  const out: [string, string][] = [];
  let chunk = "";
  for (const ch of stripComments(css)) {
    if (ch === "{" || ch === "}" || ch === ";") {
      const match = ch !== "{" && /^\s*(--)?([\w-]+)\s*:\s*([\s\S]*?)\s*$/.exec(chunk);
      // custom property definitions (--x: ...) belong to tokens.css; a local one is still checked
      if (match) out.push([(match[1] ?? "") + match[2], match[3]]);
      chunk = "";
    } else {
      chunk += ch;
    }
  }
  return out;
}

function checkValue(property: string, value: string, out: Violation[]): void {
  const text = `${property}: ${value}`;
  const bare = value.replace(/var\([^)]*\)/g, "var()"); // what a var() holds is the token's business
  if (TOKEN_ONLY.has(property) && !VAR_ONLY.test(value) && !WORD_OK.has(value.toLowerCase())) {
    out.push({ rule: property === "font-family" ? "font" : "size", text });
    return;
  }
  if (property === "font" && !VAR_ONLY.test(value) && !WORD_OK.has(value.toLowerCase())) {
    out.push({ rule: "font", text });
    return;
  }
  if (HEX_COLOUR.test(bare) || COLOUR_FUNCTION.test(bare)) {
    out.push({ rule: "colour", text });
    return;
  }
  const words = bare.toLowerCase().match(/[a-z]+/g) ?? [];
  if (words.some((w) => COLOUR_NAMES.has(w)) && COLOUR_PROPERTY.test(property)) {
    out.push({ rule: "colour", text });
    return;
  }
  if (LENGTH.test(bare)) out.push({ rule: "size", text });
}

/** The tokens a file may use: every --rb-... name tokens.css defines. */
export function definedTokens(tokensCss: string): Set<string> {
  const names = new Set<string>();
  for (const match of stripComments(tokensCss).matchAll(/(--rb-[\w-]+)\s*:/g)) names.add(match[1]);
  return names;
}

/** The `<style>` blocks of a Svelte file, or the whole text of a CSS file. */
function cssOf(source: string, isSvelte: boolean): string {
  if (!isSvelte) return source;
  return [...source.matchAll(/<style[^>]*>([\s\S]*?)<\/style>/g)].map((m) => m[1]).join("\n");
}

/**
 * Checks a .svelte or .css file against the rule. `tokens` is the set tokens.css defines (to
 * catch a misspelled var()).
 */
export function lintSource(source: string, kind: "svelte" | "css", tokens: Set<string>): Violation[] {
  const out: Violation[] = [];
  const svelte = kind === "svelte";
  const css = cssOf(source, svelte);
  for (const [property, value] of declarations(css)) checkValue(property, value, out);
  for (const match of css.matchAll(/var\(\s*(--[\w-]+)/g)) {
    if (!tokens.has(match[1])) out.push({ rule: "unknown-token", text: match[1] });
  }
  if (svelte) {
    const markupAndScript = source.replace(/<style[^>]*>[\s\S]*?<\/style>/g, "");
    for (const match of markupAndScript.matchAll(/\sstyle\s*=\s*(\{[^}]*\}|"[^"]*"|'[^']*')/g)) {
      out.push({ rule: "inline", text: `style=${match[1]}` });
    }
    for (const match of markupAndScript.matchAll(/\sstyle:([\w-]+)\s*=\s*("[^"]*"|'[^']*')/g)) {
      checkValue(match[1], match[2].slice(1, -1), out);
    }
    // a colour in a string of the script (a canvas's fillStyle, a chart's series colour)
    for (const match of markupAndScript.matchAll(/(["'`])((?:\\.|(?!\1).)*)\1/g)) {
      const text = match[2];
      if (/^#[0-9a-f]{3,8}$/i.test(text) || COLOUR_FUNCTION.test(text)) out.push({ rule: "colour", text });
    }
  }
  return out;
}
