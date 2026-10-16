/**
 * main.ts - the stage 2 test page.
 *
 * Start creates an AudioContext and the engine node (the C core running in an
 * AudioWorklet). The pitch and level controls become parameter messages, and
 * the telemetry the core sends back drives the readouts. An AnalyserNode
 * listens to the output, so the page can draw the waveform and measure the
 * pitch that is actually coming out.
 */
import './style.css';
import wasmUrl from './wasm/rb_core.wasm?url';
import { RevBenchNode } from './audio/engine-node';
import { RbCore, type CoreInfo, type ParamInfo } from './core/rb-core';

const OVERSAMPLE = 4;
/** Slider range for the test tone: the comfortable part of the core's range. */
const UI_MIN_HZ = 40;
const UI_MAX_HZ = 2000;
/** Gentler than the core's default level (0.5): a pure sine is loud. */
const START_LEVEL = 0.25;

function byId<T extends HTMLElement>(id: string): T {
  const el = document.getElementById(id);
  if (!el) throw new Error(`missing element #${id}`);
  return el as T;
}

const ui = {
  start: byId<HTMLButtonElement>('start'),
  freq: byId<HTMLInputElement>('freq'),
  freqNum: byId<HTMLInputElement>('freq-num'),
  level: byId<HTMLInputElement>('level'),
  levelOut: byId<HTMLOutputElement>('level-out'),
  scope: byId<HTMLCanvasElement>('scope'),
  meterRms: byId<HTMLElement>('meter-rms'),
  meterPeak: byId<HTMLElement>('meter-peak'),
  rmsDb: byId<HTMLOutputElement>('rms-db'),
  peakDb: byId<HTMLOutputElement>('peak-db'),
  pitch: byId<HTMLOutputElement>('pitch'),
  time: byId<HTMLElement>('t-time'),
  steps: byId<HTMLElement>('t-steps'),
  speed: byId<HTMLElement>('t-speed'),
  nonfinite: byId<HTMLElement>('t-nonfinite'),
  rates: byId<HTMLElement>('rates'),
  status: byId<HTMLElement>('status'),
  version: byId<HTMLElement>('version'),
};

/* ---- state ------------------------------------------------------------ */

interface Running {
  ctx: AudioContext;
  engine: RevBenchNode;
  analyser: AnalyserNode;
  timeData: Float32Array<ArrayBuffer>;
  freqData: Float32Array<ArrayBuffer>;
  raf: number;
  lastPitchAt: number;
  speedRef: { sim: number; wall: number } | null;
}

let wasmBytes: ArrayBuffer | null = null;
let hzParam: ParamInfo | null = null;
let levelParam: ParamInfo | null = null;
let tlm = { time: 0, steps: 0, peak: 0, rms: 0, nonfinite: 0 };
let currentHz = 220;
let currentLevel = START_LEVEL;
let running: Running | null = null;
let busy = false;

/* ---- helpers ---------------------------------------------------------- */

function setStatus(text: string, isError = false): void {
  ui.status.textContent = text;
  ui.status.classList.toggle('error', isError);
}

function errorText(err: unknown): string {
  return err instanceof Error ? err.message : String(err);
}

function findParam(info: CoreInfo, name: string): ParamInfo {
  const param = info.params.find((p) => p.name === name);
  if (!param) throw new Error(`the core has no parameter "${name}"`);
  return param;
}

function findTelemetry(info: CoreInfo, name: string): number {
  const id = info.telemetry.indexOf(name);
  if (id < 0) throw new Error(`the core has no telemetry channel "${name}"`);
  return id;
}

/* The pitch slider is logarithmic: every octave gets the same travel. */
const sliderToHz = (s: number): number => UI_MIN_HZ * (UI_MAX_HZ / UI_MIN_HZ) ** (s / 1000);
const hzToSlider = (hz: number): number =>
  (1000 * Math.log(hz / UI_MIN_HZ)) / Math.log(UI_MAX_HZ / UI_MIN_HZ);

const formatHz = (hz: number): string => (hz < 100 ? hz.toFixed(1) : String(Math.round(hz)));

function setMeter(bar: HTMLElement, label: HTMLOutputElement, x: number): void {
  const db = x > 0 ? 20 * Math.log10(x) : -Infinity;
  const frac = Math.min(1, Math.max(0, (db + 60) / 60));   // shows -60 .. 0 dBFS
  bar.style.width = `${(frac * 100).toFixed(1)}%`;
  label.textContent = Number.isFinite(db) ? `${db.toFixed(1)} dB` : '−∞ dB';
}

/* ---- controls --------------------------------------------------------- */

function setPitch(hz: number, source: 'slider' | 'number' | 'code'): void {
  if (!Number.isFinite(hz)) return;
  currentHz = Math.min(UI_MAX_HZ, Math.max(UI_MIN_HZ, hz));
  if (source !== 'slider') ui.freq.value = String(Math.round(hzToSlider(currentHz)));
  if (source !== 'number' || currentHz !== hz) ui.freqNum.value = formatHz(currentHz);
  if (running && hzParam) running.engine.setParam(hzParam.id, currentHz);
}

