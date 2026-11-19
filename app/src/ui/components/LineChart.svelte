<!--
  LineChart - lines of (x, y) points on shared axes (the dyno's curves, stage 068), drawn in SVG
  (sizes as attributes: the page's policy forbids inline styles). The first series is the one
  being read, drawn bold; the others are earlier runs, older dimmer. The axes take the data's
  range with a margin, and round ticks; a series without points draws nothing.
-->
<script lang="ts">
  interface Series {
    label: string;
    points: { x: number; y: number }[];
  }

  interface Props {
    title: string;
    xLabel: string;
    yLabel: string;
    /** the newest first */
    series: Series[];
    /** the x range when there's no data yet */
    xMin?: number;
    xMax?: number;
  }

  let { title, xLabel, yLabel, series, xMin = 1000, xMax = 7000 }: Props = $props();

  const W = 600;
  const H = 220;
  const L = 52;
  const R = 12;
  const T = 10;
  const B = 34;

  /** Round ticks over [lo, hi]: 4 to 8 of them at 1, 2 or 5 times a power of ten. */
  function ticks(lo: number, hi: number): number[] {
    const span = hi - lo || 1;
    const raw = span / 6;
    const p = 10 ** Math.floor(Math.log10(raw));
    const step = [1, 2, 5, 10].map((m) => m * p).find((s) => s >= raw) ?? 10 * p;
    const out: number[] = [];
    for (let v = Math.ceil(lo / step) * step; v <= hi + 1e-9; v += step) out.push(Number(v.toFixed(6)));
    return out;
  }

  let all = $derived(series.flatMap((s) => s.points));
  let xr = $derived.by(() => {
    if (!all.length) return [xMin, xMax] as const;
    const lo = Math.min(...all.map((p) => p.x));
    const hi = Math.max(...all.map((p) => p.x));
    const pad = Math.max(100, 0.05 * (hi - lo));
    return [lo - pad, hi + pad] as const;
  });
  let yr = $derived.by(() => {
    if (!all.length) return [0, 1] as const;
    const hi = Math.max(...all.map((p) => p.y));
    const lo = Math.min(0, ...all.map((p) => p.y));
    return [lo, hi > lo ? hi * 1.08 : lo + 1] as const;
  });
  let px = $derived((x: number) => L + ((x - xr[0]) / (xr[1] - xr[0])) * (W - L - R));
  let py = $derived((y: number) => H - B - ((y - yr[0]) / (yr[1] - yr[0])) * (H - T - B));
  let path = (s: Series) => s.points.map((p, i) => `${i ? "L" : "M"}${px(p.x).toFixed(1)},${py(p.y).toFixed(1)}`).join(" ");
</script>

<figure class="line-chart" aria-label={title}>
  <figcaption>{title}</figcaption>
  <svg viewBox={`0 0 ${W} ${H}`} role="img" aria-label={`${title}: ${yLabel} against ${xLabel}`}>
    {#each ticks(yr[0], yr[1]) as v (v)}
      <line class="grid" x1={L} x2={W - R} y1={py(v)} y2={py(v)} />
      <text class="tick" x={L - 6} y={py(v) + 4} text-anchor="end">{v}</text>
    {/each}
    {#each ticks(xr[0], xr[1]) as v (v)}
      <line class="grid" x1={px(v)} x2={px(v)} y1={T} y2={H - B} />
      <text class="tick" x={px(v)} y={H - B + 14} text-anchor="middle">{v}</text>
    {/each}
    <text class="axis" x={(L + W - R) / 2} y={H - 4} text-anchor="middle">{xLabel}</text>
    <text class="axis" x={12} y={(T + H - B) / 2} text-anchor="middle" transform={`rotate(-90 12 ${(T + H - B) / 2})`}>{yLabel}</text>
    {#each [...series].reverse() as s, i (s.label + i)}
      {@const age = series.length - 1 - i}
      {#if s.points.length > 1}
        <path class="series" data-age={Math.min(age, 4)} d={path(s)} />
      {:else if s.points.length === 1}
        <circle class="series-dot" data-age={Math.min(age, 4)} cx={px(s.points[0].x)} cy={py(s.points[0].y)} r="3" />
      {/if}
    {/each}
  </svg>
  {#if series.length}
    <ul class="legend">
      {#each series as s, i (s.label + i)}
        <li data-age={Math.min(i, 4)}>{s.label}</li>
      {/each}
    </ul>
  {/if}
</figure>

<style>
  .line-chart {
    margin: 0;
    min-width: 0;
  }

  figcaption {
    color: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-xs);
  }

  svg {
    display: block;
    width: 100%;
    height: var(--rb-chart-height);
  }

  .grid {
    stroke: var(--rb-color-chart-grid);
    stroke-width: 1;
  }

  .tick,
  .axis {
    fill: var(--rb-color-text-muted);
    font-size: var(--rb-font-size-xs);
  }

  .series {
    fill: none;
    stroke-width: var(--rb-chart-line-earlier);
  }

  .series[data-age="0"] {
    stroke: var(--rb-color-chart-current);
    stroke-width: var(--rb-chart-line);
  }

  .series[data-age="1"] {
    stroke: var(--rb-color-chart-earlier-1);
  }

  .series[data-age="2"] {
    stroke: var(--rb-color-chart-earlier-2);
  }

  .series[data-age="3"] {
    stroke: var(--rb-color-chart-earlier-3);
  }

  .series[data-age="4"] {
    stroke: var(--rb-color-chart-earlier-4);
  }

  .series-dot {
    fill: var(--rb-color-chart-current);
  }

  .legend {
    display: flex;
    flex-wrap: wrap;
    gap: var(--rb-space-3);
    margin: 0;
    padding: 0;
    list-style: none;
    font-size: var(--rb-font-size-xs);
  }

  .legend li::before {
    content: "— ";
  }

  .legend li[data-age="0"] {
    color: var(--rb-color-chart-current);
  }

  .legend li[data-age="1"] {
    color: var(--rb-color-chart-earlier-1);
  }

  .legend li[data-age="2"] {
    color: var(--rb-color-chart-earlier-2);
  }

  .legend li[data-age="3"] {
    color: var(--rb-color-chart-earlier-3);
  }

  .legend li[data-age="4"] {
    color: var(--rb-color-chart-earlier-4);
  }
</style>
