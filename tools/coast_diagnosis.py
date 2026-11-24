#!/usr/bin/env python3
"""Stage 058: what makes the overrun sound robotic, and what booms in it.

    python tools/coast_diagnosis.py WAV CSV [--label L] [--json OUT]

WAV is a float render of `rb_cli sound --sweep ... --float --csv CSV` (one channel, or the
first of two); CSV its per-block telemetry (t_s, rpm, throttle, fuel_cut). The windows: the
loaded coast (foot off, in gear: from 0.5 s after the lift until the clutch goes in) and the
neutral fall (the clutch in), each only while the fuel cut is on.

The lift and clutch times are rb_cli's own (it prints them).

For each window:
  orders      by crank angle (runs of 4 cycles, bins an eighth of an order): the dominant
              half-order and its share of the half-orders' power to order 24, and the floor
              between the half-orders below it (dB): how pure a note the overrun is. (A fixed-
              frequency peak in a time-averaged spectrum misleads in a falling sweep: it is the
              sweep's dwell, not a resonance; this follows the orders instead.)
  repetition  how alike successive cycles are: the sound band-passed 60 Hz-1.5 kHz,
              resampled by crank angle (the CSV's rpm integrated), 1024 points a 720-degree
              cycle; the median correlation of each cycle with the next (1: identical).
  crest       peak over RMS (dB), the median over 0.25 s pieces.
  modulation  the envelope's (60 Hz-1.5 kHz band) depth at the firing frequency, against
              its mean: how strongly each firing (or, on the overrun, each pulse) pumps the level.
Writes a line per window, and optionally the numbers as JSON.
"""
import argparse
import csv
import json
import sys
from pathlib import Path

import numpy as np


def read_wav(path):
    data = Path(path).read_bytes()
    i = data.find(b'fmt ')
    channels = int.from_bytes(data[i + 10:i + 12], 'little')
    rate = int.from_bytes(data[i + 12:i + 16], 'little')
    bits = int.from_bytes(data[i + 22:i + 24], 'little')
    j = data.find(b'data')
    n = int.from_bytes(data[j + 4:j + 8], 'little')
    raw = data[j + 8:j + 8 + n]
    x = np.frombuffer(raw, dtype=np.float32 if bits == 32 else np.int16).astype(float)
    if bits == 16:
        x /= 32768.0
    return x.reshape(-1, channels)[:, 0], rate


def read_csv(path):
    with open(path, newline='', encoding='utf-8') as f:
        rows = list(csv.DictReader(f))
    t = np.array([float(r['t_s']) for r in rows])
    rpm = np.array([float(r['rpm']) for r in rows])
    throttle = np.array([float(r['throttle']) for r in rows])
    cut = np.array([float(r['fuel_cut']) for r in rows])
    return t, rpm, throttle, cut


def band(x, rate, lo, hi):
    """Zero-phase FFT band-pass."""
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1.0 / rate)
    X[(f < lo) | (f > hi)] = 0.0
    return np.fft.irfft(X, len(x))


def windows(t, cut, lift, clutch):
    """The loaded coast (0.5 s after the lift to the clutch) and the neutral fall (the clutch
    on), each only while the overrun's fuel cut is on: the overrun alone, in seconds."""
    on = t[cut > 0.5]
    if not len(on):
        return None
    w = {}
    for name, (s, e) in (('loaded coast', (lift + 0.5, clutch)), ('neutral fall', (clutch, 1e9))):
        inside = on[(on >= s) & (on <= e)]
        if len(inside):
            w[name] = (float(inside[0]), float(inside[-1]))
    return w


