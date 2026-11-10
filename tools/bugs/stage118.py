"""Stage 118: a speed-up that changes what the core computes is caught.

python tools/plant_bugs.py 118 WORKDIR all
Each bug is one a speed-up could plausibly bring in: a vectorised mixture with
a lane crossed, a helper that lets a NaN through, a clamp that no longer clamps,
a shortcut in Newton's method, a branch-free replacement with the wrong sign,
and the quality tiers' own numbers moved (the default made faster silently).
The mixture's bits are tested against a hash taken before the speed-up
(tests/test_speed.c), the rest by the tests that already watch what they feed.
"""
SPEED = ["test_speed_"]
GAS = ["test_speed_gas_init", "test_gas_matches", "test_gas_textbook", "test_gas_u_and", "test_gas_mixing"]
PIPE = ["test_pipe_", "test_speed_momentum"]
QUALITY = ["test_core_quality"]
TIER = ["test_core_preset_quality"]

BUGS = {
    # the SSE2 mixture (gas.c): two coefficients at a time must give the scalar bits
    'gas-sse-subtracts-R-from-the-wrong-lane': ("core/src/gas.c",
        "const __m128d take_R[5] = { _mm_set_pd(0.0, R), _mm_setzero_pd(), _mm_set_pd(R, 0.0),",
        "const __m128d take_R[5] = { _mm_set_pd(R, 0.0), _mm_setzero_pd(), _mm_set_pd(0.0, R),",
        GAS, 600),
    'gas-sse-integral-not-applied': ("core/src/gas.c",
        "_mm_storeu_pd(g->ulo + 2 * i, _mm_mul_pd(x, _mm_loadu_pd(INTEGRAL10 + 2 * i)));",
        "_mm_storeu_pd(g->ulo + 2 * i, x);",
        GAS, 600),
    'gas-sse-drops-the-burned-term': ("core/src/gas.c",
        "                           _mm_mul_pd(vwb, _mm_loadu_pd(b->lo + 2 * i))),\n                take_R[i]);",
        "                           _mm_setzero_pd()),\n                take_R[i]);",
        GAS, 600),
    'gas-sse-hi-range-never-written': ("core/src/gas.c",
        "            _mm_storeu_pd(g->lo + 2 * i, x);\n",
        "            if (i < 2) _mm_storeu_pd(g->lo + 2 * i, x);\n",
        GAS, 600),
    # the arithmetic helpers (fastmath.h)
    'finite-lets-infinity-through': ("core/src/fastmath.h",
        "    return fabs(x) < (double)INFINITY;   /* false for NaN and for infinity */",
        "    return fabs(x) <= (double)INFINITY;   /* false for NaN and for infinity */",
        SPEED, 300),
    'finite-lets-a-NaN-through': ("core/src/fastmath.h",
        "    return fabs(x) < (double)INFINITY;   /* false for NaN and for infinity */",
        "    return !(fabs(x) >= (double)INFINITY);   /* false for NaN and for infinity */",
        SPEED, 300),
    'clamp-swaps-its-limits': ("core/src/fastmath.h",
        "    return (a > hi) ? hi : (a < lo) ? lo : a;",
        "    return (a > hi) ? lo : (a < lo) ? hi : a;",
        SPEED, 300),
    'min-returns-the-larger': ("core/src/fastmath.h",
        "    return (b < a) ? b : a;",
        "    return (b > a) ? b : a;",
        SPEED, 300),
    # Newton's method for T, inlined (gas_inline.h)
    'newton-stops-after-a-loose-step': ("core/src/gas_inline.h",
        "        if (fabs(dT) <= 1e-6 * T) break;",
        "        if (fabs(dT) <= 1e-2 * T) break;",
        ["test_gas_", "test_engine"], 600),
    'newton-forgets-the-floor': ("core/src/gas_inline.h",
        "        if (T < 1.0) {\n            if (diag) rb_diag_note(diag, RB_D_BIT(RB_D_TEMP_FLOOR));\n            T = 1.0;\n        }",
        "",
        ["test_speed_newton", "test_diagnostic", "test_gas_"], 600),
    # branch-free replacements in the pipes and the scene
    'pipe-slope-takes-the-wrong-sign': ("core/src/pipe.c",
        "const double slope = (left > 0.0) ? smaller : -smaller;",
        "const double slope = (left > 0.0) ? -smaller : smaller;",
        PIPE, 600),
    'pipe-slope-takes-the-larger': ("core/src/pipe.c",
        "const double smaller = (fabs(right) < fabs(left)) ? fabs(right) : fabs(left);",
        "const double smaller = (fabs(right) > fabs(left)) ? fabs(right) : fabs(left);",
        PIPE, 600),
    # (a bug in the scene source's clamp, rb_sim.c, was tried and retired: the
    # clamp guards a pressure the engine never reaches, so no test can make it
    # matter; the render hashes would not see it either)
    # the tiers: a faster default, silently
    'default-quality-gets-the-low-pipe-rate': ("core/src/rb_sim.c",
        "                   : (quality == RB_QUALITY_HIGH || quality == RB_QUALITY_BALANCED) ? RB_PIPE_RATE",
        "                   : (quality == RB_QUALITY_BALANCED) ? RB_PIPE_RATE",
        QUALITY, 600),
    # the declared tier: the balanced quality's rates and each preset's default
    'balanced-pipes-halved': ("core/src/rb_sim.c",
        "                   : (quality == RB_QUALITY_HIGH || quality == RB_QUALITY_BALANCED) ? RB_PIPE_RATE",
        "                   : (quality == RB_QUALITY_HIGH) ? RB_PIPE_RATE",
        QUALITY + TIER, 600),
    'balanced-physics-doubled': ("core/src/rb_sim.c",
        "                         : (quality == RB_QUALITY_LOW || quality == RB_QUALITY_BALANCED) ? 96000.0",
        "                         : (quality == RB_QUALITY_LOW) ? 96000.0",
        QUALITY + TIER, 600),
    'tier-four-at-low': ("core/src/presets.c",
        '{ "i4", "2.0 L inline-4", inline4, RB_QUALITY_BALANCED },',
        '{ "i4", "2.0 L inline-4", inline4, RB_QUALITY_LOW },',
        TIER, 600),
    'tier-past-the-last-preset': ("core/src/presets.c",
        "    return (preset >= 0 && preset < N_PRESETS) ? PRESETS[preset].quality : RB_ERR_ID;",
        "    return (preset >= 0 && preset <= N_PRESETS) ? PRESETS[preset % N_PRESETS].quality : RB_ERR_ID;",
        TIER, 600),
}
