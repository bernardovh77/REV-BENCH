/**
 * protocol.ts - messages between the main thread (engine-node.ts) and the
 * AudioWorklet (rb-worklet.ts), kept in one place so both sides agree.
 */

/** Name the processor is registered under. */
export const PROCESSOR_NAME = 'rev-bench';

/** Sent once, when the AudioWorkletNode is created. */
export interface ProcessorOptions {
  /** rb_core.wasm bytes; the worklet compiles its own instance. */
  wasmBytes: ArrayBuffer;
  /** Physics substeps per audio sample. */
  oversample: number;
  /** How many telemetry messages to send back per second. */
  telemetryHz: number;
}

/** Main thread -> worklet. */
export type ToWorklet =
  | { type: 'set-param'; id: number; value: number }
  | { type: 'reset' }
  /** Free the core and stop processing, so the node can be garbage collected. */
  | { type: 'dispose' };

/** Worklet -> main thread. */
export type FromWorklet =
  | { type: 'ready'; version: string; sampleRate: number; oversample: number }
  | { type: 'telemetry'; values: number[] }
  | { type: 'error'; message: string };
