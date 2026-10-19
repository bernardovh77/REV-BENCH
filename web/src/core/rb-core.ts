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
  rb_preset_count(): number;
  rb_preset_key(preset: number): number;
  rb_preset_name(preset: number): number;
  rb_load_preset(sim: number, preset: number): number;
  rb_get_preset(sim: number): number;
  rb_get_engine(sim: number, field: number, index: number): number;
  rb_set_engine(sim: number, field: number, index: number, value: number): number;
  rb_engine_field_size(sim: number, field: number): number;
  rb_engine_field_count(): number;
  rb_engine_field_name(field: number): number;
  rb_engine_field_unit(field: number): number;
  rb_engine_field_flags(field: number): number;
  rb_engine_field_min(field: number): number;
  rb_engine_field_max(field: number): number;
  rb_get_cylinder(sim: number, cylinder: number, field: number): number;
  rb_cylinder_field_count(): number;
  rb_cylinder_field_name(field: number): number;
  rb_cylinder_field_unit(field: number): number;
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
  'rb_preset_count', 'rb_preset_key', 'rb_preset_name', 'rb_load_preset', 'rb_get_preset',
  'rb_get_engine', 'rb_set_engine', 'rb_engine_field_size', 'rb_engine_field_count',
  'rb_engine_field_name', 'rb_engine_field_unit', 'rb_engine_field_flags',
  'rb_engine_field_min', 'rb_engine_field_max',
  'rb_get_cylinder', 'rb_cylinder_field_count', 'rb_cylinder_field_name', 'rb_cylinder_field_unit',
  'rb_sample_rate', 'rb_oversample', 'rb_alloc_floats', 'rb_free_floats',
];

/* Return codes and flags, as in rb.h. */
export const RB_OK = 0;
export const RB_CLAMPED = 1;
export const RB_ERR_ARG = -1;
export const RB_ERR_ID = -2;
export const RB_ERR_VALUE = -3;
export const RB_ERR_READONLY = -5;
export const RB_PF_LIVE = 1;
export const RB_PF_REBUILD = 2;
/* Engine field flags (RB_EF_* in rb.h). */
export const RB_EF_READONLY = 1;
export const RB_EF_INTEGER = 2;
export const RB_EF_PER_BANK = 4;
export const RB_EF_PER_CYLINDER = 8;
export const RB_EF_IN_ORDER = 16;
export const RB_EF_LIVE = 32;
/** Engine fields with one value per bank, cylinder or place in the firing order. */
export const RB_EF_ARRAY = RB_EF_PER_BANK | RB_EF_PER_CYLINDER | RB_EF_IN_ORDER;

export interface ParamInfo {
  id: number;
  name: string;
  unit: string;
  min: number;
  max: number;
  def: number;
  flags: number;
}

export interface PresetInfo {
  id: number;
  key: string;    // "i4", "v8-cross", ...
  name: string;   // "2.0 L inline-4", ...
}

/** An engine field (rb_engine_field in rb.h) or a cylinder state field (rb_cylinder_field). */
export interface FieldInfo {
  id: number;
  name: string;
  unit: string;
  flags: number;   // RB_EF_* bits; always 0 for cylinder fields
  min: number;     // the range a control should offer; NaN when it can't be set
  max: number;
}

/**
 * The engine's description, by field name: a number, or an array for the
 * fields with one value per bank, cylinder or place in the firing order.
 * Cylinders and banks count from 0, as in the core.
 */
export type EngineDescription = Record<string, number | number[]>;

export interface CoreInfo {
  version: string;
  params: ParamInfo[];
  telemetry: string[];
  presets: PresetInfo[];
  engineFields: FieldInfo[];
  cylinderFields: FieldInfo[];
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

  presets(): PresetInfo[] {
    const x = this.exports;
    const list: PresetInfo[] = [];
    for (let id = 0; id < x.rb_preset_count(); id++) {
      list.push({ id, key: this.cString(x.rb_preset_key(id)), name: this.cString(x.rb_preset_name(id)) });
    }
    return list;
  }

  /** Preset number for a key such as "v8-cross", or -1. */
  presetId(key: string): number {
    return this.presets().findIndex((p) => p.key === key);
  }

  engineFields(): FieldInfo[] {
    const x = this.exports;
    const list: FieldInfo[] = [];
    for (let id = 0; id < x.rb_engine_field_count(); id++) {
      list.push({
        id,
        name: this.cString(x.rb_engine_field_name(id)),
        unit: this.cString(x.rb_engine_field_unit(id)),
        flags: x.rb_engine_field_flags(id),
        min: x.rb_engine_field_min(id),
        max: x.rb_engine_field_max(id),
      });
    }
    return list;
  }

