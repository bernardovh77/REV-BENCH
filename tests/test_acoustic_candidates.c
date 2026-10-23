#include "rb_test.h"
#include "wave.h"
#include "euler.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846
static const double AREA=.0012566370614359173;

static double *prepare(rb_ac_plan *p, rb_ac_state *s)
{
    const size_t n=rb_ac_samples(p);
    double *storage=calloc(n ? n:1,sizeof(double));
    CHECK(storage != NULL);
    CHECK(n > 0);
    CHECK_EQ_INT(rb_ac_prepare(p,s,storage,n),RB_AC_OK);
    return storage;
}
static void chain(rb_ac_plan *p, double fraction, int interpolation, double reflection)
{
    CHECK_EQ_INT(rb_ac_init(p,48000,1.2,343),RB_AC_OK);
    CHECK_EQ_INT(rb_ac_add_node(p,RB_AC_BOUNDARY,1),0);
    CHECK_EQ_INT(rb_ac_add_node(p,RB_AC_BOUNDARY,reflection),1);
    CHECK_EQ_INT(rb_ac_add_edge(p,0,1,(64+fraction)*343/48000,AREA,1,interpolation),0);
}

void test_candidate_current_tick_interface(void)
{
    rb_ac_plan p; rb_ac_state full, phased;
    chain(&p,.37,RB_AC_ALLPASS,-1);
    double *one=prepare(&p,&full), *two=prepare(&p,&phased);
    rb_ac_drive drive={0};
    CHECK_EQ_INT(rb_ac_commit(&phased,&drive),RB_AC_ORDER);
    for (int tick=0;tick<300;tick++) {
        drive.wave[0]=tick==0 ? 10:0;
        CHECK_EQ_INT(rb_ac_receive(&phased),RB_AC_OK);
        CHECK_EQ_INT(rb_ac_receive(&phased),RB_AC_ORDER);
        const double arrival=rb_ac_incident(&phased,0);
        CHECK(isfinite(arrival));
        if (tick==128) CHECK(arrival < -1);
        if (tick==20) {
            drive.wave[0]=NAN;
            CHECK_EQ_INT(rb_ac_commit(&phased,&drive),RB_AC_ARGUMENT);
            CHECK(rb_ac_incident(&phased,0)==arrival);
            drive.wave[0]=0;
        }
        CHECK_EQ_INT(rb_ac_commit(&phased,&drive),RB_AC_OK);
        CHECK_EQ_INT(rb_ac_step(&full,&drive),RB_AC_OK);
        CHECK(phased.pressure[0]==full.pressure[0]);
        CHECK(rb_ac_energy(&phased)==rb_ac_energy(&full));
    }
    free(one); free(two);
}

void test_candidate_wave_guards(void)
{
    rb_ac_plan p; rb_ac_state s;
    CHECK_EQ_INT(rb_ac_init(&p,NAN,1.2,343),RB_AC_ARGUMENT);
    chain(&p,0,RB_AC_ALLPASS,-1);
    CHECK_EQ_INT(rb_ac_add_node(&p,RB_AC_BOUNDARY,1.01),RB_AC_ARGUMENT);
    CHECK_EQ_INT(rb_ac_add_edge(&p,0,1,.5,AREA,1.01,RB_AC_ALLPASS),RB_AC_ARGUMENT);
    const size_t n=rb_ac_samples(&p);
    double *storage=calloc(n,sizeof(double));
    CHECK_EQ_INT(rb_ac_prepare(&p,&s,storage,n-1),RB_AC_CAPACITY);
    p.edge[0].length=.001;
    CHECK_EQ_INT(rb_ac_prepare(&p,&s,storage,n),RB_AC_SHORT_DELAY);
    p.edge[0].length=.5;
    p.node[1].kind=RB_AC_RADIATION; p.node[1].radius=.03;
    CHECK_EQ_INT(rb_ac_prepare(&p,&s,storage,n),RB_AC_TOPOLOGY);
    p.node[1].kind=RB_AC_BOUNDARY;
    p.edge[0].a=20;
    CHECK_EQ_INT(rb_ac_prepare(&p,&s,storage,n),RB_AC_ARGUMENT);
    free(storage);
}

