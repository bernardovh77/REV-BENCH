#ifndef RB_EXPERIMENT_FIXED_TRANSPORT_H
#define RB_EXPERIMENT_FIXED_TRANSPORT_H
/* 114 fixed-domain research transport. Two named contact constituents,
 * production u(T), pressure-equilibrium numerical interface, one conserved E.
 * All cell/face storage is caller owned. Not production engine dispatch. */
#include "gas.h"
#include <stddef.h>
typedef struct { double r[2],momentum,energy,alpha[2]; } rb_ft_q;
typedef struct { double p,u,T[2],alpha[2],rho,c; } rb_ft_primitive;
typedef struct { rb_ft_q f;double u,p,alpha[2]; } rb_ft_flux;
typedef struct { double m[2],momentum,energy; } rb_ft_book;
typedef struct {
    rb_ft_q q,stage,next;
    rb_ft_primitive w,slope;
    rb_ft_flux face,saved;
    rb_ft_book transfer;
    double wall_heat;
} rb_ft_slot;
typedef struct {
    int cells,loaded,failed;
    double length,area,dx,p0,T_reference[2];
    rb_gas gas[2];double y[2][3];rb_ft_slot *slot;
    rb_ft_book initial;
    double max_cfl,max_pressure_fraction,wall_capacity,wall_T;
    double wall_x0,wall_x1,h_inner,outer,ambient_T,ambient_heat,gas_heat;
    unsigned long long steps,recon_fallbacks,eos_iterations;
} rb_ft_state;
int rb_ft_prepare(rb_ft_state *s,int n,double length,double area,double p0,
                  const double y0[3],const double y1[3],double T0,double T1,
                  rb_ft_slot *storage,size_t count);
rb_ft_q rb_ft_make(const rb_ft_state *s,double p,double u,double alpha,double T0,double T1);
int rb_ft_decode(const rb_ft_state *s,rb_ft_q q,rb_ft_primitive *w);
int rb_ft_load(rb_ft_state *s);
int rb_ft_wall(rb_ft_state *s,double capacity,double T,double x0,double x1,
                double h_inner,double outer,double ambient_T);
/* Conservative current face solve: flux densities in +x, u and p of contact.
 * The flux energy contains total enthalpy; pressure work is not added again. */
int rb_ft_riemann(const rb_ft_state *s,rb_ft_q left,rb_ft_q right,rb_ft_flux *out);
int rb_ft_step(rb_ft_state *s,double dt,rb_ft_q left,rb_ft_q right);
/* Same-area, same-dx, same material definitions; shared face solved ONCE at
 * each RK stage. The next engine/network caller must retain this ownership. */
int rb_ft_step_pair(rb_ft_state *left,rb_ft_state *right,double dt,
                     rb_ft_q reservoir_left,rb_ft_q reservoir_right);
rb_ft_book rb_ft_inventory(const rb_ft_state *s,int lo,int hi);
double rb_ft_perturbation(const rb_ft_state *s,double U);
#endif
