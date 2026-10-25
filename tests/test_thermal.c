#include "rb_test.h"
#include "thermal.h"
#include <math.h>

void test_thermal_capacity_and_closed_books(void)
{
    const double y[]={.1,.001,.899};rb_tw_pair s;
    double C=rb_tw_capacity(.06,1,.001,7800,500);
    CHECK_NEAR(C,3.141592653589793*.001*.061*7800*500,1e-10);
    CHECK_NEAR(rb_tw_capacity(.06,.5,.001,7800,500)*2,C,1e-10);
    CHECK(isnan(rb_tw_capacity(.06,1,-1,7800,500)));
    CHECK_EQ_INT(rb_tw_prepare(&s,.003,y,900,C,650,300,37.7,7.54),0);
    double before=900;
    for(int i=0;i<1000;i++) {
        CHECK_EQ_INT(rb_tw_step(&s,.001),0);double T=rb_tw_temperature(&s);
        CHECK(T<=before+1e-8 && T>=300);CHECK(s.wall_T<=900+1e-8 && s.wall_T>=300);
        CHECK_NEAR(rb_tw_balance(&s),0,1e-10*fabs(s.initial_energy));before=T;
    }
    CHECK(s.ambient_heat<0);CHECK(s.wall_T>640);
    CHECK_EQ_INT(rb_tw_prepare(&s,.003,y,600,C,600,600,10,20),0);
    CHECK_EQ_INT(rb_tw_step(&s,10),0);CHECK_NEAR(rb_tw_temperature(&s),600,1e-8);CHECK_NEAR(s.wall_T,600,1e-8);
    CHECK_EQ_INT(rb_tw_prepare(&s,.003,y,900,C,300,300,0,0),0);
    CHECK_EQ_INT(rb_tw_step(&s,10),0);CHECK_NEAR(rb_tw_temperature(&s),900,1e-8);CHECK_NEAR(s.wall_T,300,1e-8);
}
void test_thermal_constant_cv_convergence(void)
{
    double errors[3];const double y[]={1,0,0},cv=717.5,mass=.003,C=2,H=10,t=.1;
    double equilibrium=(mass*cv*900+C*300)/(mass*cv+C);
    double exact=equilibrium+C/(mass*cv+C)*600*exp(-H*(1/(mass*cv)+1/C)*t);
    for(int level=0;level<3;level++) {
        rb_tw_pair s;CHECK_EQ_INT(rb_tw_prepare(&s,mass,y,900,C,300,300,H,0),0);
        for(int j=0;j<5;j++)s.gas.lo[j]=s.gas.hi[j]=j ? 0:cv;
        for(int j=0;j<5;j++)s.gas.ulo[j]=s.gas.uhi[j]=j ? 0:cv;
        s.gas.k_lo=s.gas.k_hi=-cv*RB_T_REF;s.energy=mass*rb_gas_u(&s.gas,900);s.initial_energy=s.energy+C*300;
        int n=10*(1<<level);for(int i=0;i<n;i++)CHECK_EQ_INT(rb_tw_step(&s,t/n),0);
        errors[level]=fabs(rb_tw_temperature(&s)-exact);CHECK_NEAR(rb_tw_balance(&s),0,1e-9);
    }
    CHECK(errors[0]>1);CHECK(errors[0]/errors[1]>1.85);CHECK(errors[1]/errors[2]>1.9);
}
void test_thermal_failure_and_hot_property_consistency(void)
{
    const double y[]={.1,.001,.899};rb_tw_pair s;
    CHECK_EQ_INT(rb_tw_prepare(&s,.003,y,900,0,650,300,1,1),-1);
    CHECK_EQ_INT(rb_tw_prepare(&s,.003,y,900,100,650,300,1,1),0);
    CHECK_EQ_INT(rb_tw_step(&s,NAN),-1);CHECK_EQ_INT(s.steps,0);
    s.energy=NAN;CHECK_EQ_INT(rb_tw_step(&s,.001),-2);CHECK_EQ_INT(rb_tw_step(&s,.001),-2);CHECK(isnan(rb_tw_balance(&s)));
    for(int species=0;species<3;species++) {
        double pure[3]={0};pure[species]=1;rb_gas g;rb_gas_init(&g,pure);
        for(int t=200;t<=4000;t+=37) {
            double T=t,epsilon=.001,cv=rb_gas_cv(&g,T);
            CHECK(cv>0);CHECK_NEAR(rb_gas_cp(&g,T)-cv,g.R,1e-11);
            CHECK_NEAR(rb_gas_h(&g,T)-rb_gas_u(&g,T),g.R*T,1e-8);
            if(fabs(T-1000)>1 && T>200 && T<4000)CHECK_NEAR((rb_gas_u(&g,T+epsilon)-rb_gas_u(&g,T-epsilon))/(2*epsilon),cv,1e-6*cv);
            CHECK_NEAR(rb_gas_T_from_u(&g,rb_gas_u(&g,T),20),T,1e-6);
        }
        CHECK_NEAR(rb_gas_u(&g,1000+1e-8)-rb_gas_u(&g,1000-1e-8),0,1e-3);
    }
}
