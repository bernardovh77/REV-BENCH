// make-icon.mjs - turns branding/icon.svg into the files the packages need, and the three
// drafts into one preview sheet for Bernardo to choose from:
//
//   branding/icon.png               512 x 512 (Linux packages and the window icon, stage 126)
//   branding/icon.ico               16, 24, 32, 48, 64, 128 and 256 px, PNG-compressed (electron-builder, Windows)
//   branding/icon-drafts-preview.png the drafts side by side at 256, 64 and 32 px, dark and light
//
// To choose another draft: copy it over icon.svg and run `npm run icon`. The SVG is rendered by
// Chromium (Playwright, with the installed Chrome or Edge: PW_CHROMIUM names one), so no image
// library is needed. The outputs are committed; CI doesn't run this.
import { chromium } from "@playwright/test";
import { readFileSync, readdirSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const dir = resolve(dirname(fileURLToPath(import.meta.url)), "..", "branding");
const SIZES = [16, 24, 32, 48, 64, 128, 256];

async function launch() {
  if (process.env.PW_CHROMIUM) return chromium.launch({ executablePath: process.env.PW_CHROMIUM });
  for (const channel of ["chrome", "msedge"]) {
    try {
      return await chromium.launch({ channel });
    } catch {
      // try the next
    }
  }
  return chromium.launch();
}

async function render(page, svg, size) {
  await page.setViewportSize({ width: size, height: size });
  await page.setContent(
    `<html><body style="margin:0;background:transparent"><img style="display:block;width:${size}px;height:${size}px" src="data:image/svg+xml;base64,${Buffer.from(svg).toString("base64")}"></body></html>`,
  );
  await page.locator("img").evaluate((img) => img.decode());
  return page.screenshot({ omitBackground: true, type: "png", clip: { x: 0, y: 0, width: size, height: size } });
}

/** An ICO file of PNG images (Windows Vista and later read them; 256 is written as 0). */
function ico(images) {
  const head = Buffer.alloc(6 + 16 * images.length);
  head.writeUInt16LE(0, 0);
  head.writeUInt16LE(1, 2);
  head.writeUInt16LE(images.length, 4);
  let offset = head.length;
  images.forEach(({ size, png }, i) => {
    const at = 6 + 16 * i;
    head.writeUInt8(size === 256 ? 0 : size, at);
    head.writeUInt8(size === 256 ? 0 : size, at + 1);
    head.writeUInt8(0, at + 2);
    head.writeUInt8(0, at + 3);
    head.writeUInt16LE(1, at + 4);
    head.writeUInt16LE(32, at + 6);
    head.writeUInt32LE(png.length, at + 8);
    head.writeUInt32LE(offset, at + 12);
    offset += png.length;
  });
  return Buffer.concat([head, ...images.map((i) => i.png)]);
}

const browser = await launch();
try {
  const page = await browser.newPage();
  const svg = readFileSync(join(dir, "icon.svg"), "utf8");
  const images = [];
  for (const size of SIZES) images.push({ size, png: await render(page, svg, size) });
  writeFileSync(join(dir, "icon.ico"), ico(images));
  writeFileSync(join(dir, "icon.png"), await render(page, svg, 512));

  const drafts = readdirSync(dir).filter((f) => /^icon-draft-.*\.svg$/.test(f)).sort();
  const cell = (theme) =>
    drafts
      .map((f) => {
        const uri = `data:image/svg+xml;base64,${readFileSync(join(dir, f)).toString("base64")}`;
        const name = f.replace(/^icon-|\.svg$/g, "");
        return `<figure><img src="${uri}" width="256" height="256"><div class="small"><img src="${uri}" width="64" height="64"><img src="${uri}" width="32" height="32"></div><figcaption>${name}</figcaption></figure>`;
      })
      .join("");
  const sheet = (bg, fg) =>
    `<div class="row" style="background:${bg};color:${fg}">${cell()}</div>`;
  await page.setViewportSize({ width: 1000, height: 880 });
  await page.setContent(`<html><body style="margin:0;font:14px system-ui"><style>
    .row{display:flex;gap:48px;padding:32px;justify-content:center}
    figure{margin:0;text-align:center}
    .small{display:flex;gap:16px;justify-content:center;align-items:flex-end;margin-top:12px}
    figcaption{margin-top:8px}</style>${sheet("#161920", "#e6e8ec")}${sheet("#f4f5f7", "#14171c")}</body></html>`);
  await page.waitForTimeout(300);
  writeFileSync(join(dir, "icon-drafts-preview.png"), await page.screenshot({ type: "png" }));
  console.log(`wrote icon.ico (${SIZES.join(", ")} px), icon.png and icon-drafts-preview.png in ${dir}`);
} finally {
  await browser.close();
}
