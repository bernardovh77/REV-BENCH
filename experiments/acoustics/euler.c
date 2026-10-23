#include "euler.h"
#include <math.h>
#include <string.h>

size_t rb_fv_cells_required(int cells)
{
    return cells >= 4 && cells <= 65536 ? (size_t)4*cells+1:0;
}
int rb_fv_prepare(rb_fv_state *s, int cells, double dx, double gamma, int periodic,
                  rb_fv_cell *storage, size_t count)
{
    const size_t need=rb_fv_cells_required(cells);
    if (!s || !storage || !need || count < need || !isfinite(dx) || dx <= 0 ||
        !isfinite(gamma) || gamma <= 1 || gamma > 2 || (periodic != 0 && periodic != 1)) return -1;
    memset(s,0,sizeof *s); memset(storage,0,need*sizeof *storage);
    s->cells=cells; s->dx=dx; s->gamma=gamma; s->periodic=periodic;
    s->q=storage; s->stage=storage+cells; s->slope=storage+2*cells; s->flux=storage+3*cells;
    return 0;
}
rb_fv_cell rb_fv_make(double rho, double u, double p, double tag_fraction, double gamma)
{
    return (rb_fv_cell){rho,rho*u,p/(gamma-1)+.5*rho*u*u,rho*tag_fraction};
}
double rb_fv_pressure(rb_fv_cell q, double gamma)
{
    return (gamma-1)*(q.energy-.5*q.momentum*q.momentum/q.rho);
}
static int valid(rb_fv_cell q, double gamma)
{
    const double p=rb_fv_pressure(q,gamma);
    return isfinite(q.rho) && q.rho > 0 && isfinite(q.momentum) && isfinite(q.energy) &&
           isfinite(p) && p > 0 && isfinite(q.tag) && q.tag >= -1e-12*q.rho &&
           q.tag <= (1+1e-12)*q.rho;
}
static rb_fv_cell add(rb_fv_cell a, double k, rb_fv_cell b)
{
    return (rb_fv_cell){a.rho+k*b.rho,a.momentum+k*b.momentum,
                         a.energy+k*b.energy,a.tag+k*b.tag};
}
static rb_fv_cell scaled(rb_fv_cell a, double k)
{
    return (rb_fv_cell){a.rho*k,a.momentum*k,a.energy*k,a.tag*k};
}
static rb_fv_cell primitive(rb_fv_cell q, double gamma)
{
    return (rb_fv_cell){q.rho,q.momentum/q.rho,rb_fv_pressure(q,gamma),q.tag/q.rho};
}
static rb_fv_cell conservative(rb_fv_cell w, double gamma)
{
    return rb_fv_make(w.rho,w.momentum,w.energy,w.tag,gamma);
}
static double mc(double left, double right)
{
    if (left*right <= 0) return 0;
    return copysign(fmin(.5*fabs(left+right),fmin(2*fabs(left),2*fabs(right))),left);
}
static rb_fv_cell physical_flux(rb_fv_cell q, double gamma)
{
    const double u=q.momentum/q.rho,p=rb_fv_pressure(q,gamma);
    return (rb_fv_cell){q.momentum,q.momentum*u+p,(q.energy+p)*u,q.tag*u};
}
static rb_fv_cell hllc(rb_fv_state *s, rb_fv_cell l, rb_fv_cell r)
{
    const double g=s->gamma, ul=l.momentum/l.rho, ur=r.momentum/r.rho;
    const double pl=rb_fv_pressure(l,g), pr=rb_fv_pressure(r,g);
    const double cl=sqrt(g*pl/l.rho),cr=sqrt(g*pr/r.rho);
    const double sl=fmin(ul-cl,ur-cr), sr=fmax(ul+cl,ur+cr);
    const rb_fv_cell fl=physical_flux(l,g), fr=physical_flux(r,g);
    if (sl >= 0) return fl;
    if (sr <= 0) return fr;
    const double den=l.rho*(sl-ul)-r.rho*(sr-ur);
    const double sm=(pr-pl+l.rho*ul*(sl-ul)-r.rho*ur*(sr-ur))/den;
    const double ps=pl+l.rho*(sl-ul)*(sm-ul);
    if (isfinite(sm) && sm > sl && sm < sr && ps > 0) {
        const rb_fv_cell q=sm >= 0 ? l:r, f=sm >= 0 ? fl:fr;
        const double u=sm >= 0 ? ul:ur, p=sm >= 0 ? pl:pr, wave=sm >= 0 ? sl:sr;
        const double rho=q.rho*(wave-u)/(wave-sm);
        const rb_fv_cell star={rho,rho*sm,
            rho*(q.energy/q.rho+(sm-u)*(sm+p/(q.rho*(wave-u)))),rho*q.tag/q.rho};
        if (valid(star,g)) return add(f,wave,add(star,-1,q));
    }
    s->flux_fallbacks++;
    /* HLLE fallback still has to produce admissible stage states; never repair. */
    return scaled(add(add(scaled(fl,sr),-sl,fr),sl*sr,add(r,-1,l)),1/(sr-sl));
}
static int neighbor(const rb_fv_state *s,int i)
{
    if (s->periodic) return (i+s->cells)%s->cells;
    return i < 0 ? 0 : (i >= s->cells ? s->cells-1:i);
}
static int fluxes(rb_fv_state *s, const rb_fv_cell *q)
{
    const int n=s->cells;
    for (int i=0;i<n;i++) {
        if (!valid(q[i],s->gamma)) return -1;
        const rb_fv_cell w=primitive(q[i],s->gamma);
        const rb_fv_cell l=primitive(q[neighbor(s,i-1)],s->gamma),r=primitive(q[neighbor(s,i+1)],s->gamma);
        s->slope[i]=(rb_fv_cell){mc(w.rho-l.rho,r.rho-w.rho),mc(w.momentum-l.momentum,r.momentum-w.momentum),
            mc(w.energy-l.energy,r.energy-w.energy),mc(w.tag-l.tag,r.tag-w.tag)};
    }
    for (int face=0;face<=n;face++) {
        const int left=neighbor(s,face-1),right=neighbor(s,face);
        rb_fv_cell l=conservative(add(primitive(q[left],s->gamma),.5,s->slope[left]),s->gamma);
        rb_fv_cell r=conservative(add(primitive(q[right],s->gamma),-.5,s->slope[right]),s->gamma);
        if (!valid(l,s->gamma) || !valid(r,s->gamma)) {
            l=q[left]; r=q[right]; s->recon_fallbacks++;
        }
        s->flux[face]=hllc(s,l,r);
    }
    return 0;
}
int rb_fv_step(rb_fv_state *s, double dt)
{
    if (!s || !s->q || !isfinite(dt) || dt <= 0) return -1;
    double cfl=0;
    for (int i=0;i<s->cells;i++) {
        if (!valid(s->q[i],s->gamma)) { s->failures++; return -1; }
        cfl=fmax(cfl,(fabs(s->q[i].momentum/s->q[i].rho)+
            sqrt(s->gamma*rb_fv_pressure(s->q[i],s->gamma)/s->q[i].rho))*dt/s->dx);
    }
    s->max_cfl=fmax(s->max_cfl,cfl);
    if (cfl > .8) { s->failures++; return -1; }
    if (fluxes(s,s->q)) { s->failures++; return -1; }
    for (int i=0;i<s->cells;i++) {
        s->stage[i]=add(s->q[i],-dt/s->dx,add(s->flux[i+1],-1,s->flux[i]));
        if (!valid(s->stage[i],s->gamma)) { s->failures++; return -1; }
    }
    if (fluxes(s,s->stage)) { s->failures++; return -1; }
    for (int i=0;i<s->cells;i++) {
        s->q[i]=scaled(add(s->q[i],1,add(s->stage[i],-dt/s->dx,add(s->flux[i+1],-1,s->flux[i]))),.5);
        if (!valid(s->q[i],s->gamma)) { s->failures++; return -1; }
    }
    s->steps++;
    return 0;
}
