/*
 * The generated snapshot-v2 decoder, against what tools/snapshot_v2.py reads from the same
 * bytes (the golden JSON, made by tools/gen_snapshot_ts.py) and against the contract's rules.
 */
import { readFileSync } from "node:fs";
import { describe, expect, it } from "vitest";
import {
  CYLINDER_FIELD_NAMES,
  INSTRUMENT_NAMES,
  SnapshotError,
  decodeSnapshot,
  encodeSnapshot,
  snapshotSize,
  type Snapshot,
} from "../src/shared/snapshot-v2.generated";

interface GoldenSnapshot extends Omit<Snapshot, "build_id"> {
  build_id: string;
}

const example = readFileSync(new URL("../../contracts/examples/snapshot-v2-i4-idle.bin", import.meta.url));
const golden: GoldenSnapshot[] = JSON.parse(
  readFileSync(new URL("./golden/snapshot-v2-i4-idle.json", import.meta.url), "utf8"),
);

/** The example's snapshots, cut apart by the sizes the golden file says they have. */
function exampleSnapshots(): Uint8Array[] {
  const out: Uint8Array[] = [];
  let at = 0;
  for (const g of golden) {
    out.push(new Uint8Array(example.buffer, example.byteOffset + at, g.size));
    at += g.size;
  }
  expect(at).toBe(example.byteLength);
  return out;
}

function firstSnapshot(): Snapshot {
  return decodeSnapshot(exampleSnapshots()[0]);
}

/** A copy of the first example snapshot's bytes, to break one thing in. */
function copyOfFirst(): ArrayBuffer {
  const bytes = exampleSnapshots()[0];
  return bytes.slice().buffer;
}

describe("decoding the real example (contracts/examples/snapshot-v2-i4-idle.bin)", () => {
  it("has the snapshots the golden file lists", () => {
    expect(golden.length).toBeGreaterThanOrEqual(2);
  });

  it("reads every value as snapshot_v2.py does", () => {
    exampleSnapshots().forEach((bytes, i) => {
      const { build_id, ...rest } = decodeSnapshot(bytes);
      const { build_id: goldenBuildId, ...goldenRest } = golden[i];
      expect(rest).toEqual(goldenRest);
      expect(build_id).toBe(BigInt(goldenBuildId));
    });
  });

  it("reads the instruments by name, all of the contract's 24", () => {
    const s = firstSnapshot();
    expect(Object.keys(s.instruments)).toEqual([...INSTRUMENT_NAMES]);
    expect(INSTRUMENT_NAMES).toHaveLength(24);
    expect(s.instruments.ignition).toBe(1);
  });

  it("reads each cylinder's five fields", () => {
    const s = firstSnapshot();
    expect(s.cylinder).toHaveLength(4);
    for (const row of s.cylinder) expect(Object.keys(row)).toEqual([...CYLINDER_FIELD_NAMES]);
  });

  it("reads a stream of snapshots that goes forward", () => {
    const decoded = exampleSnapshots().map((b) => decodeSnapshot(b));
    decoded.slice(1).forEach((s, i) => expect(s.sequence).toBe(decoded[i].sequence + 1));
  });

  it("accepts a view into a bigger buffer, not only a whole ArrayBuffer", () => {
    const first = exampleSnapshots()[0];
    const padded = new Uint8Array(first.byteLength + 16);
    padded.set(first, 8);
    const s = decodeSnapshot(new Uint8Array(padded.buffer, 8, first.byteLength));
    expect(s.sequence).toBe(golden[0].sequence);
  });
});

describe("encoding", () => {
  it("writes back, byte for byte, the bytes the C core wrote", () => {
    for (const bytes of exampleSnapshots()) {
      const again = new Uint8Array(encodeSnapshot(decodeSnapshot(bytes)));
      expect(Buffer.from(again).equals(Buffer.from(bytes))).toBe(true);
    }
  });

  it("makes the size the contract says: 304 + 40 n bytes", () => {
    expect(snapshotSize(4)).toBe(464);
    expect(snapshotSize(8)).toBe(624);
  });
});

describe("the contract's rules: what a reader refuses", () => {
  const refused = (bytes: ArrayBuffer, pattern: RegExp) => {
    expect(() => decodeSnapshot(bytes)).toThrow(SnapshotError);
    expect(() => decodeSnapshot(bytes)).toThrow(pattern);
  };
  const withChange = (change: (s: Snapshot) => void): ArrayBuffer => {
    const s = firstSnapshot();
    change(s);
    return encodeSnapshot(s);
  };

  it("refuses bytes shorter than the header", () => {
    refused(new ArrayBuffer(40), /shorter than the header/);
  });

  it("refuses a wrong magic", () => {
    const b = copyOfFirst();
    new DataView(b).setUint32(0, 0x12345678, true); // the magic is the first word
    refused(b, /not RBS2/);
  });

  it("refuses a version that is not 2", () => {
    const b = copyOfFirst();
    new DataView(b).setUint32(4, 3, true); // the version is the second word
    refused(b, /version 3/);
  });

  it("refuses a size that disagrees with the layout", () => {
    refused(copyOfFirst().slice(0, 440), /size/);
  });

  it("refuses a reserved word that is not 0", () => {
    const b = copyOfFirst();
    new DataView(b).setUint32(24, 1, true); // reserved0 is the seventh word
    refused(b, /reserved/);
  });

  it("refuses 0 cylinders and 17", () => {
    const b = copyOfFirst();
    new DataView(b).setUint32(12, 0, true); // the cylinder count is the fourth word
    refused(b, /0 cylinders/);
    new DataView(b).setUint32(12, 17, true);
    refused(b, /17 cylinders/);
  });

  it("refuses a phase outside [0, 4 pi)", () => {
    refused(withChange((s) => (s.phase_rad = 4 * Math.PI)), /phase/);
    refused(withChange((s) => (s.phase_rad = -0.1)), /phase/);
  });

  it("refuses a negative speed and a stopped clock", () => {
    refused(withChange((s) => (s.speed_rad_s = -1)), /speed/);
    refused(withChange((s) => (s.physics_hz = 0)), /physics_hz/);
  });

  it("refuses an instrument that is not a number", () => {
    refused(withChange((s) => (s.instruments.map = Number.NaN)), /instrument/);
    refused(withChange((s) => (s.instruments.rpm = Number.POSITIVE_INFINITY)), /instrument/);
  });

  it("refuses a cylinder with no pressure, a burn fraction over 1 or a negative lift", () => {
    refused(withChange((s) => (s.cylinder[2].pressure = 0)), /cylinder 2/);
    refused(withChange((s) => (s.cylinder[1].temperature = -5)), /cylinder 1/);
    refused(withChange((s) => (s.cylinder[0].burn = 1.5)), /burn fraction/);
    refused(withChange((s) => (s.cylinder[3].exhaust_lift = -1e-3)), /negative valve lift/);
  });

  it("refuses a counter past 2^53 instead of rounding it", () => {
    const b = encodeSnapshot({ ...firstSnapshot(), sequence: 1 });
    new DataView(b).setBigUint64(48, 2n ** 60n, true); // the sequence: the 7th 8-byte word
    refused(b, /sequence/);
  });
});
