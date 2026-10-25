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
