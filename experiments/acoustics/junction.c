#include "junction.h"
#include <float.h>
#include <math.h>
#include <string.h>
enum {P=0,UX=1,T0=4,A0=6};
static double dot(const double a[3],const double b[3]){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static int positive(double v){return isfinite(v)&&v>0;}
static int failure(rb_jv_state *s){s->failed=1;return -2;}
static rb_jv_q add(rb_jv_q a,double scale,rb_jv_q b)
{
    for(int k=0;k<2;k++){a.r[k]+=scale*b.r[k];a.alpha[k]+=scale*b.alpha[k];}
    for(int j=0;j<3;j++)a.momentum[j]+=scale*b.momentum[j];
    a.energy+=scale*b.energy;return a;
}
static rb_jv_q scaled(rb_jv_q a,double f)
{
    for(int k=0;k<2;k++){a.r[k]*=f;a.alpha[k]*=f;}
    for(int j=0;j<3;j++)a.momentum[j]*=f;
    a.energy*=f;return a;
}
rb_jv_q rb_jv_make(const rb_jv_state *s,double p,const double v[3],const double a[2],const double T[2])
{
    rb_jv_q q={{0,0},{0,0,0},0,{a[0],a[1]}};
    for(int k=0;k<2;k++){q.r[k]=a[k]*p/(s->thermo.gas[k].R*T[k]);q.energy+=q.r[k]*rb_gas_u(&s->thermo.gas[k],T[k]);}
    double rho=q.r[0]+q.r[1];for(int j=0;j<3;j++)q.momentum[j]=rho*v[j];
    q.energy+=.5*rho*dot(v,v);return q;
}
int rb_jv_decode(const rb_jv_state *s,rb_jv_q q,double w[8],double *rho,double *c)
{
    double mass=q.r[0]+q.r[1];if(!s||!positive(mass)||!w||!rho||!c)return -1;
    double v[3];for(int j=0;j<3;j++){v[j]=q.momentum[j]/mass;if(!isfinite(v[j]))return -1;}
    rb_ft_q normal={{q.r[0],q.r[1]},0,q.energy-.5*mass*dot(v,v),{q.alpha[0],q.alpha[1]}};
    /* Subnormal mass and volume fractions can round to zero at different
     * SSP-RK operations. Query the exact absent-material thermodynamic limit
     * only when BOTH inventories are below 32*DBL_MIN. Keep every stored mass,
     * alpha and energy bit in the state, shared fluxes and physical books.
     * No temperature can be recovered from a 0/subnormal ratio; the limiting
     * reference temperature has vanishing weight in pressure and heat capacity.
     * Finite invalid constituents still fail the unmodified114 EOS checks. */
    for(int k=0;k<2;k++)if(q.r[k]>=0&&q.r[k]<32*DBL_MIN&&q.alpha[k]>=0&&q.alpha[k]<32*DBL_MIN) {
        normal.r[k]=0;normal.alpha[k]=0;
    }
    rb_ft_primitive a;if(rb_ft_decode(&s->thermo,normal,&a)||sqrt(dot(v,v))/a.c>.25)return -1;
    w[P]=a.p;for(int j=0;j<3;j++)w[UX+j]=v[j];
    for(int k=0;k<2;k++){w[T0+k]=a.T[k];w[A0+k]=q.alpha[k];}
    *rho=mass;*c=a.c;return 0;
}
static int primitive(const rb_jv_state *s,rb_jv_q q,const double w[8],const double n[3],rb_ft_q *normal,rb_ft_primitive *a)
{
    double v[3]={w[UX],w[UX+1],w[UX+2]},u=dot(v,n),rho=q.r[0]+q.r[1],transverse=dot(v,v)-u*u,D=0;
    *normal=(rb_ft_q){{q.r[0],q.r[1]},rho*u,q.energy-.5*rho*transverse,{q.alpha[0],q.alpha[1]}};
    a->p=w[P];a->u=u;a->rho=rho;
    for(int k=0;k<2;k++) {
        a->T[k]=w[T0+k];a->alpha[k]=w[A0+k];
        D+=a->alpha[k]*rb_gas_cv(&s->thermo.gas[k],a->T[k])/s->thermo.gas[k].R;
    }
    a->c=sqrt(a->p*(1+1/D)/rho);return 0;
}
static int flux(const rb_jv_state *s,rb_jv_q l,const double wl[8],rb_jv_q r,const double wr[8],
                  const double n[3],rb_jv_q *F,double *u,double *p)
{
    rb_ft_q a,b;rb_ft_primitive wa,wb;primitive(s,l,wl,n,&a,&wa);primitive(s,r,wr,n,&b,&wb);
    rb_ft_flux f;if(rb_ft_riemann_known(&s->thermo,a,b,&wa,&wb,&f))return -1;
    const double *w=f.u>=0?wl:wr;double v[3]={w[UX],w[UX+1],w[UX+2]},vn=dot(v,n),vt[3];
    for(int j=0;j<3;j++)vt[j]=v[j]-vn*n[j];
    memset(F,0,sizeof *F);double mass=f.f.r[0]+f.f.r[1];
    for(int k=0;k<2;k++){F->r[k]=f.f.r[k];F->alpha[k]=f.f.alpha[k];}
    for(int j=0;j<3;j++)F->momentum[j]=f.f.momentum*n[j]+mass*vt[j];
    F->energy=f.f.energy+.5*mass*dot(vt,vt);*u=f.u;*p=f.p;return 0;
}
int rb_jv_flux(const rb_jv_state *s,rb_jv_q a,rb_jv_q b,const double n[3],rb_jv_q *F,double *u,double *p)
{
    double wa[8],wb[8],rho,c;
    if(!s||!n||fabs(dot(n,n)-1)>1e-12||!F||!u||!p||rb_jv_decode(s,a,wa,&rho,&c)||rb_jv_decode(s,b,wb,&rho,&c))return -1;
    return flux(s,a,wa,b,wb,n,F,u,p);
}
static int inverse(double m[3][3],double out[3][3])
{
    double a[3][6];
    for(int i=0;i<3;i++)for(int j=0;j<6;j++)a[i][j]=j<3?m[i][j]:(j-3==i?1:0);
    for(int col=0;col<3;col++) {
        int pivot=col;for(int r=col+1;r<3;r++)if(fabs(a[r][col])>fabs(a[pivot][col]))pivot=r;
        if(!isfinite(a[pivot][col])||fabs(a[pivot][col])<1e-12)return -1;
        for(int j=0;j<6;j++){double v=a[pivot][j];a[pivot][j]=a[col][j];a[col][j]=v;}
        double d=a[col][col];for(int j=0;j<6;j++)a[col][j]/=d;
        for(int r=0;r<3;r++)if(r!=col){double f=a[r][col];for(int j=0;j<6;j++)a[r][j]-=f*a[col][j];}
    }
    for(int i=0;i<3;i++)for(int j=0;j<3;j++)out[i][j]=a[i][j+3];
    return 0;
}
/* Neighbor displacement for least-squares gradients. Solid faces provide a
 * reflected ghost; open ports use one-sided interior information. */
static int displacement(const rb_jv_state *s,int cell,int face,double d[3],int *other)
{
    const rb_jv_face_geometry *f=&s->connection[face];int sign=f->a==cell?1:-1;
    *other=sign>0?f->b:f->a;
    if(*other>=0)for(int j=0;j<3;j++)d[j]=s->geometry[*other].center[j]-s->geometry[cell].center[j];
    else {
        if(f->port>=0)return 0;
        double h=0;for(int j=0;j<3;j++)h+=(f->center[j]-s->geometry[cell].center[j])*f->normal[j]*sign;
        for(int j=0;j<3;j++)d[j]=2*h*f->normal[j]*sign;
    }
    return 1;
}
int rb_jv_prepare(rb_jv_state *s,int nc,int nf,int np,const rb_jv_cell_geometry *g,const rb_jv_face_geometry *f,
                   rb_jv_cell *cells,size_t count,rb_jv_face *faces,size_t face_count,
                   double p0,const double y0[3],const double y1[3],double T0v,double T1v)
{
    if(!s||!g||!f||!cells||!faces||nc<1||nc>8192||nf<1||nf>65536||np<0||np>RB_JV_PORTS||count<(size_t)nc||face_count<(size_t)nf)return -1;
    memset(s,0,sizeof *s);if(rb_ft_thermodynamics(&s->thermo,p0,y0,y1,T0v,T1v))return -1;
    memset(cells,0,(size_t)nc*sizeof *cells);memset(faces,0,(size_t)nf*sizeof *faces);
    s->cells=nc;s->faces=nf;s->ports=np;s->geometry=g;s->connection=f;s->cell=cells;s->face=faces;
    for(int i=0;i<nc;i++) {
        if(!positive(g[i].volume)||g[i].region<0||g[i].region>=RB_JV_REGIONS)return failure(s);
        for(int j=0;j<3;j++)if(!isfinite(g[i].center[j]))return failure(s);
    }
    for(int k=0;k<nf;k++) {
        if(f[k].a<0||f[k].a>=nc||f[k].b>=nc||f[k].b==f[k].a||f[k].b<-1||f[k].port>=np||f[k].port<-1||
           (f[k].b>=0&&f[k].port!=-1)||!positive(f[k].area)||fabs(dot(f[k].normal,f[k].normal)-1)>1e-12)return failure(s);
        for(int j=0;j<3;j++)if(!isfinite(f[k].normal[j])||!isfinite(f[k].center[j]))return failure(s);
        for(int side=0;side<2;side++) {
            int i=side?f[k].b:f[k].a;if(i<0)continue;
            rb_jv_cell *c=&cells[i];if(c->degree>=RB_JV_DEGREE)return failure(s);
            c->face[c->degree]=k;c->sign[c->degree++]=side?-1:1;
        }
        if(f[k].port>=0) {
            int p=f[k].port;
            if(s->port_area[p]&&fabs(dot(s->port_normal[p],f[k].normal)-1)>1e-12)return failure(s);
            double plane=dot(f[k].normal,f[k].center);
            if(s->port_area[p]&&fabs(plane-s->port_plane[p])>1e-10)return failure(s);
            s->port_plane[p]=plane;
            for(int j=0;j<3;j++)s->port_normal[p][j]=f[k].normal[j];
            s->port_area[p]+=f[k].area;
        }
    }
    for(int p=0;p<np;p++)if(!positive(s->port_area[p]))return failure(s);
    for(int i=0;i<nc;i++) {
        double closure[3]={0,0,0},area=0,V=0,M[3][3]={{0}};
        rb_jv_cell *c=&cells[i];
        for(int k=0;k<c->degree;k++) {
            int index=c->face[k],sign=c->sign[k];const rb_jv_face_geometry *q=&f[index];
            double h=0;for(int j=0;j<3;j++){closure[j]+=sign*q->area*q->normal[j];h+=sign*q->normal[j]*(q->center[j]-g[i].center[j]);}
            if(!positive(h))return failure(s);
            V+=q->area*h/3;area+=q->area;
            double d[3];int other;
            if(displacement(s,i,index,d,&other)) {
                double norm=dot(d,d);if(!positive(norm))return failure(s);
                for(int a=0;a<3;a++)for(int b=0;b<3;b++)M[a][b]+=d[a]*d[b]/norm;
            }
        }
        if(c->degree<4||sqrt(dot(closure,closure))>1e-12*area||fabs(V/g[i].volume-1)>1e-10||inverse(M,c->inverse_metric))return failure(s);
    }
    int queue[8192];for(int i=0;i<nc;i++)cells[i].component=-1;
    for(int first=0;first<nc;first++)if(cells[first].component<0) {
        int head=0,tail=0,component=s->components++;queue[tail++]=first;cells[first].component=component;
        while(head<tail) {
            rb_jv_cell *c=&cells[queue[head++]];
            for(int k=0;k<c->degree;k++) {
                const rb_jv_face_geometry *q=&f[c->face[k]];
                int other=c->sign[k]>0?q->b:q->a;
                if(other>=0&&cells[other].component<0){cells[other].component=component;queue[tail++]=other;}
            }
        }
    }
    for(int p=0;p<np;p++)s->port_component[p]=-1;
    for(int k=0;k<nf;k++)if(f[k].port>=0) {
        int p=f[k].port,c=cells[f[k].a].component;
        if(s->port_component[p]>=0&&s->port_component[p]!=c)return failure(s);
        s->port_component[p]=c;
    }
    return 0;
}
rb_jv_q rb_jv_inventory(const rb_jv_state *s,int region)
{
    rb_jv_q q={0};if(!s||!s->loaded||s->failed||region<-1||region>=RB_JV_REGIONS){q.energy=NAN;return q;}
    for(int i=0;i<s->cells;i++)if(region<0||s->geometry[i].region==region)q=add(q,s->geometry[i].volume,s->cell[i].q);
    return q;
}
int rb_jv_load(rb_jv_state *s)
{
    if(!s||!s->cell||s->failed||s->loaded)return -1;
    for(int i=0;i<s->cells;i++){double rho,c;if(rb_jv_decode(s,s->cell[i].q,s->cell[i].w,&rho,&c))return failure(s);}
    s->loaded=1;s->initial=rb_jv_inventory(s,-1);
    for(int r=0;r<RB_JV_REGIONS;r++)s->regional_initial[r]=rb_jv_inventory(s,r);
    return 0;
}
static void reflected(const double w[8],const double n[3],double out[8])
{
    memcpy(out,w,8*sizeof *w);double v=0;for(int j=0;j<3;j++)v+=w[UX+j]*n[j];
    for(int j=0;j<3;j++)out[UX+j]-=2*v*n[j];
}
static int clock_state(rb_jv_state *s,int i,const double w[8],double sound,double dt)
{
    double speed=sqrt(w[UX]*w[UX]+w[UX+1]*w[UX+1]+w[UX+2]*w[UX+2]),cfl=0;
    s->max_mach=fmax(s->max_mach,speed/sound);s->max_pressure_fraction=fmax(s->max_pressure_fraction,fabs(w[P]/s->thermo.p0-1));
    rb_jv_cell *c=&s->cell[i];
    for(int k=0;k<c->degree;k++) {
        const rb_jv_face_geometry *f=&s->connection[c->face[k]];double u=0;
        for(int j=0;j<3;j++)u+=w[UX+j]*f->normal[j];
        cfl+=.5*dt*f->area*(fabs(u)+sound)/s->geometry[i].volume;
    }
    s->max_cfl=fmax(s->max_cfl,cfl);return cfl>.45?failure(s):0;
}
static int gradients(rb_jv_state *s,int stage,double dt)
{
    for(int i=0;i<s->cells;i++) {
        rb_jv_cell *c=&s->cell[i];double rho,sound;
        rb_jv_q q=stage?c->stage:c->q;if(rb_jv_decode(s,q,c->w,&rho,&sound))return failure(s);
        if(clock_state(s,i,c->w,sound,dt))return -2;
    }
    for(int i=0;i<s->cells;i++) {
        rb_jv_cell *c=&s->cell[i];double rhs[8][3]={{0}},lo[8],hi[8];
        memcpy(lo,c->w,sizeof lo);memcpy(hi,c->w,sizeof hi);
        for(int k=0;k<c->degree;k++) {
            int fi=c->face[k],other;double d[3],neighbor[8];
            if(!displacement(s,i,fi,d,&other))continue;
            if(other>=0)memcpy(neighbor,s->cell[other].w,sizeof neighbor);
            else reflected(c->w,s->connection[fi].normal,neighbor);
            double norm=dot(d,d);
            for(int v=0;v<8;v++) {
                lo[v]=fmin(lo[v],neighbor[v]);hi[v]=fmax(hi[v],neighbor[v]);
                for(int j=0;j<3;j++)rhs[v][j]+=d[j]*(neighbor[v]-c->w[v])/norm;
            }
        }
        double limiter[8];for(int v=0;v<8;v++) {
            limiter[v]=1;
            for(int a=0;a<3;a++){c->gradient[v][a]=0;for(int b=0;b<3;b++)c->gradient[v][a]+=c->inverse_metric[a][b]*rhs[v][b];}
            for(int k=0;k<c->degree;k++) {
                const rb_jv_face_geometry *f=&s->connection[c->face[k]];double delta=0;
                for(int j=0;j<3;j++)delta+=c->gradient[v][j]*(f->center[j]-s->geometry[i].center[j]);
                double lower=lo[v],upper=hi[v];
                if(f->port>=0) {
                    double span=hi[v]-lo[v];lower=c->w[v]-2*span;upper=c->w[v]+2*span;
                    /* A one-sided extrapolation still has physical bounds.
                     * Allowing a negative fraction here, then falling back at
                     * just that face, can drain a trace constituent through
                     * the opposite reconstructed face. Limit the gradient,
                     * never clip a cell inventory or repair its energy. */
                    if(v>=A0){lower=fmax(lower,0);upper=fmin(upper,1+1e-12);}
                    if(v==T0||v==T0+1){lower=fmax(lower,200);upper=fmin(upper,990);}
                    if(v==P){lower=fmax(lower,.97*s->thermo.p0);upper=fmin(upper,1.03*s->thermo.p0);}
                }
                if(delta>0)limiter[v]=fmin(limiter[v],(upper-c->w[v])/delta);
                if(delta<0)limiter[v]=fmin(limiter[v],(lower-c->w[v])/delta);
            }
        }
        /* Include every accepted cell average in its admissible face bounds.
         * In particular, an independent alpha can be1+roundoff: capping its
         * face at exactly1 gives a negative coefficient and reverses the
         * OTHER constituent's shared slope. Coefficients remain in[0,1].
         * A roundoff-sized inward margin protects face positivity through
         * the short dot products; stored mass, alpha and energy are unchanged. */
        limiter[A0]=limiter[A0+1]=fmax(0,fmin(limiter[A0],limiter[A0+1]))*(1-16*DBL_EPSILON);
        for(int v=0;v<8;v++)for(int j=0;j<3;j++)c->gradient[v][j]*=fmax(0,limiter[v]);
    }
    return 0;
}
static rb_jv_q reconstruct(rb_jv_state *s,int i,int fi,int stage,double w[8])
{
    rb_jv_cell *c=&s->cell[i];memcpy(w,c->w,8*sizeof *w);
    for(int v=0;v<8;v++)for(int j=0;j<3;j++)w[v]+=c->gradient[v][j]*(s->connection[fi].center[j]-s->geometry[i].center[j]);
    rb_jv_q current=stage?c->stage:c->q;int trace[2]={0,0};
    for(int k=0;k<2;k++)if((current.r[k]!=0||current.alpha[k]!=0)&&current.r[k]>=0&&current.r[k]<32*DBL_MIN&&current.alpha[k]>=0&&current.alpha[k]<32*DBL_MIN) {
        /* The EOS limit cannot reconstruct mass from a rounded alpha/r ratio.
         * Carry the actual donor's trace inventory at first order. Retain the
         * second-order pressure/velocity/major constituent and one physical E.
         * This changes a face prediction, never a stored cell or transfer book. */
        trace[k]=1;w[A0+k]=current.alpha[k];w[T0+k]=s->thermo.T_reference[k];s->trace_face_limits++;
    }
    double v[3]={w[UX],w[UX+1],w[UX+2]},alpha[2]={w[A0],w[A0+1]},T[2]={w[T0],w[T0+1]};
    int valid=positive(w[P])&&fabs(w[P]/s->thermo.p0-1)<=.03&&fabs(alpha[0]+alpha[1]-1)<=1e-12;
    double D=0;
    for(int k=0;k<2;k++) {
        if(!isfinite(T[k])||T[k]<200||T[k]>990||!isfinite(alpha[k])||alpha[k]<0||alpha[k]>1+1e-12)valid=0;
        D+=alpha[k]*rb_gas_cv(&s->thermo.gas[k],T[k])/s->thermo.gas[k].R;
    }
    rb_jv_q q=rb_jv_make(s,w[P],v,alpha,T);double rho=q.r[0]+q.r[1],sound=sqrt(w[P]*(1+1/D)/rho);
    for(int k=0;k<2;k++)if(trace[k])q.r[k]=current.r[k];
    if(!valid||!positive(rho)||!positive(sound)||!isfinite(q.energy)||sqrt(dot(v,v))/sound>.25) {
        s->reconstruction_fallbacks++;q=stage?c->stage:c->q;memcpy(w,c->w,8*sizeof *w);
    }
    return q;
}
static int evaluate(rb_jv_state *s,int stage,double dt,const rb_jv_q *boundary)
{
    if(gradients(s,stage,dt))return -2;
    for(int i=0;i<s->cells;i++)memset(&s->cell[i].rhs,0,sizeof s->cell[i].rhs);
    for(int fi=0;fi<s->faces;fi++) {
        const rb_jv_face_geometry *g=&s->connection[fi];rb_jv_face *f=&s->face[fi];double a[8],b[8],rho,sound;
        rb_jv_q l=reconstruct(s,g->a,fi,stage,a),r;
        if(g->b>=0)r=reconstruct(s,g->b,fi,stage,b);
        else if(g->port>=0) {
            r=boundary[g->port];if(rb_jv_decode(s,r,b,&rho,&sound))return failure(s);
        } else {
            /* Algebraically the same one-sided characteristic Riemann wall:
             * reflected ghost has zero contact speed, p*=p+rho*c*u_normal. */
            rb_ft_q normal;rb_ft_primitive w;primitive(s,l,a,g->normal,&normal,&w);
            f->p=w.p+w.rho*w.c*w.u;if(!positive(f->p))return failure(s);
            memset(&f->flux,0,sizeof f->flux);f->u=0;
            for(int j=0;j<3;j++)f->flux.momentum[j]=f->p*g->normal[j];
        }
        if((g->b>=0||g->port>=0)&&flux(s,l,a,r,b,g->normal,&f->flux,&f->u,&f->p))return failure(s);
        for(int side=0;side<2;side++) {
            int i=side?g->b:g->a;if(i<0)continue;double factor=(side?1:-1)*g->area/s->geometry[i].volume;
            rb_jv_cell *c=&s->cell[i];rb_jv_q q=stage?c->stage:c->q;
            double old_alpha[2]={c->rhs.alpha[0],c->rhs.alpha[1]};
            c->rhs=add(c->rhs,factor,f->flux);
            for(int k=0;k<2;k++)c->rhs.alpha[k]=old_alpha[k]+factor*(f->flux.alpha[k]-f->u*q.alpha[k]);
        }
    }
    return 0;
}
int rb_jv_step(rb_jv_state *s,double dt,const rb_jv_q *start,const rb_jv_q *end,size_t count)
{
    if(!s||!s->cell||!s->loaded||(s->ports&&(!start||!end))||count<(size_t)s->ports||!positive(dt))return -1;
    if(s->failed)return -2;
    for(int stage=0;stage<2;stage++) {
        if(evaluate(s,stage,dt,stage?end:start))return -2;
        for(int i=0;i<s->cells;i++) {
            rb_jv_cell *c=&s->cell[i];rb_jv_q q=add(stage?c->stage:c->q,dt,c->rhs);
            if(stage)q=scaled(add(c->q,1,q),.5);
            double w[8],rho,sound;if(rb_jv_decode(s,q,w,&rho,&sound)||clock_state(s,i,w,sound,dt))return failure(s);
            if(stage)c->next=q;else c->stage=q;
        }
        if(!stage)for(int fi=0;fi<s->faces;fi++){s->face[fi].saved=s->face[fi].flux;s->face[fi].saved_p=s->face[fi].p;s->face[fi].saved_u=s->face[fi].u;}
    }
    for(int i=0;i<s->cells;i++)s->cell[i].q=s->cell[i].next;
    memset(s->port_pressure,0,sizeof s->port_pressure);memset(s->port_velocity,0,sizeof s->port_velocity);
    for(int fi=0;fi<s->faces;fi++) {
        const rb_jv_face_geometry *g=&s->connection[fi];rb_jv_face *f=&s->face[fi];
        rb_jv_q accepted=scaled(add(f->saved,1,f->flux),.5*dt*g->area);
        int a=s->geometry[g->a].region,b=g->b>=0?s->geometry[g->b].region:-1;
        if(a!=b){s->regional_transfer[a]=add(s->regional_transfer[a],-1,accepted);if(b>=0)s->regional_transfer[b]=add(s->regional_transfer[b],1,accepted);}
        if(g->b<0) {
            s->external=add(s->external,-1,accepted);
            if(g->port>=0) {
                int p=g->port;s->port_transfer[p]=add(s->port_transfer[p],-1,accepted);
                s->port_pressure[p]+=.5*g->area*(f->saved_p+f->p)/s->port_area[p];
                s->port_velocity[p]+=.5*g->area*(f->saved_u+f->u)/s->port_area[p];
            } else for(int j=0;j<3;j++)s->wall_impulse[j]-=accepted.momentum[j];
        }
    }
    s->steps++;return 0;
}
double rb_jv_perturbation(const rb_jv_state *s,const double U[3])
{
    if(!s||!s->loaded||s->failed)return NAN;
    double E=0;
    for(int i=0;i<s->cells;i++) {
        double w[8],rho,c;if(rb_jv_decode(s,s->cell[i].q,w,&rho,&c))return NAN;
        double dv[3];for(int j=0;j<3;j++)dv[j]=w[UX+j]-U[j];
        E+=s->geometry[i].volume*(.5*(w[P]-s->thermo.p0)*(w[P]-s->thermo.p0)/(rho*c*c)+.5*rho*dot(dv,dv));
    }
    return E;
}
