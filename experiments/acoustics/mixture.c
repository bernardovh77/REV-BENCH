#include "mixture.h"
#include <math.h>
#include <string.h>

enum { MOM=RB_NSPECIES,ENE=RB_NSPECIES+1,NV=RB_NSPECIES+2 };
static rb_mix_cell add(rb_mix_cell a,double k,rb_mix_cell b)
{ for(int j=0;j<NV;j++) a.v[j]+=k*b.v[j]; return a; }
static rb_mix_cell scale(rb_mix_cell a,double k)
{ for(int j=0;j<NV;j++) a.v[j]*=k; return a; }
size_t rb_mix_storage(int cells)
{ return cells>=4 && cells<=65536 ? (size_t)6*cells+1:0; }
int rb_mix_prepare(rb_mix_state *s,int cells,double dx,int boundary,
                   rb_mix_cell *storage,size_t count)
{
    size_t need=rb_mix_storage(cells);
    if(!s || !storage || !need || count<need || !isfinite(dx) || dx<=0 ||
       boundary<0 || boundary>2) return -1;
    memset(s,0,sizeof *s); memset(storage,0,need*sizeof *storage);
    s->cells=cells;s->dx=dx;s->boundary=boundary;s->q=storage;
    s->stage=storage+cells;s->next=storage+2*cells;
    s->left=storage+3*cells;s->right=storage+4*cells;s->flux=storage+5*cells;
    return 0;
}
int rb_mix_decode(rb_mix_cell q,rb_mix_primitive *w)
{
    if(!w) return -1;
    for(int j=0;j<NV;j++) if(!isfinite(q.v[j])) return -1;
    double rho=0;
    for(int j=0;j<RB_NSPECIES;j++) { if(q.v[j]<0) return -1; rho+=q.v[j]; }
    if(!isfinite(rho) || rho<=0) return -1;
    w->rho=rho;w->u=q.v[MOM]/rho;
    for(int j=0;j<RB_NSPECIES;j++) w->y[j]=q.v[j]/rho;
    rb_gas g;rb_gas_init(&g,w->y);
    const double e=q.v[ENE]/rho-.5*w->u*w->u;
    /* Bracket before Newton: production u(T) is monotone; never accept its
     * temperature floor or extrapolate beyond this experiment's 20–4000 K. */
    if(!isfinite(e) || e<rb_gas_u(&g,20) || e>rb_gas_u(&g,4000)) return -1;
    w->T=rb_gas_T_from_u(&g,e,600);
    const double cv=rb_gas_cv(&g,w->T);
    if(!isfinite(w->T) || w->T<20 || w->T>4000 || !isfinite(cv) || cv<=0 ||
       fabs(rb_gas_u(&g,w->T)-e)>1e-7*fmax(1,fabs(e))) return -1;
    w->p=rho*g.R*w->T;w->c=sqrt((1+g.R/cv)*g.R*w->T);
    return isfinite(w->p) && w->p>0 && isfinite(w->c) && w->c>0 ? 0:-1;
}
int rb_mix_make(double p,double T,double u,const double y[RB_NSPECIES],rb_mix_cell *q)
{
    if(!q || !y || !isfinite(p) || p<=0 || !isfinite(T) || T<20 || T>4000 || !isfinite(u)) return -1;
    double sum=0;
    for(int j=0;j<RB_NSPECIES;j++) { if(!isfinite(y[j]) || y[j]<0) return -1;sum+=y[j]; }
    if(fabs(sum-1)>1e-12) return -1;
    rb_gas g;rb_gas_init(&g,y);
    const double rho=p/(g.R*T);
    for(int j=0;j<RB_NSPECIES;j++) q->v[j]=rho*y[j];
    q->v[MOM]=rho*u;q->v[ENE]=rho*(rb_gas_u(&g,T)+.5*u*u);
    rb_mix_primitive check;return rb_mix_decode(*q,&check);
}
static rb_mix_cell physical(rb_mix_cell q,rb_mix_primitive w)
{
    rb_mix_cell f=scale(q,w.u);f.v[MOM]+=w.p;f.v[ENE]+=w.p*w.u;return f;
}
static int face(rb_mix_cell l,rb_mix_cell r,rb_mix_cell *out,unsigned long long *fallback)
{
    rb_mix_primitive a,b;
    if(rb_mix_decode(l,&a) || rb_mix_decode(r,&b)) return -1;
    const double sl=fmin(a.u-a.c,b.u-b.c),sr=fmax(a.u+a.c,b.u+b.c);
    rb_mix_cell fl=physical(l,a),fr=physical(r,b);
    if(sl>=0) {*out=fl;return 0;} if(sr<=0) {*out=fr;return 0;}
    const double sm=(b.p-a.p+a.rho*a.u*(sl-a.u)-b.rho*b.u*(sr-b.u))/
                    (a.rho*(sl-a.u)-b.rho*(sr-b.u));
    const double ps=a.p+a.rho*(sl-a.u)*(sm-a.u);
    if(isfinite(sm) && sm>sl && sm<sr && isfinite(ps) && ps>0) {
        rb_mix_cell q=sm>=0 ? l:r,f=sm>=0 ? fl:fr;
        rb_mix_primitive w=sm>=0 ? a:b;
        const double wave=sm>=0 ? sl:sr,ratio=(wave-w.u)/(wave-sm);
        rb_mix_cell star=scale(q,ratio);
        star.v[MOM]=w.rho*ratio*sm;
        star.v[ENE]=ratio*(q.v[ENE]+w.rho*(sm-w.u)*(sm+w.p/(w.rho*(wave-w.u))));
        rb_mix_primitive check;
        if(!rb_mix_decode(star,&check)) {*out=add(f,wave,add(star,-1,q));return 0;}
    }
    if(fallback) ++*fallback;
    *out=scale(add(add(scale(fl,sr),-sl,fr),sl*sr,add(r,-1,l)),1/(sr-sl));
    return 0;
}
int rb_mix_face(rb_mix_cell l,rb_mix_cell r,rb_mix_cell *flux)
{ return flux ? face(l,r,flux,NULL):-1; }
static int neighbor(const rb_mix_state *s,int i)
{
    if(s->boundary==0) return (i+s->cells)%s->cells;
    return i<0 ? 0:(i>=s->cells ? s->cells-1:i);
}
/* Primitive work vector: rho, velocity, pressure, first two mass fractions.
 * Last fraction is the closure, not silently normalized independent species. */
