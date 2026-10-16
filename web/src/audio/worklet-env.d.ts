/**
 * Globals of the AudioWorkletGlobalScope, which TypeScript's DOM library
 * doesn't describe. They only exist inside rb-worklet.ts at run time:
 * don't use them from main-thread code.
 */
declare const sampleRate: number;
declare const currentTime: number;
declare const currentFrame: number;

declare class AudioWorkletProcessor {
  readonly port: MessagePort;
  constructor(options?: AudioWorkletNodeOptions);
}

declare function registerProcessor(
  name: string,
  processorCtor: new (options: AudioWorkletNodeOptions) => AudioWorkletProcessor,
): void;
