# miniaudio

For THIRD_PARTY_NOTICES (stage 100), and the record of what is vendored here (stage 123).

| | |
| --- | --- |
| **What** | miniaudio, a single-file audio playback and capture library. Used for the real-time host's device layer only (`host/src/device.c`): device listing and choosing, the system's audio backends (WASAPI, DirectSound and WinMM on Windows; Core Audio on macOS; PipeWire/PulseAudio/ALSA on Linux), its sample-rate converter and its null backend, which the tests use. The decoding, encoding, generation, resource manager, node graph and engine parts are compiled out (`host/src/ma_config.h`). |
| **Version** | 0.11.25 (dated 2026-03-04 in the file's header; released 2026-03-03) |
| **Source** | https://github.com/mackron/miniaudio, tag `0.11.25`, commit `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d`; the file is https://raw.githubusercontent.com/mackron/miniaudio/0.11.25/miniaudio.h |
| **SHA-256 of `miniaudio.h`** | `ac7af4de748b7e26b777f37e01cee313a308a7296a3eb080e2906b320cc55c89` |
| **SHA-256 of `LICENSE`** | `457f1b500e0adf6bc059edddfa78a2f62012e7c3bb43476c20e0bd23b25ba0eb` (the repository's LICENSE at that tag; the same text ends `miniaudio.h`) |
| **Licence** | A choice of the public domain (Unlicense) or MIT No Attribution (MIT-0). No notice has to travel with a binary; this folder keeps the text anyway. |
| **Why this version** | The newest release at least two weeks old when it was adopted (2026-11-09; CLAUDE.md's rule). 0.11.26 was released on 2026-10-31, nine days earlier. |
| **Changes made** | None. `miniaudio.h` is the release's file, byte for byte. |

To update: take a release at least two weeks old, replace `miniaudio.h` and `LICENSE`, record the
tag, commit and both SHA-256 sums here, run the host's tests (`python tools/run_tests.py host`) and
check that `host/src/ma_config.h` still builds (it names the parts compiled out).
