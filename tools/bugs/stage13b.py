"""Batched thermodynamics must preserve scalar state and physical gas fits."""
BATCH = ["test_chamber_batch_matches_scalar", "test_chamber_batch_repairs"]
BUGS = {
    "newton-sign": ("core/src/chamber.c", "T[j] -= correction[j];",
                    "T[j] += correction[j];", BATCH, 30),
    "wrong-cached-cv": ("core/src/chamber.c",
        "g->cv = cv[j];", "g->cv = c[0][j];", BATCH, 30),
    "missing-tail": ("core/src/chamber.c",
        "for (; base < count; base++) rb_chamber_integrate(&ch[base], dt);",
        "/* BUG: omitted incomplete batch */", BATCH, 30),
    "negative-mass-fast-path": ("core/src/chamber.c",
        "normal &= m >= 0.0 && m < INFINITY;",
        "normal &= m < INFINITY;", BATCH, 30),
    "nonfinite-specific-energy": ("core/src/chamber.c",
        "isfinite(target[j]) && fabs(correction[j]) <= 1e-6 * T[j]",
        "fabs(correction[j]) <= 1e-6 * T[j]", BATCH, 30),
    "wrong-fuel-weight": ("core/src/gas.c",
        "wf * f->hi[k]", "wa * f->hi[k]",
        ["test_gas_matches_nasa_data", "test_gas_u_and_T_are_inverse"], 30),
}