void test_candidate_wave_delay_and_return(void)
{
    rb_ac_plan p; rb_ac_state s;
    chain(&p,0,RB_AC_ALLPASS,-1);
    p.node[0].reflection=0;
    double *storage=prepare(&p,&s);
    rb_ac_drive d={0};
    double at_end=0,returned=0;
    int status=0;
    for (int i=0;i<256;i++) {
        d.wave[0]=i == 0 ? 1:0;
        status|=rb_ac_step(&s,&d);
        if (i == 64) {at_end=s.incoming[1]; CHECK_NEAR(s.outgoing[1],-1,1e-14);}
        if (i == 128) returned=s.incoming[0];
    }
    CHECK_EQ_INT(status,0);
    CHECK_NEAR(at_end,1,1e-14);
    CHECK_NEAR(returned,-1,1e-14);
    CHECK_NEAR(rb_ac_energy(&s),0,1e-20);
    CHECK_NEAR(s.terminal_work,s.source_work,1e-20);
    CHECK_NEAR(rb_ac_balance(&s),0,1e-20);
    free(storage);
}

void test_candidate_wave_four_port(void)
{
    rb_ac_plan p; rb_ac_state s;
    rb_ac_init(&p,48000,1.2,343); rb_ac_add_node(&p,RB_AC_JUNCTION,0);
    for (int j=0;j<4;j++) {
        rb_ac_add_node(&p,RB_AC_BOUNDARY,0);
        rb_ac_add_edge(&p,j+1,0,16*343.0/48000,AREA,1,RB_AC_ALLPASS);
    }
    double *storage=prepare(&p,&s);
    rb_ac_drive d={0};
    int status=0;
    for (int i=0;i<80;i++) {
        d.wave[1]=i == 0 ? 1:0;
        status|=rb_ac_step(&s,&d);
        if (i == 16) {
            CHECK_NEAR(s.pressure[0],.5,1e-14);
            CHECK_NEAR(s.outgoing[1],-.5,1e-14);
            for (int k=1;k<4;k++) CHECK_NEAR(s.outgoing[2*k+1],.5,1e-14);
        }
        if (i == 32) {
            CHECK_NEAR(s.incoming[0],-.5,1e-14);
            for (int k=1;k<4;k++) CHECK_NEAR(s.incoming[2*k],.5,1e-14);
        }
    }
    CHECK_EQ_INT(status,0);
    CHECK_NEAR(rb_ac_balance(&s),0,1e-20);
    free(storage);
}

void test_candidate_wave_unequal_and_cancel(void)
{
    rb_ac_plan p; rb_ac_state s;
    rb_ac_init(&p,48000,1.2,343); rb_ac_add_node(&p,RB_AC_JUNCTION,0);
    const double areas[]={1,.5,2,3};
    for (int j=0;j<4;j++) {
        rb_ac_add_node(&p,RB_AC_BOUNDARY,0);
        rb_ac_add_edge(&p,j+1,0,16*343.0/48000,AREA*areas[j],1,RB_AC_ALLPASS);
    }
    double *storage=prepare(&p,&s);
    rb_ac_drive d={0};
    for (int i=0;i<=16;i++) {d.wave[1]=i == 0 ? 1:0; rb_ac_step(&s,&d);}
    CHECK_NEAR(s.pressure[0],2/6.5,1e-14);
    CHECK_NEAR(rb_ac_balance(&s),0,1e-20);
    /* Equal and opposite weighted inputs cancel every undriven branch. */
    rb_ac_prepare(&p,&s,storage,rb_ac_samples(&p));
    for (int i=0;i<=16;i++) {
        d.wave[1]=i == 0 ? 1:0; d.wave[2]=i == 0 ? -2:0;
        rb_ac_step(&s,&d);
    }
    CHECK_NEAR(s.pressure[0],0,1e-14);
    CHECK_NEAR(s.outgoing[5],0,1e-14);
    CHECK_NEAR(s.outgoing[7],0,1e-14);
    free(storage);
}