static rb_mix_cell primitive(rb_mix_primitive w)
{ return (rb_mix_cell){{w.rho,w.u,w.p,w.y[0],w.y[1]}}; }
static int conserved(rb_mix_cell w,rb_mix_cell *q)
{
    const double y[]={w.v[3],w.v[4],1-w.v[3]-w.v[4]};
    rb_gas g;rb_gas_init(&g,y);
    if(!isfinite(w.v[0]) || w.v[0]<=0) return -1;
    return rb_mix_make(w.v[2],w.v[2]/(w.v[0]*g.R),w.v[1],y,q);
}
static double mc(double a,double b)
{ return a*b<=0 ? 0:copysign(fmin(.5*fabs(a+b),fmin(2*fabs(a),2*fabs(b))),a); }
static int rhs(rb_mix_state *s,const rb_mix_cell *q,double dt,rb_mix_cell *dest,rb_mix_cell *book)
{
    for(int i=0;i<s->cells;i++) {
        rb_mix_primitive w,l,r;
        if(rb_mix_decode(q[i],&w) || rb_mix_decode(q[neighbor(s,i-1)],&l) ||
           rb_mix_decode(q[neighbor(s,i+1)],&r)) return -1;
        const double cfl=(fabs(w.u)+w.c)*dt/s->dx;
        s->max_cfl=fmax(s->max_cfl,cfl); if(cfl>.6) return -1;
        rb_mix_cell a=primitive(l),b=primitive(w),c=primitive(r),wl=b,wr=b;
        int smooth=fabs(l.p-r.p)<.02*w.p && fabs(l.rho-r.rho)<.02*w.rho;
        for(int j=0;j<RB_NSPECIES;j++) if(fabs(l.y[j]-r.y[j])>.02) smooth=0;
        if(!smooth) s->limited_cells++;
        for(int j=0;j<NV;j++) {
            double dl=b.v[j]-a.v[j],dr=c.v[j]-b.v[j];
            wl.v[j]-=smooth ? (2*dl+dr)/6:.5*mc(dl,dr);
            wr.v[j]+=smooth ? (dl+2*dr)/6:.5*mc(dl,dr);
        }
        if(conserved(wl,&s->left[i]) || conserved(wr,&s->right[i])) {
            s->left[i]=s->right[i]=q[i];s->reconstruction_fallbacks++;
        }
    }
    for(int f=0;f<=s->cells;f++) {
        rb_mix_cell l=s->right[neighbor(s,f-1)],r=s->left[neighbor(s,f)];
        if(s->boundary && (f==0 || f==s->cells)) {
            l=r=q[f==0 ? 0:s->cells-1];
            if(s->boundary==2) { if(f==0) l.v[MOM]*=-1;else r.v[MOM]*=-1; }
        }
        if(face(l,r,&s->flux[f],&s->flux_fallbacks)) return -1;
    }
    *book=scale(add(s->flux[s->cells],-1,s->flux[0]),dt);
    for(int i=0;i<s->cells;i++) {
        dest[i]=add(q[i],-dt/s->dx,add(s->flux[i+1],-1,s->flux[i]));
        rb_mix_primitive check;if(rb_mix_decode(dest[i],&check)) return -1;
    }
    return 0;
}
int rb_mix_step(rb_mix_state *s,double dt)
{
    if(!s || !s->q || s->failed || !isfinite(dt) || dt<=0) return -1;
    rb_mix_cell b1,b2,b3;
    if(rhs(s,s->q,dt,s->stage,&b1) || rhs(s,s->stage,dt,s->next,&b2)) {s->failed=1;return -1;}
    for(int i=0;i<s->cells;i++) s->stage[i]=add(scale(s->q[i],.75),.25,s->next[i]);
    if(rhs(s,s->stage,dt,s->next,&b3)) {s->failed=1;return -1;}
    for(int i=0;i<s->cells;i++) {
        s->stage[i]=add(scale(s->q[i],1.0/3),2.0/3,s->next[i]);
        rb_mix_primitive check;
        if(rb_mix_decode(s->stage[i],&check)) {s->failed=1;return -1;}
    }
    memcpy(s->q,s->stage,(size_t)s->cells*sizeof *s->q);
    s->boundary_book=add(s->boundary_book,1.0/6,add(add(b1,1,b2),4,b3));
    s->steps++;return 0;
}
