"""Stage 054: the noise's routing against same-state captures can fail its tests.

python tools/plant_bugs.py 054 WORKDIR all

Stage 054's candidate (noise.c's variant 3: live path times, the reversed jets heard) lost
Bernardo's A/B on 2026-11-24 and was deleted with its tests; its eight bugs (the path times
without the gas's flow, at the air's gamma, without the centre pipes or the tailpipe, routed once;
the reversed jets unheard; variant 3 never reaching the noise) went with it. What stays is the
accepted routing's checks: the catalyst's pass ignored; the chambered can passing everything.
Missed, and not kept: a straight-through passing what a chambered can does (the test compares it
with no muffler, which it still beats); a constant added to the turbulence, and the jets' pulse
written in place of their turbulence (test_routing_no_duplicated_pulse passes both: its r is
against the tailpipe flow's step-to-step swings, which the noise's envelope, delayed down the
ring, doesn't follow). That test is a guard with no planted bug that fails it.
"""
CONTROLS = ["test_routing_component_controls"]

BUGS = {
    'catalyst-passes-all': ("core/src/noise.c",
        "    n->blow_pass = (d->catalyst_laminar > 0.0 ? PASS_CATALYST : 1.0) * pass;",
        "    n->blow_pass = 1.0 * pass;",
        CONTROLS, 300),
    'can-passes-all': ("core/src/noise.c",
        "    default:                  pass = PASS_CHAMBERED; top_hz = TOP_CHAMBERED_HZ; break;",
        "    default:                  pass = PASS_NONE;      top_hz = TOP_CHAMBERED_HZ; break;",
        CONTROLS, 300),
}
