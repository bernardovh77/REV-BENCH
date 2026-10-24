#include "rb_test.h"
#include "engine.h"
#include "presets.h"
#include "ignition.h"
#include "combustion.h"
#include <math.h>
#include <string.h>
static rb_engine a,b;
static rb_ignition_trace trace;
static rb_ignition_event records[RB_IGN_MAX_RECORDS];
static double wrap(double x) {x=fmod(x,720);return x<0?x+720:x;}
static void start(int preset)
{
    rb_engine_desc d;CHECK_EQ_INT(rb_preset_desc(preset,&d),0);
    CHECK_EQ_INT(rb_engine_init_at(&a,&d,1.0/192000,48000),0);
    CHECK_EQ_INT(rb_ignition_trace_init(&trace,records,RB_IGN_MAX_RECORDS),0);
    a.ignition_trace=&trace;a.ignition=1;a.rpm=3000;a.throttle_pos=.6;
}
void test_ignition_causal_clock_and_heat(void)
{
    for(int preset=0;preset<5;preset++) for(int mode=0;mode<3;mode++) {
        start(preset);a.variation=mode==2?4:mode;
        double discharge[RB_MAX_CYLINDERS],kernel[RB_MAX_CYLINDERS];
        for(int i=0;i<a.desc.cylinders;i++) discharge[i]=kernel[i]=-1;
        for(int tick=0;tick<19200;tick++) {
            double old[RB_MAX_CYLINDERS],ahead[RB_MAX_CYLINDERS];
            for(int i=0;i<a.desc.cylinders;i++) {old[i]=rb_engine_cylinder_angle(&a,i);ahead[i]=wrap(a.burn[i].spark_angle-old[i]);}
            trace.count=0;rb_engine_step(&a);
            CHECK_EQ_INT(a.physics_tick,tick+1);
            for(size_t k=0;k<trace.count;k++) {
                const rb_ignition_event *r=&records[k];
                CHECK(r->tick==(uint64_t)tick && r->fraction>=0 && r->fraction<=1+1e-12);
                CHECK(r->kernel_delay_deg>=0 && r->cylinder>=0 && r->cylinder<a.desc.cylinders);
                const double time=(double)r->tick+r->fraction;const int c=r->cylinder;
                if(r->kind==RB_IGN_DISCHARGE) {
                    CHECK_NEAR(wrap(r->angle_deg-a.burn[c].spark_angle+360)-360,0,1e-9);
                    discharge[c]=time;kernel[c]=-1;
                }
                if(r->kind==RB_IGN_KERNEL) {CHECK(discharge[c]>=0 && time>=discharge[c]-1e-12);kernel[c]=time;}
            }
            for(int i=0;i<a.desc.cylinders;i++) {
                const rb_burn *q=&a.burn[i];
                const double span=wrap(rb_engine_cylinder_angle(&a,i)-old[i]);
                if(ahead[i]>0 && ahead[i]<90 && ahead[i]>=span) CHECK(q->heat<=1e-12);
                if(q->heat>1e-12) CHECK(q->discharged && q->kernel_started && discharge[i]>=0 && kernel[i]>=discharge[i] && kernel[i]<=(double)tick+1);
                if(q->discharged_this_step) {
                    const double available=fmax(span,6*100.0*a.dt)*(1-q->discharge_fraction)-q->kernel_delay_deg;
                    const double bound=rb_wiebe(available,q->duration,a.desc.wiebe_a,a.desc.wiebe_m)*q->burnable*rb_fuel_heat();
                    CHECK(q->heat<=bound+1e-8);
                }
            }
        }
        CHECK(trace.totals[RB_IGN_DISCHARGE]>0 && trace.totals[RB_IGN_KERNEL]>0 && trace.dropped==0);
        CHECK_EQ_INT(trace.totals[RB_IGN_COMMAND],trace.totals[RB_IGN_DISCHARGE]);
    }
}
void test_ignition_suppression_and_resume(void)
{
    for(int preset=0;preset<5;preset++) {
        start(preset);a.ignition=0;
        for(int k=0;k<19200;k++) rb_engine_step(&a);
        CHECK(trace.totals[RB_IGN_OFF]>0 && trace.totals[RB_IGN_DISCHARGE]==0 && a.heat_released==0);
        start(preset);a.afr=0;
        for(int k=0;k<19200;k++) rb_engine_step(&a);
        CHECK(trace.totals[RB_IGN_DISCHARGE]>0 && trace.totals[RB_IGN_KERNEL]==0 && a.heat_released==0);
        CHECK_EQ_INT(trace.totals[RB_IGN_NO_REACTANTS],trace.totals[RB_IGN_DISCHARGE]);
        start(preset);a.hold_speed=0;a.desc.inertia=50;
        a.omega=3000*3.14159265358979323846/30;a.kinetic=.5*a.desc.inertia*a.omega*a.omega;
        a.desc.rev_limit=2000;a.desc.rev_limit_resume=1900;
        for(int k=0;k<19200;k++) rb_engine_step(&a);
        CHECK(trace.totals[RB_IGN_HARD_CUT]>0 && trace.totals[RB_IGN_DISCHARGE]==0 && a.heat_released==0 && a.fuel_in>0);
        a.desc.rev_limit=7000;a.desc.rev_limit_resume=6900;
        for(int k=0;k<19200;k++) rb_engine_step(&a);
        CHECK(trace.totals[RB_IGN_DISCHARGE]>0 && trace.totals[RB_IGN_KERNEL]>0 && a.heat_released>0);
        start(preset);a.hold_speed=0;a.desc.inertia=50;
        a.omega=3000*3.14159265358979323846/30;a.kinetic=.5*a.desc.inertia*a.omega*a.omega;
        a.desc.rev_limit=3100;a.desc.rev_limit_resume=2900;
        for(int k=0;k<19200;k++) rb_engine_step(&a);
        CHECK(trace.totals[RB_IGN_SOFT_CUT]>0 && trace.totals[RB_IGN_DISCHARGE]>0 && trace.totals[RB_IGN_HARD_CUT]==0);
    }
}
void test_ignition_pending_kernel_expires_at_evo(void)
{
    for(int preset=0;preset<5;preset++) {
        start(preset);
        for(int k=0;k<28800;k++) {
            trace.count=0;rb_engine_step(&a);
            for(size_t j=0;j<trace.count;j++) {
                if(records[j].kind==RB_IGN_EVO_PENDING) CHECK(!a.burn[records[j].cylinder].kernel_pending && !a.burn[records[j].cylinder].burning);
                if(records[j].kind!=RB_IGN_SCHEDULED) continue;
                rb_burn *q=&a.burn[records[j].cylinder];q->kernel_delay_deg=1000;q->start_angle=wrap(q->spark_angle+1000);
            }
        }
        CHECK(trace.totals[RB_IGN_EVO_PENDING]>0 && trace.totals[RB_IGN_KERNEL]==0 && a.heat_released==0 && a.fuel_in>0);
    }
}
void test_ignition_tap_capacity_and_transparency(void)
{
    start(2);a.variation=1;
    CHECK_EQ_INT(rb_engine_init_at(&b,&a.desc,a.dt,48000),0);
    b.variation=a.variation;b.ignition=a.ignition;b.rpm=a.rpm;b.throttle_pos=a.throttle_pos;
    CHECK_EQ_INT(rb_ignition_trace_init(&trace,records,1),0);
    for(int k=0;k<19200;k++) {rb_engine_step(&a);rb_engine_step(&b);CHECK(a.theta==b.theta && a.heat_released==b.heat_released && a.rand_cycles==b.rand_cycles);}
    CHECK(trace.count==1 && trace.dropped>0);
    for(int k=0;k<rb_engine_pipe_count(&a);k++) {
        const rb_pipe *p=rb_engine_pipe(&a,k),*q=rb_engine_pipe(&b,k);
        for(int j=0;j<p->cells;j++) CHECK(p->cell[j].U==q->cell[j].U && p->cell[j].p==q->cell[j].p);
    }
    CHECK_EQ_INT(rb_ignition_trace_init(&trace,NULL,1),-1);
    CHECK_EQ_INT(rb_ignition_trace_init(&trace,records,RB_IGN_MAX_RECORDS+1),-1);
    CHECK_EQ_INT(rb_ignition_trace_init(&trace,NULL,0),0);
}