void test_candidate_wave_fractional_passivity(void)
{
    for (int interpolation=0;interpolation<=1;interpolation++) {
        rb_ac_plan p; rb_ac_state s;
        chain(&p,.37,interpolation,-1);
        double *storage=prepare(&p,&s);
        rb_ac_drive d={0};
        double max_ratio=0;
        int status=0;
        for (int i=0;i<48000;i++) {
            d.wave[0]=i == 0 ? 1:0;
            status=rb_ac_step(&s,&d);
            if (status) break;
            max_ratio=fmax(max_ratio,rb_ac_energy(&s)/s.source_work);
        }
        CHECK_EQ_INT(status,0);
        CHECK(max_ratio <= 1+1e-10);
        CHECK_NEAR(rb_ac_balance(&s)/s.source_work,0,1e-9);
        if (interpolation == RB_AC_ALLPASS) {
            CHECK_NEAR(rb_ac_energy(&s)/s.source_work,1,1e-9);
            CHECK_NEAR(s.numerical_loss,0,0);
        } else CHECK(s.numerical_loss > .9*s.source_work);
        free(storage);
    }
}

void test_candidate_wave_cavity_and_source_work(void)
{
    rb_ac_plan p; rb_ac_state s;
    rb_ac_init(&p,48000,1.2,343);
    rb_ac_add_node(&p,RB_AC_JUNCTION,.002);
    rb_ac_add_node(&p,RB_AC_BOUNDARY,1);
    rb_ac_add_node(&p,RB_AC_BOUNDARY,1);
    rb_ac_add_edge(&p,0,1,.6,AREA,1,RB_AC_ALLPASS);
    rb_ac_add_edge(&p,0,2,.8,AREA,1,RB_AC_ALLPASS);
    double *storage=prepare(&p,&s);
    rb_ac_drive d={0};
    int status=0;
    for (int i=0;i<12000;i++) {
        d.flow[0]=i < 100 ? 1e-5*sin(PI*i/100):0;
        status=rb_ac_step(&s,&d);
        if (status) break;
    }
    CHECK_EQ_INT(status,0);
    CHECK(s.source_work > 0);
    CHECK_NEAR(rb_ac_balance(&s)/s.source_work,0,1e-9);
    CHECK_NEAR(rb_ac_energy(&s)/s.source_work,1,1e-9);
    free(storage);
}

void test_candidate_wave_radiation(void)
{
    rb_ac_plan p; rb_ac_state s;
    rb_ac_init(&p,48000,1.2,343);
    rb_ac_add_node(&p,RB_AC_BOUNDARY,0);
    rb_ac_add_node(&p,RB_AC_RADIATION,.02);
    rb_ac_add_edge(&p,0,1,.75,AREA,1,RB_AC_ALLPASS);
    double *storage=prepare(&p,&s);
    rb_ac_drive d={0};
    double minimum_terminal=0,load_error=0,velocity_error=0;
    int status=0;
    for (int i=0;i<12000;i++) {
        d.wave[0]=i < 200 ? sin(PI*i/200)*sin(2*PI*2000*i/48000):0;
        status=rb_ac_step(&s,&d);
        if (status) break;
        minimum_terminal=fmin(minimum_terminal,s.terminal_work);
        load_error=fmax(load_error,fabs(s.pressure[1]-s.radiation[1].pressure));
        velocity_error=fmax(velocity_error,fabs(rb_ac_node_flow(&s,1)/AREA-s.radiation[1].v1));
    }
    CHECK_EQ_INT(status,0);
    CHECK(s.source_work > 0);
    CHECK(minimum_terminal >= -1e-15);
    CHECK(s.terminal_work > 0);
    CHECK(load_error < 1e-13);
    CHECK(velocity_error < 1e-13);
    CHECK_NEAR(rb_ac_balance(&s)/s.source_work,0,1e-9);
    CHECK(fabs(rb_ac_node_flow(&s,1)) < 1e-20);
    free(storage);
}