def orders(x, rate, t_s, rpm_s, t0):
    """The order spectrum: runs of 4 cycles (2880 degrees) resampled by crank angle, 1024
    points a cycle, so the bins are eighths of an order; their power averaged over the runs.
    Returns the dominant half-order, its share of the power at half-orders up to order 24, and
    the floor between the half-orders (the median of the other bins up to order 24) below it."""
    y = band(x, rate, 30.0, 6000.0)
    tt = t0 + np.arange(len(y)) / rate
    rpm = np.interp(tt, t_s, rpm_s)
    angle = np.cumsum(rpm / 60.0 * 360.0 / rate)
    runs = int(angle[-1] // 2880)
    if runs < 2:
        return None
    grid = np.arange(runs * 4096) * (720.0 / 1024)
    z = np.interp(grid, angle, y).reshape(runs, 4096) * np.hanning(4096)
    power = (np.abs(np.fft.rfft(z, axis=1)) ** 2).mean(axis=0)
    top = 24 * 8                                 # order 24, in eighths
    half = np.arange(4, top + 1, 4)              # half-orders 0.5 .. 24
    between = np.array([k for k in range(2, top) if k % 4 not in (0, 1, 3)])
    k = half[np.argmax(power[half])]
    share = power[k] / power[half].sum()
    floor = 10 * np.log10(power[k] / np.median(power[between]))
    return {'order': float(k / 8.0), 'order_share': float(share), 'floor_db': float(floor)}


def repetition(x, rate, t_s, rpm_s, t0):
    """Median correlation of each 720-degree cycle with the next."""
    y = band(x, rate, 60.0, 1500.0)
    tt = t0 + np.arange(len(y)) / rate
    rpm = np.interp(tt, t_s, rpm_s)
    angle = np.cumsum(rpm / 60.0 * 360.0 / rate)
    cycles = int(angle[-1] // 720)
    if cycles < 3:
        return None
    grid = np.arange(cycles * 1024) * (720.0 / 1024)
    z = np.interp(grid, angle, y).reshape(cycles, 1024)
    r = [np.corrcoef(z[i], z[i + 1])[0, 1] for i in range(cycles - 1)]
    return float(np.median(r))


def crest(x, rate):
    n = int(0.25 * rate)
    c = []
    for k in range(0, len(x) - n, n):
        p = x[k:k + n]
        rms = np.sqrt(np.mean(p * p))
        if rms > 0:
            c.append(20 * np.log10(np.max(np.abs(p)) / rms))
    return float(np.median(c)) if c else None


def modulation(x, rate, t_s, rpm_s, t0, cylinders):
    y = band(x, rate, 60.0, 1500.0)
    env = np.convolve(np.abs(y), np.ones(int(rate / 2000)) / int(rate / 2000), mode='same')
    tt = t0 + np.arange(len(y)) / rate
    rpm = np.interp(tt, t_s, rpm_s)
    phase = np.cumsum(2 * np.pi * rpm / 60.0 * cylinders / 2.0 / rate)
    depth = 2 * np.abs(np.mean((env - env.mean()) * np.exp(-1j * phase))) / env.mean()
    return float(depth)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('wav')
    ap.add_argument('csv')
    ap.add_argument('--cylinders', type=int, required=True)
    ap.add_argument('--lift', type=float, required=True, help='s: the foot off (rb_cli prints it)')
    ap.add_argument('--clutch', type=float, required=True, help='s: the clutch in')
    ap.add_argument('--label', default='')
    ap.add_argument('--json')
    a = ap.parse_args(argv)
    x, rate = read_wav(a.wav)
    t, rpm, throttle, cut = read_csv(a.csv)
    w = windows(t, cut, a.lift, a.clutch)
    if not w:
        print('coast_diagnosis: no coast found', file=sys.stderr)
        return 2
    out = {}
    for name, (s, e) in w.items():
        seg = x[int(s * rate):int(e * rate)]
        if len(seg) < rate // 2:
            continue
        r = {'from_s': float(s), 'to_s': float(e),
             'rpm': [float(np.interp(s, t, rpm)), float(np.interp(e, t, rpm))],
             'level_dbfs': float(10 * np.log10(np.mean(seg * seg) + 1e-30))}
        o = orders(seg, rate, t, rpm, s)
        if o:
            r.update(o)
        r['repetition'] = repetition(seg, rate, t, rpm, s)
        r['crest_db'] = crest(seg, rate)
        r['modulation'] = modulation(seg, rate, t, rpm, s, a.cylinders)
        out[name] = r
        print(f"{a.label:24s} {name:13s} {r['rpm'][0]:5.0f}->{r['rpm'][1]:5.0f} rpm "
              f"{r['level_dbfs']:6.1f} dBFS  order {r.get('order', 0):4.1f} ({100 * r.get('order_share', 0):3.0f} %) "
              f"floor -{r.get('floor_db', 0):4.1f} dB  "
              f"repetition {r['repetition']:.3f}  crest {r['crest_db']:4.1f} dB  "
              f"modulation {r['modulation']:.2f}")
    if a.json:
        Path(a.json).write_text(json.dumps({'label': a.label, 'windows': out}, indent=1),
                                encoding='utf-8')
    return 0


if __name__ == '__main__':
    sys.exit(main())
