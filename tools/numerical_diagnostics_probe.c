/* Stage 038 native observation/cost proof. All I/O is after stepping. */
#include "diagnostics.h"
#include "engine.h"
#include "presets.h"
#include "rb.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static rb_engine e;
static rb_diag_slot slots[RB_D_MAX_SLOTS];
static rb_diag_record trace[RB_D_MAX_TRACE];
static rb_diagnostics d;
static double energy(void) {
    return e.energy_in-e.energy_out+e.work_on_gas+e.wall_heat+rb_engine_pipe_heat(&e)
        +e.heat_released+e.fuel_energy_in;
}
static void array(const double *v,int n) {
    putchar('[');for(int j=0;j<n;j++) printf("%s%.17g",j?",":"",v[j]);putchar(']');
}
int main(int argc,char **argv) {
    if(argc!=5) return 2;
    int preset=atoi(argv[1]),mode=atoi(argv[2]),ticks=atoi(argv[3]),dump=atoi(argv[4]);
    rb_engine_desc desc;
    if(ticks<4 || ticks>1920000 || mode<0 || mode>2 || rb_preset_desc(preset,&desc)) return 2;
    if(rb_engine_init_at(&e,&desc,1.0/192000,48000)) return 2;
    if(mode && (rb_diag_init(&d,slots,RB_D_MAX_SLOTS,trace,mode==2?RB_D_MAX_TRACE:0,1)
                || rb_diag_bind_engine(&e,&d))) return 3;
    if(mode==2) d.trace_stride=1009;
    e.rpm=3000;e.throttle_pos=.7;e.ignition=1;
    ticks-=ticks%e.pipe_every;
    double m0,U0,s0[RB_NSPECIES],m,U,s[RB_NSPECIES];
    rb_engine_contents(&e,&m0,&U0,s0);
    const clock_t t=clock();
    for(int k=0;k<ticks;k++) {
        /* Held crank with abrupt lift for the second half: gas-path stress,
         * not a free-crank/ECU or full warm-engine operating map. */
        if(k==ticks/2) e.throttle_pos=.01;
        rb_engine_step(&e);
    }
    const double cpu=(double)(clock()-t)/CLOCKS_PER_SEC;
    rb_engine_contents(&e,&m,&U,s);
    double species_error[RB_NSPECIES];
    const double made[]={-RB_AFR_STOICH*e.fuel_burned,e.fuel_in-e.fuel_burned,
                         (1+RB_AFR_STOICH)*e.fuel_burned};
    for(int j=0;j<RB_NSPECIES;j++) species_error[j]=s[j]-s0[j]-e.species_in[j]+e.species_out[j]-made[j];
    double tap_mass=0,tap_energy=0,tap_species[RB_NSPECIES]={0};
    uint64_t reasons[RB_D_REASONS]={0},invalid=0;
    for(size_t i=0;i<d.count;i++) {
        rb_diag_slot *q=&slots[i];
        for(int j=0;j<RB_D_REASONS;j++) reasons[j]+=q->reason[j];
        invalid+=q->invalid;
        int sign=(q->kind==RB_D_FACE && q->role==RB_D_SNORKEL && q->part==-1)?1:
                 (q->kind==RB_D_FACE && q->role==RB_D_TAILPIPE && q->part==-2)?-1:0;
        tap_mass+=sign*q->accepted[0];tap_energy+=sign*q->accepted[RB_D_QUANTITIES-1];
        for(int j=0;j<RB_NSPECIES;j++) tap_species[j]+=sign*q->accepted[j+1];
    }
    printf("{\"preset\":\"%s\",\"mode\":%d,\"ticks\":%d,\"cpu_s\":%.9g,\"slots\":%zu,\"trace_records\":%zu,\"dropped\":%llu,\"invalid\":%llu,",
        rb_preset_key(preset),mode,ticks,cpu,d.count,d.trace_count,(unsigned long long)d.dropped,(unsigned long long)invalid);
    printf("\"generation\":%llu,",(unsigned long long)d.generation);
    printf("\"mass_error_kg\":%.17g,\"energy_error_J\":%.17g,\"energy_scale_J\":%.17g,\"species_error_kg\":",m-m0-e.mass_in+e.mass_out-e.fuel_in,U-U0-energy(),fabs(e.heat_released)+fabs(e.energy_in)+fabs(e.energy_out));array(species_error,RB_NSPECIES);
    printf(",\"mass_scale_kg\":%.17g,\"tap_boundary_mass_error_kg\":%.17g,\"tap_boundary_energy_error_J\":%.17g,\"tap_boundary_species_error_kg\":[",m0,tap_mass-e.mass_in+e.mass_out,tap_energy-e.energy_in+e.energy_out-e.acoustic_energy_out);
    for(int j=0;j<RB_NSPECIES;j++) printf("%s%.17g",j?",":"",tap_species[j]-e.species_in[j]+e.species_out[j]);
    printf("],\"state\":[%.17g,%.17g,%.17g,%.17g,%.17g],\"reasons\":{",e.theta,m,U,e.heat_released,e.junction.U);
    for(int j=0;j<RB_D_REASONS;j++) printf("%s\"%s\":%llu",j?",":"",rb_diag_reason_name(j),(unsigned long long)reasons[j]);
    printf("},\"memory_bytes\":{\"context\":%zu,\"slots\":%zu,\"trace\":%zu},\"components\":[",sizeof d,d.count*sizeof slots[0],d.trace_count*sizeof trace[0]);
    if(dump) for(size_t i=0;i<d.count;i++) {
        rb_diag_slot *q=&slots[i];
        printf("%s{\"role\":\"%s\",\"number\":%d,\"kind\":%d,\"part\":%d,\"requested\":",i?",":"",rb_diag_role_name(q->role),q->number,q->kind,q->part);
        array(q->requested,RB_D_QUANTITIES);printf(",\"accepted\":");array(q->accepted,RB_D_QUANTITIES);
        printf(",\"rejected_abs\":");array(q->rejected_abs,RB_D_QUANTITIES);printf(",\"repaired\":");array(q->repaired,RB_D_QUANTITIES);
        printf(",\"inventory\":");array(q->inventory,RB_D_QUANTITIES);
        printf(",\"pressure_Pa\":%.17g,\"temperature_K\":%.17g,\"invalid\":%llu,",q->pressure,q->temperature,(unsigned long long)q->invalid);
        printf("\"impulse_requested_Ns\":%.17g,\"impulse_accepted_Ns\":%.17g,\"length_m\":%.17g,\"area_m2\":%.17g,\"dx_m\":%.17g,\"effective_length_max_m\":%.17g,\"max_cfl\":%.17g,\"requested_cells\":%d,\"actual_cells\":%d,\"reasons\":[",q->impulse_requested,q->impulse_accepted,q->length,q->area,q->dx,q->effective_length_max,q->max_cfl,q->requested_cells,q->actual_cells);
        for(int j=0;j<RB_D_REASONS;j++) printf("%s%llu",j?",":"",(unsigned long long)q->reason[j]);
        printf("]}");
    }
    printf("],\"trace\":[");
    if(dump) for(size_t i=0;i<d.trace_count;i++) {
        const rb_diag_record *r=&trace[i];
        printf("%s{\"tick\":%llu,\"slot\":%zu,\"reason_bits\":%llu,\"requested\":",i?",":"",(unsigned long long)r->tick,r->slot,(unsigned long long)r->reasons);
        array(r->requested,RB_D_QUANTITIES);printf(",\"accepted\":");array(r->accepted,RB_D_QUANTITIES);printf("}");
    }
    puts("]}");
    return 0;
}
