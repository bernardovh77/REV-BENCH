#include "coupled_duct.h"
#include <math.h>
#include <string.h>

size_t rb_cd_workspace(int n) { return n >= 4 && n <= 8192 ? 7*(size_t)n+1:0; }
static int positive(double x) { return isfinite(x) && x > 0; }
static rb_cd_vec add(rb_cd_vec a, rb_cd_vec b, double scale)
{ for(int k=0;k<4;k++) a.v[k]+=scale*b.v[k]; return a; }
static rb_cd_vec chars(const rb_cd_state *s, rb_cd_vec q)
{
    const double pi=(s->spec.gamma-1)*(q.v[2]-s->e0);
    return (rb_cd_vec){{.5*(pi+s->c*q.v[1]),.5*(pi-s->c*q.v[1]),
        q.v[0]-s->spec.rho-pi/(s->c*s->c),q.v[3]-s->spec.tag_fraction*q.v[0]}};
}
static rb_cd_vec physical(const rb_cd_state *s, rb_cd_vec w)
{
    const double pi=w.v[0]+w.v[1], rho=s->spec.rho+w.v[2]+pi/(s->c*s->c);
    return (rb_cd_vec){{rho,(w.v[0]-w.v[1])/s->c,
        s->e0+pi/(s->spec.gamma-1),s->spec.tag_fraction*rho+w.v[3]}};
}
double rb_cd_pressure(const rb_cd_state *s, int cell)
{
    if(!s || cell<0 || cell>=s->spec.cells) return NAN;
    const rb_cd_vec q=s->q[cell];
    return (s->spec.gamma-1)*(q.v[2]-.5*q.v[1]*q.v[1]/q.v[0]);
}
static rb_cd_vec totals(const rb_cd_state *s, int side)
{
    rb_cd_vec sum={{0}};
    const int a=side ? s->spec.gas_cells:0;
    const int b=side ? s->spec.cells:s->spec.gas_cells;
    for(int i=a;i<b;i++) sum=add(sum,s->q[i],s->dx*s->spec.area);
    return sum;
}
rb_cd_vec rb_cd_balance(const rb_cd_state *s, int side)
{
    rb_cd_vec sum=add(totals(s,side),s->initial[!!side],-1);
    sum=add(sum,side ? s->external_right:s->external_left,side ? 1:-1);
    return add(sum,s->interface_transfer,side ? -1:1);
}
static int admissible(rb_cd_state *s, const rb_cd_vec *q)
{
    for(int i=0;i<s->spec.cells;i++) {
        for(int k=0;k<4;k++) if(!isfinite(q[i].v[k])) return RB_CD_STATE;
        const double rho=q[i].v[0], m=q[i].v[1];
        if(!(rho>0) || !(q[i].v[2]-.5*m*m/rho>0) ||
            q[i].v[3]<-1e-12 || q[i].v[3]>rho+1e-12) return RB_CD_STATE;
    }
    return RB_CD_OK;
}
int rb_cd_prepare(rb_cd_state *s, const rb_cd_spec *p, rb_cd_vec *memory, size_t count)
{
    if(!s || !p || !memory || !rb_cd_workspace(p->cells) ||
        p->gas_cells<1 || p->gas_cells>p->cells || !positive(p->rate) ||
        !positive(p->length) || !positive(p->area) || !positive(p->rho) ||
        !positive(p->pressure) || !isfinite(p->gamma) || p->gamma<=1 || p->gamma>2 ||
        !isfinite(p->tag_fraction) || p->tag_fraction<0 || p->tag_fraction>1 ||
        !isfinite(p->reflection) || fabs(p->reflection)>1) return RB_CD_ARGUMENT;
    if(count<rb_cd_workspace(p->cells)) return RB_CD_CAPACITY;
    const double c=sqrt(p->gamma*(p->pressure/p->rho)), dx=p->length/p->cells;
    const double e0=p->pressure/(p->gamma-1), h0=p->gamma*e0/p->rho;
    if(!positive(c) || !positive(dx) || !positive(e0) || !positive(h0) ||
       !positive(p->rho*p->area*p->length) || !positive(e0*p->area*p->length)) return RB_CD_ARGUMENT;
    const double courant=c/(p->rate*dx);
    if(!positive(courant)) return RB_CD_ARGUMENT;
    if(fabs(courant-1)>1e-10) return RB_CD_GRID;
    memset(s,0,sizeof *s); s->spec=*p; s->c=c; s->dx=dx;
    s->e0=e0; s->h0=h0;
    memset(memory,0,rb_cd_workspace(p->cells)*sizeof *memory);
    s->q=memory; s->waves=memory+p->cells; s->gas=memory+2*p->cells;
    s->stage=memory+3*p->cells; s->rhs=memory+4*p->cells;
    s->slope=memory+5*p->cells; s->face=memory+6*p->cells;
    for(int i=0;i<p->cells;i++) s->q[i]=(rb_cd_vec){{p->rho,0,s->e0,p->tag_fraction*p->rho}};
    s->initial[0]=totals(s,0); s->initial[1]=totals(s,1);
    return RB_CD_OK;
}
int rb_cd_load(rb_cd_state *s, const rb_cd_vec *initial, int cells)
{
    if(!s || !s->q || !initial || s->steps || s->failures || cells!=s->spec.cells)
        return RB_CD_ARGUMENT;
    memmove(s->stage,initial,cells*sizeof *initial);
    if(admissible(s,s->stage)) return RB_CD_STATE;
    memcpy(s->q,s->stage,cells*sizeof *initial);
    s->initial[0]=totals(s,0); s->initial[1]=totals(s,1);
    return RB_CD_OK;
}

