# Stage S1a — Reliable sound analysis before further tuning

Date: 2026-10-21. Complete. Preparation split from S1; S1 remains in progress.

The workflow asks for `rb_cli sound --float` for analysis, but compare_real.py,
orders.py and plot_sound.py rejected those files. orders.py also split its input
at the drive-letter colon, rejecting absolute Windows paths. These prevented
following the existing validation plan on Bernardo's machine.

## Changes

- tools/audio_wav.py reads little-endian PCM16 and IEEE float32 RIFF WAVs, also
  their extensible forms. It skips metadata and word padding, handles the native
  writer's float `fact` chunk, and rejects truncated, empty or non-finite audio.
- All three analysis tools use it. Comparison/order analysis averages channels
  as before; plot_sound still requires mono and removes the supplied gain.
- Float values are preserved, including peaks above full scale and amplitudes
  below a PCM16 quantisation step. No resampling, clipping or normalisation.
- orders.py separates the path before parsing the label, so Windows drive
  letters and colons within labels both work.
- Eight Python regression tests cover amplitude, metadata, stereo policy,
  extensible formats, malformed data, equivalent PCM/float measurements and
  the Windows command-line syntax.

PCM16 is decoded as sample / 32768; float full scale is 1. compare_real already
used that scale. plot_sound previously divided by 32767: its PCM levels now
move down by 20 log10(32768/32767), about 0.000265 dB. This is an analysis-scale
correction, not a change in engine sound.

Format reference: [McGill's WAVE specification and linked original Microsoft
documents](https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html).

## Validation

Run from the repository root (existing numpy and matplotlib dependencies):

```sh
python -m unittest discover -s tools -p 'test_*.py' -v
python tools/compare_real.py compare 'renders/i4-idle.wav@3-8:I4 existing idle'
python tools/orders.py 4 'C:/Users/berna/REV-BENCH/renders/i4-3000-none.wav@1-5@2900-3100:I4 existing: 3000'
```

All eight Python tests and all 16 existing web unit tests pass. Synthetic PCM
and float measurements agree within 0.01 dB for spectral shares, repetition,
crest, A-weighting loss and slope. Float samples at 1e-8 remain nonzero, while
peaks at 1.25 and -2 are retained.

Existing local stage-12c renders reproduce the documented issue:

| Measurement | Result |
|---|---:|
| I4 idle, seconds 3–8: repeat below 500 Hz | +20.4 dB |
| Same: repeat 0.5–2 kHz | -6.7 dB |
| Same: repeat 2–8 kHz | -11.9 dB |
| I4 at 3000 rpm, no muffler: orders between firing orders | -13.5 dB |

The idle low band is still above S1's 0 to +15 dB target. These are measurements
of existing local renders, not newly improved physics. Reference recordings
are not present in renders/real, so no fresh real-recording comparison is claimed.

Native validation was initially blocked by WSL access and Zig cache failures.
A portable GCC toolchain in the Codex scratch folder resolved it: the complete
151-test native suite (2455 checks, including S1b) now passes, as do the 16 web
unit tests and browser test in development and production preview. The initial
Windows CSV newline mismatch was fixed in native/csv.c; its existing regression
now passes. Claude's installed tools and settings were not changed.

No core or browser files changed. Existing renders were preserved. A core/WASM
rebuild is not needed for these analysis-only changes. graphify was subsequently installed in the isolated scratch folder and the
graph is refreshed after the final code and documentation changes.

## Next S1 work

Measure the same idle window across all five presets and retain that baseline
before changing combustion. Investigate residual-dependent partial burns and
misfires, verifying conservation, work variation, less than 1% dyno movement
and stable 30-second idles. Do not add arbitrary misfire rates merely to make
the waveform less periodic.

Research checked during handoff: [Kaul, Wagner and Green (2013)](https://saemobilus.sae.org/articles/analysis-cyclic-variability-heat-release-high-egr-gdi-engine-operation-observations-implications-effective-control-2013-01-0270)
describes dilution-related cyclic variability; [ORNL's partial-burn study](https://www.ornl.gov/publication/cyclic-dynamics-misfires-and-partial-burns-dilute-spark-ignition-engine)
describes residual coupling between weak and subsequent stronger cycles. These
support the direction of investigation, not calibrated thresholds for this sim.

Bernardo's listening checkpoint remains with S1: the held and idling renders
should stop sounding like a repeated loop. S1a itself changes no sound.
