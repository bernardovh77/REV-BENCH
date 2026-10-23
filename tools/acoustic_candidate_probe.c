/* S2d native candidate fixtures. No production dispatch/default is changed. */
#define _POSIX_C_SOURCE 200809L
#include "wave.h"
#include "euler.h"
#include "presets.h"
#include "rb.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PI 3.14159265358979323846
#define P0 101325.0
#define AREA (.04*.04*PI/4)
static const double AIR[RB_NSPECIES]={[RB_AIR]=1};
static double wall(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return t.tv_sec+1e-9*t.tv_nsec;
}
static int number(const char *text,double *out)
{
    char *end;
    *out=strtod(text,&end);
    return end != text && !*end && isfinite(*out);
}
static void background(double temperature,double *rho,double *c,double *gamma)
{
    rb_chamber b;
    rb_chamber_init(&b,.001,P0,temperature,AIR);
    *rho=b.rho; *c=rb_chamber_sound_speed(&b); *gamma=rb_chamber_gamma(&b);
}
typedef struct { double energy,re,im,centroid,weight,peak; double total[4]; } fv_metric;
static fv_metric measure(const rb_fv_state *s,double rho,double c,double u,double frequency)
{
    fv_metric m={0};
    const double k=2*PI*frequency/(c+u);
    long double sums[4]={0};
    for (int i=0;i<s->cells;i++) {
        const rb_fv_cell q=s->q[i];
        const double dp=rb_fv_pressure(q,s->gamma)-P0,du=q.momentum/q.rho-u;
        const double x=(i+.5)*s->dx;
        m.energy+=.5*(dp*dp/(rho*c*c)+rho*du*du)*AREA*s->dx;
        m.re+=dp*cos(k*x)*s->dx; m.im-=dp*sin(k*x)*s->dx;
        m.centroid+=x*dp*dp; m.weight+=dp*dp; m.peak=fmax(m.peak,fabs(dp));
        sums[0]+=q.rho; sums[1]+=q.momentum; sums[2]+=q.energy; sums[3]+=q.tag;
    }
    if (m.weight > 0) m.centroid/=m.weight;
    for (int k2=0;k2<4;k2++) m.total[k2]=(double)(sums[k2]*s->dx);
    return m;
}
static int fv(double dx,double rate,double temperature,double u,double frequency,
              double amplitude,double duration,double length)
{
    double rho,c,gamma;
    background(temperature,&rho,&c,&gamma);
    const int cells=(int)lround(length/dx),steps=(int)lround(duration*rate);
    dx=length/cells;
    const double dt=1/rate,time=steps*dt,speed=c+u,ppw=speed/(frequency*dx);
    const double sigma=.5*speed/frequency,guard=.5*length-8*sigma-(c+fabs(u))*time;
    const int allowed=ppw >= 4 && (c+fabs(u))*dt/dx <= .8 &&
        (duration > .02 || guard > 0); /* long runs measure energy after wraps, not speed */
    puts("kind,status,temperature_K,mean_flow_m_s,frequency_Hz,amplitude_Pa,dx_m,rate_Hz,cells,time_s,sound_m_s,gamma,points_per_wavelength,energy_ratio,carrier_magnitude,carrier_phase_error_deg,speed_error_percent,mass_relative_error,momentum_relative_error,total_energy_relative_error,tag_relative_error,reconstruction_fallbacks,flux_fallbacks,failures,max_cfl,wall_s,cpu_s,cpu_realtime_factor,peak_Pa");
    if (!allowed) {
        printf("A1,excluded,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%d,%.9g,%.9g,%.9g,%.9g",temperature,u,frequency,amplitude,dx,rate,cells,time,c,gamma,ppw);
        for(int j=0;j<16;j++) printf(",0");
        putchar('\n'); return 0;
    }
    const size_t need=rb_fv_cells_required(cells);
    rb_fv_cell *storage=calloc(need,sizeof *storage);
    rb_fv_state s;
    if (!storage || rb_fv_prepare(&s,cells,dx,gamma,1,storage,need)) {free(storage);return 1;}
    const double wave=2*PI*frequency/speed;
    for (int i=0;i<cells;i++) {
        const double x=(i+.5)*dx-.5*length;
        const double dp=amplitude*exp(-.5*x*x/(sigma*sigma))*cos(wave*x);
        s.q[i]=rb_fv_make(rho*pow((P0+dp)/P0,1/gamma),u+dp/(rho*c),P0+dp,.2,gamma);
    }
    const fv_metric first=measure(&s,rho,c,u,frequency);
    const double start=wall(); const clock_t cpu_start=clock();
    int failed=0;
    for(int i=0;i<steps;i++) if(rb_fv_step(&s,dt)) {failed=1;break;}
    const double cpu=(double)(clock()-cpu_start)/CLOCKS_PER_SEC,elapsed=wall()-start;
    const fv_metric last=measure(&s,rho,c,u,frequency);
    const double norm=first.re*first.re+first.im*first.im;
    const double re=(last.re*first.re+last.im*first.im)/norm;
    const double im=(last.im*first.re-last.re*first.im)/norm;
    const double angle=2*PI*frequency*time;
    const double phase=atan2(re*sin(angle)+im*cos(angle),re*cos(angle)-im*sin(angle))*180/PI;
    double relative[4];
    for(int j=0;j<4;j++) relative[j]=(last.total[j]-first.total[j])/fmax(1,fabs(first.total[j]));
    printf("A1,%s,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%d,%.9g,%.9g,%.9g,%.9g,%.12g,%.12g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%llu,%llu,%llu,%.9g,%.9g,%.9g,%.9g,%.9g\n",
        failed ? "failed":"ok",temperature,u,frequency,amplitude,dx,rate,cells,time,c,gamma,ppw,
        last.energy/first.energy,hypot(re,im),phase,100*((last.centroid-first.centroid)/time/speed-1),
        relative[0],relative[1],relative[2],relative[3],s.recon_fallbacks,s.flux_fallbacks,
        s.failures,s.max_cfl,elapsed,cpu,time/cpu,last.peak);
    free(storage); return failed;
}

