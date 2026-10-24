/* Baseline-compatible commanded-spark oracle. Native I/O only after stepping. */
#include "engine.h"
#include "presets.h"
#include "rb.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static rb_engine e;
static double wrap(double a) {a=fmod(a,720);return a<0?a+720:a;}
int main(int argc,char **argv) {
    int preset=argc>1?atoi(argv[1]):0;
    rb_engine_desc d;if(rb_preset_desc(preset,&d)||rb_engine_init_at(&e,&d,1.0/192000,48000)) return 2;
    e.rpm=3000;e.throttle_pos=.6;e.ignition=1;e.variation=1;
    double heat_before=0;unsigned long steps_before=0;
    for(int k=0;k<96000;k++) {
        double before[RB_MAX_CYLINDERS],ahead[RB_MAX_CYLINDERS];
        int waiting[RB_MAX_CYLINDERS];
        for(int i=0;i<d.cylinders;i++) {
            before[i]=rb_engine_cylinder_angle(&e,i);
            ahead[i]=wrap(e.burn[i].spark_angle-before[i]);
            waiting[i]=ahead[i]>0 && ahead[i]<90;
        }
        rb_engine_step(&e);
        for(int i=0;i<d.cylinders;i++) {
            const double span=wrap(rb_engine_cylinder_angle(&e,i)-before[i]);
            if(waiting[i] && ahead[i]>=span && e.burn[i].heat>0) {heat_before+=e.burn[i].heat;steps_before++;}
        }
    }
    printf("{\"preset\":\"%s\",\"pre_command_heat_J\":%.17g,\"pre_command_steps\":%lu,\"total_heat_J\":%.17g}\n",rb_preset_key(preset),heat_before,steps_before,e.heat_released);
    return 0;
}
