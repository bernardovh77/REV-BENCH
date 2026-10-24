"""Stage040 causal ignition faults. Applied only to isolated source copies.

python3 tools/plant_bugs.py 040 /tmp/rev-bench-040-bugs all
Historical onset/limiter fault tables describe their original source versions;
this table targets the current separate discharge/kernel lifecycle.
"""
BUGS = {
    'discharge-at-random-kernel-angle': ("core/src/engine.c",
        'passes(angle, step_deg, b->spark_angle)',
        'passes(angle, step_deg, b->start_angle)',
        ['test_ignition_causal_clock'], 300),
    'allow-negative-delay': ("core/src/engine.c",
        'b->kernel_delay_deg=delay>0 ? delay:0;',
        'b->kernel_delay_deg=delay;',
        ['test_ignition_causal_clock'], 300),
    'full-first-step-heat': ("core/src/engine.c",
        'const double first=b->discharged_this_step ? b->discharge_fraction:0;',
        'const double first=0;',
        ['test_ignition_causal_clock'], 300),
    'bypass-hard-cut': ("core/src/engine.c",
        'const int cut=e->spark_cut ? RB_IGN_HARD_CUT : (soft_cut(e) ? RB_IGN_SOFT_CUT:-1);',
        'const int cut=(soft_cut(e) ? RB_IGN_SOFT_CUT:-1);',
        ['test_ignition_suppression'], 300),
    'pending-beyond-evo': ("core/src/engine.c",
        '            b->kernel_pending=0;',
        '            /* faulty pending expiry omitted */',
        ['test_ignition_pending_kernel'], 300),
    'chemical-heat-at-ivc': ("core/src/engine.c",
        '    b->spark_due = 1;',
        '    b->spark_due = 1;\n    b->burnable=rb_burnable_fuel(g->m[RB_FUEL],g->m[RB_AIR],e->desc.mixture_spread,e->desc.combustion_efficiency);\n    b->fraction=0;b->discharged=1;b->kernel_pending=1;b->kernel_delay_deg=0;',
        ['test_ignition_causal_clock'], 300),
}
