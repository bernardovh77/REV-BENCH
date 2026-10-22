/* Complete valve-to-air configurations. Published tubing is normally OD;
 * 1.2 mm wall below is an explicit estimate, not a published bore. All
 * longitudinal lengths except the Toyota can bodies are estimated. Keep
 * measured dimensions, inferred internals and OEM upstream sections separate
 * in the research ledger. No sound-matching EQ is applied here. */
#include "exhaust_models.h"
#include "rb.h"

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
        d->muffler_type = RB_MUFFLER_STRAIGHT;
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
    }
}
