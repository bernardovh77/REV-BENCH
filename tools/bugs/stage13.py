"""
Stage 13's planted bugs (tools/plant_bugs.py): the faster gas and chambers,
the pipes without their divisions, the nozzle's one pow, Woschni's logs, the
quality setting and the fuzzer. Each one must make at least one of its tests
fail.
"""
GAS = ["test_gas_matches_nasa_data", "test_gas_textbook_air", "test_gas_u_and_T_are_inverse",
       "test_gas_mixing_is_mass_weighted"]
FLOW = ["test_nozzle_formulas", "test_two_tanks_equalise", "test_blowdown_matches_analytic",
        "test_stiff_equalisation_has_no_overshoot", "test_filling_heats_the_tank"]
PIPE = ["test_pipe_pulse_travels_at_the_speed_of_sound", "test_pipe_friction",
        "test_pipe_waves_ride_the_flow", "test_pipe_walls", "test_pipe_conserves_everything",
        "test_pipe_two_rates_agree", "test_pipe_rings", "test_pipe_packing", "test_pipe_onward"]

# name: (file, old text, new text, tests to run, timeout in s)
BUGS = {
    # the gas (gas.c): each species' u(T) constants worked out by the compiler
    "u-jumps-at-1000K": ("core/src/gas.c",
        "    (K_LO(R, l0, l1, l2, l3, l4) + RB_T_MID * (U_TERM((l0) - (R), l1, l2, l3, l4, 1.0)   \\",
        "    (K_LO(R, l0, l1, l2, l3, l4) + 0.0 * (U_TERM((l0) - (R), l1, l2, l3, l4, 1.0)   \\",
        GAS, 120),
    "mixture-k-from-air-only": ("core/src/gas.c",
        "    g->k_lo = wa * a->k_lo + wf * f->k_lo + wb * b->k_lo;",
        "    g->k_lo = a->k_lo;",
        GAS, 120),
    "mixture-hi-weights": ("core/src/gas.c",
        "        g->hi[k] = wa * a->hi[k] + wf * f->hi[k] + wb * b->hi[k];",
        "        g->hi[k] = wa * a->hi[k] + wa * f->hi[k] + wb * b->hi[k];",
        GAS, 120),
    # the chamber (chamber.c): cv from the Newton step, 1 / rho
    "cv-never-updated": ("core/src/chamber.c",
        "        ch->T = rb_gas_T_cv_from_u(&ch->gas, u, guess, &ch->cv);",
        "        double cv_ignored;\n        ch->T = rb_gas_T_cv_from_u(&ch->gas, u, guess, &cv_ignored);",
        FLOW + ["test_chamber_init_and_repairs"], 180),
    "per-rho-zero": ("core/src/chamber.c",
        "    ch->per_rho = ch->V * per_kg;",
        "    ch->per_rho = 0.0 * per_kg;",
        PIPE, 300),
    # the nozzle (flow.c): choked when 2 a >= (g + 1) r
    "choke-test-inverted": ("core/src/flow.c",
        "    if (2.0 * a >= (g + 1.0) * r)   /* choked */",
        "    if (2.0 * a <= (g + 1.0) * r)   /* choked */",
        FLOW, 120),
    # Woschni (heat.c): one exp of logs
    "woschni-bore-sign": ("core/src/heat.c",
        "    return 3.26 * exp(0.8 * log(p_kpa * w) - 0.55 * log(T) - 0.2 * log(bore));",
        "    return 3.26 * exp(0.8 * log(p_kpa * w) - 0.55 * log(T) + 0.2 * log(bore));",
        ["test_woschni"], 120),
    # the pipes (pipe.c): constants instead of divisions
    "wall-ratio-halved": ("core/src/pipe.c",
        "    p->wall_ratio = 4.0 * p->dx / diameter;",
        "    p->wall_ratio = 2.0 * p->dx / diameter;",
        PIPE, 300),
    "kinetic-heat-unscaled": ("core/src/pipe.c",
        "    rb_chamber_add_heat(c, -(kinetic - p->kinetic[k]) * per_dt);",
        "    rb_chamber_add_heat(c, -(kinetic - p->kinetic[k]) * dt);",
        PIPE, 300),
    "heads-without-area": ("core/src/pipe.c",
        "    const double force = f->area * dp - heads * 0.5 * f->mdot * fabs(f->mdot) * per_r * f->per_area",
        "    const double force = f->area * dp - heads * 0.5 * f->mdot * fabs(f->mdot) * per_r",
        PIPE, 300),
    "laminar-skipped": ("core/src/pipe.c",
        "    if (drag > 0.0) mdot /= 1.0 + dt * drag * per_r * per_length;",
        "    if (drag < 0.0) mdot /= 1.0 + dt * drag * per_r * per_length;",
        PIPE + ["test_engine_intake_losses"], 400),
    "momentum-flux-lost": ("core/src/pipe.c",
        "    return mdot * ((mdot >= 0.0) ? in : out) * p->cell[k].per_rho * p->per_area;",
        "    return 0.0 * mdot * ((mdot >= 0.0) ? in : out) * p->cell[k].per_rho * p->per_area;",
        PIPE, 300),
    # going straight on (pipe.c): only between similar bores
    "onward-any-bore": ("core/src/pipe.c",
        "    if (a->area > RB_PIPE_ONWARD_MAX_RATIO * b->area || b->area > RB_PIPE_ONWARD_MAX_RATIO * a->area)\n        return -1;\n",
        "",
        ["test_pipe_onward", "test_engine_wide_tailpipes"], 400),
    # the quality setting (rb_sim.c, rb_engine_api.c, engine.c)
    "low-quality-at-192kHz": ("core/src/rb_sim.c",
        "    const double physics = (quality == RB_QUALITY_LOW) ? 96000.0 : 192000.0;",
        "    const double physics = 192000.0;",
        ["test_core_quality"], 120),
    "reset-forgets-pipe-rate": ("core/src/rb_sim.c",
        "    rb_engine_init_at(&sim->engine, &d, sim->dt, sim->cfg.pipe_rate);",
        "    rb_engine_init(&sim->engine, &d, sim->dt);",
        ["test_core_quality"], 120),
    "preset-forgets-pipe-rate": ("core/src/rb_engine_api.c",
        "    rb_engine_init_at(&sim->engine, &d, sim->dt, sim->cfg.pipe_rate);   /* presets are possible engines */",
        "    rb_engine_init(&sim->engine, &d, sim->dt);   /* presets are possible engines */",
        ["test_core_quality"], 120),
    "pipe-rate-ignored": ("core/src/engine.c",
        "    e->pipe_every = (int)lround(1.0 / (pipe_rate * dt));",
        "    e->pipe_every = (int)lround(1.0 / (48000.0 * dt));",
        ["test_core_quality", "test_engine_pipe_rate"], 300),
    # the fuzzer (native/fuzz.h): with a limit every engine passes, every run must fail
    "fuzz-speed-limit-100": ("native/fuzz.h",
        "#define RB_FUZZ_MAX_RPM 30000.0",
        "#define RB_FUZZ_MAX_RPM 100.0",
        ["test_fuzz_random_engines"], 300),
}