static int wave(double rate,double temperature,double frequency,double length,int segments,
                 int interpolation,int loop,double duration)
{
    double rho,c,gamma;
    background(temperature,&rho,&c,&gamma);
    rb_ac_plan p;
    rb_ac_state s;
    if (rb_ac_init(&p,rate,rho,c)) return 1;
    rb_ac_add_node(&p,RB_AC_BOUNDARY,loop ? 1:0);
    for (int n=1;n<segments;n++) rb_ac_add_node(&p,RB_AC_JUNCTION,0);
    rb_ac_add_node(&p,RB_AC_BOUNDARY,loop ? -1:0);
    for (int n=0;n<segments;n++) rb_ac_add_edge(&p,n,n+1,length/segments,AREA,1,interpolation);
    const size_t need=rb_ac_samples(&p);
    double *storage=calloc(need ? need:1,sizeof(double));
    const int status=rb_ac_prepare(&p,&s,storage,need);
    if (status) {fprintf(stderr,"wave prepare rejected: %d\n",status);free(storage);return 1;}
    rb_ac_drive drive={0};
    const int steps=(int)lround(rate*duration),start_measure=loop ? steps:(int)lround(rate*.1);
    double re=0,im=0,max_energy=0;
    const double start=wall(); const clock_t cpu_start=clock();
    int failed=0;
    for(int i=0;i<steps;i++) {
        const double t=i/rate,angle=2*PI*frequency*t;
        const double ramp=t < .02 ? pow(sin(.5*PI*t/.02),2):1;
        drive.wave[0]=loop ? (i == 0 ? 1:0):ramp*sin(angle);
        if(rb_ac_step(&s,&drive)) {failed=1;break;}
        if(i >= start_measure) {re+=s.pressure[segments]*sin(angle);im+=s.pressure[segments]*cos(angle);}
        if(loop) max_energy=fmax(max_energy,rb_ac_energy(&s));
    }
    const double cpu=(double)(clock()-cpu_start)/CLOCKS_PER_SEC,elapsed=wall()-start;
    const double angle=2*PI*frequency*length/c;
    const double norm=loop ? 1:2.0/(steps-start_measure);
    const double gain=norm*hypot(re,im),phase=atan2(re*sin(angle)+im*cos(angle),re*cos(angle)-im*sin(angle))*180/PI;
    puts("kind,status,temperature_K,frequency_Hz,rate_Hz,length_m,segments,interpolation,time_s,sound_m_s,carrier_magnitude,carrier_phase_error_deg,energy_ratio,max_energy_ratio,relative_ledger_error,numerical_loss_fraction,propagation_loss_fraction,terminal_work_fraction,storage_bytes,wall_s,cpu_s,cpu_realtime_factor,failures");
    printf("%s,%s,%.9g,%.9g,%.9g,%.9g,%d,%s,%.9g,%.9g,%.12g,%.9g,%.12g,%.12g,%.9g,%.9g,%.9g,%.9g,%zu,%.9g,%.9g,%.9g,%llu\n",
        loop ? "B0-loop":"B0-transfer",failed ? "failed":"ok",temperature,frequency,rate,length,segments,
        interpolation == RB_AC_ALLPASS ? "allpass":"linear",duration,c,gain,phase,
        rb_ac_energy(&s)/s.source_work,max_energy/s.source_work,rb_ac_balance(&s)/s.source_work,
        s.numerical_loss/s.source_work,s.propagation_loss/s.source_work,s.terminal_work/s.source_work,
        sizeof s+sizeof p+need*sizeof(double),elapsed,cpu,duration/cpu,s.failures);
    free(storage); return failed;
}

