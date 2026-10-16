/**
 * engine-node.ts - the main-thread side of the audio engine.
 *
 * Loads the worklet module, creates the AudioWorkletNode that runs the C core,
 * and turns its messages into promises and callbacks.
 */
import workletUrl from './rb-worklet.ts?worker&url';
import {
  PROCESSOR_NAME,
  type FromWorklet,
  type ProcessorOptions,
  type ToWorklet,
} from './protocol';

export interface EngineOptions {
  /** rb_core.wasm bytes (copied into the worklet, the caller keeps its own). */
  wasmBytes: ArrayBuffer;
  /** Physics substeps per audio sample. Default 4. */
  oversample?: number;
  /** Telemetry messages per second. Default 30. */
  telemetryHz?: number;
  /** How long to wait for the worklet to start. Default 5 s. */
  timeoutMs?: number;
}

export class RevBenchNode {
  readonly node: AudioWorkletNode;
  /** Called with all telemetry values, indexed by telemetry id. */
  onTelemetry: ((values: readonly number[]) => void) | null = null;
  /** Called if the engine fails after it started. */
  onError: ((message: string) => void) | null = null;
  /** Core version reported by the worklet. */
  version = '';

  private started = false;
  private settle: { resolve: () => void; reject: (err: Error) => void } | null = null;

  private constructor(node: AudioWorkletNode) {
    this.node = node;
    node.port.onmessage = (event: MessageEvent<FromWorklet>) => this.handle(event.data);
    node.onprocessorerror = () => this.fail('the audio processor crashed');
  }

  /** Creates the node and waits until the core is running inside the worklet. */
  static async create(ctx: BaseAudioContext, options: EngineOptions): Promise<RevBenchNode> {
    await ctx.audioWorklet.addModule(workletUrl);
    const processorOptions: ProcessorOptions = {
      wasmBytes: options.wasmBytes,
      oversample: options.oversample ?? 4,
      telemetryHz: options.telemetryHz ?? 30,
    };
    const node = new AudioWorkletNode(ctx, PROCESSOR_NAME, {
      numberOfInputs: 0,
      numberOfOutputs: 1,
      outputChannelCount: [1],   // mono; the destination copies it to both speakers
      processorOptions,
    });
    const engine = new RevBenchNode(node);
    await engine.waitUntilStarted(options.timeoutMs ?? 5000);
    return engine;
  }

  setParam(id: number, value: number): void {
    this.send({ type: 'set-param', id, value });
  }

  reset(): void {
    this.send({ type: 'reset' });
  }

  /** Stops the engine: the worklet frees the core and stops processing. */
  dispose(): void {
    this.onTelemetry = null;
    this.onError = null;
    this.send({ type: 'dispose' });
    this.node.port.onmessage = null;
    this.node.disconnect();
  }

  private send(msg: ToWorklet): void {
    this.node.port.postMessage(msg);
  }

  private waitUntilStarted(timeoutMs: number): Promise<void> {
    if (this.started) return Promise.resolve();
    return new Promise<void>((resolve, reject) => {
      const timer = setTimeout(
        () => reject(new Error('the audio engine did not start in time')),
        timeoutMs,
      );
      this.settle = {
        resolve: () => { clearTimeout(timer); resolve(); },
        reject: (err) => { clearTimeout(timer); reject(err); },
      };
    });
  }

  private handle(msg: FromWorklet): void {
    switch (msg.type) {
      case 'ready':
        this.started = true;
        this.version = msg.version;
        this.settle?.resolve();
        this.settle = null;
        break;
      case 'telemetry':
        this.onTelemetry?.(msg.values);
        break;
      case 'error':
        this.fail(msg.message);
        break;
    }
  }

  private fail(message: string): void {
    if (this.settle) {
      this.settle.reject(new Error(message));
      this.settle = null;
    } else {
      this.onError?.(message);
    }
  }
}
