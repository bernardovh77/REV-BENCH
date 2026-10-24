/* Stage041: independent fit, stochastic mean and actual engine-clock proofs. */
#include "combustion.h"
#include "engine.h"
#include "presets.h"
#include "rb.h"
#include "rb_test.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846
#define DT (1.0/192000)
static rb_engine e;

static double phase(double a) {a=fmod(a+360,720);return (a<0?a+720:a)-360;}

void test_combustion_fit_range_and_fallback(void)
{
    const double states[][4]={{360,122000,.04,1},{400,202650,0,.8},{600,1e6,.2,1.4}};
    for(int i=0;i<3;i++) {
        const double T=states[i][0],p=states[i][1],x=states[i][2],phi=states[i][3];
        const double expected=.01*(27.58-78.34*pow(phi-1.13,2))
            *pow(T/298,2.18-.8*(phi-1))*pow(p/101325,-.16+.22*(phi-1))*(1-2.1*x);
        rb_charge_burn q=rb_charge_burn_evaluate(T,p,x,phi,3000,60);
        CHECK_EQ_INT(q.flags,0);
        CHECK_NEAR(q.speed_m_s,expected,1e-12);
        CHECK(q.demand_deg>=30 && q.capped_deg<=120);
    }
    rb_charge_burn q=rb_charge_burn_evaluate(360,122000,.04,1,3000,60);
    CHECK_NEAR(q.demand_deg,60,1e-12);
    const double bad[][4]={{360,122000,.04,.7},{300,122000,.04,1},
        {360,20000,.04,1},{360,122000,.3,1},{360,122000,.44,1},
        {360,122000,.04,.1},{NAN,122000,.04,1},{360,122000,.04,0}};
    const unsigned bits[]={RB_FLAME_PHI_RANGE,RB_FLAME_T_RANGE,RB_FLAME_P_RANGE,
        RB_FLAME_DILUTION_RANGE,RB_FLAME_DILUTION_FLOOR,RB_FLAME_SPEED_FLOOR,
        RB_FLAME_INVALID,RB_FLAME_NO_REACTANTS};
    for(int i=0;i<8;i++) {
        q=rb_charge_burn_evaluate(bad[i][0],bad[i][1],bad[i][2],bad[i][3],3000,60);
        CHECK((q.flags&bits[i])!=0);
        CHECK(isfinite(q.demand_deg) && q.demand_deg>0 && q.demand_deg<=720);
    }
    q=rb_charge_burn_evaluate(360,20000,.9,.1,10000,180);
    CHECK((q.flags&RB_FLAME_DEMAND_LIMIT)!=0);
    CHECK(q.demand_deg==720 && q.raw_demand_deg>720);
}

void test_combustion_conditional_means(void)
{
    /* Independent midpoint quadrature of the bounded Gaussian, including
     * point masses at +/-3. Also tests duration bounds and small sigma. */
    const double cases[][4]={{60,.06,1,720},{700,.24,1.08,720},
        {120,.24,1.4,180},{60,.24,.2,720},{60,1e-6,1,720}};
    for(int c=0;c<5;c++) {
        const double B=cases[c][0],sigma=cases[c][1],trim=cases[c][2],cap=cases[c][3];
        double integral=0;const int n=40000;const double dz=6.0/n;
        for(int j=0;j<n;j++) {
            const double z=-3+(j+.5)*dz;
            const double d=fmin(fmax(B*trim*exp(sigma*z),.25*B),cap);
            integral+=d*exp(-.5*z*z)/sqrt(2*PI)*dz;
        }
        const double tail=.5*erfc(3/sqrt(2));
        integral+=tail*(fmin(fmax(B*trim*exp(-3*sigma),.25*B),cap)
                      +fmin(fmax(B*trim*exp(3*sigma),.25*B),cap));
        CHECK_NEAR(rb_burn_mean_duration(B,sigma,trim,cap),integral,2e-6);
    }
    CHECK_NEAR(rb_burn_mean_duration(60,0,1,720),60,0);
    CHECK_NEAR(rb_burn_mean_duration(60,.1,-1,720),15,0);
    double positive=3*.5*erfc(3/sqrt(2));
    for(int j=0;j<40000;j++) {
        const double z=(j+.5)*3/40000;
        positive+=z*exp(-.5*z*z)/sqrt(2*PI)*3/40000;
    }
    CHECK_NEAR(rb_kernel_positive_mean(),positive,1e-8);
    for(int i=1;i<10;i++) {
        const double f=.1*i;
        CHECK_NEAR(rb_wiebe(rb_wiebe_quantile(f,5,2),1,5,2),f,1e-13);
    }
}

