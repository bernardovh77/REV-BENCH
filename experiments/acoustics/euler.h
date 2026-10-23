#ifndef RB_EXPERIMENT_EULER_H
#define RB_EXPERIMENT_EULER_H

/* S2d-A uniform-area constant-gamma control, NOT the engine's mixture EOS.
 * Conserved densities per m^3: kg, kg m/s, J, and kg of passive tagged gas.
 * Periodic or transmissive boundaries; no heat/friction or engine adapter. */
#include <stddef.h>
typedef struct { double rho, momentum, energy, tag; } rb_fv_cell;
typedef struct {
    int cells, periodic;
    double dx, gamma;
    rb_fv_cell *q, *stage, *slope, *flux;
    unsigned long long steps, recon_fallbacks, flux_fallbacks, failures;
    double max_cfl;
} rb_fv_state;
size_t rb_fv_cells_required(int cells);
int rb_fv_prepare(rb_fv_state *s, int cells, double dx, double gamma, int periodic,
                  rb_fv_cell *storage, size_t count);
rb_fv_cell rb_fv_make(double rho, double u, double p, double tag_fraction, double gamma);
double rb_fv_pressure(rb_fv_cell q, double gamma);
int rb_fv_step(rb_fv_state *s, double dt);
#endif
