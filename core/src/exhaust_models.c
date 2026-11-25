/* Complete valve-to-air configurations. Published tubing is normally OD;
 * 1.2 mm wall below is an explicit estimate, not a published bore. All
 * longitudinal lengths except the Toyota can bodies are estimated. Keep
 * measured dimensions, inferred internals and OEM upstream sections separate
 * in the research ledger. No sound-matching EQ is applied here. */
#include "exhaust_models.h"
#include "rb.h"

#include <math.h>
#include <string.h>

/* ---- the Ferrari 458 Italia (F136 FB), its exhaust as a network (stage 141) ----
 *
 * What is published (docs/stage-137.md, docs/stage-141.md):
 *   - the manifolds carry the catalysts (EC type approval e24*70/157*2007/34*1364*00,
 *     "Manifolds with catalysts: original parts"), 600 cells per square inch
 *     (the makers of replacements);
 *   - a left and a right rear silencer, each an oval clamshell 179 x 294.4 mm and
 *     229 mm long, "reflection-absorption principle", one 70 mm inlet and two
 *     outlets, 70 mm and 45 mm (the same approval, for Akrapovic's replacement,
 *     which keeps the original's connections);
 *   - three tailpipes in a triangle at the middle of the rear, the upper one
 *     opening with a valve from mid revs (launch reports; unconfirmed).
 * What is estimated (marked "est."): every length, the collectors' volumes, the
 * silencers' insides, the catalysts' loss, the tips' places, the centre tip's
 * bore. The bores published are outside diameters; walls 1.2 mm (est.).
 *
 * Each bank: four equal headers into a 4-1 collector, the manifold's catalyst
 * at its outlet, an intermediate pipe to its silencer. Each silencer: a
 * reflective chamber the inlet opens into (half the clamshell), then a
 * perforated tube through a packed chamber (the other half) to its outer
 * tailpipe; the chamber's 45 mm outlet to a small merge with the other's, and
 * on up the centre tailpipe (the valve's way, open: stage 142 makes it move).
 * Each tailpipe ends in its tip, wider: the gas leaves through the tip, and
 * a jet's noise goes as its speed to the eighth (noise.c), so the tip's bore
 * is what the jets' highs answer to. */
static const double F_WALL = 0.0012;

static rb_net_end f_node(int k, double entry)
{
    return (rb_net_end){ .kind = RB_NET_NODE, .index = k, .entry_loss = entry };
}

static rb_net_end f_into(int m, int end)
{
    return (rb_net_end){ .kind = RB_NET_INTO, .index = m, .end = end };
}

static rb_net_end f_outlet(int o)
{
    return (rb_net_end){ .kind = RB_NET_OUTLET, .index = o, .entry_loss = RB_TAILPIPE_ENTRY };
}