static int controlled_charge(int preset,int model,double v)
{
    rb_engine_desc d;rb_preset_desc(preset,&d);
    if(rb_engine_init_at(&e,&d,DT,48000) || rb_engine_combustion_model(&e,model)) return 0;
    e.rpm=3000;e.ignition=1;e.variation=v;e.spark_map=0;e.spark=25;
    e.trim_fuel[0]=e.trim_burn[0]=0;
    const double ivc=rb_valve_closes(&d.intake),step=6*e.rpm*e.dt;
    int k;for(k=0;k<20000 && !(e.theta<=ivc && e.theta+step>ivc);k++) rb_engine_step(&e);
    if(k==20000)return 0;
    const double afr=RB_AFR_STOICH/1.5;
    const double y[]={.56*afr/(afr+1),.56/(afr+1),.44};
    rb_chamber_init(&e.cyl[0].gas,e.cyl[0].gas.V,122000,360,y);
    rb_engine_step(&e);return 1;
}

void test_combustion_zero_variation_continuity(void)
{
    const double v[]={0,1e-9,1e-6,.001};
    for(int p=0;p<5;p++) for(int model=0;model<2;model++) {
        double duration0=0,heat0=0;
        for(int c=0;c<4;c++) {
            CHECK(controlled_charge(p,model,v[c]));
            rb_burn *b=&e.burn[0];
            CHECK_NEAR(b->phi_ivc,1.5,1e-12);
            CHECK_NEAR(b->raw_demand_duration,397.8800248408624*e.desc.burn_duration/60,1e-9);
            CHECK_EQ_INT(b->combustion_model,model);
            CHECK((b->flame_flags&RB_FLAME_PHI_RANGE)!=0);
            CHECK((b->flame_flags&RB_FLAME_DILUTION_RANGE)!=0);
            if(!c)duration0=b->duration;
            CHECK_NEAR(b->duration,duration0,fmax(1e-8,4*v[c]*duration0));
            CHECK((model==RB_BURN_CAPPED)==!!(b->flame_flags&RB_FLAME_CAPPED_MODEL));
            const double evo=rb_valve_opens(&e.desc.exhaust);
            double heat=0;int seen=0;
            for(int k=0;k<20000;k++) {
                const double before=rb_engine_cylinder_angle(&e,0);
                rb_engine_step(&e);heat+=b->heat;
                if(before<evo && rb_engine_cylinder_angle(&e,0)>=evo) {seen=1;break;}
            }
            CHECK(seen);
            if(!c)heat0=heat;
            if(c==1)CHECK_NEAR(heat,heat0,1e-6*fmax(1,heat0));
        }
        if(model==RB_BURN_CHARGE)CHECK(duration0>180);
        else CHECK(duration0<=180);
    }
}

