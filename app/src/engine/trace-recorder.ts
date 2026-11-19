/*
 * trace-recorder.ts - the engine's drive, written as a command trace (stage 066). Wraps the
 * add-on's Host: every parameter set and every swap the render thread acknowledged as applied is
 * written as host/include/rb_host_trace.h's `at FRAME ...` line, at the audio frame its ack
 * gives (the first frame of the block it took effect in, where rb_host_render sends it too).
 * With the host's capture tap, the trace and the audio the device was given make the stage's
 * exit evidence: rb_host_render plays the trace offline, and the two must be the same.
 *
 * Only what reaches the engine through these calls is recorded: parameters and swaps, which is
 * all the app sends today. A device change or a sleep starts a new epoch the trace can't hold;
 * the recorder notes it, and a trace with one is not evidence (`interrupted`).
 */
import { SCENES, type Ack, type BuildSpec, type Crank, type EngineFieldInfo, type Family, type Host, type ParamInfo, type PresetInfo, type Quality } from "../shared/addon";

/** RB_QUALITY_*, in order: the trace's names (host/src/trace.c). */
const QUALITY_INDEX: Record<Quality, number> = { low: 0, medium: 1, high: 2, ultra: 3, balanced: 4, lean: 5 };
/** rb.h's RB_FAMILY_* and RB_CRANK_* in order (stage 131) */
const FAMILY_INDEX: readonly Family[] = ["inline", "v", "boxer"];
const CRANK_INDEX: readonly Crank[] = ["even", "common-pin", "cross-plane", "twin-180", "twin-270"];

/** A number written so it reads back exactly (rb_host_trace.h: 17 digits). */
function exact(v: number): string {
  return Number.isInteger(v) ? String(v) : v.toPrecision(17);
}

/**
 * A swap's fade-out, in frames: host/src/host.c's fade_blocks at the default 15 ms (rounded to
 * whole 128-frame blocks, 4 to 6 of them). A swap is acknowledged when the new engine goes in,
 * after the fade-out; the trace's `at F swap` is the frame its command was taken, where the
 * fade-out starts.
 */
export function swapFadeFrames(rate: number, fadeMs = 15): number {
  const blocks = Math.min(6, Math.max(4, Math.floor((fadeMs * rate) / 1000 / 128 + 0.5)));
  return blocks * 128;
}

type Pending =
  | { kind: "set"; name: string; value: number }
  | { kind: "swap"; build: string }
  /** a dyno command (stage 068), its trace line after "at FRAME" */
  | { kind: "line"; line: string };

export class TraceRecorder {
  /** the lines after the first build, in order */
  private readonly commands: string[] = [];
  private readonly builds: string[] = [];
  private buildCount = 0;
  private readonly pending = new Map<number, Pending>();
  private readonly prepared = new Map<number, BuildSpec>();
  private readonly header: string[];
  /** a device change or a sleep happened: the trace no longer describes the audio */
  interrupted = false;

  constructor(
    private readonly params: ParamInfo[],
    private readonly presets: PresetInfo[],
    private readonly rate: number,
    initial: BuildSpec,
    /** the engine fields, for the builds' edits by name (stage 067) */
    private readonly fields: EngineFieldInfo[] = [],
  ) {
    this.header = ["rbtrace 1", `rate ${rate}`, ...this.buildLines(initial), "skip 0"];
  }

