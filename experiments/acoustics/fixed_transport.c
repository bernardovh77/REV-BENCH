#include "fixed_transport.h"
#include "thermal.h"
#include <math.h>
#include <string.h>
#define PI 3.14159265358979323846
static int positive(double x){return isfinite(x)&&x>0;}
static int fail(rb_ft_state *s){s->failed=1;return -2;}
static rb_ft_q add(rb_ft_q a,double b,rb_ft_q c)
{
    return (rb_ft_q){{a.r[0]+b*c.r[0],a.r[1]+b*c.r[1]},a.momentum+b*c.momentum,
        a.energy+b*c.energy,{a.alpha[0]+b*c.alpha[0],a.alpha[1]+b*c.alpha[1]}};
}
static rb_ft_q scale(rb_ft_q a,double b)
{
    return (rb_ft_q){{b*a.r[0],b*a.r[1]},b*a.momentum,b*a.energy,{b*a.alpha[0],b*a.alpha[1]}};
}
int rb_ft_prepare(rb_ft_state *s,int n,double length,double area,double p0,
                  const double y0[3],const double y1[3],double T0,double T1,
                  rb_ft_slot *storage,size_t count)
{
    if(!s||!storage||!y0||!y1||n<4||n>8192||count<(size_t)n+1||
       !positive(length)||!positive(area)||!positive(p0)||!isfinite(T0)||!isfinite(T1)||
       T0<200||T0>990||T1<200||T1>990)return -1;
    for(int k=0;k<2;k++) {
        const double *y=k?y1:y0;double total=0;
        for(int j=0;j<3;j++){if(!isfinite(y[j])||y[j]<0)return -1;total+=y[j];}
        if(fabs(total-1)>1e-12)return -1;
    }
    memset(s,0,sizeof *s);memset(storage,0,((size_t)n+1)*sizeof *storage);
    s->cells=n;s->length=length;s->area=area;s->dx=length/n;s->p0=p0;s->slot=storage;
    rb_gas_init(&s->gas[0],y0);rb_gas_init(&s->gas[1],y1);
    memcpy(s->y[0],y0,sizeof s->y[0]);memcpy(s->y[1],y1,sizeof s->y[1]);
    s->T_reference[0]=T0;s->T_reference[1]=T1;return 0;
}
static rb_ft_q make(const rb_ft_state *s,double p,double u,const double alpha[2],double T0,double T1)
{
    rb_ft_q q={{0,0},0,0,{alpha[0],alpha[1]}};const double T[2]={T0,T1};
    for(int k=0;k<2;k++) {
        q.r[k]=alpha[k]*p/(s->gas[k].R*T[k]);
        q.energy+=q.r[k]*rb_gas_u(&s->gas[k],T[k]);
    }
    double rho=q.r[0]+q.r[1];q.momentum=rho*u;q.energy+=.5*rho*u*u;return q;
}
rb_ft_q rb_ft_make(const rb_ft_state *s,double p,double u,double a,double T0,double T1)
{
    const double alpha[2]={a,1-a};return make(s,p,u,alpha,T0,T1);
}
int rb_ft_decode(const rb_ft_state *s,rb_ft_q q,rb_ft_primitive *w)
{
    if(!s||!w||!isfinite(q.r[0])||!isfinite(q.r[1])||q.r[0]<0||q.r[1]<0||
       !isfinite(q.momentum)||!isfinite(q.energy))return -1;
    for(int k=0;k<2;k++)if(!isfinite(q.alpha[k])||q.alpha[k]<0||q.alpha[k]>1+1e-12)return -1;
    if(fabs(q.alpha[0]+q.alpha[1]-1)>1e-12)return -1;
    const double rho=q.r[0]+q.r[1],*alpha=q.alpha;
    if(!positive(rho))return -1;
    double u=q.momentum/rho,I=q.energy-.5*q.momentum*u,p=s->p0,D=0;
    if(!isfinite(I))return -1;
    for(int k=0;k<2;k++)if((alpha[k]==0)!=(q.r[k]==0))return -1;
    if(q.r[0]==0||q.r[1]==0) {
        int k=q.r[0]==0?1:0;
        double T=rb_gas_T_from_u(&s->gas[k],I/rho,s->T_reference[k]);
        p=rho*s->gas[k].R*T;
    } else {
        int converged=0;
        for(int it=0;it<16;it++) {
            double value=0;D=0;
            for(int k=0;k<2;k++) {
                double T=p*alpha[k]/(q.r[k]*s->gas[k].R);
                if(!isfinite(T)||T<150||T>1100)return -1;
                value+=q.r[k]*rb_gas_u(&s->gas[k],T);
                D+=alpha[k]*(rb_gas_cp(&s->gas[k],T)-s->gas[k].R)/s->gas[k].R;
            }
            double delta=(value-I)/D;p-=delta;
            if(fabs(delta)<2e-8){converged=1;break;}
        }
        if(!converged)return -1;
    }
    if(!positive(p)||fabs(p/s->p0-1)>.03)return -1;
    D=0;
    for(int k=0;k<2;k++) {
        w->T[k]=q.r[k]>0?p*alpha[k]/(q.r[k]*s->gas[k].R):s->T_reference[k];
        if(!isfinite(w->T[k])||w->T[k]<200||w->T[k]>990)return -1;
        D+=alpha[k]*(rb_gas_cp(&s->gas[k],w->T[k])-s->gas[k].R)/s->gas[k].R;
    }
    w->p=p;w->u=u;w->rho=rho;w->c=sqrt(p*(1+1/D)/rho);
    for(int k=0;k<2;k++)w->alpha[k]=q.alpha[k];
    return positive(w->c)&&fabs(u)/w->c<=.25?0:-1;
}
rb_ft_book rb_ft_inventory(const rb_ft_state *s,int lo,int hi)
{
    rb_ft_book b={{0,0},0,0};
    if(!s||!s->loaded||s->failed||lo<0||hi>s->cells||hi<lo){b.energy=NAN;return b;}
    for(int i=lo;i<hi;i++) {
        rb_ft_q q=s->slot[i].q;double V=s->area*s->dx;
        for(int k=0;k<2;k++)b.m[k]+=V*q.r[k];
        b.momentum+=V*q.momentum;b.energy+=V*q.energy;
    }
    return b;
}
int rb_ft_load(rb_ft_state *s)
{
    if(!s||!s->slot||s->loaded||s->failed)return -1;
    for(int i=0;i<s->cells;i++)if(rb_ft_decode(s,s->slot[i].q,&s->slot[i].w))return fail(s);
    s->loaded=1;s->initial=rb_ft_inventory(s,0,s->cells);return 0;
}
static rb_ft_q physical(rb_ft_q q,rb_ft_primitive w)
{
    return (rb_ft_q){{q.r[0]*w.u,q.r[1]*w.u},q.momentum*w.u+w.p,(q.energy+w.p)*w.u,{q.alpha[0]*w.u,q.alpha[1]*w.u}};
}
int rb_ft_riemann(const rb_ft_state *s,rb_ft_q l,rb_ft_q r,rb_ft_flux *out)
{
    rb_ft_primitive a,b;if(!out||rb_ft_decode(s,l,&a)||rb_ft_decode(s,r,&b))return -1;
    /* One-sided physical characteristic impedances are essential at a
     * thermal/material jump. Taking the largest c on BOTH sides changes the
     * small-signal reflection coefficient. Strong shock states are excluded. */
    double sl=a.u-a.c,sr=b.u+b.c;
    rb_ft_q fl=physical(l,a),fr=physical(r,b);
    if(sl>=0){*out=(rb_ft_flux){fl,a.u,a.p,{a.alpha[0],a.alpha[1]}};return 0;}
    if(sr<=0){*out=(rb_ft_flux){fr,b.u,b.p,{b.alpha[0],b.alpha[1]}};return 0;}
    double sm=(b.p-a.p+a.rho*a.u*(sl-a.u)-b.rho*b.u*(sr-b.u))/(a.rho*(sl-a.u)-b.rho*(sr-b.u));
    double ps=a.p+a.rho*(sl-a.u)*(sm-a.u);
    if(!isfinite(sm)||!positive(ps)||sm<=sl||sm>=sr)return -1;
    rb_ft_q q=sm>=0?l:r;rb_ft_primitive w=sm>=0?a:b;
    double wave=sm>=0?sl:sr,factor=(wave-w.u)/(wave-sm);
    rb_ft_q star={{q.r[0]*factor,q.r[1]*factor},w.rho*factor*sm,
        w.rho*factor*(q.energy/w.rho+(sm-w.u)*(sm+w.p/(w.rho*(wave-w.u)))),{q.alpha[0],q.alpha[1]}};
    /* The algebraically equivalent star advective form avoids subtracting
     * O(rho*c) terms to obtain a roundoff-sized contact mass flow. Otherwise
     * the volume fraction can move while its constituent mass rounds to zero. */
    rb_ft_q flux={{star.r[0]*sm,star.r[1]*sm},star.momentum*sm+ps,
        (star.energy+ps)*sm,{0,0}};
    *out=(rb_ft_flux){flux,sm,ps,{q.alpha[0],q.alpha[1]}};
    for(int k=0;k<2;k++)out->f.alpha[k]=q.alpha[k]*sm;
    return 0;
}
static double mc(double a,double b)
{
    return a*b<=0?0:copysign(fmin(.5*fabs(a+b),fmin(2*fabs(a),2*fabs(b))),a);
}
static rb_ft_primitive slope(rb_ft_primitive l,rb_ft_primitive w,rb_ft_primitive r)
{
    rb_ft_primitive v={0};v.p=mc(w.p-l.p,r.p-w.p);v.u=mc(w.u-l.u,r.u-w.u);
    for(int k=0;k<2;k++) {
        v.alpha[k]=mc(w.alpha[k]-l.alpha[k],r.alpha[k]-w.alpha[k]);
        v.T[k]=mc(w.T[k]-l.T[k],r.T[k]-w.T[k]);
    }
    return v;
}
static rb_ft_q reconstruct(rb_ft_state *s,int i,double sign)
{
    rb_ft_primitive w=s->slot[i].w,d=s->slot[i].slope;
    double alpha[2];for(int k=0;k<2;k++)alpha[k]=w.alpha[k]+sign*d.alpha[k];
    rb_ft_q q=make(s,w.p+sign*d.p,w.u+sign*d.u,alpha,w.T[0]+sign*d.T[0],w.T[1]+sign*d.T[1]);
    rb_ft_primitive check;
    if(rb_ft_decode(s,q,&check)){s->recon_fallbacks++;q=s->slot[i].next;}
    return q;
}
/* next is the current stage's input, never an asynchronous old neighbor. */
static int faces(rb_ft_state *s,int stage,rb_ft_q left,rb_ft_q right)
{
    rb_ft_primitive wl,wr;
    if(rb_ft_decode(s,left,&wl)||rb_ft_decode(s,right,&wr))return fail(s);
    int n=s->cells;
    for(int i=0;i<n;i++) {
        s->slot[i].next=stage?s->slot[i].stage:s->slot[i].q;
        if(rb_ft_decode(s,s->slot[i].next,&s->slot[i].w))return fail(s);
    }
    for(int i=0;i<n;i++)s->slot[i].slope=slope(i?s->slot[i-1].w:wl,s->slot[i].w,i+1<n?s->slot[i+1].w:wr);
    for(int f=0;f<=n;f++) {
        rb_ft_q a=f?reconstruct(s,f-1,.5):left;
        rb_ft_q b=f<n?reconstruct(s,f,-.5):right;
        if(rb_ft_riemann(s,a,b,&s->slot[f].face))return fail(s);
    }
    return 0;
}
static rb_ft_q rhs(const rb_ft_state *s,int i,rb_ft_q q)
{
    const rb_ft_flux l=s->slot[i].face,r=s->slot[i+1].face;
    rb_ft_q d=scale(add(l.f,-1,r.f),1/s->dx);
    /* This difference form preserves an exact pure constituent, including
     * moving/accelerating gas, without clipping alpha or species masses. */
    for(int k=0;k<2;k++)d.alpha[k]=(l.u*(l.alpha[k]-q.alpha[k])-r.u*(r.alpha[k]-q.alpha[k]))/s->dx;
    return d;
}
int rb_ft_wall(rb_ft_state *s,double capacity,double T,double x0,double x1,
                double h_inner,double outer,double ambient_T)
{
    if(!s||!s->loaded||s->failed||s->steps||!positive(capacity)||T<200||T>990||!isfinite(T)||
       !isfinite(x0)||!isfinite(x1)||x0<0||x1>s->length||x1<=x0||!isfinite(h_inner)||h_inner<0||
       !isfinite(outer)||outer<0||!isfinite(ambient_T)||ambient_T<200||ambient_T>990)return -1;
    s->wall_capacity=capacity;s->wall_T=T;s->wall_x0=x0;s->wall_x1=x1;
    s->h_inner=h_inner;s->outer=outer;s->ambient_T=ambient_T;return 0;
}
static int heat(rb_ft_state *s,double dt)
{
    if(!s->wall_capacity)return 0;
    double perimeter=sqrt(4*PI*s->area),V=s->area*s->dx;
    for(int i=0;i<s->cells;i++) {
        rb_ft_primitive w;if(rb_ft_decode(s,s->slot[i].q,&w))return fail(s);
        double overlap=fmax(0,fmin((i+1)*s->dx,s->wall_x1)-fmax(i*s->dx,s->wall_x0));
        if(!overlap||s->h_inner==0)continue;
        for(int k=0;k<2;k++)if(s->slot[i].q.r[k]>0) {
            double a=w.alpha[k];rb_tw_pair pair;
            if(rb_tw_prepare(&pair,V*s->slot[i].q.r[k],s->y[k],w.T[k],s->wall_capacity,s->wall_T,
                s->ambient_T,s->h_inner*perimeter*overlap*a,0)||rb_tw_step(&pair,dt))return fail(s);
            double Q=pair.energy-pair.mass*rb_gas_u(&pair.gas,w.T[k]);
            s->slot[i].q.energy+=Q/V;s->slot[i].wall_heat+=Q;s->gas_heat+=Q;s->wall_T=pair.wall_T;
        }
    }
    double T=s->ambient_T+(s->wall_T-s->ambient_T)*exp(-s->outer*dt/s->wall_capacity);
    s->ambient_heat+=s->wall_capacity*(T-s->wall_T);s->wall_T=T;return 0;
}
static void book(rb_ft_book *b,rb_ft_q f,double factor)
{
    for(int k=0;k<2;k++)b->m[k]+=factor*f.r[k];
    b->momentum+=factor*f.momentum;b->energy+=factor*f.energy;
}
static int clock_limit(rb_ft_state *s,rb_ft_primitive w,double dt)
{
    double cfl=(fabs(w.u)+w.c)*dt/s->dx;
    s->max_cfl=fmax(s->max_cfl,cfl);
    s->max_pressure_fraction=fmax(s->max_pressure_fraction,fabs(w.p/s->p0-1));
    return cfl>.45?fail(s):0;
}
static int advance(rb_ft_state *a,rb_ft_state *b,double dt,rb_ft_q left,rb_ft_q right)
{
    if(!a||!a->slot||!a->loaded||!positive(dt)||(b&&(!b->loaded||!b->slot)))return -1;
    if(a->failed||(b&&b->failed))return -2;
    if(b&&(a==b||fabs(a->dx-b->dx)>1e-14||a->area!=b->area||a->p0!=b->p0||memcmp(a->gas,b->gas,sizeof a->gas)))return -1;
    rb_ft_state *s[2]={a,b};int count=b?2:1;
    for(int d=0;d<count;d++) {
        for(int i=0;i<s[d]->cells;i++) {
            rb_ft_primitive w;if(rb_ft_decode(s[d],s[d]->slot[i].q,&w))return fail(s[d]);
            if(clock_limit(s[d],w,dt))return -2;
        }
        if(heat(s[d],.5*dt))return -2;
    }
    for(int stage=0;stage<2;stage++) {
        rb_ft_q edgea=stage?a->slot[a->cells-1].stage:a->slot[a->cells-1].q;
        rb_ft_q edgeb=b?(stage?b->slot[0].stage:b->slot[0].q):right;
        if(faces(a,stage,left,edgeb)||(b&&faces(b,stage,edgea,right)))return -2;
        for(int d=0;d<count;d++)for(int i=0;i<s[d]->cells;i++)
            if(clock_limit(s[d],s[d]->slot[i].w,dt))return -2;
        if(b) {
            rb_ft_flux shared;
            if(rb_ft_riemann(a,reconstruct(a,a->cells-1,.5),reconstruct(b,0,-.5),&shared))return fail(a);
            a->slot[a->cells].face=b->slot[0].face=shared;
        }
        for(int d=0;d<count;d++) {
            rb_ft_state *p=s[d];
            for(int i=0;i<p->cells;i++) {
                rb_ft_q q=stage?p->slot[i].stage:p->slot[i].q;
                rb_ft_q next=add(q,dt,rhs(p,i,q));
                if(stage)next=scale(add(p->slot[i].q,1,next),.5);
                rb_ft_primitive w;
                if(rb_ft_decode(p,next,&w))return fail(p);
                p->slot[i].w=w;
                if(stage)p->slot[i].next=next;else p->slot[i].stage=next;
            }
            if(!stage)for(int i=0;i<=p->cells;i++)p->slot[i].saved=p->slot[i].face;
        }
    }
    for(int d=0;d<count;d++) {
        rb_ft_state *p=s[d];
        for(int i=0;i<p->cells;i++)p->slot[i].q=p->slot[i].next;
        if(heat(p,.5*dt))return -2;
        /* Validate the FINAL heat state in this call, not one tick later. */
        for(int i=0;i<p->cells;i++) {
            if(p->wall_capacity&&rb_ft_decode(p,p->slot[i].q,&p->slot[i].w))return fail(p);
            if(clock_limit(p,p->slot[i].w,dt))return -2;
        }
    }
    for(int d=0;d<count;d++) {
        rb_ft_state *p=s[d];
        for(int i=0;i<=p->cells;i++)
            book(&p->slot[i].transfer,add(p->slot[i].saved.f,1,p->slot[i].face.f),.5*dt*p->area);
        p->steps++;
    }
    return 0;
}
int rb_ft_step(rb_ft_state *s,double dt,rb_ft_q left,rb_ft_q right){return advance(s,0,dt,left,right);}
int rb_ft_step_pair(rb_ft_state *a,rb_ft_state *b,double dt,rb_ft_q left,rb_ft_q right)
{
    if(!b)return -1;
    int status=advance(a,b,dt,left,right);
    if(status==-2){if(a)a->failed=1;b->failed=1;}
    return status;
}
double rb_ft_perturbation(const rb_ft_state *s,double U)
{
    if(!s||!s->loaded||s->failed)return NAN;
    double E=0;
    for(int i=0;i<s->cells;i++) {
        rb_ft_primitive w;if(rb_ft_decode(s,s->slot[i].q,&w))return NAN;
        E+=s->area*s->dx*(.5*(w.p-s->p0)*(w.p-s->p0)/(w.rho*w.c*w.c)+.5*w.rho*(w.u-U)*(w.u-U));
    }
    return E;
}
