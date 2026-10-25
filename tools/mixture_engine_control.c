/* A0 explicit production valve/neighbor convergence, not an A2 adapter. */
#include "engine.h"
#include "presets.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static rb_engine engine;
int main(int argc,char **argv)
{
    if(argc!=4) return 2;
    int preset=atoi(argv[1]);double rate=atof(argv[2]),pipe=atof(argv[3]);rb_engine_desc d;
    if(rb_preset_desc(preset,&d)) return 2;
    d.heat_transfer=0;
    if(rb_engine_init_at(&engine,&d,1/rate,pipe)) return 2;
    engine.rpm=1500;engine.throttle_pos=1;
    int cycles=rb_engine_settle(&engine,100,1e-6);
    printf("{\"preset\":%d,\"engine_rate\":%.0f,\"pipe_rate\":%.0f,\"cycles\":%d,\"pmep\":%.17g,\"ve\":%.17g}\n",preset,rate,pipe,cycles,engine.result.pmep,engine.result.ve);
    return cycles>0 ? 0:1;
}