static void f136_458(rb_engine_desc *d)
{
    rb_exhaust_net *n = &d->net;
    const double PI = 3.14159265358979323846;
    const double header = d->header_diameter, pipe = 0.060;             /* est. (Kline: 63.5 OD) */
    const double inlet = 0.070 - 2.0 * F_WALL, outer = inlet;           /* published 70 mm      */
    const double small = 0.045 - 2.0 * F_WALL;                          /* published 45 mm      */
    const double centre = 0.060 - 2.0 * F_WALL;                         /* est.                 */
    const double tip = 0.095;   /* each tip's bore: the three are large, ~95 mm (est., photographs) */
    const double box = sqrt(0.179 * 0.2944);   /* the oval 179 x 294.4 mm as a round bore of its area */
    const double half = 0.5 * 0.229;           /* each chamber half the box's 229 mm (est.)  */
    memset(n, 0, sizeof *n);
    /* volumes: each bank's collector (a cone about two pipe diameters long, as
     * engine_build.c's), and the centre tip's merge (est. 0.15 L) */
    for (int b = 0; b < 2; b++)
        n->node[b] = (rb_net_node){ pipe * (4.0 * 0.25 * PI * header * header + 0.25 * PI * pipe * pipe),
                                    RB_PLACE_COLLECTOR, b };
    n->node[2] = (rb_net_node){ 1.5e-4, RB_PLACE_JUNCTION, 0 };
    n->nodes = 3;
    for (int i = 0; i < d->cylinders; i++) n->header_to[i] = f_node(d->bank[i], RB_HEADER_ENTRY);
    /* pipes, per bank b: 0 + b the catalyst pipe, 2 + b the intermediate,
     * 4 + b the reflective chamber, 6 + b the packed chamber's tube, 8 + b the
     * outer tailpipe, 10 + b the chamber's 45 mm outlet; 12 the centre tailpipe */
    for (int b = 0; b < 2; b++) {
        rb_net_pipe *cat = &n->pipe[0 + b], *mid = &n->pipe[2 + b], *room = &n->pipe[4 + b],
                    *packed = &n->pipe[6 + b], *tail = &n->pipe[8 + b], *by = &n->pipe[10 + b];
        *cat = (rb_net_pipe){ .role = RB_NET_ROLE_PIPE, .kind = RB_PIPE_EXHAUST, .number = b,
                              .from = RB_PLACE_COLLECTOR, .from_number = b, .to = RB_PLACE_CENTER, .to_number = b,
                              .length = 0.30, .diameter = pipe, .catalyst = 1.0 };      /* est. */
        cat->end[0] = f_node(b, RB_COLLECTOR_ENTRY);
        cat->end[1] = (rb_net_end){ .kind = RB_NET_ONWARD, .index = 2 + b,
                                    .area = 23e-4, .laminar = 37.5 };   /* 600 cpsi: 1.5 x the 400's (est.) */
        *mid = (rb_net_pipe){ .role = RB_NET_ROLE_PIPE, .kind = RB_PIPE_LINK, .number = b,
                              .from = RB_PLACE_CENTER, .from_number = b, .to = RB_PLACE_MUFFLER, .to_number = b,
                              .length = 0.85, .diameter = inlet };                       /* est. */
        mid->end[1] = f_into(4 + b, 0);
        *room = (rb_net_pipe){ .role = RB_NET_ROLE_CAN, .kind = RB_PIPE_MUFFLER, .number = b,
                               .from = RB_PLACE_MUFFLER, .from_number = b,
                               .to = RB_PLACE_MUFFLER_OUTLET, .to_number = b,
                               .length = half, .diameter = box, .muffler = RB_MUFFLER_CHAMBERED };
        *packed = (rb_net_pipe){ .role = RB_NET_ROLE_CAN, .kind = RB_PIPE_MUFFLER, .number = b,
                                 .from = RB_PLACE_MUFFLER, .from_number = b,
                                 .to = RB_PLACE_MUFFLER_OUTLET, .to_number = b,
                                 .length = half, .diameter = outer, .packing = RB_PACKING,
                                 .muffler = RB_MUFFLER_STRAIGHT };
        packed->end[0] = f_into(4 + b, 1);
        packed->end[1] = (rb_net_end){ .kind = RB_NET_ONWARD, .index = 8 + b, .back = 1 };
        *tail = (rb_net_pipe){ .role = RB_NET_ROLE_PIPE, .kind = RB_PIPE_LINK, .number = 4 + b,
                               .from = RB_PLACE_MUFFLER_OUTLET, .from_number = b, .to = RB_PLACE_AIR,
                               .length = 0.15, .diameter = outer };                      /* est. */
        tail->end[1] = f_into(13 + b, 0);   /* its 70 mm into its tip, a step out */
        n->pipe[13 + b] = (rb_net_pipe){ .role = RB_NET_ROLE_TAILPIPE, .kind = RB_PIPE_TAILPIPE, .number = b,
                                         .from = RB_PLACE_MUFFLER_OUTLET, .from_number = b,
                                         .to = RB_PLACE_AIR, .length = 0.12, .diameter = tip };   /* est. */
        n->pipe[13 + b].end[1] = f_outlet(b);
        *by = (rb_net_pipe){ .role = RB_NET_ROLE_PIPE, .kind = RB_PIPE_LINK, .number = 2 + b,
                             .from = RB_PLACE_MUFFLER, .from_number = b, .to = RB_PLACE_JUNCTION,
                             .length = 0.30, .diameter = small };                        /* est. */
        by->end[0] = f_into(4 + b, 1);
        by->end[1] = f_node(2, 0.0);
    }
    n->pipe[12] = (rb_net_pipe){ .role = RB_NET_ROLE_PIPE, .kind = RB_PIPE_LINK, .number = 6,
                                 .from = RB_PLACE_JUNCTION, .from_number = 0, .to = RB_PLACE_AIR,
                                 .length = 0.20, .diameter = centre };                   /* est. */
    n->pipe[12].end[0] = f_node(2, RB_COLLECTOR_ENTRY);
    n->pipe[12].end[1] = f_into(15, 0);
    n->pipe[15] = (rb_net_pipe){ .role = RB_NET_ROLE_TAILPIPE, .kind = RB_PIPE_TAILPIPE, .number = 2,
                                 .from = RB_PLACE_JUNCTION, .from_number = 0, .to = RB_PLACE_AIR,
                                 .length = 0.12, .diameter = tip };                      /* est. */
    n->pipe[15].end[1] = f_outlet(2);
    n->pipes = 16;
    n->snorkel_at = n->pipes;
    /* the tips (est.): two lower at either side of the middle, the centre one above */
    const double tips[3][3] = { { 0.11, -0.03, 0.33 }, { -0.11, -0.03, 0.33 }, { 0.0, -0.03, 0.45 } };
    for (int o = 0; o < 3; o++)
        for (int k = 0; k < 3; k++) n->outlet[o].at[k] = tips[o][k];
    n->outlets = 3;
    /* the cans at stage 050's finer cells: their plane-wave modes to their
     * cross mode, not 450-750 Hz (quality first: docs/stage-141.md) */
    d->muffler_resolution = 3.0;
}

