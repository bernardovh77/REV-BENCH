#include "material.h"
#include "thermal.h"
#include <math.h>
#include <string.h>
#define PI 3.14159265358979323846
static int positive(double x){return isfinite(x)&&x>0;}
static int failure(rb_mt_state *s){s->failed=1;return -2;}
static double width(const rb_mt_state *s,int i){return s->node[i+1].x-s->node[i].x;}
static int derive(rb_mt_state *s)
{
    for(int i=0;i<s->cells;i++) {
        rb_mt_cell *c=&s->cell[i];double V=width(s,i)*s->area;
        if(!positive(V)||!isfinite(c->E))return failure(s);
        c->T=rb_gas_T_from_u(&c->gas,c->E/c->mass,c->T);
        if(!isfinite(c->T)||c->T<200||c->T>990)return failure(s);
        c->p=c->mass*c->gas.R*c->T/V;
        c->c=sqrt(rb_gas_gamma(&c->gas,c->T)*c->gas.R*c->T);
        const double relative=fabs(c->p/s->p0-1);s->max_pressure_fraction=fmax(s->max_pressure_fraction,relative);
        if(relative>.03 || !positive(c->c) || fabs(s->reservoir_U)/c->c>.25)return failure(s);
    }
    return 0;
}
int rb_mt_prepare(rb_mt_state *s,int n,double area,double pressure,double U,
                  rb_mt_cell *cells,size_t nc,rb_mt_node *nodes,size_t nn)
{
    if(!s||n<4||n>8192||!cells||!nodes||nc<(size_t)n||nn<(size_t)n+1||!positive(area)||!positive(pressure)||!isfinite(U))return -1;
    memset(s,0,sizeof *s);memset(cells,0,(size_t)n*sizeof *cells);memset(nodes,0,((size_t)n+1)*sizeof *nodes);
    s->cells=n;s->cell=cells;s->node=nodes;s->area=area;s->p0=pressure;s->reservoir_U=s->frame_U=U;return 0;
}
double rb_mt_energy(const rb_mt_state *s)
{
    if(!s||!s->loaded||s->failed)return NAN;
    double E=s->wall_capacity*s->wall_T;
    for(int i=0;i<s->cells;i++)E+=s->cell[i].E;
    for(int i=0;i<=s->cells;i++)E+=.5*s->node[i].mass*s->node[i].v*s->node[i].v;
    return E;
}
double rb_mt_momentum(const rb_mt_state *s)
{
    if(!s||!s->loaded||s->failed)return NAN;
    double p=0;for(int i=0;i<=s->cells;i++)p+=s->node[i].mass*s->node[i].v;return p;
}
int rb_mt_load(rb_mt_state *s)
{
    if(!s||!s->cell||s->loaded||s->steps||s->failed)return -1;
    memset(s->initial_species,0,sizeof s->initial_species);
    for(int i=0;i<s->cells;i++) {
        rb_mt_cell *c=&s->cell[i];double total=0;
        for(int j=0;j<3;j++){if(!isfinite(c->m[j])||c->m[j]<0)return failure(s);total+=c->m[j];}
        if(fabs(total-1)>1e-12 || !positive(width(s,i)) || c->T<200 || c->T>990 || !positive(c->p))return failure(s);
        rb_gas_init(&c->gas,c->m);c->mass=c->p*s->area*width(s,i)/(c->gas.R*c->T);
        for(int j=0;j<3;j++){c->m[j]*=c->mass;s->initial_species[j]+=c->m[j];}
        c->E=c->mass*rb_gas_u(&c->gas,c->T);
    }
    for(int i=0;i<=s->cells;i++) {
        if(!isfinite(s->node[i].v))return failure(s);
        s->node[i].mass=.5*((i ? s->cell[i-1].mass:0)+(i<s->cells ? s->cell[i].mass:0));
    }
    if(derive(s))return -2;
    for(int end=0;end<2;end++) {
        int k=end ? s->cells-1:0;rb_mt_cell *c=&s->cell[k];
        s->impedance[end]=c->mass/(s->area*width(s,k))*c->c;
    }
    s->loaded=1;s->initial_energy=rb_mt_energy(s);s->initial_momentum=rb_mt_momentum(s);return 0;
}
int rb_mt_wall(rb_mt_state *s,double capacity,double T,double x0,double x1,double h_inner,double outer,double ambient_T)
{
    if(!s||!s->loaded||s->failed||s->steps||!positive(capacity)||!isfinite(T)||T<200||T>990||!isfinite(x0)||!isfinite(x1)||x1<=x0||
       !isfinite(h_inner)||h_inner<0||!isfinite(outer)||outer<0||ambient_T<200||ambient_T>990||!isfinite(ambient_T))return -1;
    s->wall_capacity=capacity;s->wall_T=T;s->wall_x0=x0;s->wall_x1=x1;s->h_inner=h_inner;s->outer=outer;s->ambient_T=ambient_T;
    s->initial_energy=rb_mt_energy(s);return 0;
}
static int heat(rb_mt_state *s,double dt)
{
    if(!s->wall_capacity)return 0;
    const double diameter=sqrt(4*s->area/PI);
    for(int i=0;i<s->cells;i++) {
        const double lo=s->node[i].x+s->shift,hi=s->node[i+1].x+s->shift;
        const double overlap=fmax(0,fmin(hi,s->wall_x1)-fmax(lo,s->wall_x0));if(!overlap)continue;
        rb_mt_cell *c=&s->cell[i];double y[3];for(int j=0;j<3;j++)y[j]=c->m[j]/c->mass;
        rb_tw_pair pair;
        if(rb_tw_prepare(&pair,c->mass,y,c->T,s->wall_capacity,s->wall_T,s->ambient_T,s->h_inner*PI*diameter*overlap,0))return failure(s);
        pair.gas=c->gas;pair.energy=c->E;pair.initial_energy=c->E+s->wall_capacity*s->wall_T;
        if(rb_tw_step(&pair,dt))return failure(s);
        c->wall_heat+=pair.energy-c->E;c->E=pair.energy;s->wall_T=pair.wall_T;
    }
    /* Exterior acts once per physical wall, never once per gas cell. */
    const double next=s->ambient_T+(s->wall_T-s->ambient_T)*exp(-s->outer*dt/s->wall_capacity);
    s->ambient_heat+=s->wall_capacity*(next-s->wall_T);s->wall_T=next;
    return derive(s);
}
int rb_mt_step(rb_mt_state *s,double dt,double incoming_left,double incoming_right,double acceleration)
{
    if(!s||!s->cell||!s->loaded||!positive(dt)||!isfinite(incoming_left)||!isfinite(incoming_right)||!isfinite(acceleration)||!isfinite(s->frame_U))return -1;
    if(s->failed)return -2;
    for(int i=0;i<s->cells;i++) {
        double cfl=s->cell[i].c*dt/width(s,i);s->max_cfl=fmax(s->max_cfl,cfl);
        if(cfl>.45)return failure(s);
    }
    if(heat(s,.5*dt))return -2;
    for(int i=0;i<s->cells;i++) {
        double cfl=s->cell[i].c*dt/width(s,i);s->max_cfl=fmax(s->max_cfl,cfl);
        if(cfl>.45)return failure(s);
    }
    const int N=s->cells;
    for(int i=0;i<N;i++)s->cell[i].midpoint=s->cell[i].p;
    int converged=0;
    for(int iteration=0;iteration<64;iteration++) {
        for(int i=0;i<=N;i++) {
            rb_mt_node *node=&s->node[i];double force,denominator=node->mass;
            if(i==0) {
                force=s->area*(s->p0+2*incoming_left-s->cell[0].midpoint-s->impedance[0]*(node->v-s->reservoir_U));
                denominator+=.5*dt*s->area*s->impedance[0];
            } else if(i==N) {
                force=s->area*(s->cell[N-1].midpoint-s->p0-2*incoming_right-s->impedance[1]*(node->v-s->reservoir_U));
                denominator+=.5*dt*s->area*s->impedance[1];
            } else force=s->area*(s->cell[i-1].midpoint-s->cell[i].midpoint);
            node->next_v=node->v+dt*(force+node->mass*acceleration)/denominator;
            node->next_x=node->x+dt*(.5*(node->v+node->next_v)-s->frame_U);
        }
        double error=0;
        for(int i=0;i<N;i++) {
            rb_mt_cell *c=&s->cell[i];double V=s->area*width(s,i);
            /* Compatible geometric conservation: the pressure work uses the
             * accepted midpoint velocity difference, not cancellation of two
             * O(metre) positions to recover an O(picometre) displacement. */
            const double deltaV=.5*dt*s->area*((s->node[i+1].v-s->node[i].v)+(s->node[i+1].next_v-s->node[i].next_v));
            const double nextV=V+deltaV;
            if(!positive(nextV))return failure(s);
            c->next_E=c->E-c->midpoint*deltaV;
            const double midT=rb_gas_T_from_u(&c->gas,.5*(c->E+c->next_E)/c->mass,c->T);
            const double p=c->mass*c->gas.R*midT/(.5*(V+nextV));
            if(!isfinite(p)||!isfinite(midT)||midT<200||midT>990)return failure(s);
            c->trial_pressure=p;error=fmax(error,fabs(p-c->midpoint));
        }
        s->iterations++;
        if(error<2e-8){if((unsigned long long)iteration+1>s->max_iterations)s->max_iterations=(unsigned long long)iteration+1;converged=1;s->max_iteration_residual=fmax(s->max_iteration_residual,error);break;}
        for(int i=0;i<N;i++)s->cell[i].midpoint=s->cell[i].trial_pressure;
    }
    if(!converged)return failure(s);
    const double vl=.5*(s->node[0].v+s->node[0].next_v),vr=.5*(s->node[N].v+s->node[N].next_v);
    const double pl=s->p0+2*incoming_left-s->impedance[0]*(vl-s->reservoir_U);
    const double pr=s->p0+2*incoming_right+s->impedance[1]*(vr-s->reservoir_U);
    s->boundary_work+=dt*s->area*(pl*vl-pr*vr);s->boundary_impulse+=dt*s->area*(pl-pr);
    for(int i=0;i<=N;i++) {
        rb_mt_node *node=&s->node[i];s->body_work+=dt*node->mass*acceleration*.5*(node->v+node->next_v);
        s->body_impulse+=dt*node->mass*acceleration;
        node->v=node->next_v;node->x=node->next_x;
    }
    const double shift=dt*s->frame_U-s->shift_compensation,next=s->shift+shift;
    s->shift_compensation=(next-s->shift)-shift;s->shift=next;
    for(int i=0;i<N;i++)s->cell[i].E=s->cell[i].next_E;
    if(derive(s)||heat(s,.5*dt))return -2;
    for(int i=0;i<=N;i++) {
        double c=s->cell[i==N ? N-1:i].c;
        if(fabs(s->node[i].v-s->reservoir_U)/c>.05)return failure(s);
    }
    s->steps++;return 0;
}
void rb_mt_left_inventory(const rb_mt_state *s,double x,double species[3],double *internal)
{
    for(int j=0;j<3;j++)species[j]=0;
    *internal=0;
    if(!s||!s->loaded||s->failed){for(int j=0;j<3;j++)species[j]=NAN;*internal=NAN;return;}
    for(int i=0;i<s->cells;i++) {
        double f=fmax(0,fmin(1,(x-s->shift-s->node[i].x)/width(s,i)));
        for(int j=0;j<3;j++)species[j]+=f*s->cell[i].m[j];
        *internal+=f*s->cell[i].E;
    }
}
double rb_mt_perturbation(const rb_mt_state *s)
{
    if(!s||!s->loaded||s->failed)return NAN;
    double E=0;
    for(int i=0;i<s->cells;i++) {
        const rb_mt_cell *c=&s->cell[i];double V=width(s,i)*s->area,rho=c->mass/V,dp=c->p-s->p0;
        const double vl=s->node[i].v-s->reservoir_U,vr=s->node[i+1].v-s->reservoir_U;
        E+=.5*V*dp*dp/(rho*c->c*c->c)+.25*c->mass*(vl*vl+vr*vr);
    }
    return E;
}
