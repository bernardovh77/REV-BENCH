"""Read analysis WAVs without quantising float renders or clipping their peaks.

Supports little-endian RIFF PCM16 and IEEE float32, including their WAVE_FORMAT_
EXTENSIBLE forms. Unknown metadata chunks are skipped with RIFF's word padding.
Only numpy is needed, as in the existing analysis tools.
"""
import os
import struct

import numpy as np


def read_wav(path):
    """Return (frames x channels float64 samples, sample rate).

    PCM uses 32768 units per full scale; float values retain their original
    scale, including peaks above 1. Invalid/truncated/non-finite audio raises
    ValueError rather than silently contaminating spectra and correlations.
    """
    with open(path, "rb") as stream:
        header = stream.read(12)
        if len(header) != 12 or header[:4] != b"RIFF" or header[8:] != b"WAVE":
            raise ValueError("expected a little-endian RIFF WAVE file")
        end = 8 + struct.unpack_from("<I", header, 4)[0]
        if end < 12 or end > os.fstat(stream.fileno()).st_size:
            raise ValueError("truncated or invalid RIFF size")
        fmt = data = None
        while stream.tell() < end:
            if end - stream.tell() < 8:
                raise ValueError("truncated chunk header")
            kind, size = struct.unpack("<4sI", stream.read(8))
            start = stream.tell()
            next_chunk = start + size + (size & 1)
            if next_chunk > end:
                raise ValueError("chunk extends beyond RIFF size")
            if kind == b"fmt ":
                if fmt is not None:
                    raise ValueError("duplicate format chunk")
                fmt = stream.read(min(size, 40))
            elif kind == b"data":
                if data is not None:
                    raise ValueError("multiple data chunks are unsupported")
                data = (start, size)
            stream.seek(next_chunk)
        if fmt is None or len(fmt) < 16 or data is None:
            raise ValueError("missing format or data chunk")
        tag, channels, rate, byte_rate, align, bits = struct.unpack_from("<HHIIHH", fmt)
        if tag == 0xFFFE:
            if len(fmt) < 40 or struct.unpack_from("<H", fmt, 16)[0] < 22:
                raise ValueError("truncated extensible format")
            valid = struct.unpack_from("<H", fmt, 18)[0]
            guid = fmt[24:40]
            if guid[4:] != bytes.fromhex("00001000800000aa00389b71") or valid != bits:
                raise ValueError("unsupported extensible format")
            tag = struct.unpack_from("<I", guid)[0]
        if (tag, bits) not in ((1, 16), (3, 32)):
            raise ValueError("expected PCM16 or IEEE float32 audio")
        if channels < 1 or rate < 1 or align != channels * (bits // 8) or byte_rate != rate * align:
            raise ValueError("invalid audio format dimensions")
        offset, size = data
        if size == 0 or size % align:
            raise ValueError("empty audio or incomplete sample frame")
        stream.seek(offset)
        raw = stream.read(size)
        if len(raw) != size:
            raise ValueError("truncated sample data")
    samples = np.frombuffer(raw, dtype="<i2" if tag == 1 else "<f4").astype(np.float64)
    if tag == 1:
        samples /= 32768.0
    if not np.isfinite(samples).all():
        raise ValueError("audio contains NaN or infinity")
    return samples.reshape(-1, channels), rate
