/**
 * rb-core.ts - typed wrapper around the C simulation core (rb_core.wasm).
 *
 * The same code runs in three places:
 *   - the main thread, to read the version and parameter metadata for the UI,
 *   - the AudioWorklet, to make the sound,
 *   - Node, for the unit tests.
 * So it only uses plain WebAssembly: no TextDecoder, fetch or DOM, which the
 * AudioWorkletGlobalScope doesn't have.
 *
 * C pointers are plain numbers: byte offsets into the module's memory.
 */

/** Functions exported by rb_core.wasm. Mirrors core/include/rb.h. */
export interface RbExports {
  memory: WebAssembly.Memory;
  /** Runs C static initialisers ("reactor" modules). Call once, first. */
  _initialize?: () => void;
  rb_version(): number;
  rb_create_with(sampleRate: number, oversample: number): number;
  rb_destroy(sim: number): void;
  rb_reset(sim: number): void;
  rb_render_block(sim: number, out: number, frames: number): number;
  rb_set_param(sim: number, id: number, value: number): number;
  rb_get_param(sim: number, id: number): number;
  rb_param_count(): number;
  rb_param_name(id: number): number;
  rb_param_unit(id: number): number;
  rb_param_min(id: number): number;
  rb_param_max(id: number): number;
  rb_param_default(id: number): number;
  rb_param_flags(id: number): number;
  rb_get_telemetry(sim: number, id: number): number;
  rb_telemetry_count(): number;
  rb_telemetry_name(id: number): number;
  rb_sample_rate(sim: number): number;
  rb_oversample(sim: number): number;
  rb_alloc_floats(count: number): number;
  rb_free_floats(ptr: number): void;
}

/** Exports this wrapper relies on. The unit tests check rb_core.wasm has them all. */
export const REQUIRED_EXPORTS: readonly (keyof RbExports)[] = [
  'memory', 'rb_version', 'rb_create_with', 'rb_destroy', 'rb_reset', 'rb_render_block',
  'rb_set_param', 'rb_get_param', 'rb_param_count', 'rb_param_name', 'rb_param_unit',
  'rb_param_min', 'rb_param_max', 'rb_param_default', 'rb_param_flags',
  'rb_get_telemetry', 'rb_telemetry_count', 'rb_telemetry_name',
  'rb_sample_rate', 'rb_oversample', 'rb_alloc_floats', 'rb_free_floats',
];

/* Return codes and flags, as in rb.h. */
export const RB_OK = 0;
export const RB_CLAMPED = 1;
export const RB_ERR_ARG = -1;
export const RB_ERR_ID = -2;
export const RB_ERR_VALUE = -3;
export const RB_PF_LIVE = 1;
export const RB_PF_REBUILD = 2;

export interface ParamInfo {
  id: number;
  name: string;
  unit: string;
  min: number;
  max: number;
  def: number;
  flags: number;
}

export interface CoreInfo {
  version: string;
  params: ParamInfo[];
  telemetry: string[];
}

/**
 * Builds an import object for whatever the module asks for. rb_core.wasm is
 * built to need nothing (the tests check that); if a future libc pulls an
 * import in, the module still loads and the call fails loudly instead.
 */
export function makeImports(module: WebAssembly.Module): WebAssembly.Imports {
  const imports: Record<string, Record<string, WebAssembly.ImportValue>> = {};
  for (const imp of WebAssembly.Module.imports(module)) {
    if (imp.kind !== 'function') {
      throw new Error(`rb_core.wasm: unsupported import ${imp.module}.${imp.name} (${imp.kind})`);
    }
    const table = (imports[imp.module] ??= {});
    table[imp.name] = () => {
      throw new Error(`rb_core.wasm called unexpected import ${imp.module}.${imp.name}`);
    };
  }
  return imports;
}

/** The loaded core: metadata plus a factory for simulations. */
export class RbCore {
  readonly exports: RbExports;

  private constructor(instance: WebAssembly.Instance) {
    const x = instance.exports as unknown as RbExports;
    for (const name of REQUIRED_EXPORTS) {
      if (!(name in x)) {
        throw new Error(`rb_core.wasm has no export "${name}" (rebuild it with make wasm)`);
      }
    }
    x._initialize?.();
    this.exports = x;
  }

  /** Compiles and instantiates asynchronously (main thread, Node). */
  static async load(bytes: BufferSource): Promise<RbCore> {
    const module = await WebAssembly.compile(bytes);
    const instance = await WebAssembly.instantiate(module, makeImports(module));
    return new RbCore(instance);
  }

