/* Stage044 consumer: prepare real gas storage and execute compiled B0 topology.
 * Input is a bounded numeric plan emitted by exhaust_geometry.py. No JSON ABI,
 * engine callback, losses or real exhaust-transfer qualification is implied. */
#define _POSIX_C_SOURCE 200809L
#include "mixture.h"
#include "wave.h"
#include "chamber.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
typedef struct {int a,b,n;double length,area;rb_mix_state gas;rb_mix_cell *storage;} region;
static int reject(int code,const char *message) {fprintf(stderr,"%s\n",message);return code;}
static double measured_time(void)
{
#ifdef __wasi__
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+1e-9*t.tv_nsec;
#else
    return (double)clock()/CLOCKS_PER_SEC;
#endif
}
#ifdef __wasi__
#define TIME_KIND "monotonic_wall"
#else
#define TIME_KIND "process_cpu"
#endif
int main(void)
{
    int nodes,edges,sources,outlets;double rate,T,p0;
    if(scanf("%d%d%d%d%lf%lf%lf",&nodes,&edges,&sources,&outlets,&rate,&T,&p0)!=7 ||
       nodes<1 || nodes>RB_AC_NODES || edges<1 || edges>RB_AC_EDGES ||
       sources<1 || sources>16 || outlets<1 || outlets>16) return reject(2,"plan capacity/arguments");
    const double air[]={1,0,0};rb_mix_cell base;rb_mix_primitive w;
    if(rb_mix_make(p0,T,0,air,&base) || rb_mix_decode(base,&w)) return reject(2,"frozen EOS state");
    rb_ac_plan plan;rb_ac_state state;
    if(rb_ac_init(&plan,rate,w.rho,w.c)) return reject(2,"acoustic environment");
    double volume=0,gas_mass=0,gas_energy=0;rb_chamber gas_nodes[RB_AC_NODES]={0};
    for(int j=0;j<nodes;j++) {
        int kind;double value;
        if(scanf("%d%lf",&kind,&value)!=2 || (kind!=0 && kind!=1) ||
           rb_ac_add_node(&plan,kind,value)<0) return reject(2,"node");
        if(kind==RB_AC_JUNCTION && value>0) {
            rb_chamber *node=&gas_nodes[j];rb_chamber_init(node,value,p0,T,air);
            if(node->repairs) return reject(3,"node gas state");
            volume+=node->V;gas_mass+=node->m[0];gas_energy+=node->U;
        }
    }
    region regions[RB_AC_EDGES]={0};int cells=0;
    /* Allocate only after all structural/capacity/delay checks succeed. */
    for(int j=0;j<edges;j++) {
        region *r=&regions[j];
        if(scanf("%d%d%lf%lf%d",&r->a,&r->b,&r->length,&r->area,&r->n)!=5 ||
           r->n<4 || r->n>8192 || cells>8192-r->n ||
           rb_ac_add_edge(&plan,r->a,r->b,r->length,r->area,1,RB_AC_ALLPASS)<0)
            return reject(2,"edge/cell capacity");
        cells+=r->n;
    }
    int source[16],outlet[16],bound[RB_AC_NODES]={0};
    for(int j=0;j<sources;j++) if(scanf("%d",&source[j])!=1 || source[j]<0 || source[j]>=nodes ||
        plan.node[source[j]].kind!=RB_AC_BOUNDARY || bound[source[j]]++) return reject(2,"source binding");
    for(int j=0;j<outlets;j++) if(scanf("%d",&outlet[j])!=1 || outlet[j]<0 || outlet[j]>=nodes ||
        plan.node[outlet[j]].kind!=RB_AC_BOUNDARY || bound[outlet[j]]++) return reject(2,"outlet binding");
    for(int j=0;j<nodes;j++) if(plan.node[j].kind==RB_AC_BOUNDARY && !bound[j]) return reject(2,"unbound terminal");
    char extra;if(scanf(" %c",&extra)==1) return reject(2,"trailing plan data");
    size_t samples=rb_ac_samples(&plan);
    if(!samples) return reject(4,"unsupported acoustic delay/degree/topology");
    double *storage=calloc(samples,sizeof *storage);
    if(!storage) return reject(5,"allocation");
    int result=0;
    for(int j=0;j<edges;j++) {
        region *r=&regions[j];size_t count=rb_mix_storage(r->n);
        r->storage=calloc(count,sizeof *r->storage);
        if(!r->storage || rb_mix_prepare(&r->gas,r->n,r->length/r->n,1,r->storage,count)) {result=5;goto done;}
        for(int k=0;k<r->n;k++) {
            r->gas.q[k]=base;double cell_volume=r->area*r->gas.dx;
            volume+=cell_volume;gas_mass+=w.rho*cell_volume;gas_energy+=base.v[4]*cell_volume;
        }
    }
    double max_balance=0,cpu_start=measured_time();
    unsigned long long failures=0;const int ticks=(int)ceil(.15*rate);
    printf("{\"rate\":%.17g,\"c\":%.17g,\"nodes\":%d,\"edges\":%d,\"gas_cells\":%d,\"gas_volume_m3\":%.17g,\"gas_mass_kg\":%.17g,\"gas_sensible_energy_J\":%.17g,\"delay_samples\":%zu,\"responses\":[",rate,w.c,nodes,edges,cells,volume,gas_mass,gas_energy,samples);
    for(int src=0;src<sources;src++) {
        if(rb_ac_prepare(&plan,&state,storage,samples)) {result=6;goto done;}
        double peak[16]={0};int first[16];for(int o=0;o<outlets;o++) first[o]=-1;
        rb_ac_drive drive={0};
        for(int tick=0;tick<ticks;tick++) {
            drive.wave[source[src]]=tick==0 ? 1:0;
            if(rb_ac_step(&state,&drive)) {result=7;goto done;}
            for(int o=0;o<outlets;o++) {
                double p=fabs(state.pressure[outlet[o]]);
                if(!isfinite(p)) {result=7;goto done;}
                if(p>0 && first[o]<0) first[o]=tick;
                peak[o]=fmax(peak[o],p);
            }
        }
        double balance=fabs(rb_ac_balance(&state))/fmax(1e-30,fabs(state.source_work));
        if(!isfinite(balance)) {result=7;goto done;}
        max_balance=fmax(max_balance,balance);failures+=state.failures;
        for(int o=0;o<outlets;o++) printf("%s{\"source\":%d,\"outlet\":%d,\"peak\":%.17g,\"first_tick\":%d}",src || o ? ",":"",src,o,peak[o],first[o]);
    }
    const double elapsed=measured_time()-cpu_start;
    printf("],\"gas_bindings\":[");
    int offset=0;
    for(int j=0;j<edges;j++) {
        const region *r=&regions[j];
        printf("%s{\"region\":%d,\"a\":%d,\"b\":%d,\"first_cell\":%d,\"last_cell\":%d}",j ? ",":"",j,r->a,r->b,offset,offset+r->n-1);
        offset+=r->n;
    }
    printf("],\"max_balance\":%.17g,\"failures\":%llu,\"elapsed_s\":%.17g,\"timing_kind\":\"%s\"}\n",max_balance,failures,elapsed,TIME_KIND);
done:
    for(int j=0;j<edges;j++) free(regions[j].storage);
    free(storage);
    return result ? reject(result,"prepared control failed"):0;
}
