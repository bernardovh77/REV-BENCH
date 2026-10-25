#ifndef RB_EXPERIMENT_MATERIAL_H
#define RB_EXPERIMENT_MATERIAL_H
/* 048 material-coordinate correctness reference. One uniform-area duct,
 * fixed material species, compatible nodal force/cell work, weak acoustics.
 * No remapping/engine dispatch. Nodes are relative to a translation frame;
 * lab position is x+shift, while v and kinetic energy are always lab values.
 * The externally selected coordinate frame never supplies physical work. */
#include "gas.h"
#include <stddef.h>
typedef struct {
    rb_gas gas;
    double m[3],mass,E,T,p,c,wall_heat;
    double midpoint,next_E,trial_pressure;
} rb_mt_cell;
typedef struct {double x,v,mass,next_x,next_v;} rb_mt_node;
typedef struct {
    int cells,failed,loaded;
    rb_mt_cell *cell;rb_mt_node *node;
    double area,p0,reservoir_U,frame_U,shift,shift_compensation;
    double impedance[2],boundary_work,body_work,boundary_impulse,body_impulse;
    double initial_energy,initial_momentum,initial_species[3];
    double wall_capacity,wall_T,wall_x0,wall_x1,h_inner,outer,ambient_T,ambient_heat;
    double max_cfl,max_pressure_fraction,max_iteration_residual;
    unsigned long long steps,iterations,max_iterations;
} rb_mt_state;
int rb_mt_prepare(rb_mt_state *s,int n,double area,double pressure,double U,
                  rb_mt_cell *cells,size_t nc,rb_mt_node *nodes,size_t nn);
/* Set edges/x, node v, each cell T/p and m[] as mass fractions, then load.
 * After loading m[] are immutable actual kilograms, never normalized in tick. */
int rb_mt_load(rb_mt_state *s);
int rb_mt_wall(rb_mt_state *s,double capacity,double T,double x0,double x1,
                double h_inner,double outer,double ambient_T);
int rb_mt_step(rb_mt_state *s,double dt,double incident_left,double incident_right,double acceleration);
double rb_mt_energy(const rb_mt_state *s);
double rb_mt_momentum(const rb_mt_state *s);
double rb_mt_perturbation(const rb_mt_state *s);
void rb_mt_left_inventory(const rb_mt_state *s,double x,double species[3],double *internal);
#endif
