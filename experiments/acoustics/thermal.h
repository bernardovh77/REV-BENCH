#ifndef RB_EXPERIMENT_THERMAL_H
#define RB_EXPERIMENT_THERMAL_H
/* 047 isolated finite-capacity wall; SI. Not production engine dispatch.
 * One gas internal energy, one solid energy Cw*Tw, one exterior heat book.
 * Conductances supplied explicitly, never fitted from an audio recording. */
#include "gas.h"
typedef struct {
    rb_gas gas;
    double mass,energy,wall_capacity,wall_T,ambient_T,inner,outer;
    double initial_energy,ambient_heat;
    unsigned long long steps,failures;
} rb_tw_pair;
int rb_tw_prepare(rb_tw_pair *s,double mass,const double y[RB_NSPECIES],double T,
                  double capacity,double wall_T,double ambient_T,double inner,double outer);
int rb_tw_step(rb_tw_pair *s,double dt);
double rb_tw_temperature(const rb_tw_pair *s);
double rb_tw_balance(const rb_tw_pair *s);
double rb_tw_capacity(double diameter,double length,double thickness,double density,double cp);
#endif
