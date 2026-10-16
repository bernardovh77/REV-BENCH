/**
 * rb-worklet.ts - runs on the browser's real-time audio thread.
 *
 * It compiles the C core (WebAssembly), creates a simulation, and fills every
 * audio block (128 frames) by calling rb_render_block(). Parameter changes
 * arrive as messages; telemetry goes back about 30 times per second.
 *
 * Rule for this file: process() must be fast and must not allocate. At 48 kHz
 * a 128-frame block is due every 2.7 ms, and missing that deadline is an
 * audible click.
 */
import { RbCore, type RbSim } from '../core/rb-core';
import { PROCESSOR_NAME, type FromWorklet, type ProcessorOptions, type ToWorklet } from './protocol';

class RevBenchProcessor extends AudioWorkletProcessor {
  private sim: RbSim | null = null;
  private telemetry: number[] = [];
  private blocksPerTelemetry = 1;
  private blockCount = 0;
  /** false after 'dispose': process() then returns false and the browser drops the processor. */
  private alive = true;

  constructor(options: AudioWorkletNodeOptions) {
    super();
    this.port.onmessage = (event: MessageEvent<ToWorklet>) => this.onMessage(event.data);
    try {
      const opts = options.processorOptions as ProcessorOptions;
      const core = RbCore.loadSync(opts.wasmBytes);
      this.sim = core.createSim(sampleRate, opts.oversample, 128);
      this.telemetry = new Array<number>(core.exports.rb_telemetry_count()).fill(0);
      this.blocksPerTelemetry = Math.max(1, Math.round(sampleRate / 128 / opts.telemetryHz));
      this.post({ type: 'ready', version: core.version, sampleRate, oversample: opts.oversample });
    } catch (err) {
      this.sim = null;
      this.post({ type: 'error', message: err instanceof Error ? err.message : String(err) });
    }
  }

  private post(msg: FromWorklet): void {
    this.port.postMessage(msg);
  }

  private onMessage(msg: ToWorklet): void {
    if (msg.type === 'dispose') {
      this.sim?.destroy();
      this.sim = null;
      this.alive = false;
      return;
    }
    if (!this.sim) return;
    switch (msg.type) {
      case 'set-param':
        this.sim.setParam(msg.id, msg.value);
        break;
      case 'reset':
        this.sim.reset();
        break;
    }
  }

  process(_inputs: Float32Array[][], outputs: Float32Array[][]): boolean {
    const out = outputs[0];
    const first = out?.[0];
    if (!this.sim || !out || !first) return this.alive;   // outputs stay silent

    this.sim.render(first);
    for (let ch = 1; ch < out.length; ch++) out[ch]?.set(first);

    if (++this.blockCount >= this.blocksPerTelemetry) {
      this.blockCount = 0;
      this.sim.readTelemetry(this.telemetry);
      this.post({ type: 'telemetry', values: this.telemetry });
    }
    return true;   // keep the processor alive
  }
}

registerProcessor(PROCESSOR_NAME, RevBenchProcessor);
