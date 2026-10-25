#define _POSIX_C_SOURCE 200809L
#include "thermal.h"
#include "pipe.h"
#include "diagnostics.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void properties(void)
{
    const double ys[][3]={{1,0,0},{0,1,0},{0,0,1},{.1,.001,.899},{.7,.02,.28}};
    const double ts[]={20,199,200,298.15,300,600,900,999.9999,1000,1000.0001,1200,1800,2500,3500,4000,4001,5000};
    puts("mixture,T_K,R,cv,cp,u,h,c,T_back_20,T_back_600,T_back_4000,newton_limits,temp_floors");
    for(int k=0;k<5;k++)for(size_t i=0;i<sizeof ts/sizeof *ts;i++) {
        rb_gas g;rb_gas_init(&g,ys[k]);double T=ts[i],cv=rb_gas_cv(&g,T),u=rb_gas_u(&g,T),temps[3];
        rb_diag_slot diag={0};const double guesses[]={20,600,4000};
        for(int n=0;n<3;n++)temps[n]=rb_gas_T_cv_from_u_diag(&g,u,guesses[n],&cv,&diag);
        printf("%d,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%llu,%llu\n",k,T,g.R,rb_gas_cv(&g,T),rb_gas_cp(&g,T),u,rb_gas_h(&g,T),sqrt(rb_gas_gamma(&g,T)*g.R*T),temps[0],temps[1],temps[2],(unsigned long long)diag.reason[RB_D_NEWTON_LIMIT],(unsigned long long)diag.reason[RB_D_TEMP_FLOOR]);
    }
}
static void coefficients(void)
{
    const double ys[][3]={{1,0,0},{0,1,0},{0,0,1},{.1,.001,.899},{.7,.02,.28}};
    puts("mixture,R,k_lo,k_hi,l0,l1,l2,l3,l4,h0,h1,h2,h3,h4");
    for(int k=0;k<5;k++) {rb_gas g;rb_gas_init(&g,ys[k]);printf("%d,%.17g,%.17g,%.17g",k,g.R,g.k_lo,g.k_hi);
        for(int j=0;j<5;j++)printf(",%.17g",g.lo[j]);
        for(int j=0;j<5;j++)printf(",%.17g",g.hi[j]);
        puts("");}
}
static void fixed_wall(void)
{
    puts("T_K,wall_K,mdot_kg_s,dt_s,heat_J,expected_heat_J,energy_residual_J,wall_after_K");
    const double y[]={.1,.001,.899};
    for(int t=0;t<2;t++)for(int wall=0;wall<3;wall++)for(int direction=-1;direction<=1;direction++) {
        rb_pipe p;rb_chamber cell,reservoir;double T=t ? 900:300,Tw=wall==0 ? 300:(wall==1 ? T:650),dt=1./48000;
        rb_pipe_init(&p,&cell,NULL,1,1,.06,101325,T,y,Tw,.025);
        rb_chamber_init_reservoir(&reservoir,101325,T,y);rb_pipe_join(&p,0,&reservoir,0,0);rb_pipe_join(&p,1,&reservoir,0,0);
        p.end_face[0].mdot=p.end_face[1].mdot=.03*direction;
        double velocity=.03*direction/(cell.rho*p.area);p.kinetic[0]=.5*cell.mass*velocity*velocity;
        double E0=cell.U+p.kinetic[0],expected=p.stanton*rb_gas_cp(&cell.gas,T)*.03*abs(direction)*p.wall_ratio*(Tw-T)*dt;
        rb_pipe_clear(&p,0);rb_pipe_integrate(&p,0,dt);
        printf("%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n",T,Tw,.03*direction,dt,p.heat,expected,cell.U+p.kinetic[0]-E0-p.heat,p.wall_temperature);
    }
}
int main(int argc,char **argv)
{
    if(argc==2 && !strcmp(argv[1],"properties")){properties();return 0;}
    if(argc==2 && !strcmp(argv[1],"coefficients")){coefficients();return 0;}
    if(argc==2 && !strcmp(argv[1],"fixed")){fixed_wall();return 0;}
    if(argc!=10 || strcmp(argv[1],"pair"))return 2;
    double T=atof(argv[2]),Tw=atof(argv[3]),C=atof(argv[4]),Hi=atof(argv[5]),Ho=atof(argv[6]),dt=atof(argv[7]),duration=atof(argv[8]);int mix=atoi(argv[9]);
    const double y[2][3]={{1,0,0},{.1,.001,.899}};rb_tw_pair s;
    if(mix<0||mix>1 || !isfinite(duration) || !isfinite(dt) || duration<=0 || dt<=0 || duration/dt>200000 || rb_tw_prepare(&s,.003,y[mix],T,C,Tw,300,Hi,Ho))return 2;
    puts("time_s,gas_K,wall_K,gas_J,ambient_J,balance_J");
    const double ratio=duration/dt,nearest=round(ratio);
    const int n=(int)(fabs(ratio-nearest)<=1e-12*fmax(1,ratio) ? nearest:ceil(ratio));
    for(int i=0;i<=n;i++) {
        printf("%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n",i*dt,rb_tw_temperature(&s),s.wall_T,s.energy,s.ambient_heat,rb_tw_balance(&s));
        if(i<n && rb_tw_step(&s,dt))return 3;
    }
    return 0;
}