static int pipe_id(const rb_engine *e,const rb_pipe *p)
{
    for(int k=0;k<rb_engine_pipe_count(e);k++) if(rb_engine_pipe(e,k)==p) return k;
    return -1;
}
static int component_name(const rb_engine *e,const rb_pipe *p,char name[80])
{
    if(!p) {strcpy(name,"none");return 0;}
    for(int i=0;i<e->desc.cylinders;i++) {
        if(p==&e->runner[i]) {snprintf(name,80,"runner-%d",i);return 0;}
        if(p==&e->header[i]) {snprintf(name,80,"header-%d",i);return 0;}
    }
    for(int i=0;i<e->collectors;i++) {
        if(p==&e->exhaust_pipe[i]) {snprintf(name,80,"exhaust-%d",i);return 0;}
        if(p==&e->center[i]) {snprintf(name,80,"center-%d",i);return 0;}
        if(p==&e->link[i]) {snprintf(name,80,"link-%d",i);return 0;}
    }
    for(int i=0;i<e->systems;i++) {
        if(p==&e->muffler[i]) {snprintf(name,80,"muffler-%d",i);return 0;}
        if(p==&e->tailpipe[i]) {snprintf(name,80,"tailpipe-%d",i);return 0;}
    }
    if(p==&e->snorkel) {strcpy(name,"snorkel");return 0;}
    strcpy(name,"UNKNOWN");return 1;
}
static int chamber_name(const rb_engine *e,const rb_chamber *ch,char name[80])
{
    if(!ch) {strcpy(name,"none");return 0;}
    if(ch==&e->intake_air) {strcpy(name,"ambient-intake");return 0;}
    if(ch==&e->exhaust_air) {strcpy(name,"ambient-exhaust");return 0;}
    if(ch==&e->plenum) {strcpy(name,"plenum");return 0;}
    if(ch==&e->airbox) {strcpy(name,"airbox");return 0;}
    if(ch==&e->junction) {strcpy(name,"junction");return 0;}
    for(int i=0;i<e->desc.cylinders;i++) if(ch==&e->cyl[i].gas) {snprintf(name,80,"cylinder-%d",i);return 0;}
    for(int i=0;i<e->collectors;i++) if(ch==&e->collector[i]) {snprintf(name,80,"collector-%d",i);return 0;}
    for(int k=0;k<rb_engine_pipe_count(e);k++) {
        const rb_pipe *p=rb_engine_pipe(e,k);
        for(int i=0;i<p->cells;i++) if(ch==&p->cell[i]) {
            char component[80];
            if(component_name(e,p,component)) return 1;
            snprintf(name,80,"%.55s:cell-%d",component,i);return 0;
        }
    }
    strcpy(name,"UNKNOWN"); return 1;
}
static const char *pipe_kind(const rb_engine *e,const rb_pipe *p)
{
    for(int i=0;i<e->desc.cylinders;i++) {
        if(p==&e->runner[i]) return "runner";
        if(p==&e->header[i]) return "header";
    }
    for(int i=0;i<e->collectors;i++) {
        if(p==&e->exhaust_pipe[i]) return "exhaust";
        if(p==&e->center[i]) return "center";
        if(p==&e->link[i]) return "link";
    }
    for(int i=0;i<e->systems;i++) {
        if(p==&e->muffler[i]) return "muffler";
        if(p==&e->tailpipe[i]) return "tailpipe";
    }
    return p==&e->snorkel ? "snorkel":"UNKNOWN";
}
static int topology(int without_runners)
{
    int failures=0;
    printf("{\"schema\":1,\"kind\":\"production-topology-snapshot\",\"variant\":\"%s\",\"physics_hz\":192000,\"pipe_hz\":48000,\"engines\":[\n",without_runners ? "without-runners":"accepted");
    for(int preset=0;preset<rb_preset_count();preset++) {
        rb_engine *e=calloc(1,sizeof *e); rb_engine_desc d;
        if(!e || rb_preset_desc(preset,&d)) {free(e);return 1;}
        if(without_runners) d.runner_length=0; /* snapshot-only identity fault control */
        if(rb_engine_init_at(e,&d,1.0/192000,48000)) {free(e);return 1;}
        printf("%s{\"preset\":\"%s\",\"rev_limit\":%.9g,\"collectors\":%d,\"systems\":%d,\"junction_volume_m3\":%.9g,\"pipes\":[\n",
            preset ? ",":"",rb_preset_key(preset),d.rev_limit,e->collectors,e->systems,d.junction_volume);
        for(int k=0;k<rb_engine_pipe_count(e);k++) {
            const rb_pipe *p=rb_engine_pipe(e,k);
            char component[80]; failures+=component_name(e,p,component);
            printf("%s{\"id\":\"%s/%s\",\"component\":\"%s\",\"legacy_pipe_index\":%d,\"kind\":\"%s\",\"length_m\":%.12g,\"area_m2\":%.12g,\"cells\":%d,\"ends\":[",
                k ? ",":"",rb_preset_key(preset),component,component,k,pipe_kind(e,p),p->length,p->area,p->cells);
            for(int end=0;end<2;end++) {
                const rb_pipe_end *port=&p->end[end];
                const rb_chamber *ch=port->chamber;
                if(port->kind==RB_PIPE_ORIFICE) {
                    const rb_chamber *own=&p->cell[end ? p->cells-1:0];
                    ch=port->orifice->a==own ? port->orifice->b:port->orifice->a;
                }
                char name[80]; failures+=chamber_name(e,ch,name);
                char onward[80]; failures+=component_name(e,port->pipe,onward);
                const rb_face *f=&p->end_face[end];
                printf("%s{\"port\":%d,\"connection\":%d,\"neighbor\":\"%s\",\"onward_pipe\":%d,\"onward_component\":\"%s\",\"radiation\":%d,\"laminar_Pa_s_m\":%.9g,\"exit_K\":%.9g}",
                    end ? ",":"",end,port->kind,name,port->pipe ? pipe_id(e,port->pipe):-1,
                    onward,p->radiation[end].enabled,f->laminar,f->exit);
            }
            puts("]}");
        }
        puts("]}"); free(e);
    }
    printf("],\"unknown_connections\":%d,\"scope\":\"descriptor/connectivity snapshot; not an acoustic compiler or full cap trace\"}\n",failures);
    return failures != 0;
}
int main(int argc,char **argv)
{
    if(argc==2 && !strcmp(argv[1],"topology")) return topology(0);
    if(argc==2 && !strcmp(argv[1],"topology-no-runners")) return topology(1);
    if(argc==10 && !strcmp(argv[1],"fv")) {
        double a[8];
        for(int k=0;k<8;k++) if(!number(argv[k+2],&a[k])) return 2;
        if(a[0]<.005 || a[0]>.2 || a[1]<12000 || a[1]>1000000 || a[2]<250 || a[2]>1200 ||
            fabs(a[3])>150 || a[4]<100 || a[4]>20000 || a[5]<=0 || a[5]>.2*P0 ||
            a[6]<1e-4 || a[6]>2 || a[7]<1 || a[7]>100) return 2;
        return fv(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]);
    }
    if(argc==9 && (!strcmp(argv[1],"wave") || !strcmp(argv[1],"loop"))) {
        double a[7];
        for(int k=0;k<7;k++) if(!number(argv[k+2],&a[k])) return 2;
        if(a[0]<8000 || a[0]>1000000 || a[1]<250 || a[1]>1200 || a[2]<100 || a[2]>.45*a[0] ||
            a[3]<=0 || a[3]>100 || a[4]<1 || a[4]>16 || a[4]!=floor(a[4]) ||
            a[5]<0 || a[5]>1 || a[5]!=floor(a[5]) || a[6]<.2 || a[6]>2) return 2;
        return wave(a[0],a[1],a[2],a[3],(int)a[4],(int)a[5],!strcmp(argv[1],"loop"),a[6]);
    }
    fputs("acoustic_candidate_probe topology\n"
          "acoustic_candidate_probe fv DX RATE T U FREQ AMPLITUDE SECONDS LENGTH\n"
          "acoustic_candidate_probe wave|loop RATE T FREQ LENGTH SEGMENTS INTERPOLATION SECONDS\n"
          "INTERPOLATION: 0=allpass, 1=linear. B0 supports frozen homogeneous zero flow only.\n",stderr);
    return 2;
}