function setLevel(level: number, source: 'slider' | 'code'): void {
  currentLevel = Math.min(1, Math.max(0, level));
  if (source !== 'slider') ui.level.value = String(Math.round(currentLevel * 100));
  ui.levelOut.textContent = `${Math.round(currentLevel * 100)} %`;
  if (running && levelParam) running.engine.setParam(levelParam.id, currentLevel);
}

ui.freq.addEventListener('input', () => setPitch(sliderToHz(Number(ui.freq.value)), 'slider'));
ui.freqNum.addEventListener('change', () => {
  // An emptied or invalid box reads as ''; put the current value back instead of jumping to 40 Hz.
  if (ui.freqNum.value.trim() === '') ui.freqNum.value = formatHz(currentHz);
  else setPitch(Number(ui.freqNum.value), 'number');
});
ui.level.addEventListener('input', () => setLevel(Number(ui.level.value) / 100, 'slider'));
ui.start.addEventListener('click', () => void toggle());

/* ---- start / stop ----------------------------------------------------- */

async function toggle(): Promise<void> {
  if (busy) return;
  busy = true;
  ui.start.disabled = true;
  try {
    if (running) await stop();
    else await start();
  } catch (err) {
    setStatus(`Could not start audio: ${errorText(err)}`, true);
  } finally {
    busy = false;
    ui.start.disabled = wasmBytes === null;
    ui.start.textContent = running ? 'Stop' : 'Start';
    ui.start.classList.toggle('running', running !== null);
  }
}

async function start(): Promise<void> {
  if (!wasmBytes || !hzParam || !levelParam) return;
  setStatus('Starting the audio engine…');
  // Created inside the click handler: browsers only allow audio after a user gesture.
  const ctx = new AudioContext({ latencyHint: 'interactive' });
  try {
    const engine = await RevBenchNode.create(ctx, { wasmBytes, oversample: OVERSAMPLE });
    engine.setParam(hzParam.id, currentHz);
    engine.setParam(levelParam.id, currentLevel);

    const analyser = ctx.createAnalyser();
    analyser.fftSize = 16384;              // ~0.34 s window: fine pitch resolution
    analyser.smoothingTimeConstant = 0;
    engine.node.connect(ctx.destination);
    engine.node.connect(analyser);
    await ctx.resume();

    running = {
      ctx,
      engine,
      analyser,
      timeData: new Float32Array(analyser.fftSize),
      freqData: new Float32Array(analyser.frequencyBinCount),
      raf: 0,
      lastPitchAt: 0,
      speedRef: null,
    };
    engine.onTelemetry = showTelemetry;
    engine.onError = (message) => setStatus(`Audio engine error: ${message}`, true);

    const physicsKHz = (ctx.sampleRate * OVERSAMPLE) / 1000;
    ui.rates.textContent = `${ctx.sampleRate / 1000} → ${physicsKHz} kHz`;
    const latencyMs = Math.round((ctx.baseLatency + (ctx.outputLatency || 0)) * 1000);
    setStatus(`running · core ${engine.version} in an AudioWorklet · output latency ≈ ${latencyMs} ms`);
    running.raf = requestAnimationFrame(frame);
  } catch (err) {
    await ctx.close().catch(() => undefined);
    throw err;
  }
}

async function stop(): Promise<void> {
  const r = running;
  running = null;
  if (!r) return;
  cancelAnimationFrame(r.raf);
  r.engine.dispose();
  await r.ctx.close();
  clearReadouts();
  setStatus('Stopped. Press Start to play again.');
}

function clearReadouts(): void {
  for (const el of [ui.time, ui.steps, ui.speed, ui.nonfinite]) el.textContent = '—';
  showPitch(null);
  setMeter(ui.meterRms, ui.rmsDb, 0);
  setMeter(ui.meterPeak, ui.peakDb, 0);
  drawScope(null);
}

/* ---- telemetry from the core ------------------------------------------ */

function showTelemetry(values: readonly number[]): void {
  const r = running;
  if (!r) return;
  const simTime = values[tlm.time] as number;
  ui.time.textContent = `${simTime.toFixed(2)} s`;
  ui.steps.textContent = Math.round(values[tlm.steps] as number).toLocaleString('en-US');
  ui.nonfinite.textContent = String(values[tlm.nonfinite]);
  setMeter(ui.meterRms, ui.rmsDb, values[tlm.rms] as number);
  setMeter(ui.meterPeak, ui.peakDb, values[tlm.peak] as number);

  // Simulated seconds per wall-clock second since the first message:
  // 1.00 means the core keeps up with real time.
  const wall = performance.now() / 1000;
  if (!r.speedRef) r.speedRef = { sim: simTime, wall };
  else if (wall - r.speedRef.wall > 0.5) {
    const speed = (simTime - r.speedRef.sim) / (wall - r.speedRef.wall);
    ui.speed.textContent = `${speed.toFixed(2)}×`;
  }
}

