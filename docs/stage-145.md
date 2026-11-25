# Stage 145: Bernardo's Polo 1.0 TSI as a preset, with its turbine

Worked on 2026-11-25 by Claude (Opus 5.5) (the card,
[docs/roadmap/16-realism.md](roadmap/16-realism.md#stage-145)).

**Status: 🟡** — `polo-tsi` ("VW Polo 1.0 TSI") plays: Bernardo's 2023 Polo, 81 kW (110 PS),
stock exhaust, its turbine and wastegate in the exhaust network. Its boost waits for phase D's
compressor; its level and timbre wait for his recording (146).

## The gaps before it

| Gap | Could it influence this stage? |
| --- | --- |
| Phase D (078: the turbocharger's compressor and turbine maps) | Yes: without the compressor the intake breathes at the air's pressure, so the torque off idle is a non-turbo three's (below). The turbine's part in the exhaust is built here |
| 146 (his recording) | Yes: the level and the turbine's high-frequency loss are set by it, not guessed |
| 147 (several cores) | No: it plays at the balanced tier on one core (2.2× real time on the laptop) |
| 059 (VE at or below 1.0) | Yes, as for every preset: its VE 0.90–0.95 |

## The research

| What | Value | Source |
| --- | --- | --- |
| Engine | EA211 1.0 TSI three-cylinder, 999 cc, 74.5 × 76.4 mm, 82 mm bore spacing, compression 10.5, four valves a cylinder (sodium-filled exhaust valves from 81 kW) | [de.wikipedia.org, VW EA211](https://de.wikipedia.org/wiki/VW_EA211); [en.wikipedia.org](https://en.wikipedia.org/wiki/Volkswagen_EA211_engine) |
| The 2021-on Polo's 110 PS | 81 kW at 5000–5500 rpm, 200 N m, 7-speed DSG only | [moniteurautomobile.be](https://www.moniteurautomobile.be/actu-auto/facelifts/volkswagen-polo-2021-citadine-technologique.html), [km77](https://www.km77.com/coches/volkswagen/polo/2021/estandar/estandar/polo-10-tsi-81-kw-110-cv-dsg/datos), [auto-data.net](https://www.auto-data.net/en/volkswagen-polo-vi-facelift-2021-1.0-tsi-110hp-dsg-43419) |
| Engine code | most likely DLAA (81 kW, 200 N m at 2000–3000 rpm: the Golf 8's and Leon IV's); not listed for the Polo: **inferred** | de.wikipedia.org |
| Turbo | single-scroll turbine, boost limited by a wastegate worked by an electric actuator with a position sensor; no variable geometry; rated to 950 °C | de.wikipedia.org (EA211, "Aufladung und Abgas") |
| Manifold | cast into the cylinder head (the family's) | de.wikipedia.org |
| After-treatment | three-way catalyst; a petrol particulate filter on some variants since 2017 (the Polo "1.0 TSI OPF") | de.wikipedia.org, [auto-wiki.org](https://www.auto-wiki.org/vw/polo/aw/polo-1-0-tsi-opf-r-line-dsg-81-kw-93350/) |
| Silencers | a centre silencer (aftermarket centre pipes replace it: [Supersprint 918113](https://www.supersprint.com/en-ww/sport-exhaust-supersprint-vw-polo-2g-10-tsi.aspx), Ø54 oversize) and a rear one | |
| Cabin | 38–40 dB(A) idling, 72–73 at full throttle (for 149's cabin check) | [auto-motor-und-sport.de](https://www.auto-motor-und-sport.de/test/kosten-realverbrauch-vw-polo-1-0-tsi-r-line/technische-daten/) |
| A turbine's sound | nearly transparent to sound going downstream at low frequencies (Rämmal and Åbom, KTH); the measured curves (Tiikoja et al., SAE 2011-24-0221) couldn't be read from here (the KTH archive refused the connection) | [KTH CCGEx](https://www.researchgate.net/publication/275224696_Investigations_of_Automotive_Turbocharger_Acoustics) |

**Estimated, each marked in the code:** the rod (140 mm), the valves' sizes, lifts and timing,
the runners, the plenum (1.6 L, its charge cooler in it), the air box, the inertia (0.10 kg m²,
with the dual-mass flywheel), the idle (750 rpm) and the limit (6300 rpm); in the exhaust
every volume, length and bore, the turbine's effective nozzle (3.5 cm²), the open wastegate's
(2.0 cm²), the catalyst and filter's loss, the silencers' insides, the tailpipe's place.

## The preset

`polo-tsi`, presets.c `polo_tsi`: three cylinders in line firing 1-2-3 every 240°, the balanced
tier. Its exhaust (exhaust_models.c `ea211_polo`), a network of nine pipes and two volumes:
the three ports in the head into the manifold's volume (0.15 L); out of it the **turbine's way**
(narrowed to its nozzle) and **the wastegate's** (narrowed to its valve, open off boost, VW's
strategy for low back pressure), both into the turbine's outlet (0.25 L); the downpipe with the
catalyst and filter; the front pipe; a packed straight-through centre box; the rear pipe; the rear
box (a reflective chamber, then a packed tube); the tailpipe, turned down out of sight at the rear
left.

**The turbine as modelled:** a restriction between two volumes, so the gas solver itself spends
the pulses' energy through it and smooths them, the reason such engines are quiet. Not yet in
it: the rotor's work (which cools the gas behind it), its loss of sound above the low
frequencies, and the compressor (the boost): phase D, and 146's recording for the loss.

## The evidence

| | Result |
| --- | --- |
| Idle, 30 s (`rb_cli sound --idle --seconds 30 --quality balanced`) | starts at 0.27 s; 686–783 rpm after 5 s (the block telemetry), mean 750 |
| `test_presets_run_on_their_own` (each cycle's mean) | idle 749 rpm, 6.7 rpm from cycle to cycle, the idle valve 14 % open; a blip dips to 705 and returns to 741; from the limiter back to 775 in 4.5 s; at 1.5 times the gains still 13 rpm (no hunting) |
| `test_baseline_whole_drive_books` | idle 767, held 5447 under its load, pulled to 6275 against its 6300 limit, the fuel cut and its resume, the books |
| Dyno, wide open (`rb_cli dyno`) | 88.6 N m at 4039 rpm, 51.9 kW at 6100 rpm, VE 0.90–0.95: **a non-turbo three's**, within 7 % of VW's own non-turbo 1.0 MPI of the same block (55 kW, 95 N m: [en.wikipedia.org](https://en.wikipedia.org/wiki/Volkswagen_EA211_engine), CHYB); the real 1.0 TSI's 200 N m needs the boost (phase D) |
| `test_net_polo_turbine` | the books to 1e-15 (motored wide open); the wastegate carries 0.61 of the turbine's flow (their areas 0.57); the manifold 1.3 kPa over the turbine's outlet |
| `test_presets_table`, `test_presets_fire_where_the_layout_says`, `test_core_preset_quality` | its size (999 cc), firing order, layout and tier |
| Level, 1 m from the tailpipe (`--listener tailpipe`, high quality) | idle 88.7 dB, about 65 dB(A) (A-weighting takes 23 dB off its low sound); a 3000 rpm hold at a third of the throttle 103.7 dB, about 92 dB(A); revving 98.2 dB, about 88 dB(A) |

**What the level says:** the idle is plausible for a small modern car; off idle it is likely
several dB over a real 1.0 TSI (not yet known by how much). The car's papers give the stationary
noise test (0.5 m from the tailpipe, 45°, at three quarters of rated speed: the certificate of
conformity, or the registration's field U.1 in some countries): Bernardo's reading of it, and his
recording, set it. Nothing is tuned against a guess.

**The checks (2026-11-25, the laptop, gcc):** `tools/cli_parity.py` against 144's renders, 11
cases all identical (the other presets untouched); `tools/speed_check.py HEAD`, the median
+0.1 % (the worst cell, the four balanced, −2.0 %); every suite passes (`native` 1180 s on its
own: run beside other work it passed 1800 s, its limit now 3600 as `cmake`'s, the same tests).

Renders: `renders/evidence/stage-145/` (idle, the hold and revs, at 1 m and 3 m behind; the dyno).

## Tuning, and two tests made general

- **The idle** (as every preset's, presets.c `running`): first built it idled at 772 rpm, its
  cycles 33 rpm apart, its valve 9 % open, and coasted from the limiter too slowly. Its inertia
  0.13 → 0.10 kg m² (a 1.0 L's, with its dual-mass flywheel), its plenum 2.5 → 1.6 L (the large
  plenum made the idle's control slow: near a limit cycle whatever the gains), its idle valve
  sized to 14 % open, its gains with room for 1.5 times.
- **The whole drive's load** (test_baseline.c): 40 N m held at 40 % throttle, sized for the 2 L
  and larger presets; a smaller engine now gets its share by displacement (the Polo 20 N m: it
  makes 88 N m flat out without its boost). The other presets' drives are unchanged.
- **The description check's rule against a collector pipe over 6 times its headers** (stage 131,
  engine_build.c) now applies only where the fields build the exhaust: a network's pipes replace
  that pipe, and the layout test sets the field's widest pipe on every preset.

## Planted bugs

tools/bugs/stage145.py: 4, all caught.

| Bug | Caught by |
| --- | --- |
| the turbine's nozzle not narrowed | `test_net_polo_turbine` |
| the wastegate shut (a first version of the test missed it: it compared the split with the areas the bug had changed too; the test now also checks the wastegate is open) | `test_net_polo_turbine` |
| a four-cylinder's bore | `test_presets_table` |
| the pins for 1-3-2 while it claims 1-2-3 | `test_presets_fire_where_the_layout_says` |

## What is left (why 🟡)

1. **Bernardo's recording** (146): the level and timbre at 1 m, the turbine's high-frequency loss.
2. **The stationary noise figure** from his car's papers, a published check before the recording.
3. **The boost** (phase D's compressor and the turbine's work), for the torque and the sound under load.
4. **The engine code** (his papers or the sticker under the bonnet), to confirm DLAA.

## Files

Changed: core/src/presets.c (`polo_tsi`), core/src/exhaust_models.c (`ea211_polo`),
core/src/engine_build.c (the collector rule for fields-built exhausts only), tests/test_baseline.c
(the drive's load by displacement), the tests
that list every preset (test_presets.c, test_core.c, web core.test.ts, the app's fake add-on and
app.test.ts), tests/test_exhaust_net.c (`test_net_polo_turbine`), tests/test_main.c, PLAN.md,
CODEX.md. New: tools/bugs/stage145.py, this doc.
