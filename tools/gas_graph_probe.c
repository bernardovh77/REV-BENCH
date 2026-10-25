#define _POSIX_C_SOURCE 200809L
#include "gas_graph.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static double measured(void)
{
#ifdef __wasi__
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+1e-9*t.tv_nsec;
#else
    return (double)clock()/CLOCKS_PER_SEC;
#endif
}
static void vector(rb_mix_cell q)
{fputc('[',stderr);for(int j=0;j<5;j++)fprintf(stderr,"%s%.17g",j ? ",":"",q.v[j]);fputc(']',stderr);}
static int number(const char *s,double *value)
{char *end;*value=strtod(s,&end);return end!=s && !*end && isfinite(*value);}
int main(int argc,char **argv)
{
    if(argc!=12 && argc!=13)return 2;
    double a[10];
    for(int i=0;i<10;i++)if(!number(argv[i+2],&a[i]))return 2;
    const char *mode=argv[1];
    if(strcmp(mode,"profile") && strcmp(mode,"return") && strcmp(mode,"stability") &&
       strcmp(mode,"cost") && strcmp(mode,"contact") && strcmp(mode,"eos"))return 2;
    if((a[2]!=0 && a[2]!=1) || a[5]!=floor(a[5]) || a[5]<4 || a[5]>4096 ||
       a[8]<0 || a[8]>2 || a[8]*a[0]>1000000)return 2;
    rb_gg_spec p={.rate=a[0],.temperature=a[1],.length=a[3],.interface_x=a[4],
        .gas_cells=(int)a[5],.pressure=101325,.area=.003,.reflection=a[9],.y={1,0,0}};
    if(argc==13 && !number(argv[12],&p.area))return 2;
    if(a[2]){p.y[0]=.1;p.y[1]=.001;p.y[2]=.899;}
    rb_gas gas;rb_gas_init(&gas,p.y);
    if(!strcmp(mode,"eos")) {
        fprintf(stderr,"RB_GG_RESULT {\"kind\":\"eos\",\"R\":%.17g,\"lo\":[",gas.R);
        for(int i=0;i<5;i++)fprintf(stderr,"%s%.17g",i ? ",":"",gas.lo[i]);
        fprintf(stderr,"],\"hi\":[");for(int i=0;i<5;i++)fprintf(stderr,"%s%.17g",i ? ",":"",gas.hi[i]);
        fprintf(stderr,"],\"k_lo\":%.17g,\"k_hi\":%.17g}\n",gas.k_lo,gas.k_hi);return 0;
    }
    size_t count,samples;int status=rb_gg_sizes(&p,&count,&samples);if(status)return 3;
    rb_mix_cell *cells=calloc(count,sizeof *cells);double *delays=calloc(samples,sizeof *delays);rb_gg_state s;
    if(!cells || !delays || rb_gg_prepare(&s,&p,cells,count,delays,samples)){free(cells);free(delays);return 3;}
    const double center=p.length>=1 ? .1:.3*p.interface_x,width=p.length>=1 ? .035:.08*p.interface_x;
    status=rb_gg_seed(&s,a[6],center,width,a[7]);
    if(!status && !strcmp(mode,"contact")) {
        if(!a[2]){free(cells);free(delays);return 2;}
        rb_mix_cell *initial=calloc((size_t)s.cells,sizeof *initial);if(!initial)return 3;
        memcpy(initial,s.q,(size_t)s.cells*sizeof *initial);
        const double patch[]={.099,.001,.9};
        for(int i=0;i<s.cells;i++) {
            double x=rb_gg_cell_center(&s,i);
            if(x>p.interface_x-.02 && x<p.interface_x+.02)rb_mix_make(p.pressure,p.temperature,0,patch,&initial[i]);
        }
        status=rb_gg_load(&s,initial,s.cells);free(initial);
    }
    if(status) {
        fprintf(stderr,"RB_GG_RESULT {\"status\":%d,\"failures\":%llu,\"steps\":%llu,\"reason\":\"initial-domain\"}\n",status,s.failures,s.steps);
        free(cells);free(delays);return 0;
    }
    int steps=(int)ceil(a[8]*p.rate),probe=(int)floor((p.length>=1 ? .15:.2*p.interface_x)/s.gas_dx);
    if(probe>=p.gas_cells)probe=p.gas_cells-1;
    double initial_energy=rb_gg_perturbation_energy(&s),maximum=initial_energy,clock_error=0;
    if(!strcmp(mode,"return") || !strcmp(mode,"stability"))puts("tick,time_s,probe_pressure_Pa,received_Pa,used_Pa,perturbation_J");
    double start=measured();
    for(int tick=0;tick<steps;tick++) {
        status=rb_gg_step(&s);if(status)break;
        if(strcmp(mode,"cost"))clock_error=fmax(clock_error,fabs(s.received_return-s.used_return));
        if((!strcmp(mode,"stability") && tick%64==0) || !strcmp(mode,"return")) {
            const double energy=rb_gg_perturbation_energy(&s);maximum=fmax(maximum,energy);
            printf("%llu,%.17g,%.17g,%.17g,%.17g,%.17g\n",s.steps,s.steps/p.rate,rb_gg_pressure(&s,probe)-p.pressure,s.received_return,s.used_return,energy);
        }
    }
    const double elapsed=measured()-start;
    if(!status && (!strcmp(mode,"profile") || !strcmp(mode,"contact"))) {
        puts("x_m,dx_m,rho_kg_m3,u_m_s,p_Pa,T_K,air_kg_m3,fuel_kg_m3,burned_kg_m3,total_energy_J_m3");
        rb_gg_totals(&s,1);
        for(int i=0;i<s.cells;i++) {
            rb_mix_primitive w;if(rb_mix_decode(s.q[i],&w))return 4;
            printf("%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n",
                rb_gg_cell_center(&s,i),rb_gg_cell_width(&s,i),w.rho,w.u,w.p,w.T,s.q[i].v[0],s.q[i].v[1],s.q[i].v[2],s.q[i].v[4]);
        }
    }
    fprintf(stderr,"RB_GG_RESULT {\"status\":%d,\"steps\":%llu,\"failures\":%llu,\"domain_rejections\":%llu,\"c\":%.17g,\"rho\":%.17g,\"time_s\":%.17g,\"cells\":%d,\"bytes\":%zu,\"fraction\":%.17g,\"max_residual_cfl\":%.17g,\"clock_error\":%.17g,\"elapsed_s\":%.17g,\"initial_perturbation_J\":%.17g,\"coordinate_work_J\":%.17g,\"area_m2\":%.17g,\"linear_interior\":%llu,\"linear_boundary\":%llu,\"residual_fallbacks\":%llu",
        status,s.steps,s.failures,s.domain_rejections,s.c,s.rho,s.steps/p.rate,s.cells,sizeof s+count*sizeof *cells+samples*sizeof *delays,
        s.fraction,s.max_residual_cfl,clock_error,elapsed,initial_energy,s.coordinate_work,p.area,s.linear_smooth,s.linear_limited,s.residual_fallbacks);
    if(!status) {
        double final=rb_gg_perturbation_energy(&s);maximum=fmax(maximum,final);
        fprintf(stderr,",\"final_perturbation_J\":%.17g,\"maximum_perturbation_J\":%.17g,\"graph_work_residual_J\":%.17g,\"left_balance\":",final,maximum,rb_ac_balance(&s.graph)+s.coordinate_work);
        vector(rb_gg_balance(&s,0));fprintf(stderr,",\"right_balance\":");vector(rb_gg_balance(&s,1));
        fprintf(stderr,",\"interface_transfer\":");vector(s.interface_transfer);
        fprintf(stderr,",\"initial_left\":");vector(s.initial[0]);fprintf(stderr,",\"initial_right\":");vector(s.initial[1]);
    }
    fprintf(stderr,"}\n");free(cells);free(delays);return 0;
}
