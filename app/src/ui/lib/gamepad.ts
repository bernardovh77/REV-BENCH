/*
 * gamepad.ts - analog throttle travel from the Gamepad API (stage 066), read once a frame.
 *
 *   trigger  a controller's right trigger: button 7 of the standard mapping (0..1);
 *   analog   an analog keyboard's key. Bernardo's Razer analog optical keyboard reaches the
 *            browser through Razer Synapse's controller emulation, which maps a key's travel to
 *            some axis or button of an emulated controller. Which one depends on Synapse's
 *            setup, so it is learned, not assumed: "learn" waits for the next input that moves
 *            past half its travel and keeps that one (pad, axis or button).
 *
 * An axis rests at -1 or 0 depending on the device; a learned axis is read from where it rested
 * when learning began to where it went. Whether a native Razer SDK could read the keyboard
 * without Synapse's emulation is docs/stage-066.md's question, not this file's.
 */

/** The parts of a Gamepad this reads (so tests can make their own). */
export interface PadLike {
  index: number;
  id: string;
  connected: boolean;
  mapping: string;
  axes: readonly number[];
  buttons: readonly { value: number; pressed: boolean }[];
}

export type PadSource =
  | { kind: "button"; pad: number; index: number }
  | { kind: "axis"; pad: number; index: number; rest: number; full: number };

/** The standard mapping's right trigger. */
export const RIGHT_TRIGGER = 7;
/** Learning keeps an input once it moves this far from rest (of its full travel). */
const LEARN_THRESHOLD = 0.5;

export class GamepadReader {
  /** the analog style's input, once learned */
  source: PadSource | null = null;
  learning = false;
  private rest: { pad: number; axes: number[]; buttons: number[] }[] = [];

  constructor(private readonly getPads: () => readonly (PadLike | null)[]) {}

  /** The connected controllers' names (for the interface). */
  pads(): string[] {
    return this.list().map((p) => p.id);
  }

  /** The right trigger's travel on the first controller with the standard mapping, or null. */
  trigger(): number | null {
    const pad = this.list().find((p) => p.mapping === "standard" && p.buttons.length > RIGHT_TRIGGER);
    return pad ? pad.buttons[RIGHT_TRIGGER].value : null;
  }

  /** Starts learning: what every input reads now is its rest. */
  learn(): void {
    this.learning = true;
    this.rest = this.list().map((p) => ({ pad: p.index, axes: [...p.axes], buttons: p.buttons.map((b) => b.value) }));
  }

  cancelLearning(): void {
    this.learning = false;
  }

  /** The analog style's travel, 0..1, or null (nothing learned, or its controller is gone).
   *  While learning, it also watches for the input that moves, and then keeps it. */
  analog(): number | null {
    if (this.learning) this.watch();
    const s = this.source;
    if (!s) return null;
    const pad = this.list().find((p) => p.index === s.pad);
    if (!pad) return null;
    if (s.kind === "button") return pad.buttons[s.index]?.value ?? null;
    const v = pad.axes[s.index];
    if (v === undefined) return null;
    return Math.min(1, Math.max(0, (v - s.rest) / (s.full - s.rest)));
  }

  private watch(): void {
    for (const pad of this.list()) {
      const rest = this.rest.find((r) => r.pad === pad.index);
      if (!rest) {
        // a controller connected while learning: what it reads now is its rest
        this.rest.push({ pad: pad.index, axes: [...pad.axes], buttons: pad.buttons.map((b) => b.value) });
        continue;
      }
      for (let i = 0; i < pad.buttons.length; i++) {
        if (pad.buttons[i].value - (rest.buttons[i] ?? 0) >= LEARN_THRESHOLD) {
          return this.keep({ kind: "button", pad: pad.index, index: i });
        }
      }
      for (let i = 0; i < pad.axes.length; i++) {
        const r = rest.axes[i] ?? 0;
        const moved = pad.axes[i] - r;
        // an axis spans 2 (-1..1): half its travel from rest is 1 from -1, 0.5 from 0 either way
        const span = r <= -0.9 ? 2 : 1;
        if (Math.abs(moved) >= LEARN_THRESHOLD * span) {
          const full = r <= -0.9 ? 1 : Math.sign(moved);
          return this.keep({ kind: "axis", pad: pad.index, index: i, rest: r, full });
        }
      }
    }
  }

  private keep(source: PadSource): void {
    this.source = source;
    this.learning = false;
  }

  private list(): PadLike[] {
    return this.getPads().filter((p): p is PadLike => !!p && p.connected);
  }
}
