#include "rb_test.h"
#include "diagnostics.h"
#include "engine.h"
#include "presets.h"
#include "fuzz.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static rb_engine a,b;
static rb_diagnostics diagnostic;
static rb_diag_slot slots[RB_D_MAX_SLOTS];
static rb_diag_record records[2];
static void start(int preset,size_t trace)
{
    rb_engine_desc d;
    CHECK_EQ_INT(rb_preset_desc(preset,&d),0);
    CHECK_EQ_INT(rb_engine_init_at(&a,&d,1.0/192000,48000),0);
    CHECK_EQ_INT(rb_diag_init(&diagnostic,slots,RB_D_MAX_SLOTS,records,trace,7),0);
    CHECK_EQ_INT(rb_diag_bind_engine(&a,&diagnostic),0);
}

void test_diagnostic_caps_and_reversal(void)
{
    start(2,2);
    rb_pipe *p=&a.exhaust_pipe[0];rb_face *f=&p->face[0];
    const double dt=1.0/48000, initial=f->a->mass;
    f->mdot=10;
    rb_pipe_clear(p,0);rb_pipe_flows(p,0,dt);
    CHECK_EQ_INT(f->diag->reason[RB_D_MASS_CAP],1);
    CHECK_NEAR(f->diag->accepted[0],.5*initial,1e-18);
    CHECK_NEAR(f->diag->requested[0],10*dt,1e-18);
    CHECK(f->diag->rejected_abs[0]>0);
    CHECK_NEAR(f->diag->accepted[1],f->diag->accepted[0]*f->moved_y[0],1e-18);
    CHECK_NEAR(f->diag->accepted[RB_D_QUANTITIES-1],f->diag->accepted[0]*f->moved_h,1e-14);
    f->mdot=-10;
    rb_pipe_clear(p,0);rb_pipe_flows(p,0,dt);
    CHECK_EQ_INT(f->diag->reason[RB_D_MASS_CAP],2);
    CHECK(f->moved<0);
    f->mdot=NAN;
    rb_pipe_flows(p,0,dt);
    CHECK_EQ_INT(f->diag->reason[RB_D_BAD_FLOW],1);
    CHECK(f->diag->invalid>0);
    CHECK(diagnostic.trace_count==2 && diagnostic.dropped>0);
    CHECK(records[0].slot<diagnostic.count && records[0].reasons);
}

void test_diagnostic_vacuum_geometry_and_sonic(void)
{
    start(2,0);
    rb_pipe *p=&a.exhaust_pipe[0];rb_face *f=&p->face[0];
    rb_chamber_init(f->a,f->a->V,1e-5,300,(double[]){1,0,0});
    rb_chamber_init(f->b,f->b->V,1e-5,300,(double[]){1,0,0});
    /* Reinitializing a chamber intentionally requires rebinding its tap. */
    CHECK_EQ_INT(rb_diag_bind_engine(&a,&diagnostic),0);
    p->dx=1e-5;f->length=1e-8;f->per_length=1e8;f->mdot=1e3;
    rb_pipe_accelerate(p,0,1.0/48000);
    CHECK(f->diag->reason[RB_D_DENSITY_FLOOR]>0);
    CHECK(f->diag->reason[RB_D_LENGTH_FLOOR]>0);
    CHECK(f->diag->reason[RB_D_SONIC_CAP]>0);
    CHECK(f->diag->effective_length_max>f->length);
    CHECK(p->diag->max_cfl>1);
    CHECK(f->diag->impulse_requested!=f->diag->impulse_accepted);
}

void test_diagnostic_junction_repair_and_fuzz(void)
{
    start(2,0);
    CHECK(a.desc.junction_volume>0 && a.junction.diag && a.junction.diag->role==RB_D_JUNCTION);
    a.junction.m[0]=-1e-6;
    rb_chamber_update(&a.junction);
    CHECK_EQ_INT(a.junction.diag->reason[RB_D_MASS_REPAIR],1);
    CHECK_NEAR(a.junction.diag->repaired[0],1e-6,1e-20);
    CHECK(a.junction.diag->inventories>0);
    rb_fuzz_result r={.ok=1};
    CHECK_EQ_INT(rb_fuzz_inspect_engine(&r,&a,0),0);
    CHECK(strstr(r.what,"junction")!=NULL);
    start(2,0);a.junction.p=NAN;
    memset(&r,0,sizeof r);r.ok=1;
    CHECK_EQ_INT(rb_fuzz_inspect_engine(&r,&a,0),0);
    CHECK(strstr(r.what,"junction")!=NULL);
}

