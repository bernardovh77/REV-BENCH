/* Standalone Stage043 measurements. Production A0/A1 remain separate controls. */
#define _POSIX_C_SOURCE 200809L
#include "mixture.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define PI 3.14159265358979323846
static double seconds(void) {return (double)clock()/CLOCKS_PER_SEC;}
static int number(const char *text,double *value)
{ char *end;*value=strtod(text,&end);return end!=text && !*end && isfinite(*value); }
int main(int argc,char **argv)
{
    /* mode n rate T U frequency amplitude mixture duration. wave uses a
     * periodic integer-mode duct: length=(c+U)*4/f, time=1/(c+U).
     * Thus projection is coherent and measures precisely 1 m of travel. */
    if(argc!=11) return 2;
    const char *mode=argv[1];double args[9];
    if(strcmp(mode,"wave") && strcmp(mode,"contact") && strcmp(mode,"sod") && strcmp(mode,"rarefaction")) return 2;
    for(int j=0;j<9;j++) if(!number(argv[j+2],&args[j])) return 2;
    if(args[0]<4 || args[0]>65536 || args[0]!=floor(args[0]) ||
       args[1]<1 || args[1]>1e12 || args[4]<=0 || args[4]>20000 ||
       args[7]<0 || args[7]>1 || (args[6]!=0 && args[6]!=1) ||
       args[8]<0 || args[8]>2 || args[8]!=floor(args[8])) return 2;
    int n=(int)args[0],mix=(int)args[6],boundary=(int)args[8];
    double rate=args[1],T=args[2],U=args[3],f=args[4],amp=args[5],duration=args[7];
    const double y[]={mix ? .1:1,0,mix ? .9:0};
    rb_mix_cell base;rb_mix_primitive w;
    if(rb_mix_make(101325,T,U,y,&base) || rb_mix_decode(base,&w) || rate<=0 || f<=0) return 2;
    if(w.c+U<=0) return 2;
    double length=!strcmp(mode,"wave") ? (w.c+U)*4/f:2;
    if(duration==0) duration=1/(w.c+U);
    if(duration*rate>5000000) return 2;
    rb_mix_cell *storage=calloc(rb_mix_storage(n),sizeof *storage);rb_mix_state s;
    if(!storage || rb_mix_prepare(&s,n,length/n,boundary,storage,rb_mix_storage(n))) {free(storage);return 2;}
    const double gamma=w.c*w.c/(w.p/w.rho),k=8*PI/length;
    for(int i=0;i<n;i++) {
        double x=(i+.5)*s.dx;
        if(!strcmp(mode,"wave")) {
            const double dp=amp*cos(k*x);
            if(rb_mix_make(101325+dp,T*(1+(gamma-1)*dp/(gamma*101325)),U+dp/(w.rho*w.c),y,&s.q[i])) return 2;
        } else if(!strcmp(mode,"contact")) {
            const double other[]={.1,0,.9};
            rb_mix_make(101325,i<n/2 ? T:T*1.5,U,i<n/2 ? y:other,&s.q[i]);
        } else if(!strcmp(mode,"sod")) {
            const double air[]={1,0,0};
            rb_mix_make(i<n/2 ? 100000:10000,i<n/2 ? 100:80,0,air,&s.q[i]);
        } else if(!strcmp(mode,"rarefaction")) {
            const double air[]={1,0,0};
            rb_mix_make(100000,150,i<n/2 ? -70:70,air,&s.q[i]);
        } else return 2;
    }
    long double before[5]={0},after[5]={0};double firstre=0,firstim=0;
    for(int i=0;i<n;i++) {
        rb_mix_primitive z;rb_mix_decode(s.q[i],&z);
        for(int j=0;j<5;j++) before[j]+=s.q[i].v[j]*s.dx;
        firstre+=(z.p-101325)*cos(k*(i+.5)*s.dx);firstim-=(z.p-101325)*sin(k*(i+.5)*s.dx);
    }
    int status=0,steps=(int)ceil(duration*rate);double start=seconds();
    for(int step=0;step<steps;step++) if((status=rb_mix_step(&s,1/rate))) break;
    double cpu=seconds()-start,re=0,im=0,maxp=0,peak=0;
    if(!strcmp(mode,"sod") || !strcmp(mode,"rarefaction")) puts("x,rho,u,p,T,c");
    for(int i=0;i<n;i++) {
        rb_mix_primitive z;if(rb_mix_decode(s.q[i],&z)) return 3;
        for(int j=0;j<5;j++) after[j]+=s.q[i].v[j]*s.dx;
        re+=(z.p-101325)*cos(k*(i+.5)*s.dx);im-=(z.p-101325)*sin(k*(i+.5)*s.dx);
        maxp=fmax(maxp,fabs(z.p-101325));peak=fmax(peak,fabs(z.u));
        if(!strcmp(mode,"sod") || !strcmp(mode,"rarefaction"))
            printf("%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n",(i+.5)*s.dx-1,z.rho,z.u,z.p,z.T,z.c);
    }
    if(!strcmp(mode,"sod") || !strcmp(mode,"rarefaction")) {free(storage);return status ? 1:0;}
    double norm=firstre*firstre+firstim*firstim;
    double mag=norm>0 ? hypot(re,im)/sqrt(norm):1;
    double phase=norm>0 ? remainder(atan2(im,re)-atan2(firstim,firstre)+k*(w.c+U)*s.steps/rate,2*PI)*180/PI:0;
    printf("{\"status\":%d,\"n\":%d,\"dx\":%.17g,\"rate\":%.17g,\"time\":%.17g,\"c\":%.17g,\"magnitude_db\":%.17g,\"phase_deg\":%.17g,\"pressure_drift_Pa\":%.17g,\"velocity_peak\":%.17g,\"cpu_s\":%.17g,\"realtime\":%.17g,\"bytes\":%zu,\"max_cfl\":%.17g,\"limited\":%llu,\"reconstruction_fallbacks\":%llu,\"flux_fallbacks\":%llu,\"balance\":[",
        status,n,s.dx,rate,s.steps/rate,w.c,20*log10(fmax(mag,1e-300)),phase,maxp,peak,cpu,s.steps/rate/fmax(cpu,1e-12),
        sizeof s+rb_mix_storage(n)*sizeof *storage,s.max_cfl,s.limited_cells,s.reconstruction_fallbacks,s.flux_fallbacks);
    for(int j=0;j<5;j++) printf("%s%.17g",j ? ",":"",(double)(after[j]-before[j])+s.boundary_book.v[j]);
    printf("],\"initial\":[");
    for(int j=0;j<5;j++) printf("%s%.17g",j ? ",":"",(double)before[j]);
    puts("]}");free(storage);return 0;
}
