#include "gas_graph.h"
#include <math.h>
#include <string.h>
#define PI 3.14159265358979323846
enum {MOM=RB_NSPECIES,ENE=RB_NSPECIES+1,NV=RB_NSPECIES+2};
static rb_mix_cell add(rb_mix_cell a,rb_mix_cell b,double k)
{for(int j=0;j<NV;j++)a.v[j]+=k*b.v[j];return a;}
static rb_mix_cell scale(rb_mix_cell a,double k)
{for(int j=0;j<NV;j++)a.v[j]*=k;return a;}
static int finite_positive(double x){return isfinite(x)&&x>0;}
static int plan_for(const rb_gg_spec *p,rb_ac_plan *plan,rb_mix_cell *base,rb_mix_primitive *w)
{
    if(!p || p->gas_cells<4 || p->gas_cells>4096 || !finite_positive(p->length) ||
       !finite_positive(p->interface_x) || p->interface_x>=p->length || !finite_positive(p->area) ||
       !isfinite(p->reflection) || fabs(p->reflection)>1 || p->temperature<300 || p->temperature>900 ||
       rb_mix_make(p->pressure,p->temperature,p->mean_flow,p->y,base) || rb_mix_decode(*base,w)) return RB_GG_ARGUMENT;
    if(fabs(p->mean_flow)/w->c>.25)return RB_GG_DOMAIN;
    if(rb_ac_init(plan,p->rate,w->rho,w->c) ||
       rb_ac_add_node(plan,RB_AC_BOUNDARY,0)<0 || rb_ac_add_node(plan,RB_AC_BOUNDARY,p->reflection)<0 ||
       rb_ac_add_edge(plan,0,1,p->length-p->interface_x,p->area,1,RB_AC_ALLPASS)<0) return RB_GG_ARGUMENT;
    const double delay=p->rate*(p->length-p->interface_x)/(w->c+p->mean_flow);
    const double back=p->rate*(p->length-p->interface_x)/(w->c-p->mean_flow);
    if(!isfinite(delay) || !isfinite(back) || delay<4 || back<4 || back>8192 || delay>8192-p->gas_cells ||
       (w->c+fabs(p->mean_flow))/(p->rate*(p->interface_x/p->gas_cells))>1.5 || !rb_ac_samples(plan)) return RB_GG_DOMAIN;
    return 0;
}
int rb_gg_sizes(const rb_gg_spec *p,size_t *cells,size_t *doubles)
{
    if(!cells || !doubles)return RB_GG_ARGUMENT;
    rb_ac_plan plan;rb_mix_cell base;rb_mix_primitive w;
    int status=plan_for(p,&plan,&base,&w);if(status)return status;
    const int forward=(int)floor(p->rate*(p->length-p->interface_x)/(w.c+p->mean_flow));
    const int back=(int)floor(p->rate*(p->length-p->interface_x)/(w.c-p->mean_flow));
    const int n=p->gas_cells+forward;
    *cells=(size_t)8*n+1+(p->mean_flow ? forward+back+2:0);
    *doubles=(size_t)2*(forward+back);return 0;
}
static rb_mix_cell coordinates(const rb_gg_state *s,rb_mix_cell q)
{
    double pi=s->beta*(q.v[ENE]-s->base.v[ENE]);
    double momentum=q.v[MOM];
    if(s->spec.mean_flow) {
        double density=0;for(int j=0;j<RB_NSPECIES;j++)density+=q.v[j]-s->base.v[j];
        const double dm=q.v[MOM]-s->base.v[MOM],U=s->spec.mean_flow;
        pi=s->beta*(q.v[ENE]-s->base.v[ENE]-U*dm+.5*U*U*density);momentum=dm-U*density;
    }
    for(int j=0;j<RB_NSPECIES;j++)pi+=s->coefficient[j]*(q.v[j]-s->base.v[j]);
    rb_mix_cell w={{.5*(pi+s->c*momentum),.5*(pi-s->c*momentum),0,0,0}};
    for(int j=0;j<RB_NSPECIES;j++)w.v[j+2]=q.v[j]-s->base.v[j]-s->spec.y[j]*pi/(s->c*s->c);
    return w;
}
static rb_mix_cell physical(const rb_gg_state *s,rb_mix_cell w)
{
    rb_mix_cell q=s->base;const double pi=w.v[0]+w.v[1];double correction=0;
    for(int j=0;j<RB_NSPECIES;j++) {
        q.v[j]+=w.v[j+2]+s->spec.y[j]*pi/(s->c*s->c);
        correction+=s->coefficient[j]*(q.v[j]-s->base.v[j]);
    }
    q.v[MOM]=(w.v[0]-w.v[1])/s->c;q.v[ENE]+=(pi-correction)/s->beta;
    if(s->spec.mean_flow) {
        const double U=s->spec.mean_flow;double density=0;
        for(int j=0;j<RB_NSPECIES;j++)density+=q.v[j]-s->base.v[j];
        const double dm=U*density+q.v[MOM];q.v[MOM]=s->base.v[MOM]+dm;
        q.v[ENE]+=U*dm-.5*U*U*density;
    }
    return q;
}
static double *forward(rb_gg_state *s,int i)
{
    rb_ac_line *l=&s->graph.line[0];return &l->ring[(l->index+l->count-1-(size_t)i)%l->count];
}
static double *backward(rb_gg_state *s,int i)
{
    rb_ac_line *l=&s->graph.line[1];return &l->ring[(l->index+(size_t)i)%l->count];
}
static void refresh(rb_gg_state *s)
{
    const int n=s->downstream_cells,g=s->spec.gas_cells;const double f=s->fraction;
    if(s->spec.mean_flow) {
        for(int i=0;i<n;i++)s->work[i].v[1]=0;
        for(int k=0;k<s->map_count;k++) {
            const rb_mix_cell map=s->overlap[k];const int target=(int)map.v[0],source=(int)map.v[1];
            double value=source<0 ? s->graph.line[1].memory/sqrt(s->back_fraction):*backward(s,source);
            s->work[target].v[1]+=map.v[2]*value;
        }
        for(int i=0;i<n;i++) {
            double a=*forward(s,i);
            if(i==n-1)a=(a+sqrt(f)*s->graph.line[0].memory)/(1+f);
            const double b=s->work[i].v[1]/rb_gg_cell_width(s,g+i);
            rb_mix_cell w={{a,b,0,0,0}};
            for(int j=0;j<RB_NSPECIES;j++)w.v[j+2]=s->contact[i].v[j];
            s->q[g+i]=physical(s,w);
        }
        return;
    }
    for(int i=0;i<n;i++) {
        double a=*forward(s,i),b;
        if(i==n-1)a=(a+sqrt(f)*s->graph.line[0].memory)/(1+f);
        if(i<n-1)b=(1-f)**backward(s,i)+(i ? f**backward(s,i-1):sqrt(f)*s->graph.line[1].memory);
        else b=(f**backward(s,n-2)+*backward(s,n-1))/(1+f);
        rb_mix_cell w={{a,b,0,0,0}};
        for(int j=0;j<RB_NSPECIES;j++)w.v[j+2]=s->contact[i].v[j];
        s->q[g+i]=physical(s,w);
    }
}
double rb_gg_cell_width(const rb_gg_state *s,int i)
{
    if(!s || i<0 || i>=s->cells)return NAN;
    return i<s->spec.gas_cells ? s->gas_dx:s->dx*(i==s->cells-1 ? 1+s->fraction:1);
}
double rb_gg_cell_center(const rb_gg_state *s,int i)
{
    if(!s || i<0 || i>=s->cells)return NAN;
    if(i<s->spec.gas_cells)return (i+.5)*s->gas_dx;
    return s->spec.interface_x+(i-s->spec.gas_cells)*s->dx+.5*rb_gg_cell_width(s,i);
}
rb_mix_cell rb_gg_totals(rb_gg_state *s,int side)
{
    rb_mix_cell total={{0}};
    if(!s || !s->q || s->failures){for(int j=0;j<NV;j++)total.v[j]=NAN;return total;}
    refresh(s);const int from=side ? s->spec.gas_cells:0,to=side ? s->cells:s->spec.gas_cells;
    for(int i=from;i<to;i++)total=add(total,s->q[i],rb_gg_cell_width(s,i)*s->spec.area);
    return total;
}
rb_mix_cell rb_gg_balance(rb_gg_state *s,int side)
{
    rb_mix_cell total=add(rb_gg_totals(s,side),s->initial[!!side],-1);
    total=add(total,side ? s->external_right:s->external_left,side ? 1:-1);
    return add(total,s->interface_transfer,side ? -1:1);
}
int rb_gg_prepare(rb_gg_state *s,const rb_gg_spec *p,rb_mix_cell *memory,size_t count,
                  double *delays,size_t samples)
{
    size_t need,ds;int status=rb_gg_sizes(p,&need,&ds);
    if(status)return status;
    if(!s || !memory || !delays)return RB_GG_ARGUMENT;
    if(count<need || samples<ds)return RB_GG_CAPACITY;
    rb_ac_plan plan;rb_mix_cell base;rb_mix_primitive w;
    if(plan_for(p,&plan,&base,&w))return RB_GG_ARGUMENT;
    memset(s,0,sizeof *s);memset(memory,0,need*sizeof *memory);memset(delays,0,ds*sizeof *delays);
    s->spec=*p;s->plan=plan;s->base=base;s->rho=w.rho;s->c=w.c;
    s->base_flux=scale(base,w.u);s->base_flux.v[MOM]+=w.p;s->base_flux.v[ENE]+=w.p*w.u;
    s->dx=(w.c+p->mean_flow)/p->rate;s->back_dx=(w.c-p->mean_flow)/p->rate;s->gas_dx=p->interface_x/p->gas_cells;
    const double d=p->rate*(p->length-p->interface_x)/(w.c+p->mean_flow);
    const double back=p->rate*(p->length-p->interface_x)/(w.c-p->mean_flow);
    s->downstream_cells=(int)floor(d);s->fraction=d-floor(d);s->cells=p->gas_cells+s->downstream_cells;
    s->back_cells=(int)floor(back);s->back_fraction=back-floor(back);
    rb_gas gas;rb_gas_init(&gas,p->y);s->beta=gas.R/rb_gas_cv(&gas,p->temperature);
    s->h0=rb_gas_h(&gas,p->temperature);
    for(int j=0;j<RB_NSPECIES;j++) {
        double y[RB_NSPECIES]={0};y[j]=1;rb_gas pure;rb_gas_init(&pure,y);
        s->coefficient[j]=p->temperature*pure.R-s->beta*rb_gas_u(&pure,p->temperature);
    }
    rb_mix_cell **field[]={&s->q,&s->w,&s->work,&s->original,&s->rhs,&s->contact,&s->slope,&s->face};
    for(int j=0;j<8;j++)*field[j]=memory+(size_t)j*s->cells;
    if(rb_ac_prepare(&s->plan,&s->graph,delays,p->mean_flow ? ds:ds/2))return RB_GG_ARGUMENT;
    if(p->mean_flow) {
        size_t offset=0;
        for(int dir=0;dir<2;dir++) {
            rb_ac_line *line=&s->graph.line[dir];const double f=dir ? s->back_fraction:s->fraction;
            line->count=(size_t)(dir ? s->back_cells:s->downstream_cells);line->ring=delays+offset;offset+=line->count;
            line->fraction=f;line->coefficient=(1-f)/(1+f);line->complement=sqrt(fmax(0,1-line->coefficient*line->coefficient));
        }
        s->overlap=memory+(size_t)8*s->cells+1;
        int i=0,j=s->back_fraction>0 ? -1:0;const double length=p->length-p->interface_x;
        while(i<s->downstream_cells && j<s->back_cells) {
            const double al=i*s->dx,ar=i==s->downstream_cells-1 ? length:(i+1)*s->dx;
            const double bl=j<0 ? 0:(s->back_fraction+j)*s->back_dx;
            const double br=j<0 ? s->back_fraction*s->back_dx:(j==s->back_cells-1 ? length:(s->back_fraction+j+1)*s->back_dx);
            const double lo=fmax(al,bl),hi=fmin(ar,br);
            if(hi>lo)s->overlap[s->map_count++]=(rb_mix_cell){{i,j,hi-lo,0,0}};
            if(ar<=br)i++;
            if(br<=ar)j++;
        }
    }
    s->ring_copy=delays+ds/2;
    for(int i=0;i<s->spec.gas_cells;i++)s->q[i]=base;
    s->initial[0]=rb_gg_totals(s,0);s->initial[1]=rb_gg_totals(s,1);return 0;
}
static int admissible(rb_gg_state *s)
{
    refresh(s);
    for(int i=0;i<s->cells;i++) {
        rb_mix_primitive w;
        if(rb_mix_decode(s->q[i],&w))return RB_GG_STATE;
        double pressure=fabs(w.p/s->spec.pressure-1);
        s->max_pressure_fraction=fmax(s->max_pressure_fraction,pressure);
        if(pressure>.03 || fabs(w.u-s->spec.mean_flow)/s->c>.05 || fabs(w.T/s->spec.temperature-1)>.05) {
            s->domain_rejections++;return RB_GG_DOMAIN;
        }
        for(int j=0;j<RB_NSPECIES;j++)if(fabs(w.y[j]-s->spec.y[j])>.02) {s->domain_rejections++;return RB_GG_DOMAIN;}
    }
    return 0;
}
int rb_gg_load(rb_gg_state *s,const rb_mix_cell *q,int cells)
{
    if(!s || !s->q || !q || s->steps || s->failures || cells!=s->cells)return RB_GG_ARGUMENT;
    for(int i=0;i<cells;i++) {
        rb_mix_primitive w;if(rb_mix_decode(q[i],&w))return RB_GG_DOMAIN;
        if(i>=s->spec.gas_cells) {
            rb_mix_cell z=coordinates(s,q[i]);
            if(fabs(z.v[0])+fabs(z.v[1])>1e-7)return RB_GG_DOMAIN;
        }
    }
    for(int i=0;i<s->spec.gas_cells;i++)s->q[i]=q[i];
    for(int i=s->spec.gas_cells;i<cells;i++) {
        rb_mix_cell w=coordinates(s,q[i]);for(int j=0;j<RB_NSPECIES;j++)s->contact[i-s->spec.gas_cells].v[j]=w.v[j+2];
    }
    int status=admissible(s);if(status){s->failures++;return status;}
    s->initial[0]=rb_gg_totals(s,0);s->initial[1]=rb_gg_totals(s,1);return 0;
}
/* Exact fixed-composition entropy integral (constant branch in these fixtures)
 * and Simpson enthalpy/sound integral construct a nonlinear right-simple wave. */