int rb_cd_seed(rb_cd_state *s, double amplitude, double center, double width)
{
    if(!s || !s->q || s->steps || !isfinite(amplitude) || amplitude<0 || amplitude>.1 ||
        !isfinite(center) || center<=0 || center>=1 || !positive(width) || width>.25)
        return RB_CD_ARGUMENT;
    for(int i=0;i<s->spec.cells;i++) {
        const double d=(((i+.5)/s->spec.cells)-center)/width;
        const double p=s->spec.pressure*(1+amplitude*exp(-d*d));
        const double rho=s->spec.rho*pow(p/s->spec.pressure,1/s->spec.gamma);
        const double u=2*(sqrt(s->spec.gamma*p/rho)-s->c)/(s->spec.gamma-1);
        s->q[i]=(rb_cd_vec){{rho,rho*u,p/(s->spec.gamma-1)+.5*rho*u*u,
                           s->spec.tag_fraction*rho}};
    }
    s->initial[0]=totals(s,0); s->initial[1]=totals(s,1);
    return RB_CD_OK;
}
static rb_cd_vec residual(const rb_cd_state *s, rb_cd_vec q)
{
    const double r=q.v[0], m=q.v[1], g=s->spec.gamma;
    return (rb_cd_vec){{0,(3-g)*m*m/(2*r),
        m*(g*q.v[2]/r-s->h0)-(g-1)*m*m*m/(2*r*r),
        (q.v[3]/r-s->spec.tag_fraction)*m}};
}
static rb_cd_vec end_state(const rb_cd_state *s, rb_cd_vec q, int right)
{
    rb_cd_vec w=chars(s,q);
    if(right) w.v[1]=s->spec.reflection*w.v[0]; else w.v[0]=0;
    return physical(s,w);
}
static int residual_rhs(rb_cd_state *s, const rb_cd_vec *q, double tau)
{
    const int n=s->spec.cells;
    for(int i=1;i<n;i++) {
        const rb_cd_vec a=q[i-1], b=q[i];
        const double speed=fmax(s->spec.gamma,3-s->spec.gamma)*
            fmax(fabs(a.v[1]/a.v[0]),fabs(b.v[1]/b.v[0]));
        s->max_residual_cfl=fmax(s->max_residual_cfl,tau/s->dx*speed);
        if(tau/s->dx*speed>.45) return RB_CD_STATE;
        rb_cd_vec f=add(residual(s,a),residual(s,b),1);
        for(int k=0;k<4;k++) f.v[k]=.5*f.v[k]-.5*speed*(b.v[k]-a.v[k]);
        s->face[i]=f;
    }
    s->face[0]=residual(s,end_state(s,q[0],0));
    s->face[n]=residual(s,end_state(s,q[n-1],1));
    for(int i=0;i<n;i++) {
        s->rhs[i]=add(s->face[i],s->face[i+1],-1);
        for(int k=0;k<4;k++) s->rhs[i].v[k]/=s->dx;
    }
    return RB_CD_OK;
}
static void book(rb_cd_state *s, rb_cd_vec left, rb_cd_vec right, rb_cd_vec edge, double tau)
{
    const double weight=tau*s->spec.area;
    s->external_left=add(s->external_left,left,weight);
    s->external_right=add(s->external_right,right,weight);
    s->interface_transfer=add(s->interface_transfer,edge,weight);
    s->last_interface=add(s->last_interface,edge,weight);
}
static int nonlinear_half(rb_cd_state *s)
{
    const int n=s->spec.cells, cut=s->spec.gas_cells;
    const double tau=.5/s->spec.rate;
    if(residual_rhs(s,s->q,tau)) return RB_CD_STATE;
    const rb_cd_vec l=s->face[0], r=s->face[n], edge=s->face[cut];
    for(int i=0;i<n;i++) s->stage[i]=add(s->q[i],s->rhs[i],tau);
    if(admissible(s,s->stage) || residual_rhs(s,s->stage,tau)) return RB_CD_STATE;
    for(int i=0;i<n;i++) {
        rb_cd_vec q=add(s->q[i],s->stage[i],1);
        q=add(q,s->rhs[i],tau);
        for(int k=0;k<4;k++) s->q[i].v[k]=.5*q.v[k];
    }
    book(s,add(l,s->face[0],1),add(r,s->face[n],1),add(edge,s->face[cut],1),tau*.5);
    return admissible(s,s->q);
}
static double mc(double a, double b)
{
    if(a*b<=0) return 0;
    return copysign(fmin(.5*fabs(a+b),fmin(2*fabs(a),2*fabs(b))),a);
}
static void linear_rhs(rb_cd_state *s, const rb_cd_vec *w, double ghost_a, double incoming_b)
{
    const int n=s->spec.gas_cells, whole=n==s->spec.cells;
    for(int i=0;i<n;i++) for(int k=0;k<2;k++) {
        const double before=i ? w[i-1].v[k]:(k ? w[0].v[1]:0);
        const double after=i<n-1 ? w[i+1].v[k]:(k ? incoming_b:ghost_a);
        s->slope[i].v[k]=mc(w[i].v[k]-before,after-w[i].v[k]);
    }
    s->face[0]=(rb_cd_vec){{0,w[0].v[1]-.5*s->slope[0].v[1],0,0}};
    for(int i=1;i<=n;i++) {
        const double a=w[i-1].v[0]+.5*s->slope[i-1].v[0];
        const double b=i<n ? w[i].v[1]-.5*s->slope[i].v[1]:
                      (whole ? s->spec.reflection*a:incoming_b);
        s->face[i]=(rb_cd_vec){{a,b,0,0}};
    }
    for(int i=0;i<n;i++) s->rhs[i]=(rb_cd_vec){{
        -s->c/s->dx*(s->face[i+1].v[0]-s->face[i].v[0]),
         s->c/s->dx*(s->face[i+1].v[1]-s->face[i].v[1]),0,0}};
}
static rb_cd_vec linear_flux(const rb_cd_state *s, rb_cd_vec wave)
{
    const double m=(wave.v[0]-wave.v[1])/s->c;
    return (rb_cd_vec){{m,s->spec.pressure+wave.v[0]+wave.v[1],s->h0*m,s->spec.tag_fraction*m}};
}
static int linear_tick(rb_cd_state *s)
{
    const int n=s->spec.cells, cut=s->spec.gas_cells, whole=cut==n;
    const double dt=1/s->spec.rate, tau=dt*.5;
    for(int i=0;i<n;i++) s->waves[i]=chars(s,s->q[i]);
    memcpy(s->gas,s->waves,cut*sizeof *s->gas);
    const double incoming=whole ? s->spec.reflection*s->gas[cut-1].v[0]:s->waves[cut].v[1];
    const double ghost=whole ? s->gas[cut-1].v[0]:s->waves[cut].v[0];
    s->current_return_Pa=incoming;
    double outgoing=0;
    rb_cd_vec accepted={{0}}, external={{0}};
    for(int sub=0;sub<2;sub++) {
        linear_rhs(s,s->gas,ghost,incoming);
        const rb_cd_vec l=s->face[0], r=s->face[cut];
        for(int i=0;i<cut;i++) s->stage[i]=add(s->gas[i],s->rhs[i],tau);
        linear_rhs(s,s->stage,ghost,incoming);
        for(int i=0;i<cut;i++) {
            rb_cd_vec q=add(s->gas[i],s->stage[i],1); q=add(q,s->rhs[i],tau);
            for(int k=0;k<4;k++) s->gas[i].v[k]=.5*q.v[k];
        }
        outgoing+=.25*(r.v[0]+s->face[cut].v[0]);
        external=add(external,add(linear_flux(s,l),linear_flux(s,s->face[0]),1),tau*.5);
        accepted=add(accepted,add(linear_flux(s,r),linear_flux(s,s->face[cut]),1),tau*.5);
    }
    s->accepted_outgoing_Pa=outgoing;
    for(int i=0;i<cut;i++) s->q[i]=physical(s,s->gas[i]);
    for(int i=cut;i<n;i++) {
        rb_cd_vec w=s->waves[i];
        w.v[0]=i==cut ? outgoing:s->waves[i-1].v[0];
        w.v[1]=i==n-1 ? s->spec.reflection*s->waves[i].v[0]:s->waves[i+1].v[1];
        s->q[i]=physical(s,w);
    }
    const rb_cd_vec last=s->waves[n-1];
    const rb_cd_vec right=whole ? accepted:add((rb_cd_vec){{0}},
        linear_flux(s,(rb_cd_vec){{last.v[0],s->spec.reflection*last.v[0],0,0}}),dt);
    book(s,external,right,accepted,1);
    return admissible(s,s->q);
}
int rb_cd_step(rb_cd_state *s)
{
    if(!s || !s->q) return RB_CD_ARGUMENT;
    if(s->failures) return RB_CD_STATE; /* a failed candidate must be prepared again */
    s->last_interface=(rb_cd_vec){{0}};
    int status=nonlinear_half(s); s->failed_stage=1;
    if(!status) { status=linear_tick(s); s->failed_stage=2; }
    if(!status) { status=nonlinear_half(s); s->failed_stage=3; }
    if(status) { s->failures++; return status; }
    s->failed_stage=0; s->steps++;
    for(int i=0;i<s->spec.cells;i++) s->max_pressure_fraction=fmax(s->max_pressure_fraction,
        fabs(rb_cd_pressure(s,i)/s->spec.pressure-1));
    return RB_CD_OK;
}
