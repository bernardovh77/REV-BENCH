/* Stage042: independent pre-step inventory and acoustic delivery oracles. */
#include "engine.h"
#include "injection.h"
#include "noise.h"
#include "presets.h"
#include "rb_internal.h"
#include "rb_test.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846
static rb_engine a,b;
static rb_noise na,nb;
static rb_injection_trace trace;
static rb_injection_event records[RB_INJ_MAX_RECORDS];
static double wrap(double x) { x=fmod(x,720); return x<0 ? x+720:x; }

static void start(int preset,double rate)
{
    rb_engine_desc d; CHECK_EQ_INT(rb_preset_desc(preset,&d),0);
    CHECK_EQ_INT(rb_engine_init_at(&a,&d,1/rate,48000),0);
    a.ignition=1; a.rpm=3000; a.throttle_pos=.6; a.variation=1;
    CHECK_EQ_INT(rb_engine_seed(&a,17),0);
    CHECK_EQ_INT(rb_injection_trace_init(&trace,records,RB_INJ_MAX_RECORDS),0);
    a.injection_trace=&trace;
    rb_noise_init(&na,&a,11);
}

/* Counts and mass come from pre-step inventory and physical books, not the
 * event being tested. Crank crossing uses actual displacement, independently
 * of both the event list and the sound renderer's former phase prediction. */
static void observe(void)
{
    double angle[RB_MAX_CYLINDERS],fuel[RB_MAX_CYLINDERS],air[RB_MAX_CYLINDERS],T[RB_MAX_CYLINDERS];
    uint64_t charge[RB_MAX_CYLINDERS];
    const double f0=a.fuel_in,u0=a.fuel_energy_in,theta=a.theta;
    const uint64_t tick=a.physics_tick;
    const long clicks=na.impacts[RB_IMPACT_INJECTOR];
    const double heard=na.injected_fuel_kg;
    for(int i=0;i<a.desc.cylinders;i++) {
        angle[i]=rb_engine_cylinder_angle(&a,i);fuel[i]=a.cyl[i].gas.m[RB_FUEL];
        air[i]=a.cyl[i].gas.m[RB_AIR];T[i]=a.cyl[i].gas.T;charge[i]=a.burn[i].charge_sequence;
    }
    trace.count=0; rb_engine_step(&a); rb_noise_step(&na,&a,RB_NOISE_ENGINE);
    double sum=0,energy=0,expected=0; int positive=0,attempts=0;
    const double step=wrap(a.theta-theta),ivc=rb_valve_closes(&a.desc.intake);
    for(int i=0;i<a.desc.cylinders;i++) {
        const double ahead=wrap(ivc-angle[i]);
        if(!(ahead<step)) continue;
        attempts++;
        /* Independent declared 2% cylinder fuel-trim law, not event mass. */
        double target=a.afr>0 && isfinite(a.afr) ? air[i]/a.afr*(1+.02*a.variation*a.trim_fuel[i]):0;
        if(a.ignition && !a.fuel_cut && a.afr>0 && isfinite(a.afr))
            expected+=fmax(0,target-fuel[i]);
        int found=0;
        for(int j=0;j<a.injection_count;j++) if(a.injection[j].cylinder==i) {
            const rb_injection_event *v=&a.injection[j]; found++;
            CHECK(v->tick==tick && v->charge==charge[i]+1);
            CHECK_NEAR(v->fraction,ahead/step,2e-8);
            CHECK_NEAR(v->angle_deg,ivc,1e-12);
            CHECK_NEAR(v->retained_fuel_kg,fuel[i],1e-15);
            CHECK(v->fraction>=0 && v->fraction<1);
            CHECK_NEAR(v->sensible_energy_J,v->fuel_kg*rb_gas_u(&a.fuel_gas,T[i]),1e-9);
        }
        CHECK_EQ_INT(found,1);
    }
    CHECK_EQ_INT(a.injection_count,attempts);
    CHECK_EQ_INT(trace.count,(size_t)attempts);
    for(int j=0;j<a.injection_count;j++) {
        const rb_injection_event *v=&a.injection[j];
        CHECK(v->kind>=0 && v->kind<RB_INJ_KINDS);
        CHECK(v->fuel_kg>=0 && isfinite(v->fuel_kg));
        if(v->kind==RB_INJ_DELIVERED) { CHECK(v->fuel_kg>0);positive++; }
        else CHECK(v->fuel_kg==0 && v->sensible_energy_J==0);
        sum+=v->fuel_kg;energy+=v->sensible_energy_J;
    }
    CHECK_NEAR(a.fuel_in-f0,sum,1e-15);
    CHECK_NEAR(a.fuel_energy_in-u0,energy,1e-9);
    CHECK_NEAR(sum,expected,1e-15);
    CHECK_EQ_INT(na.impacts[RB_IMPACT_INJECTOR]-clicks,positive);
    CHECK_NEAR(na.injected_fuel_kg-heard,sum,1e-15);
}