void test_candidate_wave_cavity_transfer(void)
{
    const double frequency[]={200,800,2000,4000};
    for(int f=0;f<4;f++) {
        rb_ac_plan p; rb_ac_state s;
        rb_ac_init(&p,48000,1.2,343);
        rb_ac_add_node(&p,RB_AC_BOUNDARY,0);
        rb_ac_add_node(&p,RB_AC_JUNCTION,.0002);
        rb_ac_add_node(&p,RB_AC_BOUNDARY,0);
        rb_ac_add_edge(&p,0,1,16*343.0/48000,AREA,1,RB_AC_ALLPASS);
        rb_ac_add_edge(&p,1,2,16*343.0/48000,AREA,1,RB_AC_ALLPASS);
        double *storage=prepare(&p,&s);
        rb_ac_drive d={0};
        double re=0,im=0;
        int status=0;
        for(int i=0;i<9600;i++) {
            const double angle=2*PI*frequency[f]*i/48000;
            d.wave[0]=sin(angle);
            status|=rb_ac_step(&s,&d);
            if(i>=4800) {re+=s.pressure[2]*sin(angle); im+=s.pressure[2]*cos(angle);}
        }
        const double omega_d=2*48000*tan(PI*frequency[f]/48000);
        const double compliance=.0002/(1.2*343*343),admittance=AREA/(1.2*343);
        const double ratio=omega_d*compliance/(2*admittance);
        const double expected_gain=1/sqrt(1+ratio*ratio);
        const double expected_phase=-atan(ratio)-2*PI*frequency[f]*32/48000;
        CHECK_EQ_INT(status,0);
        CHECK_NEAR(hypot(re,im)/2400,expected_gain,1e-12);
        CHECK_NEAR(remainder(atan2(im,re)-expected_phase,2*PI),0,1e-11);
        free(storage);
    }
}

void test_candidate_wave_loss_budget(void)
{
    rb_ac_plan p; rb_ac_state s;
    chain(&p,.37,RB_AC_ALLPASS,-1);
    p.edge[0].gain=.98;
    double *storage=prepare(&p,&s);
    rb_ac_drive d={0};
    int status=0;
    for(int i=0;i<12000;i++) {
        d.wave[0]=i == 0 ? 1:0;
        status=rb_ac_step(&s,&d);
        if(status) break;
    }
    CHECK_EQ_INT(status,0);
    CHECK(s.propagation_loss > .99*s.source_work);
    CHECK_NEAR(rb_ac_balance(&s)/s.source_work,0,1e-9);
    CHECK_NEAR(s.numerical_loss,0,0);
    free(storage);
}

static rb_fv_cell *fv_prepare(rb_fv_state *s, int n, double dx, int periodic)
{
    rb_fv_cell *storage=calloc(rb_fv_cells_required(n),sizeof *storage);
    CHECK(storage != NULL);
    CHECK_EQ_INT(rb_fv_prepare(s,n,dx,1.4,periodic,storage,rb_fv_cells_required(n)),0);
    return storage;
}
static void totals(const rb_fv_state *s, double out[4])
{
    long double sum[4]={0};
    for (int i=0;i<s->cells;i++) {
        sum[0]+=s->q[i].rho; sum[1]+=s->q[i].momentum;
        sum[2]+=s->q[i].energy; sum[3]+=s->q[i].tag;
    }
    for (int k=0;k<4;k++) out[k]=(double)(sum[k]*s->dx);
}

void test_candidate_fv_uniform_and_guards(void)
{
    rb_fv_state s;
    rb_fv_cell *storage=fv_prepare(&s,32,.01,1);
    const rb_fv_cell initial=rb_fv_make(1.2,70,101325,.3,1.4);
    for (int i=0;i<s.cells;i++) s.q[i]=initial;
    int status=0;
    for (int i=0;i<100;i++) status|=rb_fv_step(&s,1.0/192000);
    CHECK_EQ_INT(status,0);
    for (int i=0;i<s.cells;i++) {
        CHECK_NEAR(s.q[i].rho,initial.rho,0);
        CHECK_NEAR(s.q[i].energy,initial.energy,0);
    }
    CHECK_EQ_INT(rb_fv_step(&s,.1),-1);
    CHECK_EQ_INT(s.failures,1);
    s.q[0].rho=-1;
    CHECK_EQ_INT(rb_fv_step(&s,1e-6),-1);
    CHECK_EQ_INT(rb_fv_prepare(&s,32,.01,1,1,storage,129),-1);
    free(storage);
}