/* The Polo 1.0 TSI's exhaust (stage 145), its sound's way from the head:
 *   - published: the manifold in the head (three short ports meeting in it),
 *     a single-scroll turbine with an electric wastegate, a three-way
 *     catalyst and a petrol particulate filter close to the engine (VW's
 *     EA211 data), a centre silencer and a rear one (aftermarket centre
 *     pipes are sold to replace the centre silencer: Supersprint 918113);
 *   - estimated, every one: the volumes, lengths and bores below; the
 *     turbine's effective nozzle area (a small turbo for a 1.0 L, its flow
 *     at idle and light load), the wastegate's (open off boost: VW's
 *     strategy for low back pressure there), the catalyst-and-filter's loss,
 *     the silencers' insides, the tailpipe turned down out of sight at the
 *     rear left.
 * The turbine is a restriction (its nozzle) between the manifold's volume and
 * its outlet's, the wastegate a pipe beside it: the pulses' energy is spent
 * through them and the volumes smooth what's left, which is why such engines
 * are quiet. At low frequencies a turbine passes sound almost unchanged
 * (Rammal and Abom, KTH): what it takes off above, its rotor's own loss, is
 * not in this model; Bernardo's recording (stage 146) is what will set it. */
static void ea211_polo(rb_engine_desc *d)
{
    rb_exhaust_net *n = &d->net;
    const double pipe = 0.048 - 2.0 * F_WALL;    /* est.: about 48 mm OD (an aftermarket oversize is 54) */
    const double down = 0.055 - 2.0 * F_WALL;    /* est.: the downpipe to the catalyst */
    memset(n, 0, sizeof *n);
    n->node[0] = (rb_net_node){ 0.15e-3, RB_PLACE_COLLECTOR, 0 };   /* the manifold and the turbine's volute (est.) */
    n->node[1] = (rb_net_node){ 0.25e-3, RB_PLACE_JUNCTION, 0 };    /* the turbine's outlet (est.) */
    n->nodes = 2;
    for (int i = 0; i < d->cylinders; i++) n->header_to[i] = f_node(0, RB_HEADER_ENTRY);
    /* 0 the turbine's way, 1 the wastegate's, 2 the downpipe with the catalyst
     * and the filter, 3 the front pipe, 4 the centre silencer, 5 the rear pipe,
     * 6 the rear silencer's chamber, 7 its packed tube, 8 the tailpipe */
    n->pipe[0] = (rb_net_pipe){ .role = RB_NET_ROLE_PIPE, .kind = RB_PIPE_EXHAUST, .number = 0,
                                .from = RB_PLACE_COLLECTOR, .from_number = 0, .to = RB_PLACE_JUNCTION,
                                .length = 0.08, .diameter = 0.040 };            /* est. */
    n->pipe[0].end[0] = (rb_net_end){ .kind = RB_NET_NODE, .index = 0, .entry_loss = RB_COLLECTOR_ENTRY,
                                      .area = 3.5e-4 };   /* the turbine's nozzle: 3.5 cm^2 effective (est.) */
    n->pipe[0].end[1] = f_node(1, 0.0);
    n->pipe[1] = (rb_net_pipe){ .role = RB_NET_ROLE_PIPE, .kind = RB_PIPE_EXHAUST, .number = 1,
                                .from = RB_PLACE_COLLECTOR, .from_number = 0, .to = RB_PLACE_JUNCTION,
                                .length = 0.05, .diameter = 0.022 };            /* est. */
    n->pipe[1].end[0] = (rb_net_end){ .kind = RB_NET_NODE, .index = 0, .entry_loss = RB_COLLECTOR_ENTRY,
                                      .area = 2.0e-4 };   /* the wastegate open: 2 cm^2 effective (est.) */
    n->pipe[1].end[1] = f_node(1, 0.0);
    n->pipe[2] = (rb_net_pipe){ .role = RB_NET_ROLE_PIPE, .kind = RB_PIPE_EXHAUST, .number = 2,
                                .from = RB_PLACE_JUNCTION, .from_number = 0, .to = RB_PLACE_CENTER,
                                .length = 0.45, .diameter = down, .catalyst = 1.0 };   /* est. */
    n->pipe[2].end[0] = f_node(1, RB_COLLECTOR_ENTRY);
    n->pipe[2].end[1] = (rb_net_end){ .kind = RB_NET_ONWARD, .index = 3,
                                      .area = 18e-4, .laminar = 60.0 };   /* the catalyst and the filter (est.) */
    n->pipe[3] = (rb_net_pipe){ .role = RB_NET_ROLE_PIPE, .kind = RB_PIPE_LINK, .number = 0,
                                .from = RB_PLACE_CENTER, .from_number = 0, .to = RB_PLACE_CENTER,
                                .length = 1.0, .diameter = pipe };              /* est. */
    n->pipe[3].end[1] = (rb_net_end){ .kind = RB_NET_ONWARD, .index = 4 };
    n->pipe[4] = (rb_net_pipe){ .role = RB_NET_ROLE_CAN, .kind = RB_PIPE_CENTER, .number = 0,
                                .from = RB_PLACE_CENTER, .from_number = 0, .to = RB_PLACE_CENTER,
                                .length = 0.30, .diameter = pipe, .packing = RB_PACKING,
                                .muffler = RB_MUFFLER_STRAIGHT };   /* a packed straight-through centre box (est.) */
    n->pipe[4].end[1] = (rb_net_end){ .kind = RB_NET_ONWARD, .index = 5 };
    n->pipe[5] = (rb_net_pipe){ .role = RB_NET_ROLE_PIPE, .kind = RB_PIPE_LINK, .number = 1,
                                .from = RB_PLACE_CENTER, .from_number = 0, .to = RB_PLACE_MUFFLER,
                                .length = 0.9, .diameter = pipe };              /* est. */
    n->pipe[5].end[1] = f_into(6, 0);
    n->pipe[6] = (rb_net_pipe){ .role = RB_NET_ROLE_CAN, .kind = RB_PIPE_MUFFLER, .number = 0,
                                .from = RB_PLACE_MUFFLER, .from_number = 0, .to = RB_PLACE_MUFFLER_OUTLET,
                                .length = 0.22, .diameter = sqrt(0.30 * 0.16),
                                .muffler = RB_MUFFLER_CHAMBERED };   /* a 300 x 160 mm oval box's chamber (est.) */
    n->pipe[7] = (rb_net_pipe){ .role = RB_NET_ROLE_CAN, .kind = RB_PIPE_MUFFLER, .number = 0,
                                .from = RB_PLACE_MUFFLER, .from_number = 0, .to = RB_PLACE_MUFFLER_OUTLET,
                                .length = 0.18, .diameter = pipe, .packing = RB_PACKING,
                                .muffler = RB_MUFFLER_STRAIGHT };    /* its packed half (est.) */
    n->pipe[7].end[0] = f_into(6, 1);
    n->pipe[7].end[1] = (rb_net_end){ .kind = RB_NET_ONWARD, .index = 8, .back = 1 };
    n->pipe[8] = (rb_net_pipe){ .role = RB_NET_ROLE_TAILPIPE, .kind = RB_PIPE_TAILPIPE, .number = 0,
                                .from = RB_PLACE_MUFFLER_OUTLET, .from_number = 0, .to = RB_PLACE_AIR,
                                .length = 0.20, .diameter = pipe };             /* est. */
    n->pipe[8].end[1] = f_outlet(0);
    n->pipes = 9;
    n->snorkel_at = n->pipes;
    /* the tailpipe turned down under the bumper at the rear left (est.) */
    const double at[3] = { 0.40, 0.05, 0.22 };
    for (int k = 0; k < 3; k++) n->outlet[0].at[k] = at[k];
    n->outlets = 1;
    d->muffler_resolution = 3.0;   /* the cans at stage 050's finer cells */
}

