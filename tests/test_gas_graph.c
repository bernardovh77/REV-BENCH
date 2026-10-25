#include "rb_test.h"
#include "gas_graph.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static rb_gg_spec spec(double reflection)
{return (rb_gg_spec){.rate=192000,.length=.250317,.interface_x=.08013,.area=.003,
    .pressure=101325,.temperature=300,.y={.1,.001,.899},.reflection=reflection,.gas_cells=40};}
typedef struct {rb_mix_cell *cells;double *delays;} memory;
static memory prepare(rb_gg_state *s,rb_gg_spec p)
{
    size_t cells=0,doubles=0;CHECK_EQ_INT(rb_gg_sizes(&p,&cells,&doubles),0);
    memory m={calloc(cells,sizeof *m.cells),calloc(doubles,sizeof *m.delays)};
    CHECK(m.cells&&m.delays);CHECK_EQ_INT(rb_gg_prepare(s,&p,m.cells,cells,m.delays,doubles),0);return m;
}
static void release(memory m){free(m.cells);free(m.delays);}
void test_gas_graph_prepare_and_inventory(void)
{
    rb_gg_state s;rb_gg_spec p=spec(0);memory m=prepare(&s,p);
    double length=0;for(int i=0;i<s.cells;i++)length+=rb_gg_cell_width(&s,i);
    CHECK_NEAR(length,p.length,1e-14);CHECK(s.fraction>0&&s.fraction<1);
    rb_mix_cell a=rb_gg_totals(&s,0),right_total=rb_gg_totals(&s,1);
    for(int j=0;j<5;j++)CHECK_NEAR(a.v[j]+right_total.v[j],s.base.v[j]*p.area*p.length,1e-12);
    size_t cells,doubles;rb_gg_sizes(&p,&cells,&doubles);
    CHECK_EQ_INT(rb_gg_prepare(&s,&p,m.cells,cells-1,m.delays,doubles),RB_GG_CAPACITY);
    CHECK_EQ_INT(rb_gg_prepare(&s,&p,m.cells,cells,m.delays,doubles-1),RB_GG_CAPACITY);
    rb_gg_spec bad=p;bad.interface_x=p.length-.0001;
    CHECK_EQ_INT(rb_gg_sizes(&bad,&cells,&doubles),RB_GG_DOMAIN);
    bad=p;bad.y[0]=-1;CHECK_EQ_INT(rb_gg_sizes(&bad,&cells,&doubles),RB_GG_ARGUMENT);
    CHECK_EQ_INT(rb_gg_seed(&s,.001,.025,.006,0),0);CHECK_EQ_INT(rb_gg_step(&s),0);
    CHECK_EQ_INT(rb_gg_seed(&s,.001,.025,.006,0),RB_GG_ARGUMENT);
    release(m);
    /* Integer and nearly integer delays must preserve physical geometry and
     * signed gas inventory; no explicit vanishing-width cell is introduced. */
    const double fractions[]={0,1e-7,.5,1-1e-7};
    for(int k=0;k<4;k++) {
        p=spec(0);p.length=p.interface_x+(32+fractions[k])*s.c/p.rate;
        m=prepare(&s,p);CHECK_EQ_INT(rb_gg_seed(&s,.001,.025,.006,0),0);
        int status=0;for(int tick=0;tick<160;tick++)if((status=rb_gg_step(&s)))break;
        CHECK_EQ_INT(status,0);
        length=0;for(int i=0;i<s.cells;i++)length+=rb_gg_cell_width(&s,i);
        CHECK_NEAR(length,p.length,1e-13);
        for(int side=0;side<2;side++) {
            rb_mix_cell b=rb_gg_balance(&s,side);
            for(int j=0;j<5;j++)CHECK_NEAR(b.v[j],0,j==4 ? 1e-8:1e-11);
        }
        release(m);
    }
}
void test_gas_graph_fractional_flux_and_clock(void)
{
    for(int r=-1;r<=1;r++) {
        rb_gg_state s;rb_gg_spec p=spec(r);memory m=prepare(&s,p);
        CHECK_EQ_INT(rb_gg_seed(&s,.001,.025,.006,0),0);
        int status=0,returns=0;double moved=0;
        for(int tick=0;tick<430;tick++) {
            if((status=rb_gg_step(&s)))break;
            CHECK(s.received_return==s.used_return);
            if(fabs(s.used_return)>1e-3)returns++;
            moved=fmax(moved,fabs(s.interface_transfer.v[0]));
            if(tick%71==0)for(int side=0;side<2;side++) {
                rb_mix_cell b=rb_gg_balance(&s,side);
                for(int j=0;j<5;j++) {
                    double scale=j==3 ? s.rho*s.c*p.area*p.length:
                        (j==4 ? s.rho*s.c*s.c*p.area*p.length:s.rho*p.area*p.length);
                    CHECK_NEAR(b.v[j],0,1e-10*scale);
                }
                CHECK_NEAR(rb_ac_balance(&s.graph)+s.coordinate_work,0,1e-12);
            }
        }
        CHECK_EQ_INT(status,0);CHECK(moved>1e-10);if(r)CHECK(returns>0);
        CHECK_EQ_INT(s.failures,0);release(m);
    }
}
void test_gas_graph_contact_and_physical_pressure(void)
{
    rb_gg_state s;rb_gg_spec p=spec(0);memory m=prepare(&s,p);
    rb_mix_cell *initial=calloc((size_t)s.cells,sizeof *initial);CHECK(initial!=NULL);
    const double patch[]={.099,.001,.9};
    for(int i=0;i<s.cells;i++) {
        double x=rb_gg_cell_center(&s,i);
        rb_mix_make(p.pressure,p.temperature,0,x>.06&&x<.12 ? patch:p.y,&initial[i]);
    }
    CHECK_EQ_INT(rb_gg_load(&s,initial,s.cells),0);
    for(int t=0;t<40;t++)CHECK_EQ_INT(rb_gg_step(&s),0);
    for(int i=0;i<s.cells;i++)CHECK_NEAR(rb_gg_pressure(&s,i),p.pressure,1e-6);
    for(int i=0;i<s.cells;i++)for(int j=0;j<3;j++)CHECK_NEAR(s.q[i].v[j],initial[i].v[j],1e-10);
    rb_mix_cell balance=rb_gg_balance(&s,1);
    for(int j=0;j<5;j++)CHECK_NEAR(balance.v[j],0,1e-8);
    release(m);free(initial);
    /* Physical pressure needs kinetic energy removed; a+b is only the
     * linearized conserved-state pressure, even when the EOS is real code. */
    m=prepare(&s,p);CHECK_EQ_INT(rb_gg_seed(&s,.01,.025,.006,0),0);
    rb_mix_cell q=s.q[12];rb_mix_primitive w;CHECK_EQ_INT(rb_mix_decode(q,&w),0);
    CHECK_NEAR(rb_gg_pressure(&s,12),w.p,1e-8);
    double pi=s.beta*(q.v[4]-s.base.v[4]);
    for(int j=0;j<3;j++)pi+=s.coefficient[j]*(q.v[j]-s.base.v[j]);
    CHECK(fabs(w.p-p.pressure-pi)>.001);release(m);
}
void test_gas_graph_tagged_pulse_transport(void)
{
    rb_gg_state s;rb_gg_spec p=spec(0);memory m=prepare(&s,p);
    CHECK_EQ_INT(rb_gg_seed(&s,.003,.025,.006,0),0);
    rb_mix_cell *initial=calloc((size_t)s.cells,sizeof *initial);CHECK(initial!=NULL);
    memcpy(initial,s.q,(size_t)s.cells*sizeof *initial);
    const double patch[]={.099,.0015,.8995};
    for(int i=0;i<s.cells;i++) {
        double x=rb_gg_cell_center(&s,i);
        if(x>.06 && x<.12)rb_mix_make(p.pressure,p.temperature,0,patch,&initial[i]);
    }
    CHECK_EQ_INT(rb_gg_load(&s,initial,s.cells),0);
    int status=0;double mismatch=0;
    for(int k=0;k<300;k++) {
        if((status=rb_gg_step(&s)))break;
        double mass=s.interface_transfer.v[0]+s.interface_transfer.v[1]+s.interface_transfer.v[2];
        mismatch=fmax(mismatch,fabs(s.interface_transfer.v[1]-p.y[1]*mass));
    }
    CHECK_EQ_INT(status,0);CHECK(mismatch>1e-13);
    for(int side=0;side<2;side++) {
        rb_mix_cell b=rb_gg_balance(&s,side);
        for(int j=0;j<5;j++)CHECK_NEAR(b.v[j],0,j==4 ? 1e-8:1e-11);
    }
    free(initial);release(m);
}
void test_gas_graph_domain_failure_requires_prepare(void)
{
    rb_gg_state s;rb_gg_spec p=spec(0);memory m=prepare(&s,p);
    CHECK_EQ_INT(rb_gg_seed(&s,.1,.025,.006,0),RB_GG_DOMAIN);
    CHECK(s.failures>0);CHECK_EQ_INT(rb_gg_step(&s),RB_GG_STATE);
    CHECK(isnan(rb_gg_pressure(&s,0)));release(m);
}
/* 046 uniform signed flow: actual port transfer, not a helper-only formula. */
void test_gas_graph_mean_flux_and_energy(void)
{
    for(int sign=-1;sign<=1;sign+=2) {
        rb_gg_state s;rb_gg_spec p=spec(1);p.mean_flow=70*sign;memory m=prepare(&s,p);
        CHECK(s.graph.line[0].count!=s.graph.line[1].count);
        CHECK_NEAR((s.downstream_cells+s.fraction)*s.dx,p.length-p.interface_x,1e-14);
        CHECK_NEAR((s.back_cells+s.back_fraction)*s.back_dx,p.length-p.interface_x,1e-14);
        rb_mix_cell *initial=calloc((size_t)s.cells,sizeof *initial);CHECK(initial!=NULL);
        for(int i=0;i<s.cells;i++)initial[i]=s.base;
        const double eps=.1,H=s.h0+.5*p.mean_flow*p.mean_flow;
        for(int i=0;i<p.gas_cells;i++) {
            for(int j=0;j<3;j++)initial[i].v[j]+=p.y[j]*eps/(s.c*s.c);
            initial[i].v[3]+=eps*(p.mean_flow/(s.c*s.c)+1/s.c);
            initial[i].v[4]+=eps*(H/(s.c*s.c)+p.mean_flow/s.c);
        }
        CHECK_EQ_INT(rb_gg_load(&s,initial,s.cells),0);free(initial);
        CHECK_EQ_INT(rb_gg_step(&s),0);
        double delta_mass=0;for(int j=0;j<3;j++)delta_mass+=s.last_linear.v[j]*p.rate/p.area-s.base_flux.v[j];
        const double expected=((1+p.mean_flow/s.c)*s.accepted_outgoing-(1-p.mean_flow/s.c)*s.used_return)/s.c;
        CHECK_NEAR(delta_mass,expected,2e-8);
        const double quiescent=(s.accepted_outgoing-s.used_return)/s.c;
        CHECK(fabs(delta_mass-quiescent)>fabs(expected)*.1);
        int status=0;double maxwork=0;
        for(int tick=0;tick<430;tick++) {
            if((status=rb_gg_step(&s)))break;
            CHECK_NEAR(s.received_return,s.used_return,0);
            maxwork=fmax(maxwork,fabs(rb_gg_acoustic_balance(&s)));
            /* End-only books miss a removed returning wave after it has left.
             * Inspect both physical inventories throughout its transit. */
            if(tick%17==0)for(int side=0;side<2;side++) {
                rb_mix_cell b=rb_gg_balance(&s,side);
                for(int j=0;j<5;j++)CHECK_NEAR(b.v[j],0,j==4 ? 1e-8:1e-11);
            }
        }
        CHECK_EQ_INT(status,0);CHECK(maxwork<1e-15);
        for(int side=0;side<2;side++) {
            rb_mix_cell b=rb_gg_balance(&s,side);
            for(int j=0;j<5;j++)CHECK_NEAR(b.v[j],0,j==4 ? 1e-8:1e-11);
        }
        release(m);
    }
}
void test_gas_graph_advected_contact(void)
{
    for(int sign=-1;sign<=1;sign+=2) {
        rb_gg_state s;rb_gg_spec p=spec(0);p.mean_flow=70*sign;memory m=prepare(&s,p);
        rb_mix_cell patch;const double y[]={.099,.0015,.8995};
        rb_mix_make(p.pressure,p.temperature,p.mean_flow,y,&patch);
        rb_mix_cell *initial=calloc((size_t)s.cells,sizeof *initial);CHECK(initial!=NULL);
        const double lo=p.interface_x+(sign>0 ? -.04:.01),hi=lo+.03;
        for(int i=0;i<s.cells;i++) {
            const double dx=rb_gg_cell_width(&s,i),x=rb_gg_cell_center(&s,i);
            double fraction=fmax(0,fmin(x+.5*dx,hi)-fmax(x-.5*dx,lo))/dx;
            for(int j=0;j<5;j++)initial[i].v[j]=s.base.v[j]+fraction*(patch.v[j]-s.base.v[j]);
        }
        CHECK_EQ_INT(rb_gg_load(&s,initial,s.cells),0);free(initial);
        int status=0;for(int tick=0;tick<90;tick++)if((status=rb_gg_step(&s)))break;
        CHECK_EQ_INT(status,0);rb_gg_totals(&s,1);
        const double time=s.steps/p.rate,passed=fmax(0,fmin(.03,70*time-.01));
        for(int j=0;j<3;j++) {
            const double expected=sign*p.area*passed*(patch.v[j]-s.base.v[j]);
            double excess=s.interface_transfer.v[j]-p.area*time*s.base_flux.v[j];
            CHECK_NEAR(excess,expected,.05*fabs(expected));
        }
        for(int i=0;i<s.cells;i++) {
            rb_mix_primitive w;CHECK_EQ_INT(rb_mix_decode(s.q[i],&w),0);
            CHECK_NEAR(w.p,p.pressure,1);CHECK_NEAR(w.u,p.mean_flow,.001);
        }
        for(int side=0;side<2;side++) {
            rb_mix_cell b=rb_gg_balance(&s,side);
            for(int j=0;j<5;j++)CHECK_NEAR(b.v[j],0,j==4 ? 1e-8:1e-11);
        }
        release(m);
    }
    rb_gg_spec p=spec(0);p.mean_flow=1000;size_t a,b;
    CHECK_EQ_INT(rb_gg_sizes(&p,&a,&b),RB_GG_DOMAIN);
}
void test_gas_graph_entropy_advection(void)
{
    /* Independent entropy contact: same composition and pressure, +1 mK.
     * This weak frozen-reference test is not sharp thermal-front qualification. */
    for(int sign=-1;sign<=1;sign+=2) {
        rb_gg_state s;rb_gg_spec p=spec(0);p.mean_flow=70*sign;memory m=prepare(&s,p);
        rb_mix_cell patch;rb_mix_make(p.pressure,p.temperature+.001,p.mean_flow,p.y,&patch);
        rb_mix_cell *initial=calloc((size_t)s.cells,sizeof *initial);CHECK(initial!=NULL);
        const double lo=p.interface_x+(sign>0 ? -.04:.01),hi=lo+.03;
        for(int i=0;i<s.cells;i++) {
            const double dx=rb_gg_cell_width(&s,i),x=rb_gg_cell_center(&s,i);
            const double f=fmax(0,fmin(x+.5*dx,hi)-fmax(x-.5*dx,lo))/dx;
            for(int j=0;j<5;j++)initial[i].v[j]=s.base.v[j]+f*(patch.v[j]-s.base.v[j]);
        }
        CHECK_EQ_INT(rb_gg_load(&s,initial,s.cells),0);free(initial);
        int status=0;for(int tick=0;tick<90;tick++)if((status=rb_gg_step(&s)))break;
        CHECK_EQ_INT(status,0);rb_gg_totals(&s,1);
        double time=s.steps/p.rate,passed=fmax(0,fmin(.03,70*time-.01));
        for(int j=0;j<5;j++) {
            const double expected=sign*p.area*passed*(patch.v[j]-s.base.v[j]);
            const double excess=s.interface_transfer.v[j]-p.area*time*s.base_flux.v[j];
            CHECK_NEAR(excess,expected,.05*fabs(expected));
        }
        for(int i=0;i<s.cells;i++) {
            rb_mix_primitive w;CHECK_EQ_INT(rb_mix_decode(s.q[i],&w),0);
            CHECK_NEAR(w.p,p.pressure,1);CHECK_NEAR(w.u,p.mean_flow,.001);
        }
        for(int side=0;side<2;side++) {
            rb_mix_cell b=rb_gg_balance(&s,side);
            for(int j=0;j<5;j++)CHECK_NEAR(b.v[j],0,j==4 ? 1e-8:1e-11);
        }
        release(m);
    }
}