static double entropy_integral(const rb_gas *g,double T0,double T)
{
    const int n=32;double sum=0;
    for(int i=0;i<=n;i++){double t=T0+(T-T0)*i/n;sum+=(i==0||i==n ? 1:(i%2 ? 4:2))*rb_gas_cp(g,t)/t;}
    return sum*(T-T0)/(3*n);
}
int rb_gg_seed(rb_gg_state *s,double amplitude,double center,double width,double frequency)
{
    if(!s || !s->q || s->steps || s->failures || !isfinite(amplitude) || amplitude<0 || amplitude>.1 ||
       !finite_positive(width) || !isfinite(center) || center<=0 || center>=s->spec.interface_x ||
       !isfinite(frequency) || frequency<0 || frequency>8000)return RB_GG_ARGUMENT;
    rb_gas gas;rb_gas_init(&gas,s->spec.y);
    for(int i=0;i<s->cells;i++)s->work[i]=s->base;
    for(int i=0;i<s->spec.gas_cells;i++) {
        const double x=rb_gg_cell_center(s,i)-center,d=x/width;
        const double dp=fabs(d)>6 ? 0:amplitude*exp(-d*d)*cos(2*PI*frequency*x/(s->c+s->spec.mean_flow));
        const double p=s->spec.pressure*(1+dp),target=gas.R*log(p/s->spec.pressure);
        double T=s->spec.temperature;
        for(int k=0;k<12;k++)T-=(entropy_integral(&gas,s->spec.temperature,T)-target)*T/rb_gas_cp(&gas,T);
        double u=0;
        for(int k=0;k<=32;k++) {
            double t=s->spec.temperature+(T-s->spec.temperature)*k/32;
            double c=sqrt(rb_gas_gamma(&gas,t)*gas.R*t);
            u+=(k==0||k==32 ? 1:(k%2 ? 4:2))*rb_gas_cp(&gas,t)/c;
        }
        u*=(T-s->spec.temperature)/96;
        if(rb_mix_make(p,T,u+s->spec.mean_flow,s->spec.y,&s->work[i]))return RB_GG_DOMAIN;
    }
    return rb_gg_load(s,s->work,s->cells);
}
static rb_mix_cell linear_flux(const rb_gg_state *s,rb_mix_cell w)
{
    if(s->spec.mean_flow) {
        const double U=s->spec.mean_flow,H=s->h0+.5*U*U,pi=w.v[0]+w.v[1];
        rb_mix_cell q=physical(s,w),f=s->base_flux;double density=0;
        for(int j=0;j<RB_NSPECIES;j++)density+=q.v[j]-s->base.v[j];
        const double dm=q.v[MOM]-s->base.v[MOM];
        for(int j=0;j<RB_NSPECIES;j++)f.v[j]+=U*(q.v[j]-s->base.v[j])+s->spec.y[j]*(dm-U*density);
        f.v[MOM]+=2*U*dm-U*U*density+pi;
        f.v[ENE]+=H*dm-U*H*density+U*(q.v[ENE]-s->base.v[ENE]+pi);return f;
    }
    const double m=(w.v[0]-w.v[1])/s->c;rb_mix_cell f={{0}};
    for(int j=0;j<RB_NSPECIES;j++)f.v[j]=s->spec.y[j]*m;
    f.v[MOM]=s->spec.pressure+w.v[0]+w.v[1];f.v[ENE]=s->h0*m;return f;
}
static rb_mix_cell residual(const rb_gg_state *s,rb_mix_cell q,rb_mix_primitive w)
{
    rb_mix_cell f=scale(q,w.u);f.v[MOM]+=w.p;f.v[ENE]+=w.p*w.u;
    return add(f,linear_flux(s,coordinates(s,q)),-1);
}
static rb_mix_cell end_state(rb_gg_state *s,rb_mix_cell q,int right)
{
    rb_mix_cell w=coordinates(s,q);if(right)w.v[1]=s->spec.reflection*w.v[0];else w.v[0]=0;
    if(s->spec.mean_flow && ((right && s->spec.mean_flow<0) || (!right && s->spec.mean_flow>0)))
        for(int j=0;j<RB_NSPECIES;j++)w.v[j+2]=0;
    return physical(s,w);
}
static double residual_speed(const rb_gg_state *s,rb_mix_primitive w)
{
    double dy=0;for(int j=0;j<RB_NSPECIES;j++)dy=fmax(dy,fabs(w.y[j]-s->spec.y[j]));
    return 8*fabs(w.u-s->spec.mean_flow)+4*fabs(w.c-s->c)+4*s->c*(dy+fabs(w.T/s->spec.temperature-1));
}
static double mc(double a,double b)
{return a*b<=0 ? 0:copysign(fmin(.5*fabs(a+b),fmin(2*fabs(a),2*fabs(b))),a);}
static double weno5(double a,double b,double c,double d,double e,double pressure)
{
    const double q0=(2*a-7*b+11*c)/6,q1=(-b+5*c+2*d)/6,q2=(2*c+5*d-e)/6;
    const double b0=13.0/12*(a-2*b+c)*(a-2*b+c)+.25*(a-4*b+3*c)*(a-4*b+3*c);
    const double b1=13.0/12*(b-2*c+d)*(b-2*c+d)+.25*(b-d)*(b-d);
    const double b2=13.0/12*(c-2*d+e)*(c-2*d+e)+.25*(3*c-4*d+e)*(3*c-4*d+e);
    const double tau=fabs(b0-b2),amplitude=fmax(fmax(fabs(a),fabs(b)),fmax(fmax(fabs(c),fabs(d)),fabs(e)));
    const double epsilon=fmax(1e-40,1e-12*amplitude*amplitude);
    const double z0=tau/(b0+epsilon),z1=tau/(b1+epsilon),z2=tau/(b2+epsilon);
    const double w0=.1*(1+z0*z0),w1=.6*(1+z1*z1),w2=.3*(1+z2*z2);
    /* Preserve linear superposition for weak acoustics. Smoothly introduce
     * nonlinear WENO weights for finite perturbations; no audio EQ/gain fit. */
    double blend=fmax(0,fmin(1,(amplitude/pressure-1e-4)/3e-4));
    blend=blend*blend*(3-2*blend);
    const double linear=.1*q0+.6*q1+.3*q2,nonlinear=(w0*q0+w1*q1+w2*q2)/(w0+w1+w2);
    return linear+blend*(nonlinear-linear);
}
static int residual_rhs(rb_gg_state *s,double tau)
{
    refresh(s);
    for(int i=0;i<s->cells;i++)for(int j=0;j<NV;j++) {
        double dl=0,dr=0;
        if(i>0)dl=(s->q[i].v[j]-s->q[i-1].v[j])/(.5*(rb_gg_cell_width(s,i-1)+rb_gg_cell_width(s,i)));
        if(i<s->cells-1)dr=(s->q[i+1].v[j]-s->q[i].v[j])/(.5*(rb_gg_cell_width(s,i+1)+rb_gg_cell_width(s,i)));
        s->slope[i].v[j]=mc(dl,dr);
    }
    for(int f=1;f<s->cells;f++) {
        rb_mix_cell ql=add(s->q[f-1],s->slope[f-1],.5*rb_gg_cell_width(s,f-1));
        rb_mix_cell qr=add(s->q[f],s->slope[f],-.5*rb_gg_cell_width(s,f));
        rb_mix_primitive left,right;
        if(rb_mix_decode(ql,&left) || rb_mix_decode(qr,&right)) {
            ql=s->q[f-1];qr=s->q[f];s->residual_fallbacks++;
            if(rb_mix_decode(ql,&left) || rb_mix_decode(qr,&right))return RB_GG_STATE;
        }
        double speed=fmax(residual_speed(s,left),residual_speed(s,right));
        double cfl=speed*tau/fmin(rb_gg_cell_width(s,f-1),rb_gg_cell_width(s,f));
        s->max_residual_cfl=fmax(s->max_residual_cfl,cfl);if(cfl>.45)return RB_GG_DOMAIN;
        rb_mix_cell flux=add(residual(s,ql,left),residual(s,qr,right),1);
        rb_mix_cell difference=add(qr,ql,-1);
        double pi=s->beta*difference.v[ENE],dm=difference.v[MOM],density=0;
        for(int j=0;j<RB_NSPECIES;j++)density+=difference.v[j];
        if(s->spec.mean_flow) {
            const double U=s->spec.mean_flow;
            pi=s->beta*(difference.v[ENE]-U*dm+.5*U*U*density);dm-=U*density;
        }
        for(int j=0;j<RB_NSPECIES;j++)pi+=s->coefficient[j]*difference.v[j];
        rb_mix_cell acoustic={{0}};
        for(int j=0;j<RB_NSPECIES;j++)acoustic.v[j]=s->spec.y[j]*pi/(s->c*s->c);
        acoustic.v[MOM]=difference.v[MOM];acoustic.v[ENE]=s->h0*pi/(s->c*s->c);
        if(s->spec.mean_flow) {
            const double U=s->spec.mean_flow;
            acoustic.v[MOM]=U*pi/(s->c*s->c)+dm;
            acoustic.v[ENE]=(s->h0+.5*U*U)*pi/(s->c*s->c)+U*dm;
        }
        const double contact_speed=fmax(fabs(left.u-s->spec.mean_flow),fabs(right.u-s->spec.mean_flow));
        flux=add(add(scale(flux,.5),acoustic,-.5*speed),add(difference,acoustic,-1),-.5*contact_speed);
        s->face[f]=flux;
    }
    for(int end=0;end<2;end++) {
        rb_mix_cell q=end_state(s,s->q[end ? s->cells-1:0],end);rb_mix_primitive w;
        if(rb_mix_decode(q,&w))return RB_GG_STATE;
        s->face[end ? s->cells:0]=residual(s,q,w);
    }
    for(int i=0;i<s->cells;i++)s->rhs[i]=scale(add(s->face[i],s->face[i+1],-1),1/rb_gg_cell_width(s,i));
    return 0;
}
static void recalc_energy(rb_gg_state *s)
{
    for(int dir=0;dir<2;dir++) {
        rb_ac_line *l=&s->graph.line[dir];double sum=0;
        for(size_t j=0;j<l->count;j++)sum+=l->ring[j]*l->ring[j];
        l->energy=sum;
    }
}
static double acoustic_energy(const rb_gg_state *s)
{
    if(!s->spec.mean_flow)return rb_ac_energy(&s->graph);
    double energy=0;
    for(int dir=0;dir<2;dir++) {
        const rb_ac_line *line=&s->graph.line[dir];
        energy+=(1+(dir ? -1:1)*s->spec.mean_flow/s->c)*(line->energy+line->memory*line->memory)/
                (s->spec.rate*line->impedance);
    }
    return energy;
}
double rb_gg_acoustic_balance(const rb_gg_state *s)
{
    if(!s || !s->q || s->failures)return NAN;
    if(!s->spec.mean_flow)return rb_ac_balance(&s->graph)+s->coordinate_work;
    return s->acoustic_source_work-s->acoustic_terminal_work+s->coordinate_work-acoustic_energy(s);
}
static void apply_residual(rb_gg_state *s,double tau)
{
    const int g=s->spec.gas_cells,n=s->downstream_cells;const double f=s->fraction;
    for(int i=0;i<g;i++)s->q[i]=add(s->q[i],s->rhs[i],tau);
    for(int i=0;i<n;i++) {
        rb_mix_cell delta=scale(s->rhs[g+i],tau);double pi=s->beta*delta.v[ENE],momentum=delta.v[MOM];
        if(s->spec.mean_flow) {
            double density=0;for(int j=0;j<RB_NSPECIES;j++)density+=delta.v[j];
            const double U=s->spec.mean_flow;
            pi=s->beta*(delta.v[ENE]-U*delta.v[MOM]+.5*U*U*density);momentum-=U*density;
        }
        for(int j=0;j<RB_NSPECIES;j++)pi+=s->coefficient[j]*delta.v[j];
        double a=.5*(pi+s->c*momentum),b=.5*(pi-s->c*momentum);
        s->work[i].v[1]=b;*forward(s,i)+=a;
        if(i==n-1)s->graph.line[0].memory+=sqrt(f)*a;
        for(int j=0;j<RB_NSPECIES;j++)s->contact[i].v[j]+=delta.v[j]-s->spec.y[j]*pi/(s->c*s->c);
    }
    if(s->spec.mean_flow) {
        for(int k=0;k<s->map_count;k++) {
            const rb_mix_cell map=s->overlap[k];const int target=(int)map.v[0],source=(int)map.v[1];
            const double b=s->work[target].v[1];
            if(source<0)s->graph.line[1].memory+=map.v[2]*b/(sqrt(s->back_fraction)*s->back_dx);
            else *backward(s,source)+=map.v[2]*b/s->back_dx;
        }
        return;
    }
    s->graph.line[1].memory+=sqrt(f)*s->work[0].v[1];
    for(int i=0;i<n-1;i++)*backward(s,i)+=(1-f)*s->work[i].v[1]+f*s->work[i+1].v[1];
    *backward(s,n-1)+=s->work[n-1].v[1];
}
static void accumulate(rb_mix_cell *sum,rb_mix_cell *correction,rb_mix_cell value,double weight)
{
    for(int j=0;j<NV;j++) {
        const double y=value.v[j]*weight-correction->v[j],next=sum->v[j]+y;
        correction->v[j]=(next-sum->v[j])-y;sum->v[j]=next;
    }
}
static void book(rb_gg_state *s,rb_mix_cell left,rb_mix_cell right,rb_mix_cell face,double seconds,int linear)
{
    const double weight=seconds*s->spec.area;
    if(s->spec.mean_flow) {
        accumulate(&s->external_left,&s->compensation_left,left,weight);
        accumulate(&s->external_right,&s->compensation_right,right,weight);
        accumulate(&s->interface_transfer,&s->compensation_interface,face,weight);
    } else {
        s->external_left=add(s->external_left,left,weight);s->external_right=add(s->external_right,right,weight);
        s->interface_transfer=add(s->interface_transfer,face,weight);
    }
    s->last_interface=add(s->last_interface,face,weight);
    if(linear)s->last_linear=add(s->last_linear,face,weight);else s->last_residual=add(s->last_residual,face,weight);
}
static int nonlinear_half(rb_gg_state *s)
{
    const int g=s->spec.gas_cells,n=s->downstream_cells;const double tau=.5/s->spec.rate;
    const double energy=acoustic_energy(s),math_energy=rb_ac_energy(&s->graph);
    for(int i=0;i<g;i++)s->original[i]=s->q[i];
    for(int i=0;i<n;i++)s->original[g+i]=s->contact[i];
    size_t copy_offset=0;
    for(int dir=0;dir<2;dir++) {
        rb_ac_line *l=&s->graph.line[dir];memcpy(s->ring_copy+copy_offset,l->ring,l->count*sizeof(double));s->memory_copy[dir]=l->memory;
        copy_offset+=l->count;
    }
    int status=residual_rhs(s,tau);if(status)return status;
    rb_mix_cell left=s->face[0],right=s->face[s->cells],face=s->face[g];
    apply_residual(s,tau);status=admissible(s);if(status)return status;
    status=residual_rhs(s,tau);if(status)return status;
    apply_residual(s,tau);
    for(int i=0;i<g;i++)s->q[i]=scale(add(s->q[i],s->original[i],1),.5);
    for(int i=0;i<n;i++)s->contact[i]=scale(add(s->contact[i],s->original[g+i],1),.5);
    copy_offset=0;
    for(int dir=0;dir<2;dir++) {
        rb_ac_line *l=&s->graph.line[dir];
        for(size_t i=0;i<l->count;i++)l->ring[i]=.5*(l->ring[i]+s->ring_copy[copy_offset+i]);
        l->memory=.5*(l->memory+s->memory_copy[dir]);
        copy_offset+=l->count;
    }
    recalc_energy(s);s->coordinate_work+=acoustic_energy(s)-energy;
    s->coordinate_work_math+=rb_ac_energy(&s->graph)-math_energy;
    book(s,add(left,s->face[0],1),add(right,s->face[s->cells],1),add(face,s->face[g],1),tau*.5,0);
    return admissible(s);
}
static void linear_rhs(rb_gg_state *s,const rb_mix_cell *w,double ghost,double incoming)
{
    const int g=s->spec.gas_cells;
    for(int i=0;i<g;i++)for(int j=0;j<2;j++) {
        const double before=i ? w[i-1].v[j]:(j ? w[0].v[1]:0);
        const double after=i<g-1 ? w[i+1].v[j]:(j ? incoming:ghost);
        const double dl=w[i].v[j]-before,dr=after-w[i].v[j];
        if(i>=2 && i<g-2) {
            double face=weno5(w[i+(j ? 2:-2)].v[j],w[i+(j ? 1:-1)].v[j],w[i].v[j],
                              w[i+(j ? -1:1)].v[j],w[i+(j ? -2:2)].v[j],s->spec.pressure);
            s->slope[i].v[j]=2*(j ? w[i].v[j]-face:face-w[i].v[j]);
            s->linear_smooth++;
        } else {s->slope[i].v[j]=j ? (2*dl+dr)/3:(dl+2*dr)/3;s->linear_limited++;}
    }
    s->face[0]=(rb_mix_cell){{0,w[0].v[1]-.5*s->slope[0].v[1],0,0,0}};
    s->face[0].v[1]=(2*w[2].v[1]-7*w[1].v[1]+11*w[0].v[1])/6;
    for(int i=1;i<=g;i++)s->face[i]=(rb_mix_cell){{w[i-1].v[0]+.5*s->slope[i-1].v[0],
        i<g ? w[i].v[1]-.5*s->slope[i].v[1]:incoming,0,0,0}};
    s->face[g].v[0]=(2*w[g-3].v[0]-7*w[g-2].v[0]+11*w[g-1].v[0])/6;
    for(int i=0;i<g;i++)s->rhs[i]=(rb_mix_cell){{
        -(s->c+s->spec.mean_flow)/s->gas_dx*(s->face[i+1].v[0]-s->face[i].v[0]),
         (s->c-s->spec.mean_flow)/s->gas_dx*(s->face[i+1].v[1]-s->face[i].v[1]),0,0,0}};
    if(!s->spec.mean_flow)return;
    const int N=s->cells;const double U=s->spec.mean_flow;
    for(int i=g;i<N;i++)s->rhs[i]=(rb_mix_cell){{0}};
    for(int i=0;i<N;i++)for(int j=2;j<NV;j++) {
        double dl=0,dr=0;
        if(i)dl=(w[i].v[j]-w[i-1].v[j])/(.5*(rb_gg_cell_width(s,i)+rb_gg_cell_width(s,i-1)));
        if(i<N-1)dr=(w[i+1].v[j]-w[i].v[j])/(.5*(rb_gg_cell_width(s,i)+rb_gg_cell_width(s,i+1)));
        s->slope[i].v[j]=mc(dl,dr);
    }
    for(int f=0;f<=N;f++)for(int j=2;j<NV;j++) {
        const int i=U>0 ? f-1:f;
        s->face[f].v[j]=(i<0 || i>=N) ? 0:w[i].v[j]+(U>0 ? 1:-1)*.5*rb_gg_cell_width(s,i)*s->slope[i].v[j];
    }
    for(int i=0;i<N;i++)for(int j=2;j<NV;j++)
        s->rhs[i].v[j]=-U/rb_gg_cell_width(s,i)*(s->face[i+1].v[j]-s->face[i].v[j]);
}
static int linear_tick(rb_gg_state *s)
{
    const int g=s->spec.gas_cells,N=s->spec.mean_flow ? s->cells:g;refresh(s);
    for(int i=0;i<g;i++)s->w[i]=coordinates(s,s->q[i]);
    for(int i=g;i<N;i++) {
        s->w[i]=(rb_mix_cell){{0}};
        for(int j=0;j<RB_NSPECIES;j++)s->w[i].v[j+2]=s->contact[i-g].v[j];
    }
    const double ghost=coordinates(s,s->q[g]).v[0];
    if(rb_ac_receive(&s->graph))return RB_GG_STATE;
    s->received_return=rb_ac_incident(&s->graph,0);s->used_return=s->received_return;
    const double incoming=s->used_return,tau=.5/s->spec.rate,incident=s->graph.incoming[1];double outgoing=0;
    rb_mix_cell left={{0}},accepted={{0}},right={{0}};
    for(int sub=0;sub<2;sub++) {
        for(int i=0;i<N;i++)s->original[i]=s->w[i];
        rb_mix_cell l[3],r[3],t[3];
        for(int stage=0;stage<3;stage++) {
            const rb_mix_cell *state=stage ? s->work:s->w;
            linear_rhs(s,state,ghost,incoming);l[stage]=s->face[0];r[stage]=s->face[g];
            t[stage]=(rb_mix_cell){{incident,s->spec.reflection*incident,0,0,0}};
            if(s->spec.mean_flow)for(int j=2;j<NV;j++)t[stage].v[j]=s->face[N].v[j];
            for(int i=0;i<N;i++) {
                rb_mix_cell next=add(state[i],s->rhs[i],tau);
                if(stage==0)s->work[i]=next;
                else if(stage==1)s->work[i]=add(scale(s->original[i],.75),next,.25);
                else s->w[i]=add(scale(s->original[i],1.0/3),next,2.0/3);
            }
        }
        outgoing+=(r[0].v[0]+r[1].v[0]+4*r[2].v[0])/12;
        for(int stage=0;stage<3;stage++) {
            double weight=tau*(stage==2 ? 4:1)/6;
            left=add(left,linear_flux(s,l[stage]),weight);
            accepted=add(accepted,linear_flux(s,r[stage]),weight);
            right=add(right,linear_flux(s,t[stage]),weight);
        }
    }
    for(int i=0;i<g;i++)s->q[i]=physical(s,s->w[i]);
    for(int i=g;i<N;i++)for(int j=0;j<RB_NSPECIES;j++)s->contact[i-g].v[j]=s->w[i].v[j+2];
    rb_ac_drive drive={0};drive.wave[0]=outgoing;
    if(rb_ac_commit(&s->graph,&drive))return RB_GG_STATE;
    s->accepted_outgoing=outgoing;s->previous_return=incoming;
    const double M=s->spec.mean_flow/s->c,k=s->spec.area/(s->spec.rate*s->rho*s->c);
    s->acoustic_source_work+=k*((1+M)*outgoing*outgoing-(1-M)*incoming*incoming);
    s->acoustic_terminal_work+=k*((1+M)*incident*incident-(1-M)*s->spec.reflection*s->spec.reflection*incident*incident);
    if(!s->spec.mean_flow) {
        rb_mix_cell boundary={{incident,s->spec.reflection*incident,0,0,0}};
        right=scale(linear_flux(s,boundary),1/s->spec.rate);
    }
    book(s,left,right,accepted,1,1);
    return admissible(s);
}

