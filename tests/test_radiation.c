#include "radiation.h"
#include "pipe.h"
#include "rb_test.h"
#include <math.h>
#include <stdio.h>

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
        CHECK(work>0.0 && zr>0.0);   /* it absorbs: |R| < 1 follows from zr > 0 */
        /* A check of this test's own reference, not of the core: at low
         * frequency its load is the end correction of an unflanged pipe,
         * 0.6133 times the radius (Levine and Schwinger 1948). */
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

/* ---- stage 051: the load against independent results ---- */

/* The core's load at ka driven with v = cos(w t), as the complex impedance over rho c (real, imag)
 * from its steady response; a fine step keeps the bilinear transform's warping negligible. */
static void load_at(double ka, double radius, double c, double *zr, double *zi, double *work_per_s)
{
    const double rho = 1.2, f = ka * c / (2.0 * PI * radius), dt = 1.0 / 768000.0;
    rb_radiation r;
    rb_radiation_init(&r, radius, rho, c);
    const int settle = (int)(0.02 / dt), cycles = (int)ceil(0.05 * f);
    const int n = (int)lround(cycles / (f * dt));
    double re = 0.0, im = 0.0, work = 0.0;
    for (int k = 0; k < settle + n; k++) {
        const double phase = 2.0 * PI * f * dt * k, v = cos(phase);
        double direct, history;
        rb_radiation_linear(&r, dt, &direct, &history);
        const double p = rb_radiation_step(&r, v);
        if (k >= settle) {
            re += p * cos(phase);
            im -= p * sin(phase);
            work += p * v;
        }
    }
    *zr = 2.0 * re / n / (rho * c);
    *zi = 2.0 * im / n / (rho * c);
    *work_per_s = work / n * PI * radius * radius;   /* mean p v S: the power taken from the pipe */
}

void test_radiation_matches_an_unflanged_pipe(void)
{
    /* The core's load is Silva et al.'s (2009) rational fit of Levine and Schwinger's unflanged
     * pipe. Checked here against a different fit of the same exact result, Norris and Sheng's
     * (1989), good to about 1 % below ka 3.8: the reflection's magnitude
     *   |R| = (1 + 0.2 ka - 0.084 ka^2) / (1 + 0.2 ka + 0.416 ka^2)
     * and the end correction
     *   l / a = 0.6133 (1 + 0.044 ka^2) / (1 + 0.19 ka^2) - 0.02 sin^2(2 ka),
     * with R = (Z - 1) / (Z + 1) = -|R| exp(-2 i k l), over the plane wave's range (ka under
     * 1.84, the first cross mode) as the engine's tailpipes use it, ka 0.02 to 1.5. And their
     * low end, exact: |R| -> 1 - ka^2 / 2, l -> 0.6133 a. */
    const double radius = 0.025, c = 343.0;
    const double kas[] = { 0.02, 0.05, 0.1, 0.2, 0.3, 0.5, 0.7, 1.0, 1.2, 1.5 };
    double worst_r = 0.0, worst_l = 0.0;
    for (size_t j = 0; j < sizeof kas / sizeof kas[0]; j++) {
        const double ka = kas[j];
        double zr, zi, work;
        load_at(ka, radius, c, &zr, &zi, &work);
        /* R = (Z - 1) / (Z + 1), complex */
        const double nr = zr - 1.0, ni = zi, dr = zr + 1.0, di = zi, den = dr * dr + di * di;
        const double rr = (nr * dr + ni * di) / den, ri = (ni * dr - nr * di) / den;
        const double mag = sqrt(rr * rr + ri * ri);
        double arg = atan2(-ri, -rr);          /* -R = |R| exp(-2 i k l) */
        if (arg > 0.0) arg -= 2.0 * PI;
        const double l_over_a = -arg / (2.0 * ka);
        const double ns_r = (1.0 + 0.2 * ka - 0.084 * ka * ka) / (1.0 + 0.2 * ka + 0.416 * ka * ka);
        const double s = sin(2.0 * ka);
        const double ns_l = 0.6133 * (1.0 + 0.044 * ka * ka) / (1.0 + 0.19 * ka * ka) - 0.02 * s * s;
        printf("        ka %.2f: |R| %.4f (Norris-Sheng %.4f), l/a %.4f (%.4f)\n", ka, mag, ns_r, l_over_a, ns_l);
        worst_r = fmax(worst_r, fabs(mag - ns_r));
        worst_l = fmax(worst_l, fabs(l_over_a - ns_l));
        if (ka <= 0.05) {
            CHECK_NEAR(mag, 1.0 - 0.5 * ka * ka, 0.02 * ka * ka);
            CHECK_NEAR(l_over_a, 0.6133, 0.005);
        }
        CHECK(mag < 1.0);   /* passive: it takes energy, never gives it */
    }
    printf("        worst: |R| %.4f, l/a %.4f\n", worst_r, worst_l);
    CHECK(worst_r < 0.03);
    CHECK(worst_l < 0.03);
}

void test_radiation_takes_what_a_monopole_radiates(void)
{
    /* At low ka the open end is a monopole of the volume flow Q = S v: it radiates
     * rho w^2 |Q|^2 / (8 pi c) into the whole space. The power the load takes from the pipe
     * (its real part's work, the mean of p v S) must be that, and no more: the reactive part
     * (the end's mass of air) stores and gives back, so it does no work over a cycle. The
     * listener hears that same monopole (test_audio_listener_monopole, rho / (4 pi r) dQ/dt):
     * the load and the far field are one account, not two. */
    const double radius = 0.025, c = 343.0, rho = 1.2;
    const double kas[] = { 0.02, 0.05, 0.1 };
    for (size_t j = 0; j < sizeof kas / sizeof kas[0]; j++) {
        const double ka = kas[j], w = ka * c / radius, s = PI * radius * radius;
        double zr, zi, work;
        load_at(ka, radius, c, &zr, &zi, &work);
        const double monopole = rho * w * w * s * s / (8.0 * PI * c);   /* |Q| = S at v = cos */
        printf("        ka %.2f: the load takes %.6g W, a monopole radiates %.6g W\n", ka, work, monopole);
        CHECK_NEAR(work / monopole, 1.0, 0.6 * ka * ka + 0.002);
        CHECK_NEAR(work, 0.5 * zr * rho * c * s, 1e-6 * monopole);   /* only the real part works */
    }
}
