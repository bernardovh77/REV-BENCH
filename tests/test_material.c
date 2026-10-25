#include "rb_test.h"
#include "material.h"
#include "thermal.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define PI 3.14159265358979323846
typedef struct {rb_mt_cell *c;rb_mt_node *x;} storage;
static storage prepare(rb_mt_state *s,int n,double U)
{
    storage m={calloc((size_t)n,sizeof *m.c),calloc((size_t)n+1,sizeof *m.x)};
    CHECK(m.c&&m.x);CHECK_EQ_INT(rb_mt_prepare(s,n,.003,101325,U,m.c,(size_t)n,m.x,(size_t)n+1),0);
    const double y[][3]={{.1,.001,.899},{1,0,0}};
    for(int i=0;i<=n;i++){m.x[i].x=(double)i/n;m.x[i].v=U;}
    for(int i=0;i<n;i++){m.c[i].T=i<n/2 ? 900:300;m.c[i].p=101325;memcpy(m.c[i].m,y[i<n/2 ? 0:1],sizeof m.c[i].m);}
    CHECK_EQ_INT(rb_mt_load(s),0);return m;
}
static void release(storage m){free(m.c);free(m.x);}
static void books(rb_mt_state *s)
{
    CHECK_NEAR(rb_mt_energy(s)-s->initial_energy-s->boundary_work-s->body_work-s->ambient_heat,0,1e-10*fmax(1,fabs(s->initial_energy)));
    CHECK_NEAR(rb_mt_momentum(s)-s->initial_momentum-s->boundary_impulse-s->body_impulse,0,1e-11);
    for(int j=0;j<3;j++){double mass=0;for(int i=0;i<s->cells;i++)mass+=s->cell[i].m[j];CHECK_NEAR(mass,s->initial_species[j],1e-15);}
}
void test_material_hot_contact_and_translation(void)
{
    for(int sign=-1;sign<=1;sign++) {
        rb_mt_state s;storage m=prepare(&s,48,sign*70);
        double species[3],internal;rb_mt_left_inventory(&s,.5,species,&internal);
        double before[3];memcpy(before,species,sizeof before);
        int status=0;for(int tick=0;tick<200;tick++)if((status=rb_mt_step(&s,1e-6,0,0,0)))break;
        CHECK_EQ_INT(status,0);books(&s);
        CHECK_NEAR(s.shift,sign*70*.0002,1e-12);
        rb_mt_left_inventory(&s,.5,species,&internal);
        const rb_mt_cell *donor=&m.c[sign>0 ? 0:47];
        for(int j=0;j<3;j++)CHECK_NEAR(species[j]-before[j],-sign*.014*donor->m[j]/(1./48),1e-11);
        for(int i=0;i<s.cells;i++){CHECK_NEAR(m.c[i].p,101325,1);CHECK_NEAR(m.x[i].v,sign*70,.001);}
        release(m);
    }
}
void test_material_pulse_force_and_current_work(void)
{
    rb_mt_state s;storage m=prepare(&s,64,0);int status=0;
    for(int tick=0;tick<3000;tick++) {
        double drive=tick<100 ? 500*sin(PI*tick/100):0;
        if((status=rb_mt_step(&s,1e-6,drive,0,0)))break;
        if(tick%101==0)books(&s);
    }
    CHECK_EQ_INT(status,0);CHECK(s.boundary_work!=0);CHECK(s.max_iterations>1);CHECK(s.max_cfl<.45);books(&s);release(m);
}
void test_material_reversal_and_fixed_wall(void)
{
    rb_mt_state s;storage m=prepare(&s,32,70);int status=0;
    for(int tick=0;tick<1000;tick++) {
        double old=70*cos(2*PI*100*tick*1e-6),next=70*cos(2*PI*100*(tick+1)*1e-6);
        s.reservoir_U=s.frame_U=.5*(old+next);
        if((status=rb_mt_step(&s,1e-6,0,0,(next-old)/1e-6)))break;
    }
    CHECK_EQ_INT(status,0);CHECK_NEAR(s.shift,70*sin(2*PI*.1)/(2*PI*100),1e-8);books(&s);release(m);
    m=prepare(&s,32,70);double C=rb_tw_capacity(sqrt(.012/PI),.5,.001,7800,500);
    CHECK_EQ_INT(rb_mt_wall(&s,C,650,.25,.75,100,2,300),0);
    CHECK_EQ_INT(rb_mt_step(&s,1e-6,0,0,0),0);
    double entering_heat=.5e-6*100*sqrt(.012*PI)*(70e-6)*(650-900);
    CHECK_NEAR(s.cell[7].wall_heat,entering_heat,.1*fabs(entering_heat));
    for(int tick=1;tick<500;tick++)if((status=rb_mt_step(&s,1e-6,0,0,0)))break;
    CHECK_EQ_INT(status,0);CHECK(s.wall_capacity==C);CHECK(s.wall_x0==.25&&s.wall_x1==.75);CHECK(s.ambient_heat<0);books(&s);release(m);
    m=prepare(&s,32,0);CHECK_EQ_INT(rb_mt_wall(&s,C,650,.25,.75,0,2,300),0);
    for(int tick=0;tick<100;tick++)CHECK_EQ_INT(rb_mt_step(&s,1e-6,0,0,0),0);
    CHECK_NEAR(s.wall_T,300+350*exp(-2*.0001/C),1e-8);books(&s);release(m);
}
void test_material_coordinate_frame_and_failure(void)
{
    rb_mt_state unready;rb_mt_cell cells[4];rb_mt_node nodes[5];
    CHECK_EQ_INT(rb_mt_prepare(&unready,4,.003,101325,0,cells,3,nodes,5),-1);
    CHECK_EQ_INT(rb_mt_prepare(&unready,4,.003,101325,0,cells,4,nodes,5),0);
    CHECK_EQ_INT(rb_mt_step(&unready,1e-6,0,0,0),-1);CHECK(isnan(rb_mt_energy(&unready)));
    rb_mt_state a,b;storage ma=prepare(&a,24,0),mb=prepare(&b,24,0);
    CHECK_EQ_INT(rb_mt_load(&a),-1);
    b.frame_U=13;
    int status=0;
    for(int tick=0;tick<100;tick++){if((status=rb_mt_step(&a,1e-6,0,0,0)))break;if((status=rb_mt_step(&b,1e-6,0,0,0)))break;}
    CHECK_EQ_INT(status,0);books(&a);books(&b);
    for(int i=0;i<=a.cells;i++)CHECK_NEAR(a.node[i].x+a.shift,b.node[i].x+b.shift,1e-12);
    CHECK_NEAR(rb_mt_energy(&a),rb_mt_energy(&b),1e-8);
    CHECK_EQ_INT(rb_mt_step(&a,.01,0,0,0),-2);CHECK_EQ_INT(rb_mt_step(&a,1e-6,0,0,0),-2);CHECK(isnan(rb_mt_energy(&a)));
    release(ma);release(mb);
}
void test_material_fine_geometry_work(void)
{
    rb_mt_state s;storage m=prepare(&s,1536,70);int status=0;
    for(int tick=0;tick<4000;tick++) {
        double drive=tick<1000 ? sin(2*PI*4000*(tick+.5)*.5e-6):0;
        if((status=rb_mt_step(&s,.5e-6,drive,0,0)))break;
        if(tick%503==0)books(&s);
    }
    CHECK_EQ_INT(status,0);CHECK_EQ_INT(s.steps,4000);books(&s);release(m);
}
