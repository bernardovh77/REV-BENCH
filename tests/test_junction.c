#include "rb_test.h"
#include "junction.h"
#include <math.h>
#include <string.h>
static const double air[3]={1,0,0},burned[3]={.1,.001,.899};
static rb_jv_cell_geometry geom[2];static rb_jv_face_geometry conn[12];
static rb_jv_cell cells[2];static rb_jv_face faces[12];
static int boxes(rb_jv_state *s,int count,int connected,int ports)
{
    memset(geom,0,sizeof geom);memset(conn,0,sizeof conn);int nf=0;
    for(int i=0;i<count;i++) {
        double x=i*(connected?1.:3.);geom[i]=(rb_jv_cell_geometry){1,{x+.5,.5,.5},i};
        for(int axis=0;axis<3;axis++)for(int side=-1;side<=1;side+=2) {
            if(connected&&i==1&&axis==0&&side==-1)continue;
            rb_jv_face_geometry f={0};f.a=i;f.b=-1;f.port=-1;f.area=1;f.normal[axis]=side;
            memcpy(f.center,geom[i].center,sizeof f.center);f.center[axis]+=.5*side;
            if(connected&&count==2&&i==0&&axis==0&&side==1)f.b=1;
            if(ports&&axis==0) {
                if(connected){if(i==0&&side==-1)f.port=0;if(i==count-1&&side==1)f.port=ports-1;}
                else if(side==1)f.port=i;
            }
            conn[nf++]=f;
        }
    }
    CHECK(rb_jv_prepare(s,count,nf,ports,geom,conn,cells,2,faces,12,101325,air,burned,300,900)==0);return nf;
}
static rb_jv_q base(rb_jv_state *s,double dp,const double v[3])
{
    const double a[2]={1,0},T[2]={300,900};return rb_jv_make(s,101325+dp,v,a,T);
}
void test_junction_vector_flux_and_cached_thermodynamics(void)
{
    rb_jv_state s={0};CHECK(rb_ft_thermodynamics(&s.thermo,101325,air,burned,300,900)==0);
    const double n[3]={.6,.8,0},v[3]={3,-2,1},alpha[2]={.3,.7},T[2]={300,900};
    rb_jv_q q=rb_jv_make(&s,101330,v,alpha,T),F;double u,p,rho=q.r[0]+q.r[1],normal=.2;
    CHECK(rb_jv_flux(&s,q,q,n,&F,&u,&p)==0);
    CHECK_NEAR(u,normal,1e-12);CHECK_NEAR(p,101330,1e-7);
    for(int k=0;k<2;k++){CHECK_NEAR(F.r[k],q.r[k]*normal,1e-10);CHECK_NEAR(F.alpha[k],alpha[k]*normal,1e-12);}
    for(int j=0;j<3;j++)CHECK_NEAR(F.momentum[j],rho*v[j]*normal+101330*n[j],1e-7);
    CHECK_NEAR(F.energy,(q.energy+101330)*normal,1e-6);
    for(int i=0;i<3;i++) {
        rb_ft_q a=rb_ft_make(&s.thermo,101330,10,i*.5,300,900),b=rb_ft_make(&s.thermo,101322,-5,1-i*.5,300,900);
        rb_ft_primitive wa,wb;rb_ft_flux old,known;
        CHECK(rb_ft_decode(&s.thermo,a,&wa)==0);CHECK(rb_ft_decode(&s.thermo,b,&wb)==0);
        CHECK(rb_ft_riemann(&s.thermo,a,b,&old)==0);CHECK(rb_ft_riemann_known(&s.thermo,a,b,&wa,&wb,&known)==0);
        CHECK_NEAR(old.f.energy,known.f.energy,1e-6);CHECK_NEAR(old.f.momentum,known.f.momentum,1e-7);
        a.energy+=1;CHECK(rb_ft_riemann_known(&s.thermo,a,b,&wa,&wb,&known)!=0);
        a.energy-=1;wa.c*=1.01;CHECK(rb_ft_riemann_known(&s.thermo,a,b,&wa,&wb,&known)!=0);
    }
}
void test_junction_current_rk_wall_and_closed_energy(void)
{
    rb_jv_state s;boxes(&s,1,0,0);const double v[3]={.001,0,0};cells[0].q=base(&s,0,v);
    CHECK(rb_jv_load(&s)==0);double w[8],rho,c;
    CHECK(rb_jv_decode(&s,cells[0].q,w,&rho,&c)==0);double E=cells[0].q.energy,dt=.0001,a=2*c*dt;
    CHECK(rb_jv_step(&s,dt,0,0,0)==0);
    CHECK_NEAR(cells[0].q.momentum[0]/rho,v[0]*(1-a+.5*a*a),1e-12);
    CHECK_NEAR(cells[0].q.energy,E,1e-10);CHECK_NEAR(s.external.energy,0,1e-15);
    CHECK_NEAR(s.external.r[0],0,1e-15);
    CHECK_NEAR(cells[0].q.momentum[0]-s.initial.momentum[0],s.wall_impulse[0],1e-12);
    CHECK(rb_jv_step(&s,1,0,0,0)!=0);CHECK(s.failed);CHECK(rb_jv_step(&s,dt,0,0,0)!=0);
    CHECK(isnan(rb_jv_inventory(&s,-1).energy));
    /* An exactly quiet state makes the oversized clock invalid solely by CFL;
     * a moving-state failure could otherwise be caught by a later Mach guard. */
    boxes(&s,1,0,0);const double quiet[3]={0,0,0};cells[0].q=base(&s,0,quiet);
    CHECK(rb_jv_load(&s)==0);CHECK(rb_jv_step(&s,1,0,0,0)!=0);
    CHECK(s.max_cfl>.45);CHECK(s.failed);CHECK(s.steps==0);
}
void test_junction_regional_accounts_and_domain(void)
{
    rb_jv_state s;int nf=boxes(&s,2,1,0);const double v[3]={0,0,0};
    cells[0].q=base(&s,5,v);cells[1].q=base(&s,0,v);CHECK(rb_jv_load(&s)==0);
    for(int j=0;j<200;j++)CHECK(rb_jv_step(&s,1e-5,0,0,0)==0);
    rb_jv_q q=rb_jv_inventory(&s,-1);CHECK_NEAR(q.energy,s.initial.energy,1e-7);
    for(int region=0;region<2;region++) {
        q=rb_jv_inventory(&s,region);rb_jv_q a=s.regional_initial[region],b=s.regional_transfer[region];
        CHECK_NEAR(q.energy-a.energy,b.energy,1e-7);
        for(int k=0;k<2;k++)CHECK_NEAR(q.r[k]-a.r[k],b.r[k],1e-12);
        for(int k=0;k<3;k++)CHECK_NEAR(q.momentum[k]-a.momentum[k],b.momentum[k],1e-10);
    }
    CHECK(isnan(rb_jv_inventory(&s,100).energy));
    geom[0].volume*=1.1;CHECK(rb_jv_prepare(&s,2,nf,0,geom,conn,cells,2,faces,12,101325,air,burned,300,900)!=0);
    nf=boxes(&s,2,1,0);conn[0].normal[0]*=-1;
    CHECK(rb_jv_prepare(&s,2,nf,0,geom,conn,cells,2,faces,12,101325,air,burned,300,900)!=0);
    nf=boxes(&s,2,1,0);CHECK(rb_jv_step(&s,1e-6,0,0,0)!=0);
    CHECK(rb_jv_prepare(&s,2,nf,0,geom,conn,cells,1,faces,12,101325,air,burned,300,900)!=0);
}
void test_junction_disconnected_ports_do_not_share_ambient(void)
{
    rb_jv_state s;boxes(&s,2,0,2);const double v[3]={0,0,0};
    cells[0].q=base(&s,0,v);cells[1].q=base(&s,0,v);CHECK(rb_jv_load(&s)==0);
    CHECK(s.components==2);CHECK(s.port_component[0]!=s.port_component[1]);
    rb_jv_q boundary[2]={base(&s,1,v),base(&s,0,v)};
    for(int j=0;j<100;j++)CHECK(rb_jv_step(&s,1e-5,boundary,boundary,2)==0);
    double w[8],rho,c;CHECK(rb_jv_decode(&s,cells[1].q,w,&rho,&c)==0);
    CHECK_NEAR(w[0],101325,1e-6);CHECK_NEAR(w[1],0,1e-9);
    CHECK_NEAR(s.port_transfer[1].energy,0,1e-7);
    CHECK(s.port_transfer[0].energy>0);
}