void test_combustion_seed_and_policy_controls(void)
{
    rb_engine_desc d;rb_preset_desc(0,&d);CHECK_EQ_INT(rb_engine_init(&e,&d,DT),0);
    CHECK_EQ_INT(e.combustion_model,RB_BURN_CHARGE);
    CHECK_EQ_INT(rb_engine_seed(&e,0),-1);
    CHECK_EQ_INT(rb_engine_combustion_model(&e,2),-1);
    CHECK_EQ_INT(rb_engine_seed(&e,0xF11C4E5ULL),0);CHECK(e.rand_flicker!=0);
    CHECK_EQ_INT(rb_engine_seed(&e,17),0);
    const uint64_t state=e.rand_cycles;double trims[RB_MAX_CYLINDERS];
    memcpy(trims,e.trim_burn,sizeof trims);
    CHECK_EQ_INT(rb_engine_seed(&e,17),0);CHECK(state==e.rand_cycles);
    CHECK(!memcmp(trims,e.trim_burn,sizeof trims));
    CHECK_EQ_INT(rb_engine_seed(&e,19),0);CHECK(state!=e.rand_cycles);
    CHECK_EQ_INT(rb_engine_combustion_model(&e,RB_BURN_CAPPED),0);
    rb_engine_step(&e);const uint64_t after=e.rand_cycles;
    CHECK_EQ_INT(rb_engine_seed(&e,17),-1);CHECK(after==e.rand_cycles);
    CHECK_EQ_INT(rb_engine_combustion_model(&e,RB_BURN_CHARGE),-1);
    CHECK_EQ_INT(e.combustion_model,RB_BURN_CAPPED);
}

void test_combustion_actual_heat_ca50(void)
{
    for(int p=0;p<5;p++) for(int varied=0;varied<2;varied++) {
        rb_engine_desc d;rb_preset_desc(p,&d);rb_engine_init_at(&e,&d,DT,48000);
        e.rpm=3000;e.throttle_pos=.6;e.ignition=1;e.variation=varied;
        double heat[RB_MAX_CYLINDERS]={0};uint64_t charges[RB_MAX_CYLINDERS]={0};
        int samples=0,early=0;double error=0,total=0;
        for(int k=0;k<192000;k++) {
            double before[RB_MAX_CYLINDERS];
            for(int i=0;i<d.cylinders;i++) before[i]=rb_engine_cylinder_angle(&e,i);
            rb_engine_step(&e);
            for(int i=0;i<d.cylinders;i++) {
                rb_burn *b=&e.burn[i];
                if(charges[i]!=b->charge_sequence) {charges[i]=b->charge_sequence;heat[i]=0;}
                if(!(b->heat>0))continue;
                if(!b->discharged || !b->kernel_started)early++;
                const double half=.5*b->burnable*rb_fuel_heat();
                if(heat[i]<half && heat[i]+b->heat>=half && k>38400) {
                    const double f=(half-heat[i])/b->heat;
                    const double actual=phase(before[i]+6*e.rpm*DT*f);
                    if(!(b->flame_flags&RB_FLAME_SPARK_LIMIT)) {
                        error=fmax(error,fabs(actual-8));total+=actual;samples++;
                        CHECK_NEAR(b->expected_ca50,8,1e-10);
                    }
                }
                heat[i]+=b->heat;
            }
        }
        CHECK(samples>=10*d.cylinders);CHECK_EQ_INT(early,0);
        if(!varied)CHECK(error<.2);else CHECK(fabs(total/samples-8)<1);
    }
}

void test_combustion_gross_net_cycle_books(void)
{
    rb_engine_desc d;rb_preset_desc(0,&d);rb_engine_init_at(&e,&d,DT,48000);
    e.rpm=3000;e.ignition=1;e.variation=1;
    double gross=0,pump=0,net=0;int samples=0;
    for(int k=0;k<153600;k++) {
        const double angle=rb_engine_cylinder_angle(&e,0),old=e.burn[0].work;
        const int closes=angle<=360 && angle+6*e.rpm*DT>360;
        rb_engine_step(&e);const rb_burn *b=&e.burn[0];
        if(closes) {
            if(samples>0) {
                CHECK_NEAR(b->last_work,net,1e-9);
                CHECK_NEAR(b->last_pumping_work,pump,1e-9);
                CHECK_NEAR(b->last_gross_work,gross,1e-9);
                CHECK_NEAR(b->last_work,b->last_gross_work-b->last_pumping_work,1e-10);
            }
            gross=pump=net=0;samples++;
        }
        const double dw=b->work-(closes?0:old);
        net+=dw;
        if(angle>=180 && angle<540)pump-=dw;
        else gross+=dw;
    }
    CHECK(samples>=15);
}