void test_diagnostic_taps_preserve_native_state(void)
{
    for(int preset=0;preset<5;preset++) {
        start(preset,2);
        CHECK_EQ_INT(rb_engine_init_at(&b,&a.desc,1.0/192000,48000),0);
        a.ignition=b.ignition=1;a.rpm=b.rpm=3000;a.throttle_pos=b.throttle_pos=.7;
        diagnostic.trace_stride=17;
        for(int tick=0;tick<6000;tick++) {
            rb_engine_step(&a);rb_engine_step(&b);
            CHECK(a.theta==b.theta && a.heat_released==b.heat_released && a.rand_cycles==b.rand_cycles);
        }
        for(int k=0;k<rb_engine_pipe_count(&a);k++) {
            const rb_pipe *x=rb_engine_pipe(&a,k),*y=rb_engine_pipe(&b,k);
            for(int j=0;j<x->cells;j++) CHECK(x->cell[j].U==y->cell[j].U && x->cell[j].p==y->cell[j].p);
        }
        if(a.desc.junction_volume>0) CHECK(a.junction.U==b.junction.U && a.junction.diag->inventories>0);
    }
}

void test_diagnostic_pressure_impulse_and_capacity(void)
{
    start(2,2);
    rb_pipe *p=&a.exhaust_pipe[0];rb_face *f=&p->face[0];
    f->a->p=100100;f->b->p=100000;f->friction=0;
    const double dt=1e-7,expected=f->area*(f->a->p-f->b->p)*dt;
    rb_pipe_accelerate(p,0,dt);
    CHECK_NEAR(f->diag->impulse_requested,expected,1e-17);
    CHECK_NEAR(f->diag->impulse_accepted,expected,1e-17);
    CHECK_NEAR(f->mdot,expected/f->length,1e-17);
    rb_diagnostics too_small;
    CHECK_EQ_INT(rb_diag_init(&too_small,slots,1,NULL,0,8),0);
    CHECK_EQ_INT(rb_diag_bind_engine(&a,&too_small),-1);
    CHECK(a.diag==&diagnostic && f->diag->owner==&diagnostic);
    CHECK_EQ_INT(rb_diag_init(&too_small,slots,RB_D_MAX_SLOTS+1,NULL,0,8),-1);
    CHECK_EQ_INT(rb_diag_init(&too_small,slots,1,NULL,1,8),-1);
    CHECK_EQ_INT(rb_diag_bind_engine(&a,NULL),0);
    CHECK(!a.diag && !f->diag && !a.junction.diag);
}

void test_diagnostic_orifice_thermal_and_radiation(void)
{
    start(2,2);
    rb_orifice *o=&a.throttle;
    rb_chamber_init(o->a,o->a->V,200000,300,(double[]){1,0,0});
    rb_chamber_init(o->b,o->b->V,10000,300,(double[]){1,0,0});
    CHECK_EQ_INT(rb_diag_bind_engine(&a,&diagnostic),0);
    o->area=.1;o->cd=1;
    rb_orifice_flow(o,1);
    CHECK(o->diag->reason[RB_D_CHOKED]>0 && o->diag->reason[RB_D_EQUALISE]>0);
    CHECK(o->diag->requested[0]>o->diag->accepted[0]);
    a.junction.U=NAN;rb_chamber_update(&a.junction);
    CHECK(a.junction.diag->reason[RB_D_ENERGY_REPAIR]>0 && a.junction.diag->invalid>0);
    rb_chamber *g=&a.junction;
    g->U=-1e9;rb_chamber_update(g);
    CHECK(g->diag->reason[RB_D_TEMP_FLOOR]>0);
    rb_pipe *p=&a.tailpipe[0];rb_radiation *r=&p->radiation[1];
    rb_diag_slot *tap=r->diag;
    rb_radiation_init(r,.03,1.2,343);r->diag=tap;
    double direct,history;
    rb_radiation_linear(r,1.0/48000,&direct,&history);
    rb_radiation_step(r,INFINITY);
    CHECK(tap->reason[RB_D_RADIATION_RESET]>0);
    rb_face *f=&a.exhaust_pipe[0].face[0];
    f->a->mass=0;f->mdot=1;
    rb_pipe_flows(&a.exhaust_pipe[0],0,1.0/48000);
    CHECK(f->diag->reason[RB_D_EMPTY]>0 && f->moved==0);
}
