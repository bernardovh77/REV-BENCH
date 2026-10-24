/* Stage040 native scenario/energy proof; outputs only after each full run. */
#include "engine.h"
#include "ignition.h"
#include "presets.h"
#include "rb.h"
#include <math.h>
#include <stdio.h>
static rb_engine e;
static rb_ignition_trace trace;
static rb_ignition_event events[RB_IGN_MAX_RECORDS];
static double energy(void) {return e.energy_in-e.energy_out+e.work_on_gas+e.wall_heat+rb_engine_pipe_heat(&e)+e.heat_released+e.fuel_energy_in;}
static double crank(void) {return e.crank_gas_work-e.crank_case_work+e.starter_work-e.friction_work-e.load_work+e.stop_energy+e.hold_work;}
int main(void)
{
    static const char *const names[]={"startup","held-idle","cut-resume","overrun","retained-fuel"};
    int failed=0;
    for(int preset=0;preset<5;preset++) for(int scenario=0;scenario<5;scenario++) {
        rb_engine_desc d;if(rb_preset_desc(preset,&d)) return 2;
        if(scenario==2 || scenario==3) d.inertia=50;
        if(scenario==2) {d.rev_limit=2000;d.rev_limit_resume=1900;}
        if(scenario==4) d.combustion_efficiency=.3;
        if(rb_engine_init_at(&e,&d,1.0/192000,48000)) return 2;
        rb_ignition_trace_init(&trace,events,RB_IGN_MAX_RECORDS);e.ignition_trace=&trace;
        e.ignition=1;e.variation=1;e.throttle_pos=.6;e.rpm=3000;
        int ticks=76800;
        if(scenario==0) {e.hold_speed=0;e.starter=1;e.rpm=0;e.throttle_pos=.05;ticks=153600;}
        if(scenario==1) {e.rpm=d.idle_rpm;e.throttle_pos=.035;}
        if(scenario==2 || scenario==3) {
            e.hold_speed=0;e.omega=(scenario==2?3000:4000)*3.14159265358979323846/30;
            e.kinetic=.5*d.inertia*e.omega*e.omega;e.overrun_transition=0;
        }
        double m0,U0,s0[RB_NSPECIES],m,U,s[RB_NSPECIES];
        rb_engine_contents(&e,&m0,&U0,s0);const double K0=e.kinetic;
        unsigned early=0,cut_steps=0,unknown_heat=0;
        double cut_fuel=0;
        rb_ignition_event sample[12];size_t samples=0;
        for(int k=0;k<ticks;k++) {
            if(k==ticks/2 && scenario==2) {
                rb_engine_contents(&e,&m,&U,s);cut_fuel=s[RB_FUEL];
                e.desc.rev_limit=7000;e.desc.rev_limit_resume=6900;
            }
            if(k==ticks/2 && scenario==3) e.throttle_pos=0;
            if(scenario==0 && e.rpm>d.cranking_rpm+300) e.starter=0;
            trace.count=0;rb_engine_step(&e);cut_steps+=e.fuel_cut!=0;
            for(int i=0;i<d.cylinders;i++) if(e.burn[i].heat>1e-12 && (!e.burn[i].discharged || !e.burn[i].kernel_started)) early++;
            for(size_t j=0;j<trace.count;j++) {
                const rb_ignition_event *v=&events[j];
                if(v->cylinder==0 && samples<12) sample[samples++]=*v;
                if(v->fraction<0 || v->fraction>1+1e-12 || v->kernel_delay_deg<0) unknown_heat++;
            }
        }
        rb_engine_contents(&e,&m,&U,s);
        const double dm=m-m0-e.mass_in+e.mass_out-e.fuel_in;
        const double dU=U-U0-energy();
        const double escale=fmax(1,fabs(e.energy_in)+fabs(e.energy_out)+fabs(e.work_on_gas)+fabs(e.heat_released));
        const double made[]={-RB_AFR_STOICH*e.fuel_burned,e.fuel_in-e.fuel_burned,(1+RB_AFR_STOICH)*e.fuel_burned};
        double ds=0;for(int j=0;j<RB_NSPECIES;j++) ds=fmax(ds,fabs(s[j]-s0[j]-e.species_in[j]+e.species_out[j]-made[j]));
        const double dK=e.kinetic-K0-crank();
        const double kscale=fmax(1,fabs(e.crank_gas_work)+fabs(e.crank_case_work)+fabs(e.starter_work)+fabs(e.friction_work)+fabs(e.load_work)+fabs(e.hold_work));
        int ok=fabs(dm)<2e-11*m0 && ds<2e-11*m0 && fabs(dU)<2e-9*escale && fabs(dK)<1e-9*kscale && !early && !unknown_heat && !trace.dropped && trace.totals[RB_IGN_DISCHARGE]>0;
        if(scenario==2) ok=ok && cut_fuel>0 && trace.totals[RB_IGN_HARD_CUT]>0 && trace.totals[RB_IGN_KERNEL]>0;
        if(scenario==3) ok=ok && cut_steps>0;
        if(scenario==4) ok=ok && s[RB_FUEL]>0 && e.fuel_burned<e.fuel_in;
        printf("{\"preset\":\"%s\",\"scenario\":\"%s\",\"ok\":%s,\"ticks\":%d,\"early_heat_steps\":%u,\"invalid_events\":%u,\"mass_error_kg\":%.17g,\"species_max_error_kg\":%.17g,\"energy_error_J\":%.17g,\"crank_error_J\":%.17g,\"mass_scale_kg\":%.17g,\"energy_scale_J\":%.17g,\"crank_scale_J\":%.17g,\"fuel_cut_steps\":%u,\"fuel_at_cut_end_kg\":%.17g,\"remaining_fuel_kg\":%.17g,\"heat_J\":%.17g,\"events\":{",rb_preset_key(preset),names[scenario],ok?"true":"false",ticks,early,unknown_heat,dm,ds,dU,dK,m0,escale,kscale,cut_steps,cut_fuel,s[RB_FUEL],e.heat_released);
        for(int j=0;j<RB_IGN_KINDS;j++) printf("%s\"%s\":%llu",j?",":"",rb_ignition_kind_name(j),(unsigned long long)trace.totals[j]);
        printf("},\"final_rpm\":%.17g,\"sampled_events\":[",e.rpm);
        for(size_t j=0;j<samples;j++) {
            const rb_ignition_event *v=&sample[j];
            printf("%s{\"kind\":\"%s\",\"cylinder\":%d,\"charge\":%llu,\"tick\":%llu,\"fraction\":%.17g,\"phase_deg\":%.17g,\"raw_delay_deg\":%.17g,\"delay_deg\":%.17g,\"fuel_kg\":%.17g}",j?",":"",rb_ignition_kind_name(v->kind),v->cylinder,(unsigned long long)v->charge,(unsigned long long)v->tick,v->fraction,v->angle_deg,v->kernel_raw_delay_deg,v->kernel_delay_deg,v->fuel_kg);
        }
        puts("]}");if(!ok)failed++;
    }
    return failed?1:0;
}
