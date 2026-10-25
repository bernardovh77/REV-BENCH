#define _POSIX_C_SOURCE 200809L
#include "material.h"
#include "thermal.h"
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
static int number(const char *s,double *v){char *e;*v=strtod(s,&e);return e!=s&&!*e&&isfinite(*v);}
int main(int argc,char **argv)
{
    if(argc!=11)return 2;
    const char *mode=argv[1];double a[9];for(int j=0;j<9;j++)if(!number(argv[j+2],&a[j]))return 2;
    if(strcmp(mode,"profile")&&strcmp(mode,"contact")&&strcmp(mode,"reversal")&&strcmp(mode,"outlet")&&strcmp(mode,"outlet-positive")&&strcmp(mode,"wall")&&strcmp(mode,"stability")&&strcmp(mode,"cost"))return 2;
    if(a[0]<8||a[0]>8192||a[0]!=floor(a[0])||a[3]<0||a[3]>1||a[3]!=floor(a[3]))return 2;
    const int n=(int)a[0],kind=(int)a[3];double rate=a[1],U=a[2],front=a[4],length=a[5],amplitude=a[6],frequency=a[7],duration=a[8];
    if(n<8||n>8192||n!=a[0]||rate<8000||rate>2000000||kind<0||kind>1||front<=0||front>=length||duration<=0||duration*rate>2000000)return 2;
    rb_mt_cell *cells=calloc((size_t)n,sizeof *cells);rb_mt_node *nodes=calloc((size_t)n+1,sizeof *nodes);rb_mt_state s;
    if(!cells||!nodes||rb_mt_prepare(&s,n,.003,101325,U,cells,(size_t)n,nodes,(size_t)n+1))return 3;
    const double ys[2][3]={{1,0,0},{.1,.001,.899}},temperatures[]={300,900};
    const double center=length>=1 ? .2:.25*front,width=length>=1 ? .04:.1*front;
    const int cut=(int)round(n*front/length);
    if(cut<1||cut>=n)return 2;
    for(int i=0;i<=n;i++)nodes[i].x=i<=cut ? i*front/cut:front+(i-cut)*(length-front)/(n-cut);
    rb_gas leftgas,rightgas;rb_gas_init(&leftgas,ys[kind]);rb_gas_init(&rightgas,ys[1-kind]);
    const double cL=sqrt(rb_gas_gamma(&leftgas,temperatures[kind])*leftgas.R*temperatures[kind]);
    const double cR=sqrt(rb_gas_gamma(&rightgas,temperatures[1-kind])*rightgas.R*temperatures[1-kind]);
    const double rhoL=101325/(leftgas.R*temperatures[kind]),rhoR=101325/(rightgas.R*temperatures[1-kind]);
    const int seeded=strcmp(mode,"contact")&&strcmp(mode,"reversal")&&strcmp(mode,"outlet")&&strcmp(mode,"outlet-positive")&&strcmp(mode,"wall");
    for(int i=0;i<n;i++) {
        int which=i<cut ? kind:1-kind;double T=temperatures[which],x=.5*(nodes[i].x+nodes[i+1].x),dp=0;
        rb_gas g;rb_gas_init(&g,ys[which]);double c=sqrt(rb_gas_gamma(&g,T)*g.R*T),rho=101325/(g.R*T);
        if(seeded) {
            if(!strcmp(mode,"stability"))dp=amplitude*cos(2*PI*frequency*(x<front ? x/cL:front/cL+(x-front)/cR));
            else if(i<cut){double d=(x-center)/width;dp=amplitude*exp(-d*d)*cos(2*PI*frequency*(x-center)/cL);}
        }
        cells[i].p=101325+dp;cells[i].T=(101325+dp)/((rho+dp/(c*c))*g.R);
        for(int j=0;j<3;j++)cells[i].m[j]=ys[which][j];
    }
    for(int i=0;i<=n;i++) {
        double dp=0,x=nodes[i].x;
        if(seeded) {
            if(!strcmp(mode,"stability"))dp=amplitude*cos(2*PI*frequency*(x<front ? x/cL:front/cL+(x-front)/cR));
            else if(i<cut){double d=(x-center)/width;dp=amplitude*exp(-d*d)*cos(2*PI*frequency*(x-center)/cL);}
        }
        nodes[i].v=U+dp/(i<=cut ? rhoL*cL:rhoR*cR);
    }
    int status=rb_mt_load(&s);
    if(!status&&!strcmp(mode,"wall"))status=rb_mt_wall(&s,rb_tw_capacity(.0618038723,.5,.001,7800,500),650,.25,.75,100,2,300);
    const double initialPE=rb_mt_perturbation(&s),initialE=rb_mt_energy(&s);double maxPE=initialPE,minU=0,maxU=0,maxambient=0;
    double left[3],leftE;rb_mt_left_inventory(&s,front,left,&leftE);double originalLeftE=leftE;double originalLeft[3];memcpy(originalLeft,left,sizeof left);
    const int steps=(int)round(duration*rate);double time=0;
    const int trace=!strcmp(mode,"stability")||!strcmp(mode,"reversal")||!strncmp(mode,"outlet",6)||!strcmp(mode,"wall");
    if(trace)puts("time_s,contact_x_m,air_left_kg,fuel_left_kg,burned_left_kg,energy_J,energy_balance_J,perturbation_J,port_u_m_s,wall_K");
    const double started=measured();
    for(int tick=0;tick<steps && !status;tick++) {
        double force=0,drive=0,dt=1/rate;
        if(!strcmp(mode,"reversal")) {
            double old=70*cos(2*PI*100*tick*dt),next=70*cos(2*PI*100*(tick+1)*dt);
            s.reservoir_U=s.frame_U=.5*(old+next);force=(next-old)/dt;
        }
        if(!strncmp(mode,"outlet",6))drive=(!strcmp(mode,"outlet-positive") ? 10:-10)*sin(2*PI*200*(tick+.5)*dt);
        status=rb_mt_step(&s,dt,drive,0,force);if(status)break;time=(tick+1)*dt;
        if(strcmp(mode,"cost") && tick%32==0) {
            maxPE=fmax(maxPE,rb_mt_perturbation(&s));rb_mt_left_inventory(&s,front,left,&leftE);
            double portv=0;
            for(int i=0;i<n;i++)if(nodes[i].x+s.shift<=front && nodes[i+1].x+s.shift>=front) {
                double f=(front-s.shift-nodes[i].x)/(nodes[i+1].x-nodes[i].x);portv=(1-f)*nodes[i].v+f*nodes[i+1].v;break;
            }
            minU=fmin(minU,portv);maxU=fmax(maxU,portv);
            double cold=0;
            for(int i=cut;i<n;i++)cold+=cells[i].mass*fmax(0,fmin(1,(front-s.shift-nodes[i].x)/(nodes[i+1].x-nodes[i].x)));
            maxambient=fmax(maxambient,cold);
            if(trace)printf("%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n",time,nodes[cut].x+s.shift,left[0],left[1],left[2],rb_mt_energy(&s),rb_mt_energy(&s)-initialE-s.boundary_work-s.body_work-s.ambient_heat,rb_mt_perturbation(&s),portv,s.wall_T);
        }
    }
    const double elapsed=measured()-started;
    if(!status && (!strcmp(mode,"profile")||!strcmp(mode,"contact"))) {
        puts("x_m,dx_m,p_Pa,T_K,rho_kg_m3,u_m_s,c_m_s,air_kg,fuel_kg,burned_kg");
        for(int i=0;i<n;i++)printf("%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n",.5*(nodes[i].x+nodes[i+1].x)+s.shift,nodes[i+1].x-nodes[i].x,cells[i].p,cells[i].T,cells[i].mass/(s.area*(nodes[i+1].x-nodes[i].x)),.5*(nodes[i].v+nodes[i+1].v),cells[i].c,cells[i].m[0],cells[i].m[1],cells[i].m[2]);
    }
    rb_mt_left_inventory(&s,front,left,&leftE);
    fprintf(stderr,"RB_MT_RESULT {\"status\":%d,\"steps\":%llu,\"time_s\":%.17g,\"cpu_s\":%.17g,\"timing_clock\":\"%s\",\"cells\":%d,\"bytes\":%zu,\"c_left\":%.17g,\"c_right\":%.17g,\"rho_left\":%.17g,\"rho_right\":%.17g,\"max_cfl\":%.17g,\"iterations\":%llu,\"max_iterations\":%llu,\"initial_PE\":%.17g,\"maximum_PE\":%.17g,\"final_PE\":%.17g,\"energy_balance_J\":%.17g,\"momentum_balance\":%.17g,\"max_pressure_fraction\":%.17g,\"shift\":%.17g,\"port_velocity_min\":%.17g,\"port_velocity_max\":%.17g,\"ambient_inward_kg\":%.17g,\"wall_capacity\":%.17g,\"wall_T\":%.17g,\"initial_E_J\":%.17g,\"boundary_work_J\":%.17g,\"body_work_J\":%.17g,\"ambient_heat_J\":%.17g,\"left_internal_J\":%.17g,\"initial_left_internal_J\":%.17g,\"left_species\":[%.17g,%.17g,%.17g],\"initial_left_species\":[%.17g,%.17g,%.17g]}\n",status,s.steps,time,elapsed,TIMING_CLOCK,n,sizeof s+(size_t)n*sizeof *cells+((size_t)n+1)*sizeof *nodes,cL,cR,rhoL,rhoR,s.max_cfl,s.iterations,s.max_iterations,initialPE,maxPE,rb_mt_perturbation(&s),rb_mt_energy(&s)-initialE-s.boundary_work-s.body_work-s.ambient_heat,rb_mt_momentum(&s)-s.initial_momentum-s.boundary_impulse-s.body_impulse,s.max_pressure_fraction,s.shift,minU,maxU,maxambient,s.wall_capacity,s.wall_T,initialE,s.boundary_work,s.body_work,s.ambient_heat,leftE,originalLeftE,left[0],left[1],left[2],originalLeft[0],originalLeft[1],originalLeft[2]);
    free(cells);free(nodes);return 0;
}
