/* Stage041 native operating-map observer. No allocation or I/O while stepping.
 * Reports complete per-cylinder work windows; never infer RPM from audio. */
#include "combustion.h"
#include "diagnostics.h"
#include "engine.h"
#include "ignition.h"
#include "presets.h"
#include "rb.h"
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846
typedef struct stats {uint64_t n;double mean,m2,lo,hi;} stats;
static void add(stats *s,double x)
{
    if(!s->n)s->lo=s->hi=x;
    s->n++;const double delta=x-s->mean;s->mean+=delta/s->n;s->m2+=delta*(x-s->mean);
    s->lo=fmin(s->lo,x);s->hi=fmax(s->hi,x);
}
static void output_stats(const stats *s)
{
    if(!s->n){printf("null");return;}
    const double sd=sqrt(fmax(s->m2/s->n,0));
    printf("{\"n\":%" PRIu64 ",\"mean\":%.17g,\"sd\":%.17g,\"min\":%.17g,\"max\":%.17g,\"cov_pct\":",s->n,s->mean,sd,s->lo,s->hi);
    if(fabs(s->mean)>1e-9)printf("%.17g",100*sd/fabs(s->mean));else printf("null");
    putchar('}');
}
static int number(const char *s,double *v)
{
    char *end;errno=0;*v=strtod(s,&end);return !errno && end!=s && !*end && isfinite(*v);
}
static rb_engine e;
static rb_diag_slot slots[RB_D_MAX_SLOTS];
static rb_diagnostics diagnostics;
static rb_ignition_trace trace;
static rb_ignition_event events[RB_IGN_MAX_RECORDS];

