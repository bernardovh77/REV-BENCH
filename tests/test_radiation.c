#include "radiation.h"
#include "pipe.h"
#include "rb_test.h"
#include <math.h>

#define PI 3.14159265358979323846

void test_radiation_causal_impedance(void)
{
    const double frequencies[] = {100, 500, 2000, 8000};
    const double dt = 1.0/96000.0, radius = 0.03, rho = 1.2, c = 343.0;
    for (int j=0;j<4;j++) {
        rb_radiation r;
        rb_radiation_init(&r,radius,rho,c);
        double real=0.0,imag=0.0,work=0.0;
        for (int n=0;n<97024;n++) {
            const double phase=2.0*PI*frequencies[j]*dt*n;
            const double v=cos(phase);
            double direct,history;
            rb_radiation_linear(&r,dt,&direct,&history);
            const double p=rb_radiation_step(&r,v);
            if (n==500) CHECK_NEAR(p,direct*v+history,1e-10);
            if (n>=1024) {real+=p*cos(phase);imag-=p*sin(phase);work+=p*v*dt;}
        }
        real*=2.0/96000.0;imag*=2.0/96000.0;
        /* Independent complex division of the continuous rational load at
         * the bilinear transform's warped frequency. */
        const double x=2.0*radius/c/dt*tan(PI*frequencies[j]*dt);
        const double nr=-0.457*x*x,ni=1.226*x,dr=2.0-0.457*x*x,di=1.560*x;
        const double den=dr*dr+di*di;
        const double zr=(nr*dr+ni*di)/den,zi=(ni*dr-nr*di)/den;
        CHECK_NEAR(real,rho*c*zr,1e-6*rho*c);
        CHECK_NEAR(imag,rho*c*zi,1e-6*rho*c);
        CHECK(work>0.0 && zr>0.0);
        CHECK(((zr-1)*(zr-1)+zi*zi)/((zr+1)*(zr+1)+zi*zi) <= 1.0);
        if (j==0) CHECK_NEAR(zi/x,0.6133,0.004);
        for (int n=0;n<4096;n++) {
            double direct,history;
            rb_radiation_linear(&r,dt,&direct,&history);
            rb_radiation_step(&r,0.3);
        }
        CHECK_NEAR(r.pressure,0.0,1e-9); /* no artificial DC backpressure */
    }
    rb_radiation bad;
    rb_radiation_init(&bad,NAN,rho,c);
    CHECK_EQ_INT(bad.enabled,0);
    rb_radiation_init(&bad,radius,rho,c);
    double direct,history;
    rb_radiation_linear(&bad,0.0,&direct,&history);
    CHECK(direct==0.0 && history==0.0);
    rb_radiation_linear(&bad,dt,&direct,&history);
    CHECK(isfinite(rb_radiation_step(&bad,NAN)));
}

void test_radiation_pipe_work(void)
{
    static rb_pipe p;
    static rb_chamber cell[64], air;
    static rb_face face[64];
    const double y[3]={1,0,0},dt=1.0/96000.0;
    rb_chamber_init_reservoir(&air,101325,300,y);
    rb_pipe_init(&p,cell,face,64,0.8,0.05,101325,300,y,300,0.0);
    CHECK_EQ_INT(rb_pipe_radiate(&p,1,air.rho,rb_chamber_sound_speed(&air)),-1);
    rb_pipe_join(&p,1,&air,0.5,0);
    CHECK_EQ_INT(rb_pipe_radiate(&p,1,air.rho,rb_chamber_sound_speed(&air)),0);
    CHECK_NEAR(p.end_face[1].length,0.5*p.dx,1e-15); /* exterior inertia belongs to the causal load */
    const double gamma=rb_chamber_gamma(&air);
    for(int k=0;k<64;k++) {
        const double x=(k+0.5)*p.dx;
        double pressure=101325+100*exp(-pow((x-.25)/.07,2));
        rb_chamber_init(&cell[k],p.area*p.dx,pressure,300*pow(pressure/101325,(gamma-1)/gamma),y);
    }
    double m0,u0,m1,u1,out_m=0.0,out_u=0.0;
    rb_pipe_contents(&p,&m0,&u0,0);
    for(int n=0;n<6000;n++) {
        rb_pipe_clear(&p,0);
        rb_pipe_flows(&p,0,dt);
        out_m+=p.end_face[1].moved*dt;
        out_u+=p.end_face[1].moved*p.end_face[1].moved_h*dt;
        rb_pipe_integrate(&p,0,dt);
        rb_pipe_accelerate(&p,0,dt);
    }
    rb_pipe_contents(&p,&m1,&u1,0);
    CHECK(p.radiated>1e-8 && isfinite(p.radiated));
    CHECK_NEAR(m1-m0,-out_m,m0*1e-11);
    CHECK_NEAR(u1-u0,-out_u+p.heat-p.radiated,1e-10*fmax(fabs(u0),1.0));
    int repairs=0;
    for(int k=0;k<64;k++) repairs+=cell[k].repairs;
    CHECK_EQ_INT(repairs,0);
}
