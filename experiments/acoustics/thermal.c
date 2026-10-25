#include "thermal.h"
#include <math.h>
#include <string.h>
static int positive(double x){return isfinite(x)&&x>0;}
static int temperature(double T){return isfinite(T)&&T>=20&&T<=4000;}
int rb_tw_prepare(rb_tw_pair *s,double mass,const double y[RB_NSPECIES],double T,
                  double capacity,double wall_T,double ambient_T,double inner,double outer)
{
    if(!s || !y || !positive(mass) || !positive(capacity) || !temperature(T) ||
       !temperature(wall_T) || !temperature(ambient_T) || !isfinite(inner) || inner<0 ||
       !isfinite(outer) || outer<0)return -1;
    double sum=0;for(int j=0;j<RB_NSPECIES;j++){if(!isfinite(y[j])||y[j]<0)return -1;sum+=y[j];}
    if(fabs(sum-1)>1e-12)return -1;
    memset(s,0,sizeof *s);rb_gas_init(&s->gas,y);s->mass=mass;s->energy=mass*rb_gas_u(&s->gas,T);
    s->wall_capacity=capacity;s->wall_T=wall_T;s->ambient_T=ambient_T;s->inner=inner;s->outer=outer;
    s->initial_energy=s->energy+capacity*wall_T;return 0;
}
double rb_tw_temperature(const rb_tw_pair *s)
{return !s||s->failures||!positive(s->mass) ? NAN:rb_gas_T_from_u(&s->gas,s->energy/s->mass,600);}
double rb_tw_balance(const rb_tw_pair *s)
{return !s||s->failures ? NAN:s->energy+s->wall_capacity*s->wall_T-s->initial_energy-s->ambient_heat;}
int rb_tw_step(rb_tw_pair *s,double dt)
{
    if(!s || !positive(dt) || !positive(s->mass))return -1;
    if(s->failures)return -2;
    const double oldT=rb_tw_temperature(s),oldE=s->energy,C=s->wall_capacity;
    double lo=fmin(oldT,fmin(s->wall_T,s->ambient_T)),hi=fmax(oldT,fmax(s->wall_T,s->ambient_T));
    double T=oldT,energy=oldE,wall=s->wall_T;
    int converged=0;
    for(int iteration=0;iteration<64;iteration++) {
        energy=s->mass*rb_gas_u(&s->gas,T);const double Q=energy-oldE;
        wall=(C*s->wall_T-Q+dt*s->outer*s->ambient_T)/(C+dt*s->outer);
        const double residual=Q-dt*s->inner*(wall-T);
        const double derivative=s->mass*rb_gas_cv(&s->gas,T)*(1+dt*s->inner/(C+dt*s->outer))+dt*s->inner;
        double delta=residual/derivative;
        if(fabs(delta)<=2e-12*fmax(1,T)){converged=1;break;}
        if(residual>0)hi=T;else lo=T;
        double next=T-delta;if(next<=lo || next>=hi || !isfinite(next))next=.5*(lo+hi);T=next;
    }
    if(!converged || !temperature(T) || !temperature(wall)){s->failures++;return -2;}
    s->energy=energy;s->wall_T=wall;s->ambient_heat+=dt*s->outer*(s->ambient_T-wall);s->steps++;return 0;
}
double rb_tw_capacity(double diameter,double length,double thickness,double density,double cp)
{
    if(!positive(diameter)||!positive(length)||!positive(thickness)||!positive(density)||!positive(cp))return NAN;
    return 3.14159265358979323846*length*thickness*(diameter+thickness)*density*cp;
}