int main(int argc,char **argv)
{
    int preset=-1,idle=0,model=RB_BURN_CHARGE,diag=1;
    double seconds=4,warmup=2,rpm=3000,throttle=1,variation=1,seed=17,rate=192000;
    for(int i=1;i<argc;i++) {
        if(i+1==argc)return 2;
        const char *key=argv[i],*value=argv[++i];
        if(!strcmp(key,"--preset")){preset=rb_preset_find(value);if(preset<0)return 2;}
        else if(!strcmp(key,"--mode")){if(strcmp(value,"idle") && strcmp(value,"held"))return 2;idle=!strcmp(value,"idle");}
        else if(!strcmp(key,"--burn-model")){if(strcmp(value,"charge") && strcmp(value,"capped"))return 2;model=!strcmp(value,"capped")?RB_BURN_CAPPED:RB_BURN_CHARGE;}
        else if(!strcmp(key,"--diagnostics")){if(strcmp(value,"0") && strcmp(value,"1"))return 2;diag=value[0]-'0';}
        else {
            double *p=!strcmp(key,"--seconds")?&seconds:!strcmp(key,"--warmup")?&warmup:
                !strcmp(key,"--rpm")?&rpm:!strcmp(key,"--throttle")?&throttle:
                !strcmp(key,"--variation")?&variation:!strcmp(key,"--seed")?&seed:
                !strcmp(key,"--rate")?&rate:NULL;
            if(!p || !number(value,p))return 2;
        }
    }
    if(preset<0 || !(seconds>0 && seconds<=120 && warmup>=0 && warmup<seconds)
       || !(rpm>=300 && rpm<=12000 && throttle>=0 && throttle<=1 && variation>=0 && variation<=4)
       || !(seed>=1 && seed<=4294967295.0 && seed==floor(seed))
       || (rate!=96000 && rate!=192000 && rate!=384000))return 2;
    rb_engine_desc d;if(rb_preset_desc(preset,&d) || rb_engine_init_at(&e,&d,1/rate,48000)
       || rb_engine_seed(&e,(uint64_t)seed) || rb_engine_combustion_model(&e,model))return 2;
    const uint64_t initial_cycles=e.rand_cycles,initial_flicker=e.rand_flicker;
    e.variation=variation;e.ignition=1;e.hold_speed=!idle;e.rpm=idle?0:rpm;
    e.throttle_pos=idle?0:throttle;e.starter=idle;
    rb_ignition_trace_init(&trace,events,RB_IGN_MAX_RECORDS);e.ignition_trace=&trace;
    if(diag && (rb_diag_init(&diagnostics,slots,RB_D_MAX_SLOTS,NULL,0,1)
                  || rb_diag_bind_engine(&e,&diagnostics)))return 2;
    const long ticks=lround(seconds*rate),warm=lround(warmup*rate);
    const double vd=rb_engine_displacement(&e)/d.cylinders;
    double m0,U0,s0[RB_NSPECIES],mass,U,species[RB_NSPECIES];
    rb_engine_contents(&e,&m0,&U0,s0);const double K0=e.kinetic;
    stats net[RB_MAX_CYLINDERS]={{0}},gross[RB_MAX_CYLINDERS]={{0}},pump[RB_MAX_CYLINDERS]={{0}};
    stats residual[RB_MAX_CYLINDERS]={{0}},phi[RB_MAX_CYLINDERS]={{0}},temperature[RB_MAX_CYLINDERS]={{0}},pressure[RB_MAX_CYLINDERS]={{0}};
    stats demand[RB_MAX_CYLINDERS]={{0}},delay[RB_MAX_CYLINDERS]={{0}},ca50[RB_MAX_CYLINDERS]={{0}},prediction[RB_MAX_CYLINDERS]={{0}};
    stats speed={0},smooth={0},torque={0};
    uint64_t flags[11]={0},warm_events[RB_IGN_KINDS]={0},charge[RB_MAX_CYLINDERS]={0};
    int warm_cycle[RB_MAX_CYLINDERS]={0},warm_charge[RB_MAX_CYLINDERS]={0};
    double heat[RB_MAX_CYLINDERS]={0},reverse_intake=0,reverse_exhaust=0,max_port_T=0;
    long stalled=0;uint64_t early=0,invalid_events=0;long cycle_start=-1;
    for(long k=0;k<ticks;k++) {
        if(idle && e.starter && (e.rpm_smooth>.75*d.idle_rpm || k>=lround(3*rate)))e.starter=0;
        double angle[RB_MAX_CYLINDERS];
        for(int i=0;i<d.cylinders;i++)angle[i]=rb_engine_cylinder_angle(&e,i);
        const long oldcycles=e.cycles;
        trace.count=0;rb_engine_step(&e);
        if(!isfinite(e.rpm+e.rpm_smooth+e.heat_released))return 3;
        if(k>=warm) {
            add(&speed,e.rpm);add(&smooth,e.rpm_smooth);
            stalled+=idle && e.rpm_smooth<=d.cranking_rpm;
        }
        if(e.cycles!=oldcycles) {
            if(cycle_start>=warm)add(&torque,e.result.brake_torque);
            cycle_start=k;
        }
        for(size_t j=0;j<trace.count;j++) {
            const rb_ignition_event *v=&events[j];rb_burn *b=&e.burn[v->cylinder];
            if(v->fraction<0 || v->fraction>1+1e-12 || v->kernel_delay_deg<0)invalid_events++;
            if(k>=warm)warm_events[v->kind]++;
            if(v->kind==RB_IGN_SCHEDULED && k>=warm) {
                const int i=v->cylinder;
                add(&residual[i],b->residual);add(&phi[i],b->phi_ivc);
                add(&temperature[i],b->T_ivc);add(&pressure[i],b->p_ivc);
                add(&demand[i],b->duration);add(&delay[i],b->kernel_delay_deg);
                add(&prediction[i],b->expected_ca50);
                for(int bit=0;bit<11;bit++)flags[bit]+=!!(b->flame_flags&(1u<<bit));
            }
        }
        for(int i=0;i<d.cylinders;i++) {
            const rb_burn *b=&e.burn[i];const double next=rb_engine_cylinder_angle(&e,i);
            if(angle[i]<=360 && next>360) {
                if(warm_cycle[i]) {
                    if(!isfinite(b->last_work+b->last_gross_work+b->last_pumping_work))return 3;
                    add(&net[i],b->last_work/vd);add(&gross[i],b->last_gross_work/vd);add(&pump[i],b->last_pumping_work/vd);
                }
                warm_cycle[i]=k>=warm;
            }
            if(charge[i]!=b->charge_sequence) {
                charge[i]=b->charge_sequence;heat[i]=0;warm_charge[i]=k>=warm;
            }
            if(b->heat>1e-12 && (!b->discharged || !b->kernel_started))early++;
            if(b->heat>0) {
                const double half=.5*b->burnable*rb_fuel_heat();
                if(heat[i]<half && heat[i]+b->heat>=half && warm_charge[i]) {
                    double span=next-angle[i];if(span<0)span+=720;
                    double a=angle[i]+span*(half-heat[i])/b->heat;
                    a=fmod(a+360,720);if(a<0)a+=720;
                    add(&ca50[i],a-360);
                }
                heat[i]+=b->heat;
            }
            if(k>=warm) {
                reverse_intake+=fmax(-e.intake[i].mdot,0)/rate;
                reverse_exhaust+=fmax(-e.exhaust[i].mdot,0)/rate;
                if(e.exhaust[i].mdot>0)max_port_T=fmax(max_port_T,e.cyl[i].gas.T);
            }
        }
    }
    rb_engine_contents(&e,&mass,&U,species);
    const double dm=mass-m0-e.mass_in+e.mass_out-e.fuel_in;
    const double energy=e.energy_in-e.energy_out+e.work_on_gas+e.wall_heat+rb_engine_pipe_heat(&e)+e.heat_released+e.fuel_energy_in;
    const double du=U-U0-energy;
    const double escale=fmax(1,fabs(e.energy_in)+fabs(e.energy_out)+fabs(e.work_on_gas)+fabs(e.heat_released));
    const double crank=e.crank_gas_work-e.crank_case_work+e.starter_work-e.friction_work-e.load_work+e.stop_energy+e.hold_work;
    const double dk=e.kinetic-K0-crank;
    const double kscale=fmax(1,fabs(e.crank_gas_work)+fabs(e.crank_case_work)+fabs(e.starter_work)+fabs(e.friction_work)+fabs(e.load_work)+fabs(e.hold_work));
    const double made[]={-RB_AFR_STOICH*e.fuel_burned,e.fuel_in-e.fuel_burned,(1+RB_AFR_STOICH)*e.fuel_burned};
    double ds=0;for(int i=0;i<RB_NSPECIES;i++)ds=fmax(ds,fabs(species[i]-s0[i]-e.species_in[i]+e.species_out[i]-made[i]));
    int ok=!early && !invalid_events && !trace.dropped && !e.starter && !stalled
        && fabs(dm)<2e-11*m0 && ds<2e-11*m0 && fabs(du)<2e-9*escale && fabs(dk)<1e-9*kscale;
    for(int i=0;i<d.cylinders;i++)ok=ok && net[i].n>=2 && residual[i].n>=2;
    uint64_t reasons[RB_D_REASONS]={0},invalid=0;
    if(diag)for(size_t i=0;i<diagnostics.count;i++) {
        invalid+=slots[i].invalid;
        for(int j=0;j<RB_D_REASONS;j++)reasons[j]+=slots[i].reason[j];
    }
    printf("{\"preset\":\"%s\",\"mode\":\"%s\",\"burn_model\":%d,\"seed\":%.0f,\"initial_cycle_state\":\"%016" PRIx64 "\",\"initial_flicker_state\":\"%016" PRIx64 "\",\"variation\":%.17g,\"physics_hz\":%.0f,\"pipe_hz\":48000,\"seconds\":%.17g,\"warmup_s\":%.17g,\"commanded_rpm\":%.17g,\"commanded_throttle\":%.17g,\"ok\":%s,\"warm_stalled_steps\":%ld,\"early_heat_steps\":%" PRIu64 ",\"invalid_events\":%" PRIu64 ",\"dropped_events\":%" PRIu64 ",\"mass_error_kg\":%.17g,\"species_error_kg\":%.17g,\"energy_error_J\":%.17g,\"crank_error_J\":%.17g,\"mass_scale_kg\":%.17g,\"energy_scale_J\":%.17g,\"crank_scale_J\":%.17g,\"fuel_in_kg\":%.17g,\"fuel_burned_kg\":%.17g,\"fuel_out_kg\":%.17g,\"reverse_intake_kg\":%.17g,\"reverse_exhaust_kg\":%.17g,\"port_temperature_screen_max_K\":%.17g,\"rpm\":",rb_preset_key(preset),idle?"idle":"held",model,seed,initial_cycles,initial_flicker,variation,rate,seconds,warmup,rpm,throttle,ok?"true":"false",stalled,early,invalid_events,trace.dropped,dm,ds,du,dk,m0,escale,kscale,e.fuel_in,e.fuel_burned,e.species_out[RB_FUEL],reverse_intake,reverse_exhaust,max_port_T);
    output_stats(&speed);printf(",\"smooth_rpm\":");output_stats(&smooth);printf(",\"brake_torque_Nm\":");output_stats(&torque);
    printf(",\"flags\":[");for(int i=0;i<11;i++)printf("%s%" PRIu64,i?",":"",flags[i]);
    printf("],\"warm_events\":{");for(int i=0;i<RB_IGN_KINDS;i++)printf("%s\"%s\":%" PRIu64,i?",":"",rb_ignition_kind_name(i),warm_events[i]);
    printf("},\"diagnostics_enabled\":%s,\"diagnostic_invalid\":%" PRIu64 ",\"guard_counts\":{",diag?"true":"false",invalid);
    for(int i=0;i<RB_D_REASONS;i++)printf("%s\"%s\":%" PRIu64,i?",":"",rb_diag_reason_name(i),reasons[i]);
    printf("},\"cylinders\":[");
    for(int i=0;i<d.cylinders;i++) {
        printf("%s{\"cylinder\":%d,\"net_imep_Pa\":",i?",":"",i);output_stats(&net[i]);
        printf(",\"gross_imep_Pa\":");output_stats(&gross[i]);printf(",\"pumping_mep_Pa\":");output_stats(&pump[i]);
        printf(",\"residual_fraction\":");output_stats(&residual[i]);printf(",\"phi\":");output_stats(&phi[i]);
        printf(",\"ivc_T_K\":");output_stats(&temperature[i]);printf(",\"ivc_p_Pa\":");output_stats(&pressure[i]);
        printf(",\"duration_deg\":");output_stats(&demand[i]);printf(",\"kernel_delay_deg\":");output_stats(&delay[i]);
        printf(",\"heat_ca50_deg\":");output_stats(&ca50[i]);printf(",\"expected_ca50_deg\":");output_stats(&prediction[i]);putchar('}');
    }
    puts("]}");return ok?0:1;
}