void rb_exhaust_model(rb_engine_desc *d, int preset)
{
    const double wall = 0.0012;
    d->outlet_radiation = 1;
    switch (preset) {
    case 0: /* 1AZ-FE/RAV4: BRExhaust 106-0135, 2-inch, two silencers. */
        d->exhaust_pipe_diameter = 0.0508 - 2.0 * wall;
        d->tailpipe_diameter = d->exhaust_pipe_diameter;
        d->exhaust_pipe_length = 0.65;
        d->center_length = 0.2413;
        d->center_diameter = d->exhaust_pipe_diameter;
        d->center_volume = 0.0026; /* estimated free volume within 127x127x241 mm body */
        d->link_length = 1.05;
        d->muffler_type = RB_MUFFLER_CHAMBERED;
        d->muffler_length = 0.7239;
        d->muffler_volume = 0.012; /* estimated free volume; exterior is NOT gas volume */
        d->muffler_area = 0.0019;
        d->tailpipe_length = 0.32;
        break;
    case 1: /* M54B30/E46 330i: Supersprint oversize twin 54 mm system. */
        d->exhaust_systems = 2;
        d->exhaust_pipe_diameter = 0.054 - 2.0 * wall;
        d->tailpipe_diameter = d->exhaust_pipe_diameter;
        d->exhaust_pipe_length = 0.55;
        d->center_length = 0.70; /* unresonated center pipes 787413 */
        d->center_diameter = d->exhaust_pipe_diameter;
        d->link_length = 0.55;
        d->muffler_type = RB_MUFFLER_STRAIGHT; /* two equivalent cores in rear 787406 */
        d->muffler_length = 0.52;
        d->tailpipe_length = 0.25;
        break;
    case 2: /* Mustang GT: Borla ATAK 140961, true dual 3-inch + merge X. */
        d->exhaust_pipe_diameter = 0.0762 - 2.0 * wall;
        d->tailpipe_diameter = d->exhaust_pipe_diameter; /* four-inch cosmetic tips excluded */
        d->exhaust_pipe_length = 0.80; /* retained OEM front pipes + cats, equivalent */
        d->junction_volume = 0.0012; /* estimated finite merge volume */
        d->junction_before_center = 1;
        d->center_length = 0.80;
        d->center_diameter = d->exhaust_pipe_diameter;
        d->link_length = 0.65;
        d->muffler_type = RB_MUFFLER_NONE; /* physical rear-delete, NOT catalog 140961 */
        d->muffler_length = 0.45;
        d->tailpipe_length = 0.30;
        break;
    case 3: /* 458: Tubi Style TSFE458C09.003.ANT, unvalved pipe option. */
        d->exhaust_pipe_diameter = 0.060; /* unpublished bore: explicit estimate */
        d->tailpipe_diameter = 0.060;
        d->exhaust_pipe_length = 0.75; /* OEM manifold/catalyst path retained */
        d->muffler_type = RB_MUFFLER_NONE;
        d->muffler_length = 0.35;
        d->tailpipe_length = 0.28;
        break;
    case 4: /* FB20/XV GP7: HKS LEGAMAX Premium 32018-AF012, 50.8 -> 45x2. */
        d->exhaust_pipe_diameter = 0.0508 - 2.0 * wall;
        d->exhaust_pipe_length = 1.0;
        d->center_length = 0.42;
        d->center_diameter = d->exhaust_pipe_diameter;
        d->center_packing = 1500.0; /* equivalent OEM center silencer, unmeasured */
        d->link_length = 0.85;
        d->junction_volume = 0.0007; /* equivalent Y ahead of twin rear cores */
        d->exhaust_systems = 2;
        d->muffler_type = RB_MUFFLER_STRAIGHT;
        d->muffler_length = 0.42;
        d->tailpipe_diameter = 0.045 - 2.0 * wall;
        d->tailpipe_length = 0.22;
        break;
    case 5: /* the Ferrari 458 Italia with its silencers, as a network (stage 141) */
        rb_exhaust_model(d, 3);   /* the legacy fields as the flat V8's (unused) */
        d->outlet_radiation = 1;
        f136_458(d);
        break;
    case 6: /* the Polo 1.0 TSI with its turbine, as a network (stage 145) */
        d->outlet_radiation = 1;
        ea211_polo(d);
        break;
    }
}