  /** Engine field id for a name such as "bore". Throws for an unknown name. */
  engineFieldId(name: string): number {
    const id = this.engineFields().findIndex((f) => f.name === name);
    if (id < 0) throw new Error(`the core has no engine field "${name}"`);
    return id;
  }

  cylinderFields(): FieldInfo[] {
    const x = this.exports;
    const list: FieldInfo[] = [];
    for (let id = 0; id < x.rb_cylinder_field_count(); id++) {
      list.push({
        id,
        name: this.cString(x.rb_cylinder_field_name(id)),
        unit: this.cString(x.rb_cylinder_field_unit(id)),
        flags: 0,
        min: Number.NaN,
        max: Number.NaN,
      });
    }
    return list;
  }

  info(): CoreInfo {
    return {
      version: this.version,
      params: this.params(),
      telemetry: this.telemetryNames(),
      presets: this.presets(),
      engineFields: this.engineFields(),
      cylinderFields: this.cylinderFields(),
    };
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
  private readonly cylindersField: number;
  private view: Float32Array;
  private ptr: number;

  constructor(core: RbCore, ptr: number, buf: number, bufFrames: number) {
    this.core = core;
    this.ptr = ptr;
    this.buf = buf;
    this.bufFrames = bufFrames;
    this.view = new Float32Array(core.exports.memory.buffer, buf, bufFrames);
    this.cylindersField = core.engineFieldId('cylinders');
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

  /** Loads a preset, which rebuilds the engine at rest. Returns RB_OK or an RB_ERR_* code. */
  loadPreset(preset: number): number {
    return this.core.exports.rb_load_preset(this.ptr, preset);
  }

  /** The preset the engine was built from (changing fields since doesn't change it). */
  get preset(): number {
    return this.core.exports.rb_get_preset(this.ptr);
  }

  /** How many values an engine field has: 1, the banks or the cylinders. */
  engineFieldSize(field: number): number {
    return this.core.exports.rb_engine_field_size(this.ptr, field);
  }

  /**
   * One engine field's value. `index` is the bank, cylinder or place in the
   * firing order for the fields that have one value each. NaN if unknown.
   */
  getEngine(field: number, index = 0): number {
    return this.core.exports.rb_get_engine(this.ptr, field, index);
  }

  /**
   * Changes an engine field. Returns RB_OK, RB_CLAMPED (outside the field's
   * min..max: the nearest end was used), or RB_ERR_ARG (bad index), RB_ERR_ID
   * (unknown field), RB_ERR_READONLY, or RB_ERR_VALUE when the value would
   * make an impossible engine (the engine stays as it was). Pin and bank
   * angles wrap around. Fields without RB_EF_LIVE rebuild the engine, at rest.
   */
  setEngine(field: number, index: number, value: number): number {
    return this.core.exports.rb_set_engine(this.ptr, field, index, value);
  }

  /** The whole engine description, read from the core. */
  engine(): EngineDescription {
    const out: EngineDescription = {};
    for (const f of this.core.engineFields()) {
      if (f.flags & RB_EF_ARRAY) {
        const values: number[] = [];
        for (let i = 0; i < this.engineFieldSize(f.id); i++) values.push(this.getEngine(f.id, i));
        out[f.name] = values;
      } else {
        out[f.name] = this.getEngine(f.id, 0);
      }
    }
    return out;
  }

  /** One field of one cylinder's state (cylinders count from 0). NaN if unknown. */
  getCylinder(cylinder: number, field: number): number {
    return this.core.exports.rb_get_cylinder(this.ptr, cylinder, field);
  }

  /**
   * Copies every cylinder's state into `into`, cylinder by cylinder:
   * into[c * fields + f] is field f of cylinder c. Allocates nothing.
   * Returns how many cylinders it copied (as many as fit).
   */
  readCylinders(into: Float64Array): number {
    const x = this.core.exports;
    const fields = x.rb_cylinder_field_count();
    const engineCylinders = this.getEngine(this.cylindersField) || 0;   // NaN once destroyed
    const cylinders = Math.min(engineCylinders, Math.floor(into.length / fields));
    for (let c = 0; c < cylinders; c++) {
      for (let f = 0; f < fields; f++) into[c * fields + f] = x.rb_get_cylinder(this.ptr, c, f);
    }
    return cylinders;
  }

  destroy(): void {
    if (!this.ptr) return;
    this.core.exports.rb_free_floats(this.buf);
    this.core.exports.rb_destroy(this.ptr);
    this.ptr = 0;
  }
}