void test_junction_current_ports_and_reversal(void)
{
    rb_jv_state s;boxes(&s,2,1,2);const double v[3]={0,0,0};
    cells[0].q=base(&s,0,v);cells[1].q=base(&s,0,v);CHECK(rb_jv_load(&s)==0);
    const double dt=1e-5;
    for(int step=0;step<40;step++) {
        rb_jv_q start[2]={base(&s,step<20?10:-10,v),base(&s,0,v)};
        rb_jv_q end[2]={base(&s,step<20?15:-15,v),base(&s,0,v)};
        CHECK(rb_jv_step(&s,dt,start,end,2)==0);
        for(int p=0;p<2;p++) {
            int fi=-1;for(int f=0;f<s.faces;f++)if(conn[f].port==p)fi=f;
            CHECK(fi>=0);
            CHECK_NEAR(s.port_pressure[p],.5*(faces[fi].saved_p+faces[fi].p),1e-10);
            CHECK_NEAR(s.port_velocity[p],.5*(faces[fi].saved_u+faces[fi].u),1e-14);
        }
    }
    rb_jv_q q=rb_jv_inventory(&s,-1);
    CHECK_NEAR(q.energy-s.initial.energy,s.port_transfer[0].energy+s.port_transfer[1].energy,1e-8);
    for(int k=0;k<2;k++)CHECK_NEAR(q.r[k]-s.initial.r[k],s.port_transfer[0].r[k]+s.port_transfer[1].r[k],1e-13);
    for(int j=0;j<3;j++)CHECK_NEAR(q.momentum[j]-s.initial.momentum[j],s.port_transfer[0].momentum[j]+s.port_transfer[1].momentum[j]+s.wall_impulse[j],1e-10);
}

