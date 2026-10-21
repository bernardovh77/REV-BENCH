#!/usr/bin/env python3
"""
orders.py - how much of an engine's sound is between its firing orders.

    python3 tools/orders.py CYL WAV@T0-T1@RPMLO-RPMHI:LABEL ...

For each steady stretch (t0..t1 seconds) of a WAV, a render or a recording of
a real engine (a bare id is renders/real/ID.wav, from compare_real.py fetch),
of an engine of CYL cylinders: its speed, found within RPMLO..RPMHI from the
firing frequency's harmonics, then the energy of each engine order (0.5, 1,
1.5, ... 16: per crank turn) and

  firing   the share of it on the firing orders (CYL/2 and its multiples)
  others   the orders between them, up to four times the firing order, against
           the firing orders (dB): how differently the cylinders sound. When
           every firing sounds the same there is nothing between the firing
           orders: a pure note. Real engines read -12 to +11 dB (stage 12c:
           the cylinders' own mixtures and burns, unequal headers, the banks of
           a V8 heard apart); REV-BENCH's four read -26 at 3000 rpm and -30
           idling before stage 12c.
  strongest the seven strongest orders, and their level against the strongest

PCM16 or float32 WAVs, as compare_real.py. Needs numpy.
"""
import os
import sys

import numpy as np

from compare_real import REAL_DIR, read_wav


def orders(path, cyl, t0, t1, rpm_lo, rpm_hi):
    x, fs = read_wav(path)
    x = x[int(t0 * fs):int(t1 * fs)]
    x = x - x.mean()
    power = np.abs(np.fft.rfft(x * np.hanning(len(x)))) ** 2
    f = np.fft.rfftfreq(len(x), 1.0 / fs)

    def comb(f0, n=8):   # the firing frequency's first n harmonics, within 1 % each
        return sum(power[(f > k * f0 * 0.99) & (f < k * f0 * 1.01)].sum() for k in range(1, n + 1))

    candidates = np.arange(rpm_lo * cyl / 120.0, rpm_hi * cyl / 120.0, 0.05)
    firing = candidates[np.argmax([comb(c) for c in candidates])]
    turn = firing * 2.0 / cyl                      # order 1: once per crank turn
    names = np.arange(0.5, 16.01, 0.5)
    energy = np.array([power[(f > (o - 0.12) * turn) & (f < (o + 0.12) * turn)].sum() for o in names])
    fire = cyl / 2.0
    on = np.array([abs(o / fire - round(o / fire)) < 1e-9 for o in names])
    low = names <= 4.0 * fire
    others = 10.0 * np.log10(energy[~on & low].sum() / energy[on & low].sum() + 1e-30)
    top = np.argsort(energy)[::-1][:7]
    strongest = [(names[i], 10.0 * np.log10(energy[i] / energy.max() + 1e-30)) for i in top]
    return firing * 120.0 / cyl, energy[on].sum() / energy.sum(), others, strongest


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    cyl = int(sys.argv[1])
    print(f"{'':34s} {'rpm':>6s} {'firing':>7s} {'others':>7s}  strongest orders (dB)")
    for spec in sys.argv[2:]:
        try:
            path, span, tail = spec.split("@", 2)
            speeds, label = tail.split(":", 1)
            t0, t1 = (float(v) for v in span.split("-"))
            rpm_lo, rpm_hi = (float(v) for v in speeds.split("-"))
        except ValueError:
            sys.exit(f"orders: {spec} isn't WAV@T0-T1@RPMLO-RPMHI:LABEL")
        if os.sep not in path and not path.endswith(".wav"):
            path = os.path.join(REAL_DIR, path + ".wav")
        rpm, share, others, strongest = orders(path, cyl, t0, t1, rpm_lo, rpm_hi)
        print(f"{label[:34]:34s} {rpm:6.0f} {100.0 * share:6.0f}% {others:7.1f}  "
              + ", ".join(f"{o:g} ({v:.0f})" for o, v in strongest))


if __name__ == "__main__":
    main()
