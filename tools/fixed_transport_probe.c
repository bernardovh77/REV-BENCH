#define _POSIX_C_SOURCE 200809L
#include "fixed_transport.h"
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
    if(strcmp(mode,"profile")&&strcmp(mode,"contact")&&strcmp(mode,"throughflow")&&strcmp(mode,"outlet")&&strcmp(mode,"wall")&&strcmp(mode,"stability")&&strcmp(mode,"cost"))return 2;
    if(a[0]<8||a[0]>8192||a[0]!=floor(a[0])||a[3]<0||a[3]>1||a[3]!=floor(a[3]))return 2;
    int n=(int)a[0],kind=(int)a[3];double rate=a[1],U=a[2],front=a[4],length=a[5],amp=a[6],freq=a[7],duration=a[8];
    if(rate<8000||rate>8000000||front<=0||front>=length||duration<=0||duration*rate>8000000)return 2;
    rb_ft_slot *storage=calloc((size_t)n+1,sizeof *storage);rb_ft_state s;
    const double ys[2][3]={{1,0,0},{.1,.001,.899}},Ts[2]={300,900};
    if(!storage||rb_ft_prepare(&s,n,length,.003,101325,ys[0],ys[1],300,900,storage,(size_t)n+1))return 3;
    double c[2],rho[2];
    for(int k=0;k<2;k++){rho[k]=101325/(s.gas[k].R*Ts[k]);c[k]=sqrt(rb_gas_gamma(&s.gas[k],Ts[k])*s.gas[k].R*Ts[k]);}
    int single=!strcmp(mode,"wall")||!strcmp(mode,"stability")||!strcmp(mode,"cost")||!strcmp(mode,"outlet");
    int wave=!strcmp(mode,"profile")||!strcmp(mode,"stability")||!strcmp(mode,"cost");
    for(int i=0;i<n;i++) {
        double x=(i+.5)*s.dx,frac=fmax(0,fmin(1,(front-i*s.dx)/s.dx)),alpha=kind?1-frac:frac;
        if(single)alpha=!strcmp(mode,"wall")?1:0;
        double dp=0;
        if(wave) {
            if(!strcmp(mode,"stability"))dp=amp*cos(2*PI*freq*x/c[1]);
            else if(x<front){double z=(x-.2)/.04;dp=amp*exp(-z*z)*cos(2*PI*freq*(x-.2)/c[kind]);}
        }
        int k=single?(alpha==1?0:1):(x<front?kind:1-kind);
        double T[2]={300,900};T[k]=(101325+dp)/((rho[k]+dp/(c[k]*c[k]))*s.gas[k].R);
        s.slot[i].q=rb_ft_make(&s,101325+dp,U+dp/(rho[k]*c[k]),alpha,T[0],T[1]);
    }
    int status=rb_ft_load(&s);
    if(!status&&!strcmp(mode,"wall"))status=rb_ft_wall(&s,400,650,.25*length,.75*length,100,2,300);
    rb_ft_book initial=s.initial;int partition=n/2;rb_ft_book regional=rb_ft_inventory(&s,0,partition);
    double initialPE=rb_ft_perturbation(&s,U),maxPE=initialPE,oldwall=s.wall_capacity*s.wall_T;
    int steps=(int)round(duration*rate);double time=0,minmass=0,maxmass=0,maxcold=0,maxdp=0,maxdu=0;
    int trace=!strcmp(mode,"outlet")||!strcmp(mode,"wall")||!strcmp(mode,"stability")||!strcmp(mode,"throughflow");
    if(trace)puts("time_s,energy_J,energy_balance_J,mass0_kg,mass1_kg,left_mass0_kg,right_mass0_kg,perturbation_J,right_u_m_s,wall_K");
    double started=measured();
    for(int tick=0;tick<steps&&!status;tick++) {
        double pleft=101325,ul=U,al=kind?0:1,ar=kind?1:0;
        if(single)al=ar=!strcmp(mode,"wall")?1:0;
        if(!strcmp(mode,"outlet")){double drive=-10*sin(2*PI*200*(tick+.5)/rate);pleft+=drive;ul=U+drive/(rho[1]*c[1]);ar=1;}
        rb_ft_q left=rb_ft_make(&s,pleft,ul,al,300,900),right=rb_ft_make(&s,101325,U,ar,300,900);
        status=rb_ft_step(&s,1/rate,left,right);if(status)break;time=(tick+1)/rate;
        if(strcmp(mode,"cost")&&tick%32==0) {
            double PE=rb_ft_perturbation(&s,U);maxPE=fmax(maxPE,PE);
            rb_ft_book b=rb_ft_inventory(&s,0,n);double balance=b.energy-initial.energy-s.slot[0].transfer.energy+s.slot[n].transfer.energy-s.gas_heat;
            double flow=s.slot[n].face.f.r[0]+s.slot[n].face.f.r[1];minmass=fmin(minmass,flow*s.area);maxmass=fmax(maxmass,flow*s.area);maxcold=fmax(maxcold,b.m[0]);
            if(trace)printf("%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n",time,b.energy,balance,b.m[0],b.m[1],s.slot[0].transfer.m[0],s.slot[n].transfer.m[0],PE,s.slot[n].face.u,s.wall_T);
        }
    }
    double elapsed=measured()-started;
    if(!status&&(!strcmp(mode,"profile")||!strcmp(mode,"contact"))) {
        puts("x_m,dx_m,p_Pa,T_K,rho_kg_m3,u_m_s,c_m_s,air_kg,fuel_kg,burned_kg,alpha0");
        for(int i=0;i<n;i++) {
            rb_ft_primitive w;rb_ft_decode(&s,s.slot[i].q,&w);
            maxdp=fmax(maxdp,fabs(w.p-101325));maxdu=fmax(maxdu,fabs(w.u-U));
            double V=s.area*s.dx,ms[3]={0,0,0};for(int k=0;k<2;k++)for(int j=0;j<3;j++)ms[j]+=V*s.slot[i].q.r[k]*ys[k][j];
            printf("%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n",(i+.5)*s.dx,s.dx,w.p,w.alpha[0]*w.T[0]+w.alpha[1]*w.T[1],w.rho,w.u,w.c,ms[0],ms[1],ms[2],w.alpha[0]);
        }
    }
    rb_ft_book b=rb_ft_inventory(&s,0,n),local=rb_ft_inventory(&s,0,partition),l=s.slot[0].transfer,r=s.slot[n].transfer,m=s.slot[partition].transfer;
    double local_heat=0;for(int i=0;i<partition;i++)local_heat+=s.slot[i].wall_heat;
    double Eb=b.energy-initial.energy-l.energy+r.energy-s.gas_heat;
    double Etotal=b.energy+s.wall_capacity*s.wall_T-initial.energy-oldwall-l.energy+r.energy-s.ambient_heat;
    fprintf(stderr,"RB_FT_RESULT {\"status\":%d,\"steps\":%llu,\"time_s\":%.17g,\"cpu_s\":%.17g,\"timing_clock\":\"%s\",\"cells\":%d,\"bytes\":%zu,\"max_cfl\":%.17g,\"recon_fallbacks\":%llu,\"c_left\":%.17g,\"c_right\":%.17g,\"rho_left\":%.17g,\"rho_right\":%.17g,\"energy_balance_J\":%.17g,\"total_energy_balance_J\":%.17g,\"momentum_balance\":%.17g,\"mass_balance\":[%.17g,%.17g],\"regional_energy_balance_J\":%.17g,\"regional_mass_balance\":[%.17g,%.17g],\"initial_E_J\":%.17g,\"initial_PE\":%.17g,\"maximum_PE\":%.17g,\"final_PE\":%.17g,\"initial_mass\":[%.17g,%.17g],\"final_mass\":[%.17g,%.17g],\"max_pressure_error\":%.17g,\"max_velocity_error\":%.17g,\"port_mass_min\":%.17g,\"port_mass_max\":%.17g,\"max_cold_kg\":%.17g,\"wall_T\":%.17g,\"gas_heat_J\":%.17g,\"left_mass\":[%.17g,%.17g],\"right_mass\":[%.17g,%.17g],\"left_energy_J\":%.17g,\"right_energy_J\":%.17g}\n",status,s.steps,time,elapsed,TIMING_CLOCK,n,sizeof s+((size_t)n+1)*sizeof *storage,s.max_cfl,s.recon_fallbacks,c[kind],c[1-kind],rho[kind],rho[1-kind],Eb,Etotal,b.momentum-initial.momentum-l.momentum+r.momentum,b.m[0]-initial.m[0]-l.m[0]+r.m[0],b.m[1]-initial.m[1]-l.m[1]+r.m[1],local.energy-regional.energy-l.energy+m.energy-local_heat,local.m[0]-regional.m[0]-l.m[0]+m.m[0],local.m[1]-regional.m[1]-l.m[1]+m.m[1],initial.energy,initialPE,maxPE,rb_ft_perturbation(&s,U),initial.m[0],initial.m[1],b.m[0],b.m[1],maxdp,maxdu,minmass,maxmass,maxcold,s.wall_T,s.gas_heat,l.m[0],l.m[1],r.m[0],r.m[1],l.energy,r.energy);
    free(storage);return 0;
}