void test_injection_clock_mass_and_sound(void)
{
    const double rates[]={96000,192000,384000};
    for(int p=0;p<5;p++) for(int r=0;r<3;r++) for(int v=0;v<2;v++) {
        start(p,rates[r]);a.variation=v;
        for(int k=0;k<(int)(.081*rates[r]);k++) observe();
        CHECK(trace.totals[RB_INJ_DELIVERED]>0);
        CHECK(trace.dropped==0 && na.impacts[RB_IMPACT_INJECTOR]>0);
        CHECK_NEAR(na.injected_fuel_kg,a.fuel_in,1e-15);
    }
}

static void pose_ivc(int p,double ahead,double fuel_ratio)
{
    start(p,192000);a.variation=0;
    const double ivc=rb_valve_closes(&a.desc.intake);
    a.theta=wrap(ivc-ahead+a.desc.firing[0]);
    const double afr=RB_AFR_STOICH/fuel_ratio;
    const double y[]={afr/(afr+1),1/(afr+1),0};
    rb_chamber_init(&a.cyl[0].gas,a.cyl[0].gas.V,122000,360,y);
    rb_noise_init(&na,&a,11);
}

void test_injection_retained_disabled_and_boundaries(void)
{
    for(int p=0;p<5;p++) {
        /* Exactly at the interval start and just inside the preceding end. */
        for(int k=0;k<2;k++) {
            pose_ivc(p,k ? .093749:0,.5);observe();
            CHECK_EQ_INT(a.injection_count,1);CHECK(a.injection[0].fuel_kg>0);
        }
        pose_ivc(p,.09375,.5);observe();CHECK_EQ_INT(a.injection_count,0);
        observe();CHECK_EQ_INT(a.injection_count,1);
        for(int scenario=0;scenario<7;scenario++) {
            pose_ivc(p,.04,scenario==0 ? 1.2:.5);
            if(scenario==1)a.ignition=0;
            if(scenario==2)a.afr=0;
            if(scenario==3)a.afr=NAN;
            if(scenario==4) {const double pure[]={0,1,0};rb_chamber_init(&a.cyl[0].gas,a.cyl[0].gas.V,122000,360,pure);}
            if(scenario==5) {
                rb_chamber *g=&a.cyl[0].gas;
                g->m[RB_FUEL]=g->m[RB_AIR]/a.afr-1e-14;rb_chamber_update(g);
            }
            if(scenario==6) {a.variation=1;a.trim_fuel[0]=NAN;}
            observe();CHECK_EQ_INT(a.injection_count,1);
            const int reasons[]={RB_INJ_RETAINED,RB_INJ_OFF,RB_INJ_AFR_DISABLED,RB_INJ_AFR_DISABLED,RB_INJ_NO_AIR,RB_INJ_DELIVERED,RB_INJ_INVALID};
            CHECK_EQ_INT(a.injection[0].kind,reasons[scenario]);
            CHECK_EQ_INT(na.impacts[RB_IMPACT_INJECTOR],scenario==5 ? 1:0);
        }
        /* No fuel request, but retained fuel can still burn after spark. */
        pose_ivc(p,.04,.8);a.afr=0;
        for(int k=0;k<6000;k++)observe();
        CHECK(a.heat_released>0 && a.fuel_in==0);
        CHECK_EQ_INT(na.impacts[RB_IMPACT_INJECTOR],0);
        /* A hard spark cut is not a fuel cut. Observe an independently
         * required positive dose while the ECU is suppressing ignition. */
        pose_ivc(p,.04,.5);a.hold_speed=0;a.desc.inertia=50;
        a.desc.rev_limit=2000;a.desc.rev_limit_resume=1900;
        a.omega=3000*PI/30;a.kinetic=.5*a.desc.inertia*a.omega*a.omega;
        observe();CHECK(a.spark_cut && !a.fuel_cut);
        CHECK(a.injection[0].fuel_kg>0 && na.impacts[RB_IMPACT_INJECTOR]==1);
    }
}