void test_junction_trace_material_at_one_sided_open_port(void)
{
    rb_jv_state s;boxes(&s,2,1,2);
    const double v[3]={-1,0,0},T[2]={300,900};
    const double left[2]={1e-10,1-1e-10},right[2]={1e-20,1},pure[2]={0,1};
    cells[0].q=rb_jv_make(&s,101325,v,left,T);
    cells[1].q=rb_jv_make(&s,101325,v,right,T);CHECK(rb_jv_load(&s)==0);
    rb_jv_q ports[2]={cells[0].q,rb_jv_make(&s,101325,v,pure,T)};
    for(int i=0;i<20;i++)CHECK(rb_jv_step(&s,1e-4,ports,ports,2)==0);
    CHECK(cells[1].q.r[0]>=0);CHECK(cells[1].q.alpha[0]>=0);
    CHECK(cells[1].q.r[0]<=1.2e-20);CHECK(s.reconstruction_fallbacks==0);
    rb_jv_q q=rb_jv_inventory(&s,-1);
    CHECK_NEAR(q.energy-s.initial.energy,s.external.energy,1e-8);
    for(int k=0;k<2;k++)CHECK_NEAR(q.r[k]-s.initial.r[k],s.external.r[k],1e-13);
    /* Thermodynamic queries must not erase unrepresentably small inventory. */
    rb_jv_q tiny=rb_jv_make(&s,101325,v,pure,T);double w[8],rho,sound;
    tiny.r[0]=nextafter(0,1);rb_jv_q original=tiny;
    CHECK(rb_jv_decode(&s,tiny,w,&rho,&sound)==0);
    CHECK_NEAR(w[0],101325,1e-8);CHECK(memcmp(&tiny,&original,sizeof tiny)==0);
    tiny.r[0]=1e-100;CHECK(rb_jv_decode(&s,tiny,w,&rho,&sound)!=0);
    tiny=original;tiny.alpha[0]=-nextafter(0,1);CHECK(rb_jv_decode(&s,tiny,w,&rho,&sound)!=0);
    /* Rounded trace alpha cannot manufacture outgoing constituent mass. */
    boxes(&s,2,1,2);
    cells[0].q=rb_jv_make(&s,101325,v,pure,T);cells[1].q=cells[0].q;
    cells[1].q.alpha[0]=1e-308;CHECK(rb_jv_load(&s)==0);
    ports[0]=cells[0].q;ports[1]=cells[0].q;
    for(int i=0;i<20;i++)CHECK(rb_jv_step(&s,1e-4,ports,ports,2)==0);
    CHECK(s.trace_face_limits>0);CHECK(rb_jv_inventory(&s,-1).r[0]==0);
    CHECK(cells[1].q.alpha[0]>0);
    /* A valid near-unit average must stay inside its reconstruction bounds.
     * An upper face bound of exactly1 can reverse the shared fraction slope. */
    boxes(&s,2,1,2);const double rounded[2]={1e-20,nextafter(1,INFINITY)};
    cells[0].q=rb_jv_make(&s,101325,v,left,T);cells[1].q=rb_jv_make(&s,101325,v,rounded,T);
    CHECK(rb_jv_load(&s)==0);ports[0]=cells[0].q;ports[1]=rb_jv_make(&s,101325,v,pure,T);
    CHECK(rb_jv_step(&s,1e-4,ports,ports,2)==0);
    CHECK(cells[1].gradient[6][0]<0);CHECK(s.reconstruction_fallbacks==0);
}

void test_junction_closed_source_free_second(void)
{
    rb_jv_state s;boxes(&s,2,1,0);const double v[3]={0,0,0};
    cells[0].q=base(&s,1,v);cells[1].q=base(&s,-1,v);CHECK(rb_jv_load(&s)==0);
    double initial=rb_jv_perturbation(&s,v),maximum=initial;
    for(int i=0;i<40000;i++) {
        CHECK(rb_jv_step(&s,2.5e-5,0,0,0)==0);
        double e=rb_jv_perturbation(&s,v);maximum=fmax(maximum,e);CHECK(isfinite(e));
    }
    rb_jv_q q=rb_jv_inventory(&s,-1);
    CHECK(maximum<=initial*1.00001);CHECK(rb_jv_perturbation(&s,v)<initial*1e-5);
    CHECK_NEAR(q.energy,s.initial.energy,1e-7);CHECK_NEAR(s.external.energy,0,1e-15);
}