  /** Compiles synchronously, for the AudioWorklet constructor (it can't await). */
  static loadSync(bytes: BufferSource): RbCore {
    const module = new WebAssembly.Module(bytes);
    return new RbCore(new WebAssembly.Instance(module, makeImports(module)));
  }

  /** Reads a NUL-terminated ASCII string from the module's memory. */
  cString(ptr: number): string {
    if (!ptr) return '';
    const mem = new Uint8Array(this.exports.memory.buffer);
    let s = '';
    for (let i = ptr; i < mem.length && mem[i] !== 0; i++) s += String.fromCharCode(mem[i] as number);
    return s;
  }

  get version(): string {
    return this.cString(this.exports.rb_version());
  }

  params(): ParamInfo[] {
    const x = this.exports;
    const list: ParamInfo[] = [];
    for (let id = 0; id < x.rb_param_count(); id++) {
      list.push({
        id,
        name: this.cString(x.rb_param_name(id)),
        unit: this.cString(x.rb_param_unit(id)),
        min: x.rb_param_min(id),
        max: x.rb_param_max(id),
        def: x.rb_param_default(id),
        flags: x.rb_param_flags(id),
      });
    }
    return list;
  }

  telemetryNames(): string[] {
    const x = this.exports;
    const names: string[] = [];
    for (let id = 0; id < x.rb_telemetry_count(); id++) names.push(this.cString(x.rb_telemetry_name(id)));
    return names;
  }

  info(): CoreInfo {
    return { version: this.version, params: this.params(), telemetry: this.telemetryNames() };
  }

  /** Creates a simulation. Throws if the core rejects the configuration. */
  createSim(sampleRate: number, oversample: number, blockFrames = 128): RbSim {
    const x = this.exports;
    const sim = x.rb_create_with(sampleRate, oversample);
    if (!sim) throw new Error(`the core rejected ${sampleRate} Hz with oversample ${oversample}`);
    const buf = x.rb_alloc_floats(blockFrames);
    if (!buf) {
      x.rb_destroy(sim);
      throw new Error('rb_core.wasm is out of memory');
    }
    return new RbSim(this, sim, buf, blockFrames);
  }
}

/** One simulation running inside the core. */
export class RbSim {
  private readonly core: RbCore;
  private readonly buf: number;
  private readonly bufFrames: number;
  private view: Float32Array;
  private ptr: number;

  constructor(core: RbCore, ptr: number, buf: number, bufFrames: number) {
    this.core = core;
    this.ptr = ptr;
    this.buf = buf;
    this.bufFrames = bufFrames;
    this.view = new Float32Array(core.exports.memory.buffer, buf, bufFrames);
  }

  /**
   * Fills `out` with the next out.length samples. When out.length equals the
   * block size (128 in the worklet) nothing is allocated, which matters on the
   * real-time audio thread.
   */
  render(out: Float32Array): void {
    if (!this.ptr) {
      out.fill(0);   // destroyed: play silence, never stale memory
      return;
    }
    const x = this.core.exports;
    for (let done = 0; done < out.length; ) {
      const n = Math.min(this.bufFrames, out.length - done);
      x.rb_render_block(this.ptr, this.buf, n);
      if (this.view.buffer !== x.memory.buffer) {
        // Growing the memory detaches old views, so make a new one.
        this.view = new Float32Array(x.memory.buffer, this.buf, this.bufFrames);
      }
      if (n === this.bufFrames) out.set(this.view, done);
      else out.set(this.view.subarray(0, n), done);
      done += n;
    }
  }

  /** Returns RB_OK, RB_CLAMPED or an RB_ERR_* code. */
  setParam(id: number, value: number): number {
    return this.core.exports.rb_set_param(this.ptr, id, value);
  }

  getParam(id: number): number {
    return this.core.exports.rb_get_param(this.ptr, id);
  }

  telemetry(id: number): number {
    return this.core.exports.rb_get_telemetry(this.ptr, id);
  }

  /** Copies every telemetry value into `into` (indexed by telemetry id). */
  readTelemetry(into: number[]): void {
    const x = this.core.exports;
    for (let id = 0; id < into.length; id++) into[id] = x.rb_get_telemetry(this.ptr, id);
  }

  reset(): void {
    this.core.exports.rb_reset(this.ptr);
  }

  destroy(): void {
    if (!this.ptr) return;
    this.core.exports.rb_free_floats(this.buf);
    this.core.exports.rb_destroy(this.ptr);
    this.ptr = 0;
  }
}
