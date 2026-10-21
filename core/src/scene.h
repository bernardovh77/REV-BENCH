/* Audio-rate propagation of separate engine sources; no allocation in step. */
#ifndef RB_SCENE_H
#define RB_SCENE_H
#include "audio.h"
#include <stdint.h>

#define RB_SCENE_SOURCES 5 /* two tailpipes, mouth, bay, cabin mounts */
#define RB_SCENE_RING 32768
#define RB_ROOM_LINES 4
#define RB_ROOM_RING 8192

typedef struct rb_scene_path {
    double delay, delay_step, gain, gain_step, spread, spread_step;
    double shadow, shadow_step, low;
} rb_scene_path;

typedef struct rb_scene {
    double fs, c, diameter, time;
    int kind, tailpipes, pos, geometry_tick;
    float source[4][RB_SCENE_RING];
    rb_scene_path path[2][4][3]; /* ear, source, direct/ground/wall */
    rb_cabin cabin[2];
    float room[RB_ROOM_LINES][RB_ROOM_RING];
    int room_pos[RB_ROOM_LINES], room_length[RB_ROOM_LINES];
    double room_gain[RB_ROOM_LINES], room_low[RB_ROOM_LINES], room_a, head_a;
} rb_scene;

int rb_scene_init(rb_scene *s, int kind, double fs, double c, int tailpipes, double diameter);
void rb_scene_step(rb_scene *s, const double source[RB_SCENE_SOURCES], double out[2]);
/* Exact retarded propagation time for a constant-velocity source. */
double rb_scene_flight_time(const double relative[3], double velocity, double c);
#endif