int rb_gg_step(rb_gg_state *s)
{
    if(!s || !s->q)return RB_GG_ARGUMENT;
    if(s->failures)return RB_GG_STATE;
    s->last_interface=s->last_linear=s->last_residual=(rb_mix_cell){{0}};
    int status=nonlinear_half(s);if(!status)status=linear_tick(s);if(!status)status=nonlinear_half(s);
    if(status){s->failures++;return status;}s->steps++;return 0;
}
double rb_gg_pressure(rb_gg_state *s,int cell)
{
    if(!s || !s->q || s->failures || cell<0 || cell>=s->cells)return NAN;
    refresh(s);rb_mix_primitive w;return rb_mix_decode(s->q[cell],&w) ? NAN:w.p;
}
double rb_gg_perturbation_energy(rb_gg_state *s)
{
    if(!s || !s->q || s->failures)return NAN;
    double energy=acoustic_energy(s);
    for(int i=0;i<s->spec.gas_cells;i++) {
        rb_mix_primitive w;if(rb_mix_decode(s->q[i],&w))return NAN;
        double dp=w.p-s->spec.pressure;energy+=s->spec.area*s->gas_dx*(dp*dp/(s->rho*s->c*s->c)+s->rho*(w.u-s->spec.mean_flow)*(w.u-s->spec.mean_flow))*.5;
    }
    return energy;
}