  /** The host, recording what it is told and what it acknowledges. Every call is passed on with
   *  the host itself as `this` (the add-on's methods need their own object). */
  wrap(host: Host): Host {
    const name = (id: number) => this.params.find((p) => p.id === id)?.name ?? `#${id}`;
    return {
      start: () => host.start(),
      stop: () => host.stop(),
      destroy: () => host.destroy(),
      sampleRate: () => host.sampleRate(),
      generation: () => host.generation(),
      setParam: (id, value, expected) => {
        const request = host.setParam(id, value, expected);
        this.pending.set(request, { kind: "set", name: name(id), value });
        return request;
      },
      dynoAttach: (rpm, expected) => this.line(host.dynoAttach(rpm, expected), `dyno_attach ${exact(rpm)}`),
      dynoSweep: (to, rate, expected) => this.line(host.dynoSweep(to, rate, expected), `dyno_sweep ${exact(to)} ${exact(rate)}`),
      dynoClear: (expected) => this.line(host.dynoClear(expected), "dyno_clear"),
      dynoRelease: (expected) => this.line(host.dynoRelease(expected), "dyno_release"),
      setEngineLive: (field, index, value, expected) => {
        // not something the app sends yet: a trace can't name the field from here
        this.interrupted = true;
        return host.setEngineLive(field, index, value, expected);
      },
      prepare: (spec) => {
        const id = host.prepare(spec);
        this.prepared.set(id, spec);
        return id;
      },
      prepareState: (id) => host.prepareState(id),
      swap: (prepareId, expected) => {
        const request = host.swap(prepareId, expected);
        const spec = this.prepared.get(prepareId);
        if (spec) {
          const build = `b${this.buildCount + 1}`;
          this.buildCount++;
          this.builds.push(`build ${build}`, ...this.buildLines(spec), "end");
          this.pending.set(request, { kind: "swap", build });
        }
        return request;
      },
      discard: (id) => host.discard(id),
      poll: () => {
        const acks = host.poll();
        for (const ack of acks) this.ack(ack);
        return acks;
      },
      readSnapshot: () => host.readSnapshot(),
      stats: () => host.stats(),
      captureRead: (max) => host.captureRead(max),
      selectDevice: (index) => {
        this.interrupted = true;
        host.selectDevice(index);
      },
      suspend: () => {
        this.interrupted = true;
        host.suspend();
      },
      resume: () => host.resume(),
    };
  }

  /** Remembers a command's trace line until its acknowledgment dates it; the request id back. */
  private line(request: number, line: string): number {
    this.pending.set(request, { kind: "line", line });
    return request;
  }

  /** The trace for `frames` recorded frames from frame 0 (rb_host_trace.h). */
  text(frames: number): string {
    // a swap's build is defined before the line that swaps it in (the parser reads in order)
    return [...this.header, `frames ${frames}`, ...this.builds, ...this.commands, ""].join("\n");
  }

  private ack(ack: Ack): void {
    const p = this.pending.get(ack.requestId);
    if (!p) return;
    this.pending.delete(ack.requestId);
    if (ack.outcome !== "ok") return;
    if (p.kind === "set") this.commands.push(`at ${ack.audioFrame} set ${p.name} ${exact(p.value)}`);
    else if (p.kind === "line") this.commands.push(`at ${ack.audioFrame} ${p.line}`);
    else this.commands.push(`at ${ack.audioFrame - swapFadeFrames(this.rate)} swap ${p.build}`);
  }

  private buildLines(spec: BuildSpec): string[] {
    const quality = spec.quality ?? this.presets.find((p) => p.key === spec.preset)?.quality;
    const lines = [`preset ${spec.preset}`, `quality ${quality === undefined ? -1 : QUALITY_INDEX[quality]}`];
    // the layout (stage 131) by the core's numbers, then the edits by name, as rb_cli writes them
    if (spec.layout) {
      const l = spec.layout;
      lines.push(`layout ${FAMILY_INDEX.indexOf(l.family)} ${l.cylinders} ${CRANK_INDEX.indexOf(l.crank)} ${exact(l.bankAngle)}`);
    }
    for (const e of spec.edits ?? []) {
      const name = this.fields.find((f) => f.id === e.field)?.name;
      if (name) lines.push(`edit ${name} ${e.index} ${exact(e.value)}`);
    }
    for (const p of spec.params ?? []) {
      const name = this.params.find((q) => q.id === p.id)?.name;
      if (name) lines.push(`param ${name} ${exact(p.value)}`);
    }
    if (spec.scene) lines.push(`scene ${SCENES.indexOf(spec.scene as (typeof SCENES)[number])}`);
    if (spec.settleBlocks) lines.push(`settle ${spec.settleBlocks}`);
    return lines;
  }
}