/* ---- scope and pitch -------------------------------------------------- */

function frame(now: number): void {
  const r = running;
  if (!r) return;
  r.analyser.getFloatTimeDomainData(r.timeData);
  drawScope(r);
  if (now - r.lastPitchAt > 150) {
    r.lastPitchAt = now;
    showPitch(measurePitch(r));
  }
  r.raf = requestAnimationFrame(frame);
}

/** The strongest frequency in the output, refined between FFT bins. */
function measurePitch(r: Running): number | null {
  const spec = r.freqData;
  r.analyser.getFloatFrequencyData(spec);   // dB per bin
  let k = 2;                                // skip DC
  for (let i = 3; i < spec.length - 1; i++) if ((spec[i] as number) > (spec[k] as number)) k = i;
  const a = spec[k - 1] as number;
  const b = spec[k] as number;
  const c = spec[k + 1] as number;
  if (!(b > -100)) return null;             // silence
  // A parabola through the peak bin and its neighbours finds the true peak.
  const denom = a - 2 * b + c;
  const offset = Number.isFinite(denom) && denom !== 0 ? (0.5 * (a - c)) / denom : 0;
  return ((k + offset) * r.ctx.sampleRate) / r.analyser.fftSize;
}

function showPitch(hz: number | null): void {
  ui.pitch.textContent = hz === null ? '—' : `${hz.toFixed(1)} Hz`;
  ui.pitch.dataset.hz = hz === null ? '' : hz.toFixed(2);
}

function fitCanvas(): void {
  const dpr = Math.min(2, window.devicePixelRatio || 1);
  const w = Math.round(ui.scope.clientWidth * dpr);
  const h = Math.round(ui.scope.clientHeight * dpr);
  if (w > 0 && h > 0 && (ui.scope.width !== w || ui.scope.height !== h)) {
    ui.scope.width = w;
    ui.scope.height = h;
  }
}

function drawScope(r: Running | null): void {
  const g = ui.scope.getContext('2d');
  if (!g) return;
  const w = ui.scope.width;
  const h = ui.scope.height;
  g.fillStyle = '#0a0c0f';
  g.fillRect(0, 0, w, h);
  g.strokeStyle = '#1d232c';
  g.lineWidth = 1;
  g.beginPath();
  for (let i = 1; i < 4; i++) { g.moveTo(0, (h * i) / 4); g.lineTo(w, (h * i) / 4); }
  for (let i = 1; i < 10; i++) { g.moveTo((w * i) / 10, 0); g.lineTo((w * i) / 10, h); }
  g.stroke();
  if (!r) return;

  const data = r.timeData;
  const span = Math.round(r.ctx.sampleRate * 0.02);   // 20 ms across the screen
  // Start at a rising zero crossing, so a steady tone stands still.
  let first = 0;
  for (let i = 1; i < data.length - span; i++) {
    if ((data[i - 1] as number) < 0 && (data[i] as number) >= 0) { first = i; break; }
  }
  g.strokeStyle = '#ff6a1a';
  g.lineWidth = 2;
  g.beginPath();
  for (let i = 0; i < span; i++) {
    const x = (i / (span - 1)) * w;
    const y = h / 2 - (data[first + i] as number) * (h / 2) * 0.95;
    if (i === 0) g.moveTo(x, y);
    else g.lineTo(x, y);
  }
  g.stroke();
}

/* ---- boot ------------------------------------------------------------- */

async function boot(): Promise<void> {
  fitCanvas();
  drawScope(null);
  window.addEventListener('resize', () => {
    fitCanvas();
    if (!running) drawScope(null);
  });

  try {
    const response = await fetch(wasmUrl);
    if (!response.ok) throw new Error(`HTTP ${response.status} for ${wasmUrl}`);
    const bytes = await response.arrayBuffer();
    const info = (await RbCore.load(bytes)).info();   // metadata only; the sound runs in the worklet
    hzParam = findParam(info, 'test_tone_hz');
    levelParam = findParam(info, 'test_tone_level');
    tlm = {
      time: findTelemetry(info, 'time_s'),
      steps: findTelemetry(info, 'steps'),
      peak: findTelemetry(info, 'out_peak'),
      rms: findTelemetry(info, 'out_rms'),
      nonfinite: findTelemetry(info, 'nonfinite'),
    };
    wasmBytes = bytes;
    ui.version.textContent = info.version;
    setPitch(hzParam.def, 'code');
    setLevel(START_LEVEL, 'code');
    ui.start.textContent = 'Start';
    ui.start.disabled = false;
    setStatus('Ready. Press Start.');
  } catch (err) {
    ui.start.textContent = 'Unavailable';
    setStatus(`Could not load the simulation core: ${errorText(err)}`, true);
  }
}

void boot();
