#ifndef RB_EXPERIMENT_MIXTURE_H
#define RB_EXPERIMENT_MIXTURE_H
/* Stage 043 isolated uniform-area Euler experiment, not production dispatch.
 * SI densities: species kg/m3, momentum kg/m2/s, sensible total energy J/m3.
 * A negative sensible energy is admissible. No chemistry/wall/area sources.
 * Caller owns storage. Numerical failure poisons the state until prepare.
 * Boundary 0=periodic, 1=transmissive, 2=closed reflecting walls. */
#include "gas.h"
#include <stddef.h>
typedef struct { double v[RB_NSPECIES+2]; } rb_mix_cell;
typedef struct { double rho,u,p,T,c,y[RB_NSPECIES]; } rb_mix_primitive;
typedef struct {
    int cells,boundary,failed;
    double dx,max_cfl;
    rb_mix_cell *q,*stage,*next,*left,*right,*flux;
    rb_mix_cell boundary_book; /* integrated (right-left) outward flux per area */
    unsigned long long steps,reconstruction_fallbacks,limited_cells,flux_fallbacks;
} rb_mix_state;
size_t rb_mix_storage(int cells);
int rb_mix_prepare(rb_mix_state *s,int cells,double dx,int boundary,
                   rb_mix_cell *storage,size_t count);
int rb_mix_decode(rb_mix_cell q,rb_mix_primitive *w);
int rb_mix_make(double p,double T,double u,const double y[RB_NSPECIES],rb_mix_cell *q);
int rb_mix_face(rb_mix_cell l,rb_mix_cell r,rb_mix_cell *flux);
int rb_mix_step(rb_mix_state *s,double dt);
#endif
