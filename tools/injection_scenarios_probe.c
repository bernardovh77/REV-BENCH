/* Stage042 native dose/clock/whole-engine proof; no I/O in the step loop. */
#define _POSIX_C_SOURCE 200809L
#include "engine.h"
#include "noise.h"
#include "ignition.h"
#include "diagnostics.h"
#include "presets.h"
#include "rb.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static rb_engine e; static rb_noise noise;
static rb_injection_trace injection;static rb_injection_event doses[RB_INJ_MAX_RECORDS];
static rb_ignition_trace ignition;static rb_ignition_event sparks[RB_IGN_MAX_RECORDS];
static rb_diagnostics diagnostics;static rb_diag_slot slots[RB_D_MAX_SLOTS];
static const char *const scenarios[]={"loaded","startup-restart","afr-toggle","dfco-resume","hard-cut-resume","soft-cut","between-ivo-ivc"};
static double now(void) {struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+1e-9*t.tv_nsec;}
static double wrap(double a){a=fmod(a,720);return a<0?a+720:a;}
int main(int argc,char **argv)
{
    int preset=0,scenario=0,tap=1;unsigned seed=17;
    for(int i=1;i<argc;i+=2) {
        if(i+1==argc)return 2;
        if(!strcmp(argv[i],"--preset"))preset=rb_preset_find(argv[i+1]);
        else if(!strcmp(argv[i],"--seed")) {char *end;unsigned long n=strtoul(argv[i+1],&end,10);if(*end||!n||n>4294967295UL)return 2;seed=(unsigned)n;}
        else if(!strcmp(argv[i],"--scenario")) {scenario=-1;for(int k=0;k<7;k++)if(!strcmp(argv[i+1],scenarios[k]))scenario=k;}
        else if(!strcmp(argv[i],"--trace")) {if(strcmp(argv[i+1],"0")&&strcmp(argv[i+1],"1"))return 2;tap=atoi(argv[i+1]);}
        else return 2;
    }
    if(preset<0||scenario<0)return 2;
    rb_engine_desc d;if(rb_preset_desc(preset,&d))return 2;
    if(scenario>=3&&scenario<=5)d.inertia=50;
    if(scenario==4){d.rev_limit=2000;d.rev_limit_resume=1900;}
    if(scenario==5){d.rev_limit=3075;d.rev_limit_resume=2900;}
    if(rb_engine_init_at(&e,&d,1./192000,48000)||rb_engine_seed(&e,seed))return 2;
    rb_injection_trace_init(&injection,doses,RB_INJ_MAX_RECORDS);
    rb_ignition_trace_init(&ignition,sparks,RB_IGN_MAX_RECORDS);
    e.injection_trace=tap?&injection:NULL;e.ignition_trace=&ignition;
    if(rb_diag_init(&diagnostics,slots,RB_D_MAX_SLOTS,NULL,0,1)||rb_diag_bind_engine(&e,&diagnostics))return 2;
    e.rpm=3000;e.ignition=1;e.variation=1;e.throttle_pos=.6;
    if(scenario==1){e.hold_speed=0;e.starter=1;e.rpm=0;e.throttle_pos=.05;}
    if(scenario>=3&&scenario<=5){
        e.hold_speed=0;e.omega=(scenario==3?4000:3000)*3.14159265358979323846/30;
        e.kinetic=.5*d.inertia*e.omega*e.omega;e.overrun_transition=.03;
    }
    rb_noise_init(&noise,&e,11);
    double m0,U0,s0[RB_NSPECIES],mass,U,s[RB_NSPECIES];rb_engine_contents(&e,&m0,&U0,s0);
    const double K0=e.kinetic,seconds=scenario==1?.8:.6;
    const int ticks=(int)(seconds/e.dt);uint64_t attempts=0,positive=0,bad=0,missing=0,phantom=0,early=0;
    uint64_t kinds[RB_INJ_KINDS]={0},cut_steps=0,cut_clicks=0,restarts=0;double max_dm=0,max_du=0,total_dose=0,total_sensible=0;
    int was_cut=0;const double begin=now();
    for(int k=0;k<ticks;k++) {
        const double t=k*e.dt;
        if(scenario==1) {e.ignition=!(t>=.3&&t<.4);e.starter=e.ignition && e.rpm<d.cranking_rpm+300;}
        if(scenario==2)e.afr=t>=.2&&t<.4?0:RB_AFR_STOICH;
        if(scenario==3)e.throttle_pos=t>=.15&&t<.4?0:.6;
        if(scenario==4&&t>=.3){e.desc.rev_limit=7000;e.desc.rev_limit_resume=6900;}
        if(scenario==6)e.ignition=((int)(t/.007)%2)==0;
        const double f0=e.fuel_in,u0=e.fuel_energy_in,theta=e.theta;const uint64_t tick=e.physics_tick;
        long previous=noise.impacts[RB_IMPACT_INJECTOR];double angle[RB_MAX_CYLINDERS];
        for(int i=0;i<d.cylinders;i++)angle[i]=rb_engine_cylinder_angle(&e,i);
        injection.count=ignition.count=0;rb_engine_step(&e);rb_noise_step(&noise,&e,RB_NOISE_ENGINE);
        const long clicks=noise.impacts[RB_IMPACT_INJECTOR]-previous;
        double dose=0,energy=0;int count=0;const double step=wrap(e.theta-theta),ivc=rb_valve_closes(&d.intake);
        for(int j=0;j<e.injection_count;j++) {
            const rb_injection_event *v=&e.injection[j];attempts++;
            if(v->cylinder<0||v->cylinder>=d.cylinders||v->kind<0||v->kind>=RB_INJ_KINDS){bad++;continue;}
            kinds[v->kind]++;dose+=v->fuel_kg;energy+=v->sensible_energy_J;
            if(v->fuel_kg>0)count++;
            if(v->tick!=tick||!(v->fraction>=0&&v->fraction<1)||v->angle_deg!=ivc||v->charge!=e.burn[v->cylinder].charge_sequence
                || !(step>0) || fabs(v->fraction-wrap(ivc-angle[v->cylinder])/step)>2e-8)bad++;
            if((v->kind==RB_INJ_DELIVERED)!=(v->fuel_kg>0)||!isfinite(v->fuel_kg+v->sensible_energy_J))bad++;
        }
        positive+=count;max_dm=fmax(max_dm,fabs((e.fuel_in-f0)-dose));max_du=fmax(max_du,fabs((e.fuel_energy_in-u0)-energy));
        total_dose+=dose;total_sensible+=energy;
        missing+=count>clicks;phantom+=clicks>count;
        if(e.fuel_cut){cut_steps++;cut_clicks+=(uint64_t)clicks;}
        if(was_cut&&!e.fuel_cut)restarts++;
        was_cut=e.fuel_cut;
        if(tap && injection.count!=(size_t)e.injection_count)bad++;
        for(int i=0;i<d.cylinders;i++)if(e.burn[i].heat>1e-12&&(!e.burn[i].discharged||!e.burn[i].kernel_started))early++;
    }
    const double elapsed=now()-begin;
    rb_engine_contents(&e,&mass,&U,s);
    const double dm=mass-m0-e.mass_in+e.mass_out-e.fuel_in;
    const double energy=e.energy_in-e.energy_out+e.work_on_gas+e.wall_heat+rb_engine_pipe_heat(&e)+e.heat_released+e.fuel_energy_in;
    const double du=U-U0-energy,escale=fmax(1,fabs(e.energy_in)+fabs(e.energy_out)+fabs(e.work_on_gas)+fabs(e.heat_released));
    const double crank=e.crank_gas_work-e.crank_case_work+e.starter_work-e.friction_work-e.load_work+e.stop_energy+e.hold_work;
    const double dk=e.kinetic-K0-crank,kscale=fmax(1,fabs(e.crank_gas_work)+fabs(e.crank_case_work)+fabs(e.starter_work)+fabs(e.friction_work)+fabs(e.load_work)+fabs(e.hold_work));
    const double made[]={-RB_AFR_STOICH*e.fuel_burned,e.fuel_in-e.fuel_burned,(1+RB_AFR_STOICH)*e.fuel_burned};
    double ds=0;for(int i=0;i<RB_NSPECIES;i++)ds=fmax(ds,fabs(s[i]-s0[i]-e.species_in[i]+e.species_out[i]-made[i]));
    uint64_t reasons[RB_D_REASONS]={0},invalid=0;
    for(size_t i=0;i<diagnostics.count;i++){invalid+=slots[i].invalid;for(int j=0;j<RB_D_REASONS;j++)reasons[j]+=slots[i].reason[j];}
    int ok=positive>0 && !bad && !missing && !phantom && !early && !injection.dropped && !ignition.dropped
        && max_dm<=1e-15 && max_du<=1e-9 && fabs(dm)<2e-11*m0 && ds<2e-11*m0 && fabs(du)<2e-9*escale && fabs(dk)<1e-9*kscale;
    if(scenario==1||scenario==6)ok=ok&&kinds[RB_INJ_OFF]>0;
    if(scenario==2)ok=ok&&kinds[RB_INJ_AFR_DISABLED]>0;
    if(scenario==3)ok=ok&&kinds[RB_INJ_DFCO]>0&&cut_steps>0&&cut_clicks==0&&restarts>0;
    if(scenario==4)ok=ok&&ignition.totals[RB_IGN_HARD_CUT]>0&&ignition.totals[RB_IGN_DISCHARGE]>0;
    if(scenario==5)ok=ok&&ignition.totals[RB_IGN_SOFT_CUT]>0&&ignition.totals[RB_IGN_DISCHARGE]>0;
    printf("{\"preset\":\"%s\",\"scenario\":\"%s\",\"seed\":%u,\"trace_enabled\":%s,\"ok\":%s,\"seconds\":%.17g,\"elapsed_s\":%.17g,\"attempts\":%llu,\"positive_doses\":%llu,\"clicks\":%ld,\"invalid_events\":%llu,\"missing_click_intervals\":%llu,\"phantom_click_intervals\":%llu,\"early_heat_steps\":%llu,\"dropped_events\":%llu,\"step_mass_error_kg\":%.17g,\"step_energy_error_J\":%.17g,\"event_fuel_kg\":%.17g,\"event_sensible_J\":%.17g,\"fuel_in_kg\":%.17g,\"fuel_sensible_J\":%.17g,\"heard_fuel_kg\":%.17g,\"mass_error_kg\":%.17g,\"species_error_kg\":%.17g,\"energy_error_J\":%.17g,\"crank_error_J\":%.17g,\"mass_scale_kg\":%.17g,\"energy_scale_J\":%.17g,\"crank_scale_J\":%.17g,\"fuel_cut_steps\":%llu,\"fuel_cut_clicks\":%llu,\"cut_resumes\":%llu,\"diagnostic_invalid\":%llu,\"kinds\":{",rb_preset_key(preset),scenarios[scenario],seed,tap?"true":"false",ok?"true":"false",seconds,elapsed,(unsigned long long)attempts,(unsigned long long)positive,noise.impacts[RB_IMPACT_INJECTOR],(unsigned long long)bad,(unsigned long long)missing,(unsigned long long)phantom,(unsigned long long)early,(unsigned long long)(injection.dropped+ignition.dropped),max_dm,max_du,total_dose,total_sensible,e.fuel_in,e.fuel_energy_in,noise.injected_fuel_kg,dm,ds,du,dk,m0,escale,kscale,(unsigned long long)cut_steps,(unsigned long long)cut_clicks,(unsigned long long)restarts,(unsigned long long)invalid);
    for(int i=0;i<RB_INJ_KINDS;i++)printf("%s\"%s\":%llu",i?",":"",rb_injection_kind_name(i),(unsigned long long)kinds[i]);
    printf("},\"spark_events\":{");for(int i=0;i<RB_IGN_KINDS;i++)printf("%s\"%s\":%llu",i?",":"",rb_ignition_kind_name(i),(unsigned long long)ignition.totals[i]);
    printf("},\"guard_counts\":{");for(int i=0;i<RB_D_REASONS;i++)printf("%s\"%s\":%llu",i?",":"",rb_diag_reason_name(i),(unsigned long long)reasons[i]);
    puts("}}");return ok?0:1;
}
