/* Production render-path timing without diagnostic listeners or file output.
 * Standalone native control, not a browser real-time acceptance test. */
#define _POSIX_C_SOURCE 200809L
#include "rb_internal.h"
#include "presets.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec+1e-9*t.tv_nsec;
}
static int compare(const void *a, const void *b)
{
    const double x=*(const double *)a, y=*(const double *)b;
    return (x>y)-(x<y);
}
int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr,"audio_render_bench PHYSICS_HZ PIPE_HZ REPEATS\n");
        return 2;
    }
    rb_config cfg;
    rb_config_default(&cfg);
    const double rate=strtod(argv[1],NULL), pipe=strtod(argv[2],NULL);
    const int repeats=atoi(argv[3]);
    cfg.oversample=(int)lround(rate/cfg.sample_rate);
    cfg.pipe_rate=pipe;
    if (cfg.oversample<1 || cfg.oversample>32 || fabs(cfg.oversample*cfg.sample_rate-rate)>1e-6
        || pipe<12000 || pipe>96000 || repeats<1 || repeats>10) return 2;
    puts("repeat,physics_hz,pipe_hz,cells,rendered_s,cpu_s,cpu_realtime_factor,callback_p99_ms,callback_max_ms,nonfinite,limited");
    for (int repeat=0; repeat<repeats; repeat++) {
        rb_sim *sim=rb_create(&cfg);
        rb_engine_desc d;
        if (!sim || rb_preset_desc(2,&d) || rb_engine_init_at(&sim->engine,&d,sim->dt,cfg.pipe_rate)) return 1;
        sim->source=RB_SOURCE_ENGINE; sim->hear=RB_HEAR_BEHIND; sim->layers=RB_LAYER_ALL;
        rb_sim_listen(sim);
        rb_set_param(sim,RB_PARAM_MASTER_GAIN,.1);
        sim->engine.ignition=1; sim->engine.variation=1; sim->engine.rpm=6500;
        sim->engine.throttle_pos=1;
        if (rb_set_scene(sim,0)!=RB_OK) return 1; /* asphalt stereo */
        float samples[256];
        for (int k=0;k<375;k++) rb_render_stereo(sim,samples,128); /* 1 s warmup */
        double callbacks[750];
        const clock_t start=clock();
        const uint64_t nonfinite=sim->nonfinite, limited=sim->limited;
        for (int k=0;k<750;k++) {
            const double t=now();
            rb_render_stereo(sim,samples,128);
            callbacks[k]=1000*(now()-t);
        }
        const double cpu=(double)(clock()-start)/CLOCKS_PER_SEC;
        int cells=0;
        for (int k=0;k<rb_engine_pipe_count(&sim->engine);k++) cells+=rb_engine_pipe(&sim->engine,k)->cells;
        qsort(callbacks,750,sizeof(double),compare);
        printf("%d,%.0f,%.0f,%d,2,%.9g,%.9g,%.9g,%.9g,%llu,%llu\n",repeat,rate,1/sim->engine.pipe_dt,
            cells,cpu,2/cpu,callbacks[742],callbacks[749],
            (unsigned long long)(sim->nonfinite-nonfinite),(unsigned long long)(sim->limited-limited));
        if (sim->nonfinite!=nonfinite || sim->limited!=limited) return 1;
        rb_destroy(sim);
    }
    return 0;
}
