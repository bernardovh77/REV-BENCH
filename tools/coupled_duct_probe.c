#define _POSIX_C_SOURCE 200809L
#include "coupled_duct.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

static double number(const char *arg)
{ char *end; double v=strtod(arg,&end); return end==arg || *end ? NAN:v; }
static double seconds(void)
{ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
typedef struct { double pressure, incoming, outgoing, mass, energy; } sample;
static double perturbation_energy(const rb_cd_state *s)
{
    double value=0;
    for(int i=0;i<s->spec.cells;i++) {
        const double p=rb_cd_pressure(s,i)-s->spec.pressure, m=s->q[i].v[1];
        value+=p*p/(2*s->spec.rho*s->c*s->c)+m*m/(2*s->spec.rho);
    }
    return value*s->dx*s->spec.area;
}
int main(int argc, char **argv)
{
    if(argc!=7) { fprintf(stderr,"usage: coupled_probe CELLS CUT AMPLITUDE STEPS REFLECTION PREFIX|-\n");return 2; }
    double nn=number(argv[1]), cc=number(argv[2]), amp=number(argv[3]), tt=number(argv[4]), rr=number(argv[5]);
    if(!isfinite(nn) || !isfinite(cc) || !isfinite(tt) || nn!=floor(nn) || cc!=floor(cc) ||
       tt!=floor(tt) || nn<4 || nn>8192 || cc<1 || cc>nn || tt<1 || tt>1000000) return 2;
    int n=(int)nn, steps=(int)tt;
    rb_cd_spec p={.cells=n,.gas_cells=(int)cc,.rate=192000,.rho=1.2,
        .pressure=1.2*343*343/1.4,.gamma=1.4,.area=.003,.tag_fraction=.25,
        .length=n*343.0/192000,.reflection=rr};
    rb_cd_state s;
    const size_t count=rb_cd_workspace(n);
    rb_cd_vec *storage=calloc(count,sizeof *storage);
    sample *trace=strcmp(argv[6],"-") ? calloc(steps,sizeof *trace):NULL;
    if(!storage || (strcmp(argv[6],"-") && !trace)) return 3;
    int status=rb_cd_prepare(&s,&p,storage,count);
    if(!status) status=rb_cd_seed(&s,amp,.15,.04);
    if(status) { fprintf(stderr,"prepare/seed rejected %d\n",status);free(storage);free(trace);return 2; }
    int probe=(int)floor(.15*n);
    const double initial_energy=perturbation_energy(&s);
    double peak_energy=initial_energy;
    const double start=seconds();
    for(int t=0;t<steps && !status;t++) {
        status=rb_cd_step(&s);
        if(trace) trace[t]=(sample){rb_cd_pressure(&s,probe)-p.pressure,
            s.current_return_Pa,s.accepted_outgoing_Pa,s.last_interface.v[0],s.last_interface.v[2]};
        if(trace && t%32==0) peak_energy=fmax(peak_energy,perturbation_energy(&s));
    }
    const double elapsed=seconds()-start;
    if(trace && !status) {
        char name[4096];
        if(snprintf(name,sizeof name,"%s.field.csv",argv[6])>=(int)sizeof name) return 2;
        FILE *f=fopen(name,"w"); if(!f) return 3;
        fprintf(f,"x_m,rho_kg_m3,momentum_kg_m2_s,energy_J_m3,tag_kg_m3,pressure_Pa\n");
        for(int i=0;i<n;i++) fprintf(f,"%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n",
            (i+.5)*s.dx,s.q[i].v[0],s.q[i].v[1],s.q[i].v[2],s.q[i].v[3],rb_cd_pressure(&s,i));
        fclose(f);
        if(snprintf(name,sizeof name,"%s.trace.csv",argv[6])>=(int)sizeof name) return 2;
        f=fopen(name,"w");if(!f) return 3;
        fprintf(f,"time_s,gas_probe_Pa,return_characteristic_Pa,outgoing_characteristic_Pa,accepted_mass_kg,accepted_energy_J\n");
        for(int i=0;i<steps;i++) fprintf(f,"%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n",
            (i+1)/p.rate,trace[i].pressure,trace[i].incoming,trace[i].outgoing,trace[i].mass,trace[i].energy);
        fclose(f);
    }
    const rb_cd_vec a=rb_cd_balance(&s,0), b=rb_cd_balance(&s,1);
    printf("{\"status\":%d,\"cells\":%d,\"cut\":%d,\"amplitude\":%.17g,\"steps\":%llu,"
        "\"rate_Hz\":%.17g,\"length_m\":%.17g,\"reflection\":%.17g,\"seconds\":%.17g,"
        "\"timing_has_trace\":%s,\"failures\":%llu,\"failed_stage\":%d,"
        "\"maximum_pressure_fraction\":%.17g,\"residual_cfl\":%.17g,"
        "\"left_balance\":[%.17g,%.17g,%.17g,%.17g],\"right_balance\":[%.17g,%.17g,%.17g,%.17g],"
        "\"accepted_mass_kg\":%.17g,\"accepted_energy_J\":%.17g,"
        "\"initial_perturbation_J\":%.17g,\"peak_perturbation_J\":%.17g,\"final_perturbation_J\":%.17g}\n",
        status,n,p.gas_cells,amp,s.steps,p.rate,p.length,rr,elapsed,trace ? "true":"false",
        s.failures,s.failed_stage,s.max_pressure_fraction,s.max_residual_cfl,
        a.v[0],a.v[1],a.v[2],a.v[3],b.v[0],b.v[1],b.v[2],b.v[3],
        s.interface_transfer.v[0],s.interface_transfer.v[2],initial_energy,peak_energy,perturbation_energy(&s));
    free(storage);free(trace);return status ? 1:0;
}