void test_candidate_fv_stationary_contact(void)
{
    rb_fv_state s;
    rb_fv_cell *storage=fv_prepare(&s,64,1.0/64,1);
    for (int i=0;i<s.cells;i++) s.q[i]=rb_fv_make(i < 32 ? 1:.125,0,1,i < 32 ? .2:.8,1.4);
    int status=0;
    for (int t=0;t<100;t++) status|=rb_fv_step(&s,.001);
    CHECK_EQ_INT(status,0);
    for (int i=0;i<s.cells;i++) {
        CHECK_NEAR(s.q[i].rho,i < 32 ? 1:.125,1e-14);
        CHECK_NEAR(rb_fv_pressure(s.q[i],1.4),1,1e-14);
    }
    free(storage);
}

void test_candidate_fv_sod(void)
{
    rb_fv_state s;
    rb_fv_cell *storage=fv_prepare(&s,400,1.0/400,0);
    for (int i=0;i<s.cells;i++) s.q[i]=rb_fv_make(i < 200 ? 1:.125,0,i < 200 ? 1:.1,0,1.4);
    double time=0;
    int status=0,steps=0;
    while (time < .2 && steps++ < 5000) {
        double speed=0;
        for (int i=0;i<s.cells;i++) speed=fmax(speed,fabs(s.q[i].momentum/s.q[i].rho)+
            sqrt(1.4*rb_fv_pressure(s.q[i],1.4)/s.q[i].rho));
        const double dt=fmin(.3*s.dx/speed,.2-time);
        status=rb_fv_step(&s,dt);
        if (status) break;
        time+=dt;
    }
    CHECK_EQ_INT(status,0);
    CHECK_NEAR(time,.2,1e-14);
    const rb_fv_cell q=s.q[200];
    CHECK_NEAR(q.rho,.4263194,.012);
    CHECK_NEAR(q.momentum/q.rho,.9274526,.02);
    CHECK_NEAR(rb_fv_pressure(q,1.4),.3031302,.012);
    CHECK_EQ_INT(s.failures,0);
    free(storage);
}

void test_candidate_fv_packet_conservation(void)
{
    rb_fv_state s;
    rb_fv_cell *storage=fv_prepare(&s,512,.0125,1);
    const double rho=1.2,p0=101325,c=sqrt(1.4*p0/rho),frequency=800,k=2*PI*frequency/c;
    double re[2]={0},im[2]={0},before[4],after[4];
    for (int i=0;i<s.cells;i++) {
        const double x=(i+.5)*s.dx-1.6;
        const double dp=10*exp(-.5*x*x/(.4*.4))*cos(k*x);
        s.q[i]=rb_fv_make(rho*pow((p0+dp)/p0,1/1.4),dp/(rho*c),p0+dp,.2,1.4);
        re[0]+=dp*cos(k*x); im[0]-=dp*sin(k*x);
    }
    totals(&s,before);
    int status=0;
    for (int i=0;i<480;i++) {status=rb_fv_step(&s,1.0/192000); if(status) break;}
    totals(&s,after);
    for (int i=0;i<s.cells;i++) {
        const double x=(i+.5)*s.dx-1.6,dp=rb_fv_pressure(s.q[i],1.4)-p0;
        re[1]+=dp*cos(k*x); im[1]-=dp*sin(k*x);
    }
    const double phase=atan2(im[1]*re[0]-re[1]*im[0],re[1]*re[0]+im[1]*im[0]);
    const double error=remainder(phase+2*PI*frequency*.0025,2*PI)*180/PI;
    CHECK_EQ_INT(status,0);
    for (int j=0;j<4;j++) CHECK_NEAR(after[j],before[j],1e-11*fmax(1,fabs(before[j])));
    CHECK(fabs(error) < 5);
    CHECK_EQ_INT(s.failures,0);
    free(storage);
}

void test_candidate_fv_rarefaction(void)
{
    rb_fv_state s;
    rb_fv_cell *storage=fv_prepare(&s,200,1.0/200,0);
    for (int i=0;i<s.cells;i++) s.q[i]=rb_fv_make(1,i < 100 ? -2:2,.4,.3,1.4);
    int status=0;
    for (int i=0;i<400;i++) {status=rb_fv_step(&s,.0002); if(status) break;}
    CHECK_EQ_INT(status,0);
    CHECK(rb_fv_pressure(s.q[100],1.4) > 0);
    CHECK(rb_fv_pressure(s.q[100],1.4) < .05);
    CHECK_EQ_INT(s.failures,0);
    free(storage);
}