void test_injection_consumption_and_no_replay(void)
{
    for(int p=0;p<5;p++) {
        pose_ivc(p,.04,.5);observe();long count=na.impacts[RB_IMPACT_INJECTOR];CHECK_EQ_INT(count,1);
        for(int k=0;k<4;k++)rb_noise_step(&na,&a,RB_NOISE_ENGINE);
        CHECK_EQ_INT(na.impacts[RB_IMPACT_INJECTOR],count);
        rb_noise_init(&nb,&a,22);rb_noise_step(&nb,&a,RB_NOISE_ENGINE);
        CHECK_EQ_INT(nb.impacts[RB_IMPACT_INJECTOR],0);
        /* Later control reads cannot cancel a completed delivery or invent
         * one for an earlier disabled attempt. The source record owns it. */
        pose_ivc(p,.04,.5);rb_engine_step(&a);a.ignition=0;a.fuel_cut=1;
        rb_noise_step(&na,&a,RB_NOISE_ENGINE);CHECK_EQ_INT(na.impacts[RB_IMPACT_INJECTOR],1);
        pose_ivc(p,.04,.5);a.ignition=0;rb_engine_step(&a);a.ignition=1;
        rb_noise_step(&na,&a,RB_NOISE_ENGINE);CHECK_EQ_INT(na.impacts[RB_IMPACT_INJECTOR],0);
        pose_ivc(p,.04,.5);rb_engine_step(&a);CHECK(a.fuel_in>0);
        rb_noise_step(&na,&a,RB_NOISE_FLOW);rb_noise_step(&na,&a,RB_NOISE_ENGINE);
        CHECK_EQ_INT(na.impacts[RB_IMPACT_INJECTOR],0);
        /* A skipped newest interval cannot replay an older stored dose. */
        pose_ivc(p,.04,.5);rb_engine_step(&a);rb_engine_step(&a);
        rb_noise_step(&na,&a,RB_NOISE_ENGINE);CHECK_EQ_INT(na.impacts[RB_IMPACT_INJECTOR],0);
        /* Resume after many disabled steps; consume the fresh dose even
         * when the mechanical angle tracker has skipped a large interval. */
        start(p,192000);
        for(int k=0;k<12000;k++){rb_engine_step(&a);rb_noise_step(&na,&a,0);}
        for(int k=0;k<2000;k++) {
            const double before=a.fuel_in;rb_engine_step(&a);
            if(a.fuel_in>before){rb_noise_step(&na,&a,RB_NOISE_ENGINE);break;}
            rb_noise_step(&na,&a,0);
        }
        CHECK_EQ_INT(na.impacts[RB_IMPACT_INJECTOR],1);
        pose_ivc(p,.04,.5);a.rpm=0;
        for(int k=0;k<10;k++)observe();
        CHECK(a.injection_count==0 && a.fuel_in==0 && na.impacts[RB_IMPACT_INJECTOR]==0);
    }
}

void test_injection_trace_capacity_and_transparency(void)
{
    start(2,192000);CHECK_EQ_INT(rb_engine_init_at(&b,&a.desc,a.dt,48000),0);
    b.ignition=a.ignition;b.rpm=a.rpm;b.throttle_pos=a.throttle_pos;b.variation=a.variation;
    rb_engine_seed(&b,17);rb_noise_init(&nb,&b,11);
    CHECK_EQ_INT(rb_injection_trace_init(&trace,records,1),0);
    for(int k=0;k<20000;k++) {
        rb_engine_step(&a);rb_engine_step(&b);
        rb_noise_step(&na,&a,RB_NOISE_ENGINE);rb_noise_step(&nb,&b,RB_NOISE_ENGINE);
        CHECK(a.theta==b.theta && a.fuel_in==b.fuel_in && a.heat_released==b.heat_released && a.rand_cycles==b.rand_cycles);
        for(int j=0;j<RB_NOISE_OUTPUTS;j++)CHECK(na.out[j]==nb.out[j]);
    }
    CHECK(trace.count==1 && trace.dropped>0);
    CHECK(na.impacts[RB_IMPACT_INJECTOR]==nb.impacts[RB_IMPACT_INJECTOR]);
    CHECK_NEAR(na.injected_fuel_kg,a.fuel_in,1e-15);
    for(int k=0;k<rb_engine_pipe_count(&a);k++) {
        const rb_pipe *x=rb_engine_pipe(&a,k),*y=rb_engine_pipe(&b,k);
        for(int j=0;j<x->cells;j++)CHECK(x->cell[j].U==y->cell[j].U && x->cell[j].p==y->cell[j].p);
    }
    CHECK_EQ_INT(rb_injection_trace_init(&trace,NULL,0),0);
    for(int k=0;k<10000;k++) {rb_engine_step(&a);rb_noise_step(&na,&a,RB_NOISE_ENGINE);}
    CHECK(trace.count==0 && trace.dropped>0 && trace.totals[RB_INJ_DELIVERED]>0);
    CHECK_NEAR(na.injected_fuel_kg,a.fuel_in,1e-15);
    CHECK_EQ_INT(rb_injection_trace_init(NULL,NULL,0),-1);
    CHECK_EQ_INT(rb_injection_trace_init(&trace,NULL,1),-1);
    CHECK_EQ_INT(rb_injection_trace_init(&trace,records,RB_INJ_MAX_RECORDS+1),-1);
    CHECK(strcmp(rb_injection_kind_name(-1),"unknown")==0);
}

void test_injection_actual_render_path(void)
{
    rb_config cfg={48000,4,48000};
    rb_sim *s=rb_create(&cfg);CHECK(s!=NULL);if(!s)return;
    s->source=RB_SOURCE_ENGINE;s->engine.ignition=1;s->engine.rpm=3000;s->engine.throttle_pos=.6;
    float audio[128];
    for(int k=0;k<40;k++)CHECK_EQ_INT(rb_render_block(s,audio,128),128);
    CHECK(s->noise.impacts[RB_IMPACT_INJECTOR]>0);
    CHECK_NEAR(s->noise.injected_fuel_kg,s->engine.fuel_in,1e-15);
    const long previous=s->noise.impacts[RB_IMPACT_INJECTOR];s->engine.ignition=0;
    for(int k=0;k<40;k++)CHECK_EQ_INT(rb_render_block(s,audio,128),128);
    CHECK_EQ_INT(s->noise.impacts[RB_IMPACT_INJECTOR],previous);
    rb_destroy(s);
}
