#include "rb_test.h"
#include "coupled_duct.h"
#include <math.h>
#include <stdlib.h>

void test_coupled_duct_entropy_and_guards(void)
{
    rb_cd_spec p={.cells=64,.gas_cells=24,.rate=192000,.rho=1.2,
        .pressure=1.2*343*343/1.4,.gamma=1.4,.area=.003,.tag_fraction=.25,
        .length=64*343.0/192000,.reflection=1};
    rb_cd_state s;
    const size_t count=rb_cd_workspace(p.cells);
    rb_cd_vec *mem=calloc(count,sizeof *mem), input[64];
    CHECK_EQ_INT(rb_cd_prepare(&s,&p,mem,count-1),RB_CD_CAPACITY);
    p.length*=1.01;
    CHECK_EQ_INT(rb_cd_prepare(&s,&p,mem,count),RB_CD_GRID);
    p.length/=1.01;
    CHECK_EQ_INT(rb_cd_prepare(&s,&p,mem,count),RB_CD_OK);
    rb_cd_spec enormous=p;
    enormous.rho=1e-308; enormous.pressure=1e308;
    enormous.rate=1e308; enormous.length=1e308;
    CHECK_EQ_INT(rb_cd_prepare(&s,&enormous,mem,count),RB_CD_ARGUMENT);
    CHECK_EQ_INT(rb_cd_prepare(&s,&p,mem,count),RB_CD_OK);
    for(int i=0;i<64;i++) input[i]=(rb_cd_vec){{i<32 ? 1.1:1.3,0,
        p.pressure/(p.gamma-1),i<32 ? .1:.7}};
    CHECK_EQ_INT(rb_cd_load(&s,input,64),RB_CD_OK);
    for(int t=0;t<128;t++) CHECK_EQ_INT(rb_cd_step(&s),RB_CD_OK);
    for(int i=0;i<64;i++) {
        CHECK_NEAR(s.q[i].v[0],input[i].v[0],1e-13);
        CHECK_NEAR(s.q[i].v[3],input[i].v[3],1e-13);
        CHECK_NEAR(rb_cd_pressure(&s,i),p.pressure,1e-9);
    }
    free(mem);
}

void test_coupled_duct_moving_tagged_contact(void)
{
    const int n=256, steps=16;
    rb_cd_spec p={.cells=n,.gas_cells=128,.rate=192000,.rho=1.2,
        .pressure=1.2*343*343/1.4,.gamma=1.4,.area=.003,.tag_fraction=.25,
        .length=n*343.0/192000,.reflection=0};
    size_t count=rb_cd_workspace(n);
    rb_cd_vec *mem=calloc(count,sizeof *mem), *input=calloc(n,sizeof *input);
    rb_cd_state s;
    for(int direction=-1;direction<=1;direction+=2) {
        const double u=direction*.02*343, k=2*3.14159265358979323846/p.length;
        CHECK_EQ_INT(rb_cd_prepare(&s,&p,mem,count),0);
        for(int i=0;i<n;i++) {
            const double x=(i+.5)*p.length/n-p.length*.5;
            input[i]=(rb_cd_vec){{p.rho,p.rho*u,p.pressure/(p.gamma-1)+.5*p.rho*u*u,
                                 p.rho*(.25+.05*cos(k*x))}};
        }
        CHECK_EQ_INT(rb_cd_load(&s,input,n),0);
        for(int i=0;i<steps;i++) CHECK_EQ_INT(rb_cd_step(&s),0);
        const double t=steps/p.rate;
        const double expected=p.area*p.rho*u*(.25*t+.05*sin(k*u*t)/(k*u));
        CHECK_NEAR(s.interface_transfer.v[3],expected,fabs(expected)*.002);
        CHECK_NEAR(s.interface_transfer.v[0],p.area*p.rho*u*t,1e-12);
        CHECK_NEAR(rb_cd_pressure(&s,128),p.pressure,1e-7);
        CHECK_NEAR(s.accepted_outgoing_Pa+s.current_return_Pa,
                   .5*(p.gamma-1)*p.rho*u*u,1e-7);
    }
    free(input);free(mem);
}

void test_coupled_duct_conservative_open_port(void)
{
    rb_cd_spec p={.cells=560,.gas_cells=168,.rate=192000,.rho=1.2,
        .pressure=1.2*343*343/1.4,.gamma=1.4,.area=.003,.tag_fraction=.25,
        .length=560*343.0/192000,.reflection=0};
    rb_cd_state s;
    size_t count=rb_cd_workspace(p.cells);
    rb_cd_vec *memory=calloc(count,sizeof *memory);
    CHECK(memory != NULL);
    CHECK_EQ_INT(rb_cd_prepare(&s,&p,memory,count),0);
    CHECK_EQ_INT(rb_cd_seed(&s,.001,.15,.04),0);
    for(int t=0;t<448;t++) CHECK_EQ_INT(rb_cd_step(&s),0);
    rb_cd_vec left=rb_cd_balance(&s,0), right=rb_cd_balance(&s,1);
    for(int k=0;k<4;k++) {
        CHECK_NEAR(left.v[k],0,1e-10);
        CHECK_NEAR(right.v[k],0,1e-10);
    }
    CHECK(fabs(s.interface_transfer.v[0]) > 1e-10); /* real transported gas, no diaphragm */
    CHECK(s.max_pressure_fraction > .0009);
    CHECK_EQ_INT(s.failures,0);
    free(memory);
}
