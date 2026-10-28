#define _POSIX_C_SOURCE 200809L
#include "junction.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define PI 3.14159265358979323846
#ifdef __wasi__
#define TIMING_CLOCK "monotonic_wall"
#else
#define TIMING_CLOCK "process_cpu"
#endif
static double measured(void)
{
#ifdef __wasi__
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+1e-9*t.tv_nsec;
#else
    return (double)clock()/CLOCKS_PER_SEC;
#endif
}
static rb_jv_q boundary(const rb_jv_state *s,int port,double T,int mode,int source,double amplitude,double time,double U,double *drive)
{
    *drive=0;
    double alpha[2]={T<=450?1:0,T<=450?0:1},temperatures[2]={300,900},v[3]={U,0,0};
    int transport=mode==2||mode==3;
    if(transport&&port==source){alpha[0]=1;alpha[1]=0;T=300;}
    for(int k=0;k<2;k++)if(alpha[k])temperatures[k]=T;
    double pressure=0;
    if(port==source) {
        if(mode==0){double z=(time-.0005)/.00012;pressure=amplitude*exp(-z*z);}
        if(mode==2)pressure=amplitude*(1-exp(-time/.0002));
        if(mode==3)pressure=amplitude*sin(2*PI*200*time);
    }
    rb_jv_q base=rb_jv_make(s,101325,v,alpha,temperatures);double w[8],rho,c;
    if(rb_jv_decode(s,base,w,&rho,&c)){base.energy=NAN;return base;}
    for(int j=0;j<3;j++)v[j]-=pressure/(rho*c)*s->port_normal[port][j];
    double factor=(1+pressure/101325)/(1+pressure/(rho*c*c));
    for(int k=0;k<2;k++)if(alpha[k])temperatures[k]*=factor;
    *drive=pressure;return rb_jv_make(s,101325+pressure,v,alpha,temperatures);
}
int main(void)
{
    int nc,nf,np,source;double rate,T,amplitude,duration,U;char name[20];
    if(scanf("%d%d%d%19s%lf%lf%d%lf%lf%lf",&nc,&nf,&np,name,&rate,&T,&source,&amplitude,&duration,&U)!=10||
       nc<1||nc>8192||nf<1||nf>65536||np<1||np>RB_JV_PORTS||source<0||source>=np||!isfinite(rate)||rate<8000||rate>8000000||
       !isfinite(T)||T<200||T>990||!isfinite(amplitude)||!isfinite(duration)||duration<=0||duration*rate>8000000||!isfinite(U))return 2;
    int mode;
    if(!strcmp(name,"pulse"))mode=0;else if(!strcmp(name,"translation"))mode=1;else if(!strcmp(name,"flow"))mode=2;
    else if(!strcmp(name,"reversal"))mode=3;else if(!strcmp(name,"stability"))mode=4;else if(!strcmp(name,"cost"))mode=5;
    else if(!strcmp(name,"quiet"))mode=6;else return 2;
    rb_jv_cell_geometry *g=calloc((size_t)nc,sizeof *g);rb_jv_face_geometry *f=calloc((size_t)nf,sizeof *f);
    rb_jv_cell *cells=calloc((size_t)nc,sizeof *cells);rb_jv_face *faces=calloc((size_t)nf,sizeof *faces);rb_jv_state s;
    double *fraction=calloc((size_t)nc,sizeof *fraction);
    if(!g||!f||!cells||!faces||!fraction)return 3;
    for(int i=0;i<nc;i++)if(scanf("%lf%lf%lf%lf%d%lf",&g[i].volume,&g[i].center[0],&g[i].center[1],&g[i].center[2],&g[i].region,&fraction[i])!=6||!isfinite(fraction[i])||fraction[i]<0||fraction[i]>1)return 2;
    for(int i=0;i<nf;i++)if(scanf("%d%d%d%lf%lf%lf%lf%lf%lf%lf",&f[i].a,&f[i].b,&f[i].port,&f[i].area,&f[i].normal[0],&f[i].normal[1],&f[i].normal[2],&f[i].center[0],&f[i].center[1],&f[i].center[2])!=10)return 2;
    char extra;if(scanf(" %c",&extra)==1)return 2;
    const double air[3]={1,0,0},burned[3]={.1,.001,.899};
    if(rb_jv_prepare(&s,nc,nf,np,g,f,cells,(size_t)nc,faces,(size_t)nf,101325,air,burned,300,900))return 4;
    double alpha[2]={T<=450?1:0,T<=450?0:1},temperatures[2]={300,900},v0[3]={U,0,0};
    for(int k=0;k<2;k++)if(alpha[k])temperatures[k]=T;
    rb_jv_q base=rb_jv_make(&s,101325,v0,alpha,temperatures);double basew[8],rho,c;
    if(rb_jv_decode(&s,base,basew,&rho,&c))return 4;
    double volume=0,xlo=HUGE_VAL,xhi=-HUGE_VAL;
    for(int i=0;i<nc;i++){volume+=g[i].volume;xlo=fmin(xlo,g[i].center[0]);xhi=fmax(xhi,g[i].center[0]);}
    for(int i=0;i<nc;i++) {
        double a[2]={alpha[0],alpha[1]},temps[2]={temperatures[0],temperatures[1]},v[3]={U,0,0},dp=0;
        if(mode==1||mode==2||mode==3){a[0]=fraction[i];a[1]=1-a[0];temps[0]=300;temps[1]=900;}
        if(mode==4){double center=.5*(xlo+xhi),width=.1*(xhi-xlo),z=(g[i].center[0]-center)/width;dp=amplitude*exp(-z*z)*cos(2*PI*4000*(g[i].center[0]-center)/c);v[0]+=dp/(rho*c);}
        for(int k=0;k<2;k++)if(a[k])temps[k]*=(1+dp/101325)/(1+dp/(rho*c*c));
        cells[i].q=rb_jv_make(&s,101325+dp,v,a,temps);
    }
    int status=rb_jv_load(&s);double initialPE=rb_jv_perturbation(&s,v0),maximumPE=initialPE;
    int steps=(int)round(duration*rate),stride=(mode==0?1:32);double time=0,minflow[RB_JV_PORTS]={0},maxflow[RB_JV_PORTS]={0};
    if(mode!=5) {
        printf("time_s,drive_Pa");for(int p=0;p<np;p++)printf(",port_%d_delta_p_Pa,port_%d_outward_u_m_s",p,p);
        puts(",air_material_kg,burned_material_kg,energy_J,energy_balance_J,perturbation_J");
    }
    double started=measured();
    for(int tick=0;tick<steps&&!status;tick++) {
        rb_jv_q ports[2][RB_JV_PORTS];double source_drive[2]={0,0};
        for(int stage=0;stage<2;stage++)for(int p=0;p<np;p++) {
            double d;ports[stage][p]=boundary(&s,p,T,mode,source,amplitude,(tick+stage)/rate,U,&d);
            if(mode==1) {
                double a[2]={s.port_normal[p][0]<0?1:0,s.port_normal[p][0]<0?0:1},temps[2]={300,900};
                ports[stage][p]=rb_jv_make(&s,101325,v0,a,temps);
            }
            if(p==source)source_drive[stage]=d;
        }
        status=rb_jv_step(&s,1/rate,ports[0],ports[1],(size_t)np);if(status)break;time=(tick+1)/rate;
        if(mode!=5&&tick%stride==0) {
            rb_jv_q q=rb_jv_inventory(&s,-1);double PE=rb_jv_perturbation(&s,v0);maximumPE=fmax(maximumPE,PE);
            printf("%.17g,%.17g",(tick+.5)/rate,.5*(source_drive[0]+source_drive[1]));
            for(int p=0;p<np;p++){printf(",%.17g,%.17g",s.port_pressure[p]-101325,s.port_velocity[p]);minflow[p]=fmin(minflow[p],s.port_velocity[p]);maxflow[p]=fmax(maxflow[p],s.port_velocity[p]);}
            printf(",%.17g,%.17g,%.17g,%.17g,%.17g\n",q.r[0],q.r[1],q.energy,q.energy-s.initial.energy-s.external.energy,PE);
        }
    }
    double elapsed=measured()-started;rb_jv_q total=rb_jv_inventory(&s,-1);
    double regionE=0,regionMass=0,regionMomentum=0;
    for(int r=0;r<RB_JV_REGIONS;r++) {
        rb_jv_q q=rb_jv_inventory(&s,r),b=s.regional_transfer[r],initial=s.regional_initial[r];
        regionE=fmax(regionE,fabs(q.energy-initial.energy-b.energy));
        for(int k=0;k<2;k++)regionMass=fmax(regionMass,fabs(q.r[k]-initial.r[k]-b.r[k]));
        for(int j=0;j<3;j++)regionMomentum=fmax(regionMomentum,fabs(q.momentum[j]-initial.momentum[j]-b.momentum[j]));
    }
    fprintf(stderr,"RB_JV_RESULT {\"status\":%d,\"steps\":%llu,\"time_s\":%.17g,\"elapsed_s\":%.17g,\"timing_clock\":\"%s\",\"cells\":%d,\"faces\":%d,\"ports\":%d,\"components\":%d,\"volume_m3\":%.17g,\"bytes\":%zu,\"rho_reference\":%.17g,\"c_reference\":%.17g,\"max_cfl\":%.17g,\"max_mach\":%.17g,\"max_pressure_fraction\":%.17g,\"reconstruction_fallbacks\":%llu,\"trace_face_limits\":%llu,\"energy_balance_J\":%.17g,\"mass_balance\":[%.17g,%.17g],\"momentum_balance\":[%.17g,%.17g,%.17g],\"regional_mass_error_kg\":%.17g,\"regional_momentum_error_Ns\":%.17g,\"regional_energy_error_J\":%.17g,\"initial_E_J\":%.17g,\"initial_PE\":%.17g,\"maximum_PE\":%.17g,\"final_PE\":%.17g,\"initial_mass\":[%.17g,%.17g],\"final_mass\":[%.17g,%.17g],\"port_areas\":[",status,s.steps,time,elapsed,TIMING_CLOCK,nc,nf,np,s.components,volume,sizeof s+(size_t)nc*(sizeof *g+sizeof *cells)+(size_t)nf*(sizeof *f+sizeof *faces),rho,c,s.max_cfl,s.max_mach,s.max_pressure_fraction,s.reconstruction_fallbacks,s.trace_face_limits,total.energy-s.initial.energy-s.external.energy,total.r[0]-s.initial.r[0]-s.external.r[0],total.r[1]-s.initial.r[1]-s.external.r[1],total.momentum[0]-s.initial.momentum[0]-s.external.momentum[0],total.momentum[1]-s.initial.momentum[1]-s.external.momentum[1],total.momentum[2]-s.initial.momentum[2]-s.external.momentum[2],regionMass,regionMomentum,regionE,s.initial.energy,initialPE,maximumPE,rb_jv_perturbation(&s,v0),s.initial.r[0],s.initial.r[1],total.r[0],total.r[1]);
    for(int p=0;p<np;p++)fprintf(stderr,"%s%.17g",p?",":"",s.port_area[p]);
    fprintf(stderr,"],\"port_min_velocity\":[");for(int p=0;p<np;p++)fprintf(stderr,"%s%.17g",p?",":"",minflow[p]);
    fprintf(stderr,"],\"port_max_velocity\":[");for(int p=0;p<np;p++)fprintf(stderr,"%s%.17g",p?",":"",maxflow[p]);
    fprintf(stderr,"]}\n");free(g);free(f);free(cells);free(faces);free(fraction);return 0;
}
