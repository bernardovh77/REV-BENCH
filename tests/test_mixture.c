#include "rb_test.h"
#include "mixture.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static rb_mix_cell *prepare(rb_mix_state *s,int n,int boundary)
{
    rb_mix_cell *mem=calloc(rb_mix_storage(n),sizeof *mem);
    CHECK(mem!=NULL);CHECK_EQ_INT(rb_mix_prepare(s,n,1.0/n,boundary,mem,rb_mix_storage(n)),0);
    return mem;
}
static rb_mix_cell totals(rb_mix_state *s)
{
    rb_mix_cell q={{0}};
    for(int i=0;i<s->cells;i++) for(int j=0;j<5;j++) q.v[j]+=s->q[i].v[j]*s->dx;
    return q;
}
void test_mixture_eos_and_guards(void)
{
    const double ys[][3]={{1,0,0},{0,0,1},{.2,.05,.75}};
    for(int y=0;y<3;y++) for(int t=0;t<8;t++) {
        double T=100+500*t;rb_mix_cell q;rb_mix_primitive w;
        CHECK_EQ_INT(rb_mix_make(200000,T,-70,ys[y],&q),0);
        CHECK_EQ_INT(rb_mix_decode(q,&w),0);
        CHECK_NEAR(w.p,200000,1e-5);CHECK_NEAR(w.T,T,1e-7);
        rb_gas g;rb_gas_init(&g,ys[y]);
        CHECK_NEAR(w.c,sqrt(rb_gas_gamma(&g,T)*g.R*T),1e-7);
    }
    rb_mix_state s;rb_mix_cell *mem=prepare(&s,16,0),q;rb_mix_primitive w;
    CHECK_EQ_INT(rb_mix_make(101325,300,0,ys[0],&q),0);
    q.v[1]=-1e-20;CHECK_EQ_INT(rb_mix_decode(q,&w),-1);
    q.v[1]=0;q.v[4]=NAN;CHECK_EQ_INT(rb_mix_decode(q,&w),-1);
    CHECK_EQ_INT(rb_mix_prepare(&s,16,.01,0,mem,1),-1);
    for(int i=0;i<16;i++) rb_mix_make(101325,300,0,ys[0],&s.q[i]);
    CHECK_EQ_INT(rb_mix_step(&s,.1),-1);CHECK_EQ_INT(s.failed,1);
    CHECK_EQ_INT(rb_mix_step(&s,1e-6),-1);free(mem);
}
void test_mixture_flux_and_neighbor(void)
{
    const double y[]={.2,.01,.79};rb_mix_cell l,r,f,g;
    rb_mix_make(101325,900,0,y,&l);r=l;
    CHECK_EQ_INT(rb_mix_face(l,r,&f),0);
    CHECK_NEAR(f.v[0]+f.v[1]+f.v[2],0,1e-12);
    CHECK_NEAR(f.v[3],101325,1e-8);CHECK_NEAR(f.v[4],0,1e-8);
    rb_mix_make(90000,700,0,y,&r);CHECK_EQ_INT(rb_mix_face(l,r,&f),0);
    CHECK(f.v[0]+f.v[1]+f.v[2]>0);CHECK(f.v[4]>0);
    CHECK_EQ_INT(rb_mix_face(r,l,&g),0);
    for(int j=0;j<5;j++) CHECK_NEAR(f.v[j],(j==3 ? 1:-1)*g.v[j],1e-7);
    rb_mix_make(100000,900,0,y,&r);rb_mix_face(l,r,&g);
    CHECK(g.v[0]<f.v[0]); /* current neighbor pressure changes accepted flux */
}
void test_mixture_conservation_and_boundary(void)
{
    for(int boundary=0;boundary<3;boundary++) {
        rb_mix_state s;rb_mix_cell *mem=prepare(&s,64,boundary);
        const double y[]={.7,.01,.29};
        for(int i=0;i<64;i++) rb_mix_make(101325+100*sin(6.283185307179586*(i+.5)/64),600,20,y,&s.q[i]);
        rb_mix_cell before=totals(&s);
        int status=0;for(int k=0;k<200;k++) if((status=rb_mix_step(&s,1e-6))) break;
        CHECK_EQ_INT(status,0);rb_mix_cell after=totals(&s);
        for(int j=0;j<5;j++) CHECK_NEAR(after.v[j]-before.v[j]+s.boundary_book.v[j],0,
            1e-10*fmax(j==3 ? 300:1,fabs(before.v[j])));
        if(boundary==2) {CHECK_NEAR(s.boundary_book.v[0],0,1e-15);CHECK_NEAR(s.boundary_book.v[4],0,1e-8);}
        CHECK_EQ_INT(s.failed,0);free(mem);
    }
}
void test_mixture_stationary_contact_and_wave(void)
{
    rb_mix_state s;rb_mix_cell *mem=prepare(&s,128,0);
    const double air[]={1,0,0},burn[]={.1,0,.9};
    for(int i=0;i<128;i++) rb_mix_make(101325,i<64 ? 300:900,0,i<64 ? air:burn,&s.q[i]);
    rb_mix_cell copy[128];memcpy(copy,s.q,sizeof copy);
    int status=0;for(int k=0;k<100;k++) if((status=rb_mix_step(&s,1e-6))) break;
    CHECK_EQ_INT(status,0);
    for(int i=0;i<128;i++) {rb_mix_primitive w;CHECK_EQ_INT(rb_mix_decode(s.q[i],&w),0);CHECK_NEAR(w.p,101325,.001);}
    CHECK_NEAR(s.q[0].v[0],copy[0].v[0],1e-12);
    /* Smooth acoustic carrier after one metre: analytic amplitude/phase.
     * This one supported row is a regression, not the full promotion screen. */
    rb_mix_primitive base;rb_mix_make(101325,300,0,air,&s.q[0]);rb_mix_decode(s.q[0],&base);
    const double k=2*3.14159265358979323846*4,dt=1.0/192000;
    for(int i=0;i<128;i++) {
        double dp=.1*cos(k*(i+.5)/128);
        rb_mix_make(101325+dp,300*(1+(base.c*base.c/(base.p/base.rho)-1)*dp/(base.c*base.c*base.rho)),
                    dp/(base.rho*base.c),air,&s.q[i]);
    }
    double re=0,im=0;int steps=(int)lround(192000/base.c);
    for(int t=0;t<steps;t++) CHECK_EQ_INT(rb_mix_step(&s,dt),0);
    for(int i=0;i<128;i++) {rb_mix_primitive w;rb_mix_decode(s.q[i],&w);
        re+=(w.p-101325)*cos(k*(i+.5)/128)*2/128;im-=(w.p-101325)*sin(k*(i+.5)/128)*2/128;}
    double phase=atan2(im,re)+k*base.c*steps*dt;
    phase=remainder(phase,2*3.14159265358979323846);
    CHECK(fabs(20*log10(hypot(re,im)/.1))<1);CHECK(fabs(phase)*180/3.14159265358979323846<10);
    free(mem);
}
