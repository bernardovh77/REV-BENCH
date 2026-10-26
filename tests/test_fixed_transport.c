#include "rb_test.h"
#include "fixed_transport.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static const double air[3]={1,0,0},burned[3]={.1,.001,.899};
static rb_ft_slot data[3][257];
static void setup(rb_ft_state *s,int n,double L,rb_ft_slot *v,double U,int wave)
{
    CHECK(rb_ft_prepare(s,n,L,.003,101325,air,burned,300,900,v,257)==0);
    for(int i=0;i<n;i++) {
        double x=(i+.5)*s->dx,dp=wave?5*exp(-pow((x-.25)/.07,2)):0;
        rb_gas *g=&s->gas[0];double rho=101325/(g->R*300),c=sqrt(rb_gas_gamma(g,300)*g->R*300);
        s->slot[i].q=rb_ft_make(s,101325+dp,U+dp/(rho*c),1,(101325+dp)/((rho+dp/(c*c))*g->R),900);
    }
    CHECK(rb_ft_load(s)==0);
}
static void balanced(rb_ft_state *s,rb_ft_book initial,int lo,int hi,rb_ft_book left0,rb_ft_book right0)
{
    rb_ft_book now=rb_ft_inventory(s,lo,hi),l=s->slot[lo].transfer,r=s->slot[hi].transfer;
    for(int k=0;k<2;k++)CHECK_NEAR(now.m[k]-initial.m[k],l.m[k]-left0.m[k]-r.m[k]+right0.m[k],1e-13);
    CHECK_NEAR(now.momentum-initial.momentum,l.momentum-left0.momentum-r.momentum+right0.momentum,1e-11);
    CHECK_NEAR(now.energy-initial.energy,l.energy-left0.energy-r.energy+right0.energy,1e-9);
}
void test_fixed_transport_contact_and_stationary_books(void)
{
    for(int sign=-1;sign<=1;sign++) {
        rb_ft_state s;setup(&s,64,1,data[0],70*sign,0);
        s.loaded=0;
        for(int i=0;i<64;i++)s.slot[i].q=rb_ft_make(&s,101325,70*sign,i<32?1:0,300,900);
        CHECK(rb_ft_load(&s)==0);
        rb_ft_book zero={{0,0},0,0},regional=rb_ft_inventory(&s,0,32);
        rb_ft_q left=rb_ft_make(&s,101325,70*sign,1,300,900),right=rb_ft_make(&s,101325,70*sign,0,300,900);
        for(int j=0;j<500;j++)CHECK(rb_ft_step(&s,1e-6,left,right)==0);
        balanced(&s,s.initial,0,64,zero,zero);balanced(&s,regional,0,32,zero,zero);
        for(int i=0;i<64;i++) {
            rb_ft_primitive w;CHECK(rb_ft_decode(&s,s.slot[i].q,&w)==0);
            CHECK_NEAR(w.p,101325,1e-5);CHECK_NEAR(w.u,70*sign,1e-7);
        }
    }
}
void test_fixed_transport_current_shared_port(void)
{
    for(int cut=32;cut<=80;cut+=48) {
        rb_ft_state full,a,b;setup(&full,128,1,data[0],0,1);
        full.loaded=0;
        for(int i=89;i<128;i++)full.slot[i].q=rb_ft_make(&full,101325,0,0,300,900);
        CHECK(rb_ft_load(&full)==0);
        setup(&a,cut,cut/128.,data[1],0,0);setup(&b,128-cut,(128-cut)/128.,data[2],0,0);
        for(int i=0;i<cut;i++)a.slot[i].q=full.slot[i].q;
        for(int i=cut;i<128;i++)b.slot[i-cut].q=full.slot[i].q;
        a.loaded=b.loaded=0;CHECK(rb_ft_load(&a)==0);CHECK(rb_ft_load(&b)==0);
        rb_ft_q left=rb_ft_make(&full,101325,0,1,300,900),right=rb_ft_make(&full,101325,0,0,300,900);
        rb_ft_book zero={{0,0},0,0};
        for(int tick=0;tick<2500;tick++) {
            CHECK(rb_ft_step(&full,1e-6,left,right)==0);
            CHECK(rb_ft_step_pair(&a,&b,1e-6,left,right)==0);
        }
        for(int i=0;i<128;i++) {
            rb_ft_q q=(i<cut?a.slot[i].q:b.slot[i-cut].q),r=full.slot[i].q;
            CHECK_NEAR(q.momentum,r.momentum,1e-13);CHECK_NEAR(q.energy,r.energy,1e-9);
            for(int k=0;k<2;k++)CHECK_NEAR(q.r[k],r.r[k],1e-13);
        }
        CHECK(!memcmp(&a.slot[cut].transfer,&b.slot[0].transfer,sizeof(rb_ft_book)));
        balanced(&a,a.initial,0,cut,zero,zero);balanced(&b,b.initial,0,128-cut,zero,zero);
    }
    rb_ft_state s;setup(&s,16,1,data[0],0,0);
    double rho0=101325/(s.gas[0].R*300),c0=sqrt(rb_gas_gamma(&s.gas[0],300)*s.gas[0].R*300);
    double rho1=101325/(s.gas[1].R*900),c1=sqrt(rb_gas_gamma(&s.gas[1],900)*s.gas[1].R*900);
    rb_ft_q l=rb_ft_make(&s,101330,5/(rho0*c0),1,300,900),r=rb_ft_make(&s,101325,0,0,300,900);
    rb_ft_flux f;CHECK(rb_ft_riemann(&s,l,r,&f)==0);
    double reflection=(rho1*c1-rho0*c0)/(rho1*c1+rho0*c0);
    CHECK_NEAR(f.p-101330,5*reflection,.001);CHECK(f.p<101330);

}
void test_fixed_transport_reversal_wall_and_domain(void)
{
    rb_ft_state s;setup(&s,32,1,data[0],0,0);
    rb_ft_q q=rb_ft_make(&s,101325,0,1,300,900);
    CHECK(rb_ft_wall(&s,400,650,.2,.8,100,2,300)==0);
    double old=rb_ft_inventory(&s,0,32).energy+s.wall_capacity*s.wall_T;
    for(int j=0;j<100;j++)CHECK(rb_ft_step(&s,1e-6,q,q)==0);
    double E=rb_ft_inventory(&s,0,32).energy+s.wall_capacity*s.wall_T;
    CHECK_NEAR(E-old,s.slot[0].transfer.energy-s.slot[32].transfer.energy+s.ambient_heat,1e-8);
    CHECK(s.wall_T<650);CHECK(s.gas_heat>0);
    CHECK(rb_ft_step(&s,1,q,q)!=0);CHECK(s.failed);
    CHECK(rb_ft_step(&s,1e-6,q,q)!=0);CHECK(isnan(rb_ft_inventory(&s,0,32).energy));
    CHECK(rb_ft_prepare(&s,32,1,.003,101325,air,burned,300,900,data[0],33)==0);
    CHECK(rb_ft_step(&s,1e-6,q,q)!=0);
    CHECK(rb_ft_prepare(&s,32,1,.003,101325,air,burned,300,1100,data[0],33)!=0);
    setup(&s,32,1,data[0],0,0);
    rb_ft_flux f;rb_ft_q hot=rb_ft_make(&s,101325,-10,0,300,900),cold=rb_ft_make(&s,101325,-10,1,300,900);
    CHECK(rb_ft_riemann(&s,hot,cold,&f)==0);CHECK(f.f.r[0]<0);CHECK_NEAR(f.f.r[1],0,1e-15);
    CHECK_NEAR(f.p,101325,1e-8);CHECK_NEAR(f.u,-10,1e-10);
    CHECK_NEAR(f.f.energy,-10*(101325/(s.gas[0].R*300)*(rb_gas_u(&s.gas[0],300)+50)+101325),1e-8);
    setup(&s,32,1,data[0],0,0);q=rb_ft_make(&s,101325,0,1,300,900);
    CHECK(rb_ft_step(&s,1,q,q)!=0);CHECK(s.failed);CHECK(rb_ft_step(&s,1e-6,q,q)!=0);
    CHECK(rb_ft_prepare(&s,32,1,.003,101325,air,burned,300,900,data[0],33)==0);
    for(int i=0;i<32;i++)s.slot[i].q=rb_ft_make(&s,101325,0,1,300,900);
    CHECK(rb_ft_step(&s,1e-6,q,q)!=0);
    CHECK(rb_ft_load(&s)==0);
    rb_ft_q bad=rb_ft_make(&s,101325,0,1,1100,900);rb_ft_primitive w;
    CHECK(rb_ft_decode(&s,bad,&w)!=0);
    /* Each heat half-step is physical. Crossing the pressure envelope in the
     * LAST half must reject this call, not wait for the next caller tick. */
    setup(&s,32,1,data[0],0,0);q=rb_ft_make(&s,101325,0,1,300,900);
    CHECK(rb_ft_wall(&s,1e9,650,0,1,4e6,0,300)==0);
    CHECK(rb_ft_step(&s,1e-7,q,q)!=0);CHECK(s.failed);
}
void test_fixed_transport_separate_fronts_and_fixed_exterior(void)
{
    for(int species=0;species<2;species++)for(int sign=-1;sign<=1;sign++) {
        rb_ft_state s;double T0=species?600:300,T1=species?600:900,U=70*sign;
        CHECK(rb_ft_prepare(&s,64,1,.003,101325,air,species?burned:air,T0,T1,data[0],257)==0);
        for(int i=0;i<64;i++) {
            double a=fmax(0,fmin(1,(.37-i*s.dx)/s.dx));
            s.slot[i].q=rb_ft_make(&s,101325,U,a,T0,T1);
        }
        CHECK(rb_ft_load(&s)==0);
        rb_ft_q l=rb_ft_make(&s,101325,U,1,T0,T1),r=rb_ft_make(&s,101325,U,0,T0,T1);
        for(int j=0;j<200;j++)CHECK(rb_ft_step(&s,1e-6,l,r)==0);
        rb_ft_book zero={{0,0},0,0};balanced(&s,s.initial,0,64,zero,zero);
        for(int i=0;i<64;i++) {
            rb_ft_primitive w;CHECK(rb_ft_decode(&s,s.slot[i].q,&w)==0);
            CHECK_NEAR(w.p,101325,1e-5);CHECK_NEAR(w.u,U,1e-7);
        }
    }
    for(int n=32;n<=64;n+=32) {
        rb_ft_state s;setup(&s,n,1,data[0],0,0);
        CHECK(rb_ft_wall(&s,400,650,.2,.8,0,2,300)==0);
        rb_ft_q q=rb_ft_make(&s,101325,0,1,300,900);
        for(int j=0;j<100;j++)CHECK(rb_ft_step(&s,1e-6,q,q)==0);
        CHECK_NEAR(s.wall_T,300+350*exp(-2*.0001/400),1e-9);
        CHECK_NEAR(s.ambient_heat,400*(s.wall_T-650),1e-8);
        CHECK_NEAR(s.gas_heat,0,1e-14);
    }
}
